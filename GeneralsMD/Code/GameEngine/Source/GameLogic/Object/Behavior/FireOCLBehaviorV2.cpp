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

// FILE: FireOCLBehaviorV2.cpp ///////////////////////////////////////////////////////////////////////
//
//	GeneralsMod @feature Dimitar 08/09/2026
//	See FireOCLBehaviorV2.h. This is GrantStealthBehavior.cpp's update()/scan logic verbatim, with
//	grantStealthToObject() replaced by fireOCLAtObject(), which fires the configured OCL instead.
//	Uses the ObjectCreationList::create(ocl, primary, secondary) overload with the AFFECTED unit as
//	primary -- so each matching unit behaves as if it fired the OCL itself: the OCL spawns at that
//	unit's own position/orientation, and that unit (not this short-lived trigger object) supplies team
//	ownership and becomes the "master" for anything the OCL creates that cares about one (e.g. a
//	spawned unit with SlavedUpdate). The trigger object itself is never a valid master -- it typically
//	self-destructs within a few frames of firing.
//	Because the scan radius is cumulative (grows each frame, but always queried from 0), the same
//	object would otherwise be re-found and re-fired on every remaining frame of the sweep; m_firedObjectIDs
//	makes sure each matching object only ever triggers the OCL once.
//
///////////////////////////////////////////////////////////////////////////////////////////////////


// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine
#include <algorithm>
#include "Common/Thing.h"
#include "Common/ThingTemplate.h"
#include "Common/INI.h"
#include "Common/Player.h"
#include "Common/Xfer.h"
#include "GameClient/ParticleSys.h"
#include "GameLogic/Module/FireOCLBehaviorV2.h"
#include "GameLogic/ObjectCreationList.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"
#include "GameLogic/PartitionManager.h"


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
FireOCLBehaviorV2::FireOCLBehaviorV2( Thing *thing, const ModuleData* moduleData ) : UpdateModule( thing, moduleData )
{
	const FireOCLBehaviorV2ModuleData *d = getFireOCLBehaviorV2ModuleData();

	m_radiusParticleSystemID = INVALID_PARTICLE_SYSTEM_ID;

	m_currentScanRadius = d->m_startRadius;

	setWakeFrame( getObject(), UPDATE_SLEEP_NONE );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
FireOCLBehaviorV2::~FireOCLBehaviorV2()
{

	if( m_radiusParticleSystemID != INVALID_PARTICLE_SYSTEM_ID )
		TheParticleSystemManager->destroyParticleSystemByID( m_radiusParticleSystemID );

}



//-------------------------------------------------------------------------------------------------
/** The update callback. */
//-------------------------------------------------------------------------------------------------
UpdateSleepTime FireOCLBehaviorV2::update()
{

	Object *self = getObject();

	if ( self->isEffectivelyDead())
		return UPDATE_SLEEP_FOREVER;

	// TheSuperHackers @bugfix stephanmeesters 18/04/2026 Delay emitter creation until update, to ensure that the particle
	// systems are not created before ParticleManager has xfer-loaded.
	createEmitters();

	const FireOCLBehaviorV2ModuleData *d = getFireOCLBehaviorV2ModuleData();
	// setup scan filters
	PartitionFilterRelationship relationship( self, PartitionFilterRelationship::ALLOW_ALLIES );
	PartitionFilterSameMapStatus filterMapStatus( self );
	PartitionFilterAlive filterAlive;
	PartitionFilter *filters[] = { &relationship, &filterAlive, &filterMapStatus, nullptr };


	m_currentScanRadius += d->m_radiusGrowRate;

	Bool thisIsFinalScan = FALSE;
	if ( m_currentScanRadius >=  d->m_finalRadius )
	{
		m_currentScanRadius = d->m_finalRadius;
		thisIsFinalScan = TRUE;
	}

	// scan objects in our region
	ObjectIterator *iter = ThePartitionManager->iterateObjectsInRange( self->getPosition(), m_currentScanRadius, FROM_CENTER_2D, filters );
	MemoryPoolObjectHolder hold( iter );
	// FIRE THE OCL FOR EVERY FRIENDLY IN RADIUS
	for( Object *obj = iter->first(); obj; obj = iter->next() )
		fireOCLAtObject( obj );

	if ( thisIsFinalScan )
	{

		TheGameLogic->destroyObject( self );
		return UPDATE_SLEEP_FOREVER;
	}

	return UPDATE_SLEEP_NONE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void FireOCLBehaviorV2::fireOCLAtObject( Object *obj )
{

	if ( obj == getObject() )
		return;

	const FireOCLBehaviorV2ModuleData *d = getFireOCLBehaviorV2ModuleData();
	if ( ! obj->isKindOfMulti( d->m_kindOf, d->m_forbiddenKindOf ) )
		return;

	// GeneralsMod @feature Dimitar 08/09/2026: the cumulative radius scan re-finds the same object on
	// later frames of the sweep -- only fire once per object for this trigger's whole lifetime.
	ObjectID objID = obj->getID();
	if ( std::find( m_firedObjectIDs.begin(), m_firedObjectIDs.end(), objID ) != m_firedObjectIDs.end() )
		return;
	m_firedObjectIDs.push_back( objID );

	// GeneralsMod @feature Dimitar 08/09/2026: obj (the affected unit) is both the position/orientation
	// source AND the team-ownership/master source -- i.e. obj behaves as though it fired the OCL itself.
	if ( d->m_ocl )
		ObjectCreationList::create( d->m_ocl, obj, nullptr );

}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void FireOCLBehaviorV2::crc( Xfer *xfer )
{

	// extend base class
	UpdateModule::crc( xfer );


}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void FireOCLBehaviorV2::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpdateModule::xfer( xfer );


	// particle system id
	xfer->xferUser( &m_radiusParticleSystemID, sizeof( ParticleSystemID ) );

	// Timer safety
	xfer->xferReal( &m_currentScanRadius );

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void FireOCLBehaviorV2::loadPostProcess()
{

	// extend base class
	UpdateModule::loadPostProcess();

}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void FireOCLBehaviorV2::createEmitters()
{
	if( m_radiusParticleSystemID == INVALID_PARTICLE_SYSTEM_ID )
	{
		const FireOCLBehaviorV2ModuleData *d = getFireOCLBehaviorV2ModuleData();
		ParticleSystem *particleSystem = TheParticleSystemManager->createParticleSystem(d->m_radiusParticleSystemTmpl);
		if( particleSystem )
		{
			particleSystem->setPosition( getObject()->getPosition() );
			m_radiusParticleSystemID = particleSystem->getSystemID();
		}
	}
}
