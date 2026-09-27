/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

////////////////////////////////////////////////////////////////////////////////
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

// FILE: ContainedTransitionDamageFXV2.cpp ////////////////////////////////////////////////////////
// Desc:   See ContainedTransitionDamageFXV2.h. Effect code is a copy of TransitionDamageFX's.
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/Xfer.h"
#include "GameClient/Drawable.h"
#include "GameClient/FXList.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"
#include "GameLogic/ObjectCreationList.h"
#include "GameLogic/Module/BodyModule.h"
#include "GameLogic/Module/ContainedTransitionDamageFXV2.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ContainedTransitionDamageFXV2ModuleData::ContainedTransitionDamageFXV2ModuleData()
{
	m_checkIntervalFrames = LOGICFRAMES_PER_SECOND / 10;	// ~100ms
}

//-------------------------------------------------------------------------------------------------
/** Copy of the file-local helper in TransitionDamageFX.cpp (not reachable from here).
	* Given an FXLoc info struct, return the effect position that we are supposed to use.
	* The position is local to to the object */
//-------------------------------------------------------------------------------------------------
static Coord3D getLocalEffectPosV2( const FXLocInfo *locInfo, Drawable *draw, const RandomValueClass &random = LogicRandomValueClass() )
{

	DEBUG_ASSERTCRASH( locInfo, ("getLocalEffectPosV2: locInfo is null") );

	if( locInfo->locType == FX_DAMAGE_LOC_TYPE_BONE && draw )
	{

		if( locInfo->randomBone == FALSE )
		{
			Coord3D pos;

			// get the bone position
			Int count = draw->getPristineBonePositions( locInfo->boneName.str(), 0, &pos, nullptr, 1 );

			// sanity, if bone not found revert back to location defined in struct (which is 0,0,0)
			if( count == 0 )
				return locInfo->loc;

			// return the position retrieved
			return pos;

		}
		else
		{
		  const Int MAX_BONES = 32;
			Coord3D positions[ MAX_BONES ];

			// get the bone positions
			Int boneCount;
			boneCount = draw->getPristineBonePositions( locInfo->boneName.str(), 1, positions, nullptr, MAX_BONES );

			// sanity, if bone not found revert back to location defined in struct (which is 0,0,0)
			if( boneCount == 0 )
				return locInfo->loc;

			// pick one of the bone positions
			Int pick = RandomValueInt( random, 0, boneCount - 1 );
			return positions[ pick ];

		}

	}
	else
		return locInfo->loc;

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ContainedTransitionDamageFXV2::ContainedTransitionDamageFXV2( Thing *thing, const ModuleData* moduleData )
																		  : UpdateModule( thing, moduleData )
{
	Int i, j;

	for( i = 0; i < BODYDAMAGETYPE_COUNT; i++ )
		for( j = 0; j < DAMAGE_MODULE_MAX_FX; j++ )
			m_particleSystemID[ i ][ j ] = INVALID_PARTICLE_SYSTEM_ID;

	// real value taken in onObjectCreated(), once the body module is guaranteed to exist
	m_lastKnownState = BODY_PRISTINE;
	m_relayDamageInfo = nullptr;

	setWakeFrame( getObject(), UPDATE_SLEEP_NONE );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ContainedTransitionDamageFXV2::~ContainedTransitionDamageFXV2()
{

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ContainedTransitionDamageFXV2::onObjectCreated()
{
	// Start from whatever state we were created in, exactly like TransitionDamageFX (which never
	// plays anything for the initial state either).
	BodyModuleInterface *body = getObject()->getBodyModule();
	if( body )
		m_lastKnownState = body->getDamageState();
}

//-------------------------------------------------------------------------------------------------
/** Normal notification path (attemptDamage / attemptHealing on this object). */
//-------------------------------------------------------------------------------------------------
void ContainedTransitionDamageFXV2::onBodyDamageStateChange( const DamageInfo* damageInfo,
																														 BodyDamageType oldState,
																														 BodyDamageType newState )
{
	// record first, so the next poll doesn't play this same transition again
	m_lastKnownState = newState;

	doTransitionEffects( damageInfo, oldState, newState );
}

//-------------------------------------------------------------------------------------------------
/** Poll for state changes that bypassed the notification (e.g. OverlordContain's setDamageState). */
//-------------------------------------------------------------------------------------------------
UpdateSleepTime ContainedTransitionDamageFXV2::update()
{
	const ContainedTransitionDamageFXV2ModuleData *modData = getContainedTransitionDamageFXV2ModuleData();
	UnsignedInt interval = modData->m_checkIntervalFrames > 0 ? modData->m_checkIntervalFrames : 1;

	Object *obj = getObject();
	BodyModuleInterface *body = obj->getBodyModule();
	if( body == nullptr )
		return UPDATE_SLEEP( interval );

	BodyDamageType curState = body->getDamageState();
	if( curState != m_lastKnownState )
	{
		BodyDamageType oldState = m_lastKnownState;
		m_lastKnownState = curState;

		// If we are contained, the hit that caused this lives on the container -- use it for the
		// damage-type filters and the OCL damage source.
		const DamageInfo *relayInfo = nullptr;
		Object *container = obj->getContainedBy();
		if( container && container->getBodyModule() )
			relayInfo = container->getBodyModule()->getLastDamageInfo();

		m_relayDamageInfo = relayInfo;
		doTransitionEffects( relayInfo, oldState, curState );
		m_relayDamageInfo = nullptr;
	}

	return UPDATE_SLEEP( interval );
}

//-------------------------------------------------------------------------------------------------
/** Switching damage states -- copy of TransitionDamageFX::onBodyDamageStateChange(), except that
	* the damage-type filter uses the relayed (container's) DamageInfo when one is being relayed. */
//-------------------------------------------------------------------------------------------------
void ContainedTransitionDamageFXV2::doTransitionEffects( const DamageInfo* damageInfo,
																												 BodyDamageType oldState,
																												 BodyDamageType newState )
{
	Object *damageSource = nullptr;
	Int i;
	Drawable *draw = getObject()->getDrawable();
	const ContainedTransitionDamageFXV2ModuleData *modData = getContainedTransitionDamageFXV2ModuleData();

	// get the source of the damage if present
	if( damageInfo )
		damageSource = TheGameLogic->findObjectByID( damageInfo->in.m_sourceID );

	// remove any particle systems that might be emitting from our old state
	for( i = 0; i < DAMAGE_MODULE_MAX_FX; i++ )
	{

		if( m_particleSystemID[ oldState ][ i ] != INVALID_PARTICLE_SYSTEM_ID )
		{

			TheParticleSystemManager->destroyParticleSystemByID( m_particleSystemID[ oldState ][ i ] );
			m_particleSystemID[ oldState ][ i ] = INVALID_PARTICLE_SYSTEM_ID;

		}

	}

	//
	// when we are transitioning to a "worse" state we will play a set of effects for that
	// new state to make the transition
	//
	if( IS_CONDITION_WORSE( newState, oldState ) )
	{
		const ParticleSystemTemplate *pSystemT;
		Coord3D pos;

		// if we are restricted by the damage type executing effect, bail out of here
		const DamageInfo *lastDamageInfo = m_relayDamageInfo;
		if( lastDamageInfo == nullptr && getObject()->getBodyModule() )
			lastDamageInfo = getObject()->getBodyModule()->getLastDamageInfo();

		for( i = 0; i < DAMAGE_MODULE_MAX_FX; i++ )
		{

			// play fx list for our new state
			if( modData->m_fxList[ newState ][ i ].fx )
			{

				if( lastDamageInfo == nullptr ||
						getDamageTypeFlag( modData->m_damageFXTypes, lastDamageInfo->in.m_damageType ) )
				{

					pos = getLocalEffectPosV2( &modData->m_fxList[ newState ][ i ].locInfo, draw );
					getObject()->convertBonePosToWorldPos( &pos, nullptr, &pos, nullptr );
					FXList::doFXPos( modData->m_fxList[ newState ][ i ].fx, &pos );

				}

			}

			// do any object creation list for our new state
			if( damageSource && modData->m_OCL[ newState ][ i ].ocl )
			{

				if( lastDamageInfo == nullptr ||
						getDamageTypeFlag( modData->m_damageOCLTypes, lastDamageInfo->in.m_damageType ) )
				{

					pos = getLocalEffectPosV2( &modData->m_OCL[ newState ][ i ].locInfo, draw );
					getObject()->convertBonePosToWorldPos( &pos, nullptr, &pos, nullptr );
					ObjectCreationList::create( modData->m_OCL[ newState ][ i ].ocl,
																			getObject(), &pos, damageSource->getPosition(), INVALID_ANGLE );

				}

			}

			// get the template of the system to create
			pSystemT = modData->m_particleSystem[ newState ][ i ].particleSysTemplate;
			if( pSystemT )
			{

				if( lastDamageInfo == nullptr ||
						getDamageTypeFlag( modData->m_damageParticleTypes, lastDamageInfo->in.m_damageType ) )
				{
#if RETAIL_COMPATIBLE_CRC
					// keep the same logic-random side effect TransitionDamageFX has in retail-compatible builds
					getLocalEffectPosV2( &modData->m_particleSystem[ newState ][ i ].locInfo, draw, LogicRandomValueClass() );
#endif

					// create a new particle system based on the template provided
					ParticleSystem* pSystem = TheParticleSystemManager->createParticleSystem( pSystemT );
					if( pSystem )
					{

						// get the what is the position we're going to played the effect at
						pos = getLocalEffectPosV2( &modData->m_particleSystem[ newState ][ i ].locInfo, draw, ClientRandomValueClass() );

						// position is local to the object; attachToObject handles the world transform
						pSystem->setPosition( &pos );

						// attach to object
						pSystem->attachToObject( getObject() );

						// save the id of this particle system so we can remove it later if it still exists
						m_particleSystemID[ newState ][ i ] = pSystem->getSystemID();

					}

				}

			}

		}

	}

}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void ContainedTransitionDamageFXV2::crc( Xfer *xfer )
{

	// extend base class
	UpdateModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void ContainedTransitionDamageFXV2::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpdateModule::xfer( xfer );

	// particle systems ids
	xfer->xferUser( m_particleSystemID, sizeof( ParticleSystemID ) * BODYDAMAGETYPE_COUNT * DAMAGE_MODULE_MAX_FX );

	// last state we played effects for (so a load doesn't replay a transition)
	xfer->xferUser( &m_lastKnownState, sizeof( BodyDamageType ) );

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void ContainedTransitionDamageFXV2::loadPostProcess()
{

	// extend base class
	UpdateModule::loadPostProcess();

	m_relayDamageInfo = nullptr;

}
