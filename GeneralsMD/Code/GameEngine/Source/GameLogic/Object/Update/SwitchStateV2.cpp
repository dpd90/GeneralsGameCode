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

// FILE: SwitchStateV2.cpp ////////////////////////////////////////////////////////////////////////
// Desc:   See SwitchStateV2.h.
///////////////////////////////////////////////////////////////////////////////////////////////////

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#define DEFINE_LOCOMOTORSET_NAMES //Gain access to TheLocomotorSetNames[]

#include "Common/INI.h"
#include "Common/Xfer.h"

#include "GameClient/ControlBar.h"
#include "GameClient/FXList.h"

#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"
#include "GameLogic/ObjectCreationList.h"
#include "GameLogic/Weapon.h"
#include "GameLogic/WeaponStatus.h"

#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/BodyModule.h"
#include "GameLogic/Module/SwitchStateV2.h"

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
SwitchStateInfo::SwitchStateInfo()
{
	m_modelCondition = MODELCONDITION_INVALID;
	m_weaponSetFlag = (WeaponSetType)0;
	m_armorSetFlag = (ArmorSetType)0;
	m_objectStatusType = (ObjectStatusType)0;
	m_commandSet = AsciiString::TheEmptyString;
	m_locomotorSetType = (LocomotorSetType)0;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
SwitchStateV2ModuleData::SwitchStateV2ModuleData()
{
	m_specialPowerTemplate = nullptr;
	m_conditionState = MODELCONDITION_INVALID;
	m_lifetimeFrames = 0;
	m_weaponInTemplate = nullptr;
	m_weaponOutTemplate = nullptr;
	m_oclIn = nullptr;
	m_oclOut = nullptr;
	m_fxListIn = nullptr;
	m_fxListOut = nullptr;
}

// ------------------------------------------------------------------------------------------------
// Reads one full SwitchStateInfo off a single INI line: ModelConditionState WeaponSetCondition
// ArmorSetCondition StatusBitType CommandSetEntry LocomotorSetType, in that order. Mirrors
// RiderChangeContainModuleData::parseRiderInfo.
// ------------------------------------------------------------------------------------------------
/*static*/ void SwitchStateV2ModuleData::parseStateInfo( INI* ini, void *instance, void *store, const void* /*userData*/ )
{
	SwitchStateInfo* info = (SwitchStateInfo*)store;

	// Model condition state
	INI::parseIndexList( ini, instance, &(info->m_modelCondition), ModelConditionFlags::getBitNames() );

	// Weapon set condition
	INI::parseIndexList( ini, instance, &(info->m_weaponSetFlag), WeaponSetFlags::getBitNames() );

	// Armor set condition
	INI::parseIndexList( ini, instance, &(info->m_armorSetFlag), ArmorSetFlags::getBitNames() );

	// Status bit
	INI::parseIndexList( ini, instance, &(info->m_objectStatusType), ObjectStatusMaskType::getBitNames() );

	// Command set override (entry from CommandSet.ini)
	const char* name = ini->getNextToken();
	info->m_commandSet.format( name );

	// Locomotor set
	info->m_locomotorSetType = (LocomotorSetType)INI::scanIndexList( ini->getNextToken(), TheLocomotorSetNames );
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
/*static*/ void SwitchStateV2ModuleData::buildFieldParse(MultiIniFieldParse& p)
{
	UpdateModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "SpecialPowerTemplate",	INI::parseSpecialPowerTemplate,	nullptr, offsetof( SwitchStateV2ModuleData, m_specialPowerTemplate ) },
		{ "DefaultState",					parseStateInfo,										nullptr, offsetof( SwitchStateV2ModuleData, m_defaultState ) },
		{ "AlteredState",					parseStateInfo,										nullptr, offsetof( SwitchStateV2ModuleData, m_alteredState ) },
		{ "ConditionStateType",		INI::parseIndexList,	ModelConditionFlags::getBitNames(), offsetof( SwitchStateV2ModuleData, m_conditionState ) },
		{ "Lifetime",							INI::parseDurationUnsignedInt,		nullptr, offsetof( SwitchStateV2ModuleData, m_lifetimeFrames ) },
		{ "WeaponIn",							INI::parseWeaponTemplate,				nullptr, offsetof( SwitchStateV2ModuleData, m_weaponInTemplate ) },
		{ "WeaponOut",						INI::parseWeaponTemplate,				nullptr, offsetof( SwitchStateV2ModuleData, m_weaponOutTemplate ) },
		{ "OCLIn",								INI::parseObjectCreationList,		nullptr, offsetof( SwitchStateV2ModuleData, m_oclIn ) },
		{ "OCLOut",								INI::parseObjectCreationList,		nullptr, offsetof( SwitchStateV2ModuleData, m_oclOut ) },
		{ "FXListIn",							INI::parseFXList,								nullptr, offsetof( SwitchStateV2ModuleData, m_fxListIn ) },
		{ "FXListOut",						INI::parseFXList,								nullptr, offsetof( SwitchStateV2ModuleData, m_fxListOut ) },
		{ nullptr, nullptr, nullptr, 0 }
	};
	p.add(dataFieldParse);
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
SwitchStateV2::SwitchStateV2( Thing *thing, const ModuleData *moduleData ) : SpecialPowerUpdateModule( thing, moduleData )
{
	m_isAltered = FALSE;
	m_revertFrame = 0;

	const SwitchStateV2ModuleData *data = getSwitchStateV2ModuleData();

	m_weaponIn = nullptr;
	if( data->m_weaponInTemplate )
	{
		m_weaponIn = TheWeaponStore->allocateNewWeapon( data->m_weaponInTemplate, PRIMARY_WEAPON );
		m_weaponIn->loadAmmoNow( getObject() );
	}

	m_weaponOut = nullptr;
	if( data->m_weaponOutTemplate )
	{
		m_weaponOut = TheWeaponStore->allocateNewWeapon( data->m_weaponOutTemplate, PRIMARY_WEAPON );
		m_weaponOut->loadAmmoNow( getObject() );
	}
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
SwitchStateV2::~SwitchStateV2()
{
	deleteInstance( m_weaponIn );
	deleteInstance( m_weaponOut );
}

// ------------------------------------------------------------------------------------------------
/** Apply every property of the target state. For the flag-style properties (model condition,
	* weapon set, armor set, status) we clear whatever the OTHER state would have set first, so
	* nothing lingers if DefaultState and AlteredState differ there. Command set override and
	* locomotor set are plain selectors, so they're just re-applied directly. */
// ------------------------------------------------------------------------------------------------
void SwitchStateV2::applyState( Bool goToAltered )
{
	const SwitchStateV2ModuleData *data = getSwitchStateV2ModuleData();
	Object *obj = getObject();

	// GeneralsMod @feature Dimitar 14/09/2026: ConditionStateType, when given, REPLACES
	// DefaultState/AlteredState entirely -- toggle just the one flag and return, completely ignoring
	// model condition/weapon set/armor set/status bit/command set/locomotor set from both states.
	if( data->m_conditionState != MODELCONDITION_INVALID )
	{
		if( goToAltered )
			obj->setModelConditionState( data->m_conditionState );
		else
			obj->clearModelConditionFlags( MAKE_MODELCONDITION_MASK( data->m_conditionState ) );

		m_isAltered = goToAltered;
		return;
	}

	const SwitchStateInfo &target = goToAltered ? data->m_alteredState : data->m_defaultState;
	const SwitchStateInfo &previous = goToAltered ? data->m_defaultState : data->m_alteredState;

	// Model condition
	if( previous.m_modelCondition != target.m_modelCondition )
	{
		if( previous.m_modelCondition != MODELCONDITION_INVALID )
			obj->clearModelConditionFlags( MAKE_MODELCONDITION_MASK( previous.m_modelCondition ) );
		if( target.m_modelCondition != MODELCONDITION_INVALID )
			obj->setModelConditionState( target.m_modelCondition );
	}

	// Weapon set
	if( previous.m_weaponSetFlag != target.m_weaponSetFlag )
	{
		obj->clearWeaponSetFlag( previous.m_weaponSetFlag );
		obj->setWeaponSetFlag( target.m_weaponSetFlag );
	}

	// Armor set
	BodyModuleInterface *body = obj->getBodyModule();
	if( body && previous.m_armorSetFlag != target.m_armorSetFlag )
	{
		body->clearArmorSetFlag( previous.m_armorSetFlag );
		body->setArmorSetFlag( target.m_armorSetFlag );
	}

	// Status bit
	if( previous.m_objectStatusType != target.m_objectStatusType )
	{
		obj->clearStatus( MAKE_OBJECT_STATUS_MASK( previous.m_objectStatusType ) );
		obj->setStatus( MAKE_OBJECT_STATUS_MASK( target.m_objectStatusType ) );
	}

	// Command set override
	obj->setCommandSetStringOverride( target.m_commandSet );
	TheControlBar->markUIDirty();	// Refresh the UI in case we are selected

	// Locomotor set
	AIUpdateInterface *ai = obj->getAI();
	if( ai )
		ai->chooseLocomotorSet( target.m_locomotorSetType );

	m_isAltered = goToAltered;
}

// ------------------------------------------------------------------------------------------------
/** Fires WeaponIn/OCLIn/FXListIn once (at our own position). Called whenever we switch TO
	* AlteredState -- both from the manual button (initiateIntentToDoSpecialPower) and from automatic
	* damage-driven triggers (notifyQualifyingDamage). Each is independently optional and safe to
	* call redundantly: the weapon naturally rate-limits to its own reload time, and OCL/FXList are
	* simply skipped if not configured. */
// ------------------------------------------------------------------------------------------------
void SwitchStateV2::fireInEffects()
{
	const SwitchStateV2ModuleData *data = getSwitchStateV2ModuleData();
	Object *obj = getObject();

	if( m_weaponIn && m_weaponIn->getStatus() == READY_TO_FIRE )
		m_weaponIn->forceFireWeapon( obj, obj->getPosition() );

	if( data->m_oclIn )
		ObjectCreationList::create( data->m_oclIn, obj, nullptr );

	if( data->m_fxListIn )
		FXList::doFXObj( data->m_fxListIn, obj );
}

// ------------------------------------------------------------------------------------------------
/** Fires WeaponOut/OCLOut/FXListOut once (at our own position). Called whenever we switch back TO
	* DefaultState -- from the manual button toggling back, or from Lifetime auto-expiry in update(). */
// ------------------------------------------------------------------------------------------------
void SwitchStateV2::fireOutEffects()
{
	const SwitchStateV2ModuleData *data = getSwitchStateV2ModuleData();
	Object *obj = getObject();

	if( m_weaponOut && m_weaponOut->getStatus() == READY_TO_FIRE )
		m_weaponOut->forceFireWeapon( obj, obj->getPosition() );

	if( data->m_oclOut )
		ObjectCreationList::create( data->m_oclOut, obj, nullptr );

	if( data->m_fxListOut )
		FXList::doFXObj( data->m_fxListOut, obj );
}

// ------------------------------------------------------------------------------------------------
/** Called by our companion SwitchStateV2Activate module (via the base SpecialPowerModule's
	* doSpecialPower -> initiateIntentToDoSpecialPower hand-off) whenever the button is pressed.
	* This is a pure self-toggle -- no target object, position, or waypoint is used. */
// ------------------------------------------------------------------------------------------------
Bool SwitchStateV2::initiateIntentToDoSpecialPower( const SpecialPowerTemplate *specialPowerTemplate,
																										 const Object * /*targetObj*/,
																										 const Coord3D * /*targetPos*/,
																										 const Waypoint * /*way*/,
																										 UnsignedInt /*commandOptions*/ )
{
	const SwitchStateV2ModuleData *data = getSwitchStateV2ModuleData();

	// Make sure this call is actually meant for us -- an object can have more than one special power.
	if( specialPowerTemplate != data->m_specialPowerTemplate )
		return FALSE;

	Bool goingToAltered = !m_isAltered;
	applyState( goingToAltered );

	// Fire the effects for whichever direction we just switched.
	if( goingToAltered )
		fireInEffects();
	else
		fireOutEffects();

	if( goingToAltered && data->m_lifetimeFrames > 0 )
	{
		// Schedule the automatic revert and make sure our update() actually gets called again to check it.
		m_revertFrame = TheGameLogic->getFrame() + data->m_lifetimeFrames;
		setWakeFrame( getObject(), frameToSleepTime( m_revertFrame ) );
	}
	else
	{
		// Either we just went back to DefaultState, or Lifetime is 0 (permanent) -- nothing to schedule.
		m_revertFrame = 0;
	}

	return TRUE;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
/** Called by a companion SwitchStateWhenDamagedBehaviorV2 module whenever its damage condition is
	* met. See the declaration comment in SwitchStateV2.h for the full semantics -- in short: drives
	* us toward AlteredState (never toggles back to Default), and if Lifetime is configured, refreshes
	* the revert countdown every time this is called instead of only on the first call. */
// ------------------------------------------------------------------------------------------------
Bool SwitchStateV2::notifyQualifyingDamage( const SpecialPowerTemplate *specialPowerTemplate )
{
	const SwitchStateV2ModuleData *data = getSwitchStateV2ModuleData();

	// Make sure this call is actually meant for us -- an object can have more than one special power.
	if( specialPowerTemplate != data->m_specialPowerTemplate )
		return FALSE;

	// Permanent config (no Lifetime): this is a one-time switch. Once Altered, further qualifying
	// damage is a no-op -- we never toggle back to Default automatically here, and there's no revert
	// countdown to refresh.
	if( data->m_lifetimeFrames == 0 && m_isAltered )
		return FALSE;

	applyState( TRUE );	// idempotent if we're already Altered -- just re-affirms it

	// Fire the entry effects, same as the manual activation path. WeaponIn naturally rate-limits
	// to its own reload time even under sustained qualifying damage; OCLIn/FXListIn, if configured,
	// simply fire again every call -- keep that in mind if repeated qualifying damage is expected.
	fireInEffects();

	if( data->m_lifetimeFrames > 0 )
	{
		// (Re)start the revert countdown from now -- whether this is the call that first switched us
		// to Altered, or a refresh while we were already Altered.
		m_revertFrame = TheGameLogic->getFrame() + data->m_lifetimeFrames;
		setWakeFrame( getObject(), frameToSleepTime( m_revertFrame ) );
	}

	return TRUE;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
UpdateSleepTime SwitchStateV2::update()
{
	if( m_revertFrame != 0 )
	{
		if( TheGameLogic->getFrame() >= m_revertFrame )
		{
			applyState( FALSE );	// time's up -- revert to DefaultState automatically
			fireOutEffects();
			m_revertFrame = 0;
			return UPDATE_SLEEP_FOREVER;
		}
		return frameToSleepTime( m_revertFrame );
	}
	return UPDATE_SLEEP_FOREVER;
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void SwitchStateV2::crc( Xfer *xfer )
{
	// extend base class
	SpecialPowerUpdateModule::crc( xfer );
}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version
	* 2: added optional Weapon field (GeneralsMod)
	* 3: GeneralsMod -- replaced the single Weapon field with WeaponIn/WeaponOut (paired with new
	*    OCLIn/OCLOut/FXListIn/FXListOut fields), matching ShieldGeneratorUpdateV2's entry/exit
	*    effect convention. Does NOT preserve compatibility with saves made under versions 1-2. */
// ------------------------------------------------------------------------------------------------
void SwitchStateV2::xfer( Xfer *xfer )
{
	// version
	XferVersion currentVersion = 3;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	SpecialPowerUpdateModule::xfer( xfer );

	xfer->xferBool( &m_isAltered );
	xfer->xferUnsignedInt( &m_revertFrame );

	xfer->xferSnapshot( m_weaponIn );
	xfer->xferSnapshot( m_weaponOut );
}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void SwitchStateV2::loadPostProcess()
{
	// extend base class
	SpecialPowerUpdateModule::loadPostProcess();
}
