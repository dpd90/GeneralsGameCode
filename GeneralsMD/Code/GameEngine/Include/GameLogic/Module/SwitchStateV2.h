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

// FILE: SwitchStateV2.h //////////////////////////////////////////////////////////////////////////
// Desc:   Flips a unit between two fully-specified configurations (model condition, weapon set,
//         armor set, status bit, command set override, locomotor set) in place, with no Contain
//         module and no second unit involved. Optionally auto-reverts to DefaultState after
//         Lifetime frames; a Lifetime of 0 means the switch is permanent until manually toggled
//         back.
//
//         This module holds the state and does the actual work. It cannot also be what the
//         CommandButton (Command = SPECIAL_POWER) talks to directly, because Object::doSpecialPower
//         only ever looks for a module satisfying SpecialPowerModuleInterface (getSpecialPower()),
//         and the GameLogic update scheduler only ever schedules real UpdateModule* instances -- a
//         single class cannot be both without multiply inheriting from two incompatible concrete
//         BehaviorModule branches. So the button-facing half lives in the companion module
//         SwitchStateV2Activate (see SwitchStateV2Activate.h); both must be given on the object
//         with the SAME SpecialPowerTemplate so they find each other. This mirrors how the retail
//         game already pairs SpecialAbility (trigger) with SpecialAbilityUpdate (the module that
//         actually does the work).
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "GameLogic/Module/SpecialPowerUpdateModule.h"

// FORWARD REFERENCES /////////////////////////////////////////////////////////////////////////////
enum ModelConditionFlagType CPP_11(: Int);
enum WeaponSetType CPP_11(: Int);
enum ArmorSetType CPP_11(: Int);
enum ObjectStatusType CPP_11(: Int);
enum LocomotorSetType CPP_11(: Int);
class WeaponTemplate;
class Weapon;

//-------------------------------------------------------------------------------------------------
// One full configuration for the unit -- everything SwitchStateV2 is allowed to change about the
// object. DefaultState and AlteredState are each one of these; toggling flips between them. Every
// field must be given in the INI (duplicate the current value in both states for anything that
// should not change).
struct SwitchStateInfo
{
	ModelConditionFlagType	m_modelCondition;
	WeaponSetType						m_weaponSetFlag;
	ArmorSetType						m_armorSetFlag;
	ObjectStatusType				m_objectStatusType;
	AsciiString							m_commandSet;
	LocomotorSetType				m_locomotorSetType;

	SwitchStateInfo();
};

//-------------------------------------------------------------------------------------------------
class SwitchStateV2ModuleData : public UpdateModuleData
{
public:

	const SpecialPowerTemplate*	m_specialPowerTemplate;	///< must match the SpecialPowerTemplate on the companion SwitchStateV2Activate module
	SwitchStateInfo								m_defaultState;
	SwitchStateInfo								m_alteredState;
	UnsignedInt										m_lifetimeFrames;					///< 0 = permanent until manually toggled back; otherwise, frames (parsed from milliseconds) AlteredState lasts before auto-reverting
	const WeaponTemplate*				m_weaponTemplate;				///< optional; fired once (at our own position) every time we are activated by the companion SwitchStateV2Activate button. nullptr if not given in INI.

	SwitchStateV2ModuleData();

	static void buildFieldParse(MultiIniFieldParse& p);
	static void parseStateInfo( INI* ini, void *instance, void *store, const void* userData );

};

//-------------------------------------------------------------------------------------------------
class SwitchStateV2 : public SpecialPowerUpdateModule
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( SwitchStateV2, "SwitchStateV2" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( SwitchStateV2, SwitchStateV2ModuleData )

public:

	SwitchStateV2( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	// UpdateModuleInterface
	virtual UpdateSleepTime update() override;
	virtual DisabledMaskType getDisabledTypesToProcess() const override { return DISABLEDMASK_ALL; } ///< a scheduled Lifetime revert must still happen even if we get disabled/held

	// SpecialPowerUpdateInterface (reached via our companion SwitchStateV2Activate's initiateIntentToDoSpecialPower)
	virtual SpecialPowerUpdateInterface* getSpecialPowerUpdateInterface() override { return this; }
	virtual Bool initiateIntentToDoSpecialPower( const SpecialPowerTemplate *specialPowerTemplate, const Object *targetObj, const Coord3D *targetPos, const Waypoint *way, UnsignedInt commandOptions ) override;
	virtual Bool isSpecialAbility() const override { return false; } ///< IMPORTANT: this is not cosmetic -- returning true here makes the engine blind-cast this SpecialPowerUpdateInterface* to a SpecialAbilityUpdate* in several places (e.g. ControlBarCommand.cpp's per-frame command-button update, via Object::findSpecialAbilityUpdate()). SwitchStateV2 is not a SpecialAbilityUpdate subclass, so that cast is memory corruption.
	virtual Bool isSpecialPower() const override { return false; }
	virtual Bool isActive() const override { return false; }
	virtual CommandOption getCommandOption() const override { return (CommandOption)0; }
	virtual Bool doesSpecialPowerHaveOverridableDestinationActive() const override { return false; }
	virtual Bool doesSpecialPowerHaveOverridableDestination() const override { return false; }
	virtual void setSpecialPowerOverridableDestination( const Coord3D *loc ) override { }
	virtual Bool isPowerCurrentlyInUse( const CommandButton *command = nullptr ) const override { return false; }

protected:

	void applyState( Bool goToAltered );	///< sets everything for the target state, clearing whatever the other state would have set first

private:

	Bool					m_isAltered;		///< which of the two states we're currently in
	UnsignedInt		m_revertFrame;	///< absolute frame to auto-revert on; 0 = no revert scheduled
	Weapon*				m_weapon;		///< allocated from m_weaponTemplate in the constructor; nullptr if no Weapon was given in INI

};
