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

// FILE: WeaponBonusUpdateV2.h /////////////////////////////////////////////////
//-----------------------------------------------------------------------------
//
//	GeneralsMod @feature Dimitar 08/09/2026
//	WeaponBonusUpdateV2 is a copy of WeaponBonusUpdate (see WeaponBonusUpdate.h)
//	that additionally lets the INI specify a custom "Tint" color to use instead
//	of the hardcoded FRENZY_COLOR / FRENZY_COLOR_INFANTRY constants in
//	Drawable.cpp, and applies the same color to both infantry and vehicles
//	(no kindof-based branching). It is a separate module purely so that
//	WeaponBonusUpdate itself does not need to change, keeping this feature
//	easy to merge/rebase against upstream changes.
//
//	purpose:	Like healing in that it can affect just me or people around,
//						except this gives a Weapon Bonus instead of health, with a
//						modder-specified tint color.
//
//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////

#pragma once

//-----------------------------------------------------------------------------
// USER INCLUDES //////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
#include "GameLogic/Module/UpdateModule.h"
//-----------------------------------------------------------------------------
// FORWARD REFERENCES /////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
enum WeaponBonusConditionType CPP_11(: Int);

//-----------------------------------------------------------------------------
// TYPE DEFINES ///////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
class WeaponBonusUpdateV2ModuleData : public UpdateModuleData
{
public:

	WeaponBonusUpdateV2ModuleData();

	KindOfMaskType						m_requiredAffectKindOf;		///< Must be set on target
	KindOfMaskType						m_forbiddenAffectKindOf;	///< Must be clear on target
	UnsignedInt								m_bonusDuration;					///< How long a hit lasts on target
	UnsignedInt								m_bonusDelay;							///< How often to pulse
	Real											m_bonusRange;							///< How far to affect
	WeaponBonusConditionType	m_bonusConditionType;			///< Status to give
	RGBColor									m_tintColor;							///< GeneralsMod @feature: custom tint color, same for all kindofs

	static void buildFieldParse(MultiIniFieldParse& p);
};


//-------------------------------------------------------------------------------------------------
class WeaponBonusUpdateV2 : public UpdateModule
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( WeaponBonusUpdateV2, "WeaponBonusUpdateV2" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( WeaponBonusUpdateV2, WeaponBonusUpdateV2ModuleData )

public:

	WeaponBonusUpdateV2( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	virtual UpdateSleepTime update() override;

protected:

};
