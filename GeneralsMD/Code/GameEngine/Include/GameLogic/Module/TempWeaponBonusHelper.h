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

// FILE: TempWeaponBonusHelper.h ////////////////////////////////////////////////////////////////////////
// Author: Graham Smallwood, June 2003
// Desc:   Object helper - Clears Temporary weapon bonus effects
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "GameLogic/Module/ObjectHelper.h"
#include "GameLogic/WeaponBonusConditionFlags.h"	///< GeneralsMod @feature Dimitar 22/10/2026

enum WeaponBonusConditionType CPP_11(: Int);

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
class TempWeaponBonusHelperModuleData : public ModuleData
{

};

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
class TempWeaponBonusHelper : public ObjectHelper
{

	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( TempWeaponBonusHelper, TempWeaponBonusHelperModuleData )
	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE(TempWeaponBonusHelper, "TempWeaponBonusHelper" )

public:

	TempWeaponBonusHelper( Thing *thing, const ModuleData *modData );
	// virtual destructor prototype provided by memory pool object

	virtual DisabledMaskType getDisabledTypesToProcess() const override { return DISABLEDMASK_ALL; }
	virtual UpdateSleepTime update() override;

	// GeneralsMod @feature Dimitar 08/09/2026: optional custom tint color for WeaponBonusUpdateV2
	// GeneralsMod @feature Dimitar 22/10/2026: per-condition stacking. Each WeaponBonusConditionType has its own slot
	// and timer, so DIFFERENT conditions stack (FRENZY_ONE + FRENZY_TWO + FRENZY_THREE are all active together)
	// while the SAME condition from several sources is still just one bit (re-applying only extends its timer).
	//   tintColorOverride   - non-null: this condition contributes that (already scaled) color to the blended tint.
	//   suppressDefaultTint - only read when tintColorOverride is null: TRUE = contribute no tint at all
	//                         (WeaponBonusUpdateV2 with no Tint line); FALSE = contribute the vanilla FRENZY color
	//                         (WeaponBonusUpdate / V1, unchanged behavior).
	//   projectileOnly      - this condition only applies to weapons with a ProjectileObject (see
	//                         Object::getProjectileOnlyWeaponBonusFlags / Weapon::computeBonus).
	void doTempWeaponBonus( WeaponBonusConditionType status, UnsignedInt duration, const RGBColor *tintColorOverride = nullptr,
		Bool suppressDefaultTint = FALSE, Bool projectileOnly = FALSE );

	/// GeneralsMod @feature Dimitar 22/10/2026: bits of currently active conditions that apply to projectile weapons only
	WeaponBonusConditionFlags getProjectileOnlyFlags() const { return m_projectileOnlyMask; }

protected:

	// GeneralsMod @feature Dimitar 22/10/2026: replaces the single m_currentBonus / m_frameToRemove pair
	enum TintMode CPP_11(: Int)
	{
		TINT_MODE_NONE = 0,				///< contributes nothing to the blended tint
		TINT_MODE_CUSTOM,					///< contributes m_tint
		TINT_MODE_DEFAULT_FRENZY,	///< contributes the drawable's default FRENZY color (vanilla look)
	};

	struct BonusSlot
	{
		Bool				m_active;
		UnsignedInt	m_frameToRemove;	///< frame on which this condition is removed
		Bool				m_projectileOnly;
		Int					m_tintMode;				///< TintMode (stored as Int for xfer)
		RGBColor		m_tint;
	};

	// One slot per possible condition bit. WeaponBonusConditionFlags is a 32-bit mask, so 32 always covers
	// WEAPONBONUSCONDITION_COUNT (static_assert'd in the .cpp); avoids pulling all of Weapon.h into this header.
	enum { MAX_BONUS_SLOTS = 32 };
	BonusSlot									m_slots[ MAX_BONUS_SLOTS ];
	WeaponBonusConditionFlags	m_projectileOnlyMask;	///< derived from m_slots, not saved

	void clearTempWeaponBonus();								///< clears every slot
	void clearSlot( Int index );								///< clears one slot (no tint refresh)
	void rebuildProjectileOnlyMask();
	void refreshTint();													///< recomputes the blended FRENZY tint from all active slots
	UpdateSleepTime calcSleepTime() const;			///< sleep until the earliest active slot expires
};
