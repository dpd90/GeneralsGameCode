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

// FILE: ShieldGeneratorActivateV2.h /////////////////////////////////////////////////////////
// Desc:   The button-facing half of ShieldGeneratorUpdateV2 (see ShieldGeneratorUpdateV2.h for why
//         this is a separate module -- same reasoning as SwitchStateV2Activate / SwitchStateV2).
//         This is what a CommandButton with Command = SPECIAL_POWER actually finds and calls; it
//         does nothing on its own beyond the standard special power bookkeeping (science gating,
//         recharge timer, FX) already provided by SpecialPowerModule, which in turn hands off to
//         whichever module on the same object implements SpecialPowerUpdateInterface for the same
//         SpecialPowerTemplate -- that's ShieldGeneratorUpdateV2, which does the actual shielding.
//
//         Both this module and ShieldGeneratorUpdateV2 must be given on the object with the SAME
//         SpecialPowerTemplate so they find each other, e.g.:
//
//         Behavior = ShieldGeneratorActivateV2 ModuleTag_01
//           SpecialPowerTemplate = SpecialPower_Shield
//         End
//         Behavior = ShieldGeneratorUpdateV2 ModuleTag_02
//           SpecialPowerTemplate = SpecialPower_Shield
//           Lifetime = 10000
//           ShieldAmount = 500.0
//           ConditionStateType = USER_1
//         End
//
//         (see ShieldGeneratorUpdateV2.h for the full field list -- ShieldAmount%, DamageTypes,
//         WeaponIn/WeaponOut, OCLIn/OCLOut, FXListIn/FXListOut, RevertEarlyWhenDepleted, and
//         RunOutLogicOnlyWhenDepleted all live on that module, not this one.)
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "GameLogic/Module/SpecialPowerModule.h"

//-------------------------------------------------------------------------------------------------
class ShieldGeneratorActivateV2ModuleData : public SpecialPowerModuleData
{
	// Nothing extra -- SpecialPowerTemplate (and the standard recharge/science fields) already
	// come from SpecialPowerModuleData. All of the actual configuration lives on the companion
	// ShieldGeneratorUpdateV2 module.
};

//-------------------------------------------------------------------------------------------------
class ShieldGeneratorActivateV2 : public SpecialPowerModule
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( ShieldGeneratorActivateV2, "ShieldGeneratorActivateV2" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( ShieldGeneratorActivateV2, ShieldGeneratorActivateV2ModuleData )

public:

	ShieldGeneratorActivateV2( Thing *thing, const ModuleData *moduleData );
	// virtual destructor prototype provided by memory pool declaration

	virtual void doSpecialPower( UnsignedInt commandOptions ) override;

};
