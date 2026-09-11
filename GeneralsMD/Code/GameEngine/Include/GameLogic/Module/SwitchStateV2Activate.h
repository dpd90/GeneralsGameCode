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

// FILE: SwitchStateV2Activate.h //////////////////////////////////////////////////////////////////
// Desc:   The button-facing half of SwitchStateV2 (see SwitchStateV2.h for why this is a separate
//         module). This is what a CommandButton with Command = SPECIAL_POWER actually finds and
//         calls; it does nothing on its own beyond the standard special power bookkeeping
//         (science gating, recharge timer, FX) already provided by SpecialPowerModule, which in
//         turn hands off to whichever module on the same object implements
//         SpecialPowerUpdateInterface for the same SpecialPowerTemplate -- that's SwitchStateV2,
//         which does the actual state toggling.
//
//         Both this module and SwitchStateV2 must be given on the object with the SAME
//         SpecialPowerTemplate so they find each other, e.g.:
//
//         Behavior = SwitchStateV2Activate ModuleTag_01
//           SpecialPowerTemplate = SpecialPower_SwitchState
//         End
//         Behavior = SwitchStateV2 ModuleTag_02
//           SpecialPowerTemplate = SpecialPower_SwitchState
//           DefaultState = ...
//           AlteredState = ...
//           Lifetime = 0
//         End
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "GameLogic/Module/SpecialPowerModule.h"

//-------------------------------------------------------------------------------------------------
class SwitchStateV2ActivateModuleData : public SpecialPowerModuleData
{
	// Nothing extra -- SpecialPowerTemplate (and the standard recharge/science fields) already
	// come from SpecialPowerModuleData. All of the actual configuration lives on the companion
	// SwitchStateV2 module.
};

//-------------------------------------------------------------------------------------------------
class SwitchStateV2Activate : public SpecialPowerModule
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( SwitchStateV2Activate, "SwitchStateV2Activate" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( SwitchStateV2Activate, SwitchStateV2ActivateModuleData )

public:

	SwitchStateV2Activate( Thing *thing, const ModuleData *moduleData );
	// virtual destructor prototype provided by memory pool declaration

	virtual void doSpecialPower( UnsignedInt commandOptions ) override;

};
