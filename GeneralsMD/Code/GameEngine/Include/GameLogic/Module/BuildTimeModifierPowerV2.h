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

// FILE: BuildTimeModifierPowerV2.h /////////////////////////////////////////////////////////////
//
//	GeneralsMod @feature Dimitar 10/11/2026
//	Science-unlocked, permanent, player-wide build-time modifier. Build-time counterpart of
//	CostModifierUpgrade, but activated the same way CashBountyPower is: it is a button-less
//	SpecialPowerModule whose SpecialPower template carries RequiredScience, so
//	Player::addScience() wakes it via onSpecialPowerCreation() when the science is bought, and
//	onObjectCreated() applies it for any host object created after the science is already owned.
//	The effect lives on the Player (Player::addKindOfProductionTimeChange) and is never removed --
//	losing the host object (e.g. the Command Center) does not take the bonus away.
//	Applied in ThingTemplate::calcTimeToBuild(), so it affects both factory production and dozer
//	construction, live (in-progress builds speed up immediately). Does NOT affect upgrade research.
//
//	INI usage (on the Command Center or any other host):
//		Behavior = BuildTimeModifierPowerV2 ModuleTag_XX
//			SpecialPowerTemplate = SpecialPowerBuildTimeModifier  ; Enum = SPECIAL_BUILD_TIME_MODIFIER, RequiredScience = SCIENCE_X
//			EffectKindOf = VEHICLE INFANTRY
//			Percentage = -25%                                    ; negative = faster (0.75x build time)
//		End
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "GameLogic/Module/SpecialPowerModule.h"
#include "Common/KindOf.h"

class Thing;
class Player;

//-------------------------------------------------------------------------------------------------
class BuildTimeModifierPowerV2ModuleData : public SpecialPowerModuleData
{
public:
	KindOfMaskType	m_kindOf;			///< which things get the build time change
	Real						m_percentage;	///< -25% => builds take 0.75x as long

	BuildTimeModifierPowerV2ModuleData();
	static void buildFieldParse(MultiIniFieldParse& p);
};

//-------------------------------------------------------------------------------------------------
class BuildTimeModifierPowerV2 : public SpecialPowerModule
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( BuildTimeModifierPowerV2, "BuildTimeModifierPowerV2" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( BuildTimeModifierPowerV2, BuildTimeModifierPowerV2ModuleData );

public:

	BuildTimeModifierPowerV2( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype defined by MemoryPoolObject

	virtual void onObjectCreated() override;
	virtual void onSpecialPowerCreation() override;	///< called by Player::addScience() when RequiredScience is bought
	virtual void doSpecialPower( UnsignedInt commandOptions ) override { return; }

protected:

	void applyToPlayer( Player *player );

};
