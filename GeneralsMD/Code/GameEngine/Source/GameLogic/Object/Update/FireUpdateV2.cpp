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

// FILE: FireUpdateV2.cpp /////////////////////////////////////////////////////////////////////////
//	GeneralsMod @feature Dimitar 11/11/2026
//	FireWeaponUpdate + FX + OCL, all at the unit's geometric center. See FireUpdateV2.h.
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/Xfer.h"
#include "GameClient/FXList.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"
#include "GameLogic/ObjectCreationList.h"
#include "GameLogic/WeaponStatus.h"
#include "GameLogic/Module/FireUpdateV2.h"

//-------------------------------------------------------------------------------------------------
FireUpdateV2ModuleData::FireUpdateV2ModuleData()
{
	m_weaponTemplate = nullptr;
	m_fxList = nullptr;
	m_ocl = nullptr;
	m_initialDelayFrames = 0;
	m_exclusiveWeaponDelay = 0;
	m_rateFrames = 0;
}

//-------------------------------------------------------------------------------------------------
/*static*/ void FireUpdateV2ModuleData::buildFieldParse(MultiIniFieldParse& p)
{
	UpdateModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "Weapon",					INI::parseWeaponTemplate,			nullptr, offsetof( FireUpdateV2ModuleData, m_weaponTemplate ) },
		{ "FX",						INI::parseFXList,					nullptr, offsetof( FireUpdateV2ModuleData, m_fxList ) },
		{ "OCL",					INI::parseObjectCreationList,		nullptr, offsetof( FireUpdateV2ModuleData, m_ocl ) },
		{ "InitialDelay",			INI::parseDurationUnsignedInt,		nullptr, offsetof( FireUpdateV2ModuleData, m_initialDelayFrames ) },
		{ "Rate",					INI::parseDurationUnsignedInt,		nullptr, offsetof( FireUpdateV2ModuleData, m_rateFrames ) },
		{ "ExclusiveWeaponDelay",	INI::parseDurationUnsignedInt,		nullptr, offsetof( FireUpdateV2ModuleData, m_exclusiveWeaponDelay ) },
		{ nullptr, nullptr, nullptr, 0 }
	};
	p.add(dataFieldParse);
}

//-------------------------------------------------------------------------------------------------
FireUpdateV2::FireUpdateV2( Thing *thing, const ModuleData* moduleData ) :
	UpdateModule( thing, moduleData ),
	m_weapon(nullptr),
	m_effectsFiredOnce(FALSE)
{
	const FireUpdateV2ModuleData *data = getFireUpdateV2ModuleData();
	if (data->m_weaponTemplate)
	{
		m_weapon = TheWeaponStore->allocateNewWeapon(data->m_weaponTemplate, PRIMARY_WEAPON);
		m_weapon->loadAmmoNow( getObject() );
	}

	m_initialDelayFrame = TheGameLogic->getFrame() + data->m_initialDelayFrames;
	m_nextEffectFrame = m_initialDelayFrame;
}

//-------------------------------------------------------------------------------------------------
FireUpdateV2::~FireUpdateV2()
{
	deleteInstance(m_weapon);
}

//-------------------------------------------------------------------------------------------------
void FireUpdateV2::getCenter( Coord3D& center )
{
	const Object *me = getObject();
	me->getGeometryInfo().getCenterPosition( *me->getPosition(), center );
}

//-------------------------------------------------------------------------------------------------
void FireUpdateV2::doEffects( const Coord3D& center )
{
	const FireUpdateV2ModuleData *data = getFireUpdateV2ModuleData();
	Object *me = getObject();

	if( data->m_fxList )
		FXList::doFXPos( data->m_fxList, &center, me->getTransformMatrix() );

	if( data->m_ocl )
		ObjectCreationList::create( data->m_ocl, me, &center, nullptr, me->getOrientation() );
}

//-------------------------------------------------------------------------------------------------
UpdateSleepTime FireUpdateV2::update()
{
	const FireUpdateV2ModuleData *data = getFireUpdateV2ModuleData();
	UnsignedInt now = TheGameLogic->getFrame();

	if( now < m_initialDelayFrame )
		return UPDATE_SLEEP_NONE;

	if( !isOkayToFire() )
		return UPDATE_SLEEP_NONE;

	Coord3D center;
	getCenter( center );

	// Weapon: fires whenever its own reload allows
	Bool weaponFired = FALSE;
	if( m_weapon && m_weapon->getStatus() == READY_TO_FIRE )
	{
		m_weapon->forceFireWeapon( getObject(), &center );
		weaponFired = TRUE;
	}

	// FX / OCL
	if( data->m_fxList || data->m_ocl )
	{
		if( data->m_rateFrames > 0 )
		{
			if( now >= m_nextEffectFrame )
			{
				doEffects( center );
				m_nextEffectFrame = now + data->m_rateFrames;
			}
		}
		else if( m_weapon )
		{
			if( weaponFired )
				doEffects( center );
		}
		else if( !m_effectsFiredOnce )
		{
			doEffects( center );
			m_effectsFiredOnce = TRUE;
		}
	}

	return UPDATE_SLEEP_NONE;
}

//-------------------------------------------------------------------------------------------------
Bool FireUpdateV2::isOkayToFire()
{
	const Object *me = getObject();
	const FireUpdateV2ModuleData *data = getFireUpdateV2ModuleData();

	if( me->testStatus(OBJECT_STATUS_UNDER_CONSTRUCTION) )
		return FALSE; // no hitting with a 0% building, cheater

	// Firing a real weapon suppresses this module
	if( data->m_exclusiveWeaponDelay > 0 && ( TheGameLogic->getFrame() < (me->getLastShotFiredFrame() + data->m_exclusiveWeaponDelay) ) )
		return FALSE;

	return TRUE;
}

// ------------------------------------------------------------------------------------------------
void FireUpdateV2::crc( Xfer *xfer )
{
	UpdateModule::crc( xfer );
}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void FireUpdateV2::xfer( Xfer *xfer )
{
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	UpdateModule::xfer( xfer );

	Bool hasWeapon = (m_weapon != nullptr);
	xfer->xferBool( &hasWeapon );
	if( hasWeapon && m_weapon )
		xfer->xferSnapshot( m_weapon );

	xfer->xferUnsignedInt( &m_initialDelayFrame );
	xfer->xferUnsignedInt( &m_nextEffectFrame );
	xfer->xferBool( &m_effectsFiredOnce );
}

// ------------------------------------------------------------------------------------------------
void FireUpdateV2::loadPostProcess()
{
	UpdateModule::loadPostProcess();
}
