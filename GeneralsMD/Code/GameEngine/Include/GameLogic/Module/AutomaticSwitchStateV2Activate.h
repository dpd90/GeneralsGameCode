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

// FILE: AutomaticSwitchStateV2Activate.h /////////////////////////////////////////////////////////
// Desc:   GeneralsMod @feature Dimitar 01/10/2026: automatic, timer-driven counterpart to
//         SwitchStateV2Activate. Instead of waiting for the player to press a CommandButton, this
//         module waits InitialDelay (counted from the moment the object is created) and then drives
//         the companion SwitchStateV2 module (matched by SpecialPowerTemplate) into its AlteredState
//         exactly once. After that it sleeps forever and costs nothing.
//
//         It uses the same hand-off SwitchStateWhenDamagedBehaviorV2 uses
//         (SpecialPowerUpdateInterface::notifyQualifyingDamage), so the semantics are identical to
//         an automatic trigger rather than the button's toggle: it always drives TOWARD
//         AlteredState, fires WeaponIn/OCLIn/FXListIn, and if SwitchStateV2's Lifetime > 0 the
//         normal auto-revert countdown starts from the moment we fire. It never toggles the unit
//         back to DefaultState.
//
//         The timer keeps running while the object is disabled (EMP, underpowered, etc.), so the
//         switch always happens at a deterministic time after creation.
//
//         Example:
//
//         Behavior = AutomaticSwitchStateV2Activate ModuleTag_01
//           SpecialPowerTemplate = SpecialPower_SwitchState
//           InitialDelay = 750
//         End
//         Behavior = SwitchStateV2 ModuleTag_02
//           SpecialPowerTemplate = SpecialPower_SwitchState
//           DefaultState = ...
//           AlteredState = ...
//         End
//
//         It can coexist with SwitchStateV2Activate (button) and/or SwitchStateWhenDamagedBehaviorV2
//         on the same object, all sharing the same SpecialPowerTemplate.
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "GameLogic/Module/UpdateModule.h"

// FORWARD REFERENCES /////////////////////////////////////////////////////////////////////////////
class SpecialPowerTemplate;

//-------------------------------------------------------------------------------------------------
class AutomaticSwitchStateV2ActivateModuleData : public UpdateModuleData
{
public:

	const SpecialPowerTemplate*	m_specialPowerTemplate;	///< must match the SpecialPowerTemplate on the companion SwitchStateV2 module
	UnsignedInt									m_initialDelayFrames;		///< frames (parsed from milliseconds) after creation before we trigger the switch; 0 = on the first logic frame

	AutomaticSwitchStateV2ActivateModuleData();

	static void buildFieldParse(MultiIniFieldParse& p);

};

//-------------------------------------------------------------------------------------------------
class AutomaticSwitchStateV2Activate : public UpdateModule
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( AutomaticSwitchStateV2Activate, "AutomaticSwitchStateV2Activate" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( AutomaticSwitchStateV2Activate, AutomaticSwitchStateV2ActivateModuleData )

public:

	AutomaticSwitchStateV2Activate( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	virtual UpdateSleepTime update() override;
	virtual DisabledMaskType getDisabledTypesToProcess() const override { return DISABLEDMASK_ALL; } ///< the delay must elapse even if we get disabled

private:

	Bool m_hasFired;	///< TRUE once we've triggered the switch; we never trigger again

};
