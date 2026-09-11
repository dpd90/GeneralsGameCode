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

// FILE: SwitchStateWhenDamagedBehaviorV2.h ///////////////////////////////////////////////////////
//
//	GeneralsMod @feature Dimitar 11/09/2026
//	Automatic, damage-triggered counterpart to the button-driven SwitchStateV2 / SwitchStateV2Activate
//	pair (see SwitchStateV2.h). Modeled on FireWeaponWhenDamagedBehavior's DamageTypes/DamageAmount
//	INI shape, but instead of firing a weapon when the threshold is met, it calls
//	notifyQualifyingDamage() on the companion SwitchStateV2 module (matched by SpecialPowerTemplate,
//	same pairing convention SwitchStateV2Activate uses) so that module reacts to the damage itself.
//
//	This module does no state-holding of its own -- it is a pure DamageModule (no per-frame Update
//	needed) that just watches onDamage() and, when the configured threshold is met, hands off to
//	SwitchStateV2. See SwitchStateV2::notifyQualifyingDamage()'s declaration comment in SwitchStateV2.h
//	for exactly what happens on the receiving end (short version: drives toward AlteredState, refreshes
//	the revert countdown on every qualifying hit if Lifetime is configured, is a one-time switch if not).
//
//	Usage -- give the target object all three modules, all sharing the same SpecialPowerTemplate:
//
//	Behavior = SwitchStateV2Activate ModuleTag_01     ; optional -- only needed if a player can also
//	  SpecialPowerTemplate = SpecialPower_SwitchState  ; manually trigger/toggle the same switch
//	End
//	Behavior = SwitchStateV2 ModuleTag_02
//	  SpecialPowerTemplate = SpecialPower_SwitchState
//	  DefaultState  = ...
//	  AlteredState  = ...
//	  Lifetime      = 0
//	End
//	Behavior = SwitchStateWhenDamagedBehaviorV2 ModuleTag_03
//	  SpecialPowerTemplate = SpecialPower_SwitchState
//	  DamageTypes    = NONE +EXPLOSION +FLAME         ; DamageTypes uses the same NONE/ALL/+X/-X token
//	                                                   ; syntax as every other DamageTypeFlags field in
//	                                                   ; the engine (see INI::parseDamageTypeFlags) --
//	                                                   ; bare names with no +/- prefix are a parse error.
//	  DamageAmount   = 25.0
//	  GreaterOrEqual = Yes                            ; No = trigger when damage <= DamageAmount instead
//
//	  ; Optional -- same UpgradeMux gating FireWeaponWhenDamagedBehavior/AutoHealBehavior use. Omit both
//	  ; and the module is simply always active (the default). Upgrades are permanent/one-shot, so this
//	  ; is a durable unlock, not a re-togglable enable/disable switch.
//	  StartsActive   = No
//	  TriggeredBy    = Upgrade_SomeUpgradeName        ; from Upgrade.ini; module activates once this completes
//	End
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "GameLogic/Module/DamageModule.h"
#include "GameLogic/Module/UpgradeModule.h"

// FORWARD REFERENCES /////////////////////////////////////////////////////////////////////////////
class SpecialPowerTemplate;

//-------------------------------------------------------------------------------------------------
class SwitchStateWhenDamagedBehaviorV2ModuleData : public DamageModuleData
{
public:

	UpgradeMuxData					m_upgradeMuxData;		///< optional StartsActive=No + TriggeredBy=... gating (see UpgradeModule.h) -- same mechanism FireWeaponWhenDamagedBehavior/AutoHealBehavior use
	Bool									m_initiallyActive;		///< StartsActive -- if No, this module does nothing until a matching upgrade in TriggeredBy completes (permanently, per UpgradeMux semantics)
	const SpecialPowerTemplate*	m_specialPowerTemplate;	///< must match the SpecialPowerTemplate shared by the target's SwitchStateV2 (+ optional SwitchStateV2Activate)
	DamageTypeFlags								m_damageTypes;					///< which damage types can trigger the switch
	Real													m_damageAmount;					///< threshold compared against the actual (post-armor) damage dealt
	Bool													m_greaterOrEqual;				///< Yes: trigger when damage >= DamageAmount. No: trigger when damage <= DamageAmount

	SwitchStateWhenDamagedBehaviorV2ModuleData();

	static void buildFieldParse(MultiIniFieldParse& p);

};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
class SwitchStateWhenDamagedBehaviorV2 : public DamageModule, public UpgradeMux
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( SwitchStateWhenDamagedBehaviorV2, "SwitchStateWhenDamagedBehaviorV2" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( SwitchStateWhenDamagedBehaviorV2, SwitchStateWhenDamagedBehaviorV2ModuleData )

public:

	SwitchStateWhenDamagedBehaviorV2( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	// module methods
	static Int getInterfaceMask() { return DamageModule::getInterfaceMask() | (MODULEINTERFACE_UPGRADE); }

	// BehaviorModule
	virtual UpgradeModuleInterface* getUpgrade() override { return this; }

	// DamageModuleInterface
	virtual void onDamage( DamageInfo *damageInfo ) override;
	virtual void onHealing( DamageInfo *damageInfo ) override { }
	virtual void onBodyDamageStateChange( const DamageInfo* damageInfo, BodyDamageType oldState, BodyDamageType newState ) override { }

protected:

	// UpgradeMux -- thin pass-throughs to m_upgradeMuxData, same shape as FireWeaponWhenDamagedBehavior's
	virtual void upgradeImplementation() override { }	///< nothing to do beyond the executed-flag UpgradeMux already tracks -- we just gate onDamage() on isUpgradeActive()

	virtual void getUpgradeActivationMasks(UpgradeMaskType& activation, UpgradeMaskType& conflicting) const override
	{
		getSwitchStateWhenDamagedBehaviorV2ModuleData()->m_upgradeMuxData.getUpgradeActivationMasks(activation, conflicting);
	}

	virtual void performUpgradeFX() override
	{
		getSwitchStateWhenDamagedBehaviorV2ModuleData()->m_upgradeMuxData.performUpgradeFX(getObject());
	}

	virtual void processUpgradeRemoval() override
	{
		getSwitchStateWhenDamagedBehaviorV2ModuleData()->m_upgradeMuxData.muxDataProcessUpgradeRemoval(getObject());
	}

	virtual Bool requiresAllActivationUpgrades() const override
	{
		return getSwitchStateWhenDamagedBehaviorV2ModuleData()->m_upgradeMuxData.m_requiresAllTriggers;
	}

	virtual Bool isSubObjectsUpgrade() override { return false; }

	Bool isUpgradeActive() const { return isAlreadyUpgraded(); }

};
