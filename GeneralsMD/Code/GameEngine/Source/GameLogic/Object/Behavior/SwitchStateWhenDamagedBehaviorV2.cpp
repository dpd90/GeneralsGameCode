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

// FILE: SwitchStateWhenDamagedBehaviorV2.cpp /////////////////////////////////////////////////////
// Desc:   See SwitchStateWhenDamagedBehaviorV2.h.
///////////////////////////////////////////////////////////////////////////////////////////////////

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/INI.h"
#include "Common/Xfer.h"

#include "GameLogic/Object.h"

#include "GameLogic/Module/SwitchStateWhenDamagedBehaviorV2.h"
#include "GameLogic/Module/SpecialPowerUpdateModule.h"

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
SwitchStateWhenDamagedBehaviorV2ModuleData::SwitchStateWhenDamagedBehaviorV2ModuleData()
{
	m_initiallyActive = TRUE;	// no StartsActive/TriggeredBy given at all == always active, same default FireWeaponWhenDamagedBehavior uses
	m_specialPowerTemplate = nullptr;
	m_damageTypes = DAMAGE_TYPE_FLAGS_ALL;
	m_damageAmount = 0.0f;
	m_greaterOrEqual = TRUE;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
/*static*/ void SwitchStateWhenDamagedBehaviorV2ModuleData::buildFieldParse(MultiIniFieldParse& p)
{
	DamageModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "StartsActive",				INI::parseBool,										nullptr, offsetof( SwitchStateWhenDamagedBehaviorV2ModuleData, m_initiallyActive ) },
		{ "SpecialPowerTemplate",	INI::parseSpecialPowerTemplate,	nullptr, offsetof( SwitchStateWhenDamagedBehaviorV2ModuleData, m_specialPowerTemplate ) },
		{ "DamageTypes",					INI::parseDamageTypeFlags,				nullptr, offsetof( SwitchStateWhenDamagedBehaviorV2ModuleData, m_damageTypes ) },
		{ "DamageAmount",					INI::parseReal,										nullptr, offsetof( SwitchStateWhenDamagedBehaviorV2ModuleData, m_damageAmount ) },
		{ "GreaterOrEqual",				INI::parseBool,										nullptr, offsetof( SwitchStateWhenDamagedBehaviorV2ModuleData, m_greaterOrEqual ) },
		{ nullptr, nullptr, nullptr, 0 }
	};
	p.add(dataFieldParse);

	// TriggeredBy / ConflictsWith / RemovesUpgrades / FXListUpgrade / RequiresAllTriggers -- see UpgradeModule.h
	p.add(UpgradeMuxData::getFieldParse(), offsetof( SwitchStateWhenDamagedBehaviorV2ModuleData, m_upgradeMuxData ));
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
SwitchStateWhenDamagedBehaviorV2::SwitchStateWhenDamagedBehaviorV2( Thing *thing, const ModuleData* moduleData ) : DamageModule( thing, moduleData )
{
	if( getSwitchStateWhenDamagedBehaviorV2ModuleData()->m_initiallyActive )
	{
		giveSelfUpgrade();
	}
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
SwitchStateWhenDamagedBehaviorV2::~SwitchStateWhenDamagedBehaviorV2()
{
}

// ------------------------------------------------------------------------------------------------
/** Damage has been dealt -- check whether it qualifies, and if so hand off to whichever companion
	* module (matched by SpecialPowerTemplate) implements SpecialPowerUpdateInterface. In practice
	* that's the object's SwitchStateV2 module; see SwitchStateWhenDamagedBehaviorV2.h for the intended
	* INI pairing. */
// ------------------------------------------------------------------------------------------------
void SwitchStateWhenDamagedBehaviorV2::onDamage( DamageInfo *damageInfo )
{
	if( !isUpgradeActive() )
		return;

	const SwitchStateWhenDamagedBehaviorV2ModuleData *data = getSwitchStateWhenDamagedBehaviorV2ModuleData();

	if( data->m_specialPowerTemplate == nullptr )
		return;

	// right type?
	if( !getDamageTypeFlag( data->m_damageTypes, damageInfo->in.m_damageType ) )
		return;

	// right amount? (use actual [post-armor] damage dealt, same field FireWeaponWhenDamagedBehavior uses)
	Real actualDamage = damageInfo->out.m_actualDamageDealt;
	if( data->m_greaterOrEqual )
	{
		if( actualDamage < data->m_damageAmount )
			return;
	}
	else
	{
		if( actualDamage > data->m_damageAmount )
			return;
	}

	// Find the companion module for our SpecialPowerTemplate and let it react. This is the same
	// lookup SpecialPowerModule::initiateIntentToDoSpecialPower() uses to find the module a command
	// button hands off to -- we're just an automatic trigger using the same mechanism.
	Object *obj = getObject();
	for( BehaviorModule** u = obj->getBehaviorModules(); *u; ++u )
	{
		SpecialPowerUpdateInterface *spu = (*u)->getSpecialPowerUpdateInterface();
		if( spu && spu->notifyQualifyingDamage( data->m_specialPowerTemplate ) )
			break;
	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void SwitchStateWhenDamagedBehaviorV2::crc( Xfer *xfer )
{
	// extend base class
	DamageModule::crc( xfer );

	// extend upgrade mux
	UpgradeMux::upgradeMuxCRC( xfer );
}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version
	* 2: added UpgradeMux (StartsActive/TriggeredBy/etc.) support (GeneralsMod) */
// ------------------------------------------------------------------------------------------------
void SwitchStateWhenDamagedBehaviorV2::xfer( Xfer *xfer )
{
	// version
	XferVersion currentVersion = 2;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	DamageModule::xfer( xfer );

	if( version >= 2 )
		UpgradeMux::upgradeMuxXfer( xfer );
}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void SwitchStateWhenDamagedBehaviorV2::loadPostProcess()
{
	// extend base class
	DamageModule::loadPostProcess();

	// extend upgrade mux
	UpgradeMux::upgradeMuxLoadPostProcess();
}
