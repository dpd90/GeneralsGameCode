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

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"
#include "Common/Xfer.h"

#include "GameLogic/Module/TempWeaponBonusHelper.h"

#include "GameClient/Drawable.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"
#include "GameLogic/Weapon.h"

// GeneralsMod @feature Dimitar 22/10/2026: per-condition stacking + additive tint blending.
// The original helper held exactly ONE temporary bonus: applying a different condition cleared the previous one,
// so two different auras (e.g. FRENZY_ONE and FRENZY_TWO, or a WeaponBonusUpdate and a WeaponBonusUpdateV2) kept
// overwriting each other. Now every condition has its own slot and timer:
//   - different conditions stack (their WeaponBonus values add up in Weapon::computeBonus as before),
//   - the same condition from several sources is still one bit; re-applying only extends its timer,
//   - the FRENZY tint shown is the SUM of every active condition's tint contribution (signed light offsets,
//     clamped per channel), and a condition can contribute no tint at all.
// See the project doc "weaponbonus-stacking-projectile-spec.md".

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
TempWeaponBonusHelper::TempWeaponBonusHelper( Thing *thing, const ModuleData *modData ) : ObjectHelper( thing, modData )
{
	static_assert( WEAPONBONUSCONDITION_COUNT <= MAX_BONUS_SLOTS, "TempWeaponBonusHelper needs a slot per WeaponBonusConditionType" );

	for( Int i = 0; i < MAX_BONUS_SLOTS; ++i )
	{
		m_slots[i].m_active = FALSE;
		m_slots[i].m_frameToRemove = 0;
		m_slots[i].m_projectileOnly = FALSE;
		m_slots[i].m_tintMode = TINT_MODE_NONE;
		m_slots[i].m_tint.red = 0.0f;
		m_slots[i].m_tint.green = 0.0f;
		m_slots[i].m_tint.blue = 0.0f;
	}
	m_projectileOnlyMask = 0;

	setWakeFrame(getObject(), UPDATE_SLEEP_FOREVER);
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
TempWeaponBonusHelper::~TempWeaponBonusHelper()
{

}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
UpdateSleepTime TempWeaponBonusHelper::update()
{
	// We are sleep driven and wake for the earliest expiring slot; clear every slot whose time is up.
	const UnsignedInt now = TheGameLogic->getFrame();
	Bool changed = FALSE;
	for( Int i = 0; i < WEAPONBONUSCONDITION_COUNT; ++i )
	{
		if( m_slots[i].m_active && m_slots[i].m_frameToRemove <= now )
		{
			clearSlot( i );
			changed = TRUE;
		}
	}

	if( changed )
	{
		rebuildProjectileOnlyMask();
		refreshTint();
	}

	return calcSleepTime();
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
UpdateSleepTime TempWeaponBonusHelper::calcSleepTime() const
{
	const UnsignedInt now = TheGameLogic->getFrame();
	Bool any = FALSE;
	UnsignedInt earliest = 0;
	for( Int i = 0; i < WEAPONBONUSCONDITION_COUNT; ++i )
	{
		if( !m_slots[i].m_active )
			continue;
		if( !any || m_slots[i].m_frameToRemove < earliest )
			earliest = m_slots[i].m_frameToRemove;
		any = TRUE;
	}

	if( !any )
		return UPDATE_SLEEP_FOREVER;

	UnsignedInt delta = ( earliest > now ) ? ( earliest - now ) : 1;
	return UPDATE_SLEEP( delta );
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void TempWeaponBonusHelper::clearSlot( Int index )
{
	BonusSlot &slot = m_slots[index];
	if( !slot.m_active )
		return;

	getObject()->clearWeaponBonusCondition( (WeaponBonusConditionType)index );
	slot.m_active = FALSE;
	slot.m_frameToRemove = 0;
	slot.m_projectileOnly = FALSE;
	slot.m_tintMode = TINT_MODE_NONE;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void TempWeaponBonusHelper::clearTempWeaponBonus()
{
	for( Int i = 0; i < WEAPONBONUSCONDITION_COUNT; ++i )
		clearSlot( i );

	rebuildProjectileOnlyMask();
	refreshTint();
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void TempWeaponBonusHelper::rebuildProjectileOnlyMask()
{
	m_projectileOnlyMask = 0;
	for( Int i = 0; i < WEAPONBONUSCONDITION_COUNT; ++i )
	{
		if( m_slots[i].m_active && m_slots[i].m_projectileOnly )
			m_projectileOnlyMask |= ( 1 << i );
	}
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void TempWeaponBonusHelper::refreshTint()
{
	Drawable *draw = getObject()->getDrawable();
	if( draw == nullptr )
		return;

	Bool anyTint = FALSE;
	RGBColor sum;
	sum.red = sum.green = sum.blue = 0.0f;

	for( Int i = 0; i < WEAPONBONUSCONDITION_COUNT; ++i )
	{
		const BonusSlot &slot = m_slots[i];
		if( !slot.m_active )
			continue;

		if( slot.m_tintMode == TINT_MODE_CUSTOM )
		{
			sum.red		+= slot.m_tint.red;
			sum.green	+= slot.m_tint.green;
			sum.blue	+= slot.m_tint.blue;
			anyTint = TRUE;
		}
		else if( slot.m_tintMode == TINT_MODE_DEFAULT_FRENZY )
		{
			const RGBColor def = draw->getDefaultFrenzyTintColor();
			sum.red		+= def.red;
			sum.green	+= def.green;
			sum.blue	+= def.blue;
			anyTint = TRUE;
		}
	}

	if( !anyTint )
	{
		// Nothing active wants a tint (or nothing is active at all): show the unit untinted.
		draw->clearTintStatus( TINT_STATUS_FRENZY );
		draw->clearFrenzyTintColorOverride();
//    if (getObject()->isKindOf(KINDOF_INFANTRY))
//      getObject()->getDrawable()->setSecondMaterialPassOpacity( 0.0f );
		return;
	}

	// Tints are signed light offsets added to the scene lights, so plain addition is the right blend
	// (e.g. red {0.3,0,0} + blue {0,0,0.3} = violet {0.3,0,0.3}). Clamp so many stacked tints can't blow out the model.
	sum.red		= clamp( -1.0f, sum.red,		1.0f );
	sum.green	= clamp( -1.0f, sum.green,	1.0f );
	sum.blue	= clamp( -1.0f, sum.blue,		1.0f );

	draw->setFrenzyTintColorOverride( sum );
	draw->setTintStatus( TINT_STATUS_FRENZY );
//    if (getObject()->isKindOf(KINDOF_INFANTRY))
//      getObject()->getDrawable()->setSecondMaterialPassOpacity( 1.0f );
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void TempWeaponBonusHelper::doTempWeaponBonus( WeaponBonusConditionType status, UnsignedInt duration, const RGBColor *tintColorOverride,
	Bool suppressDefaultTint, Bool projectileOnly )
{
	if( status < 0 || status >= WEAPONBONUSCONDITION_COUNT )
	{
		DEBUG_CRASH(( "TempWeaponBonusHelper::doTempWeaponBonus - invalid condition %d", (Int)status ));
		return;
	}

	BonusSlot &slot = m_slots[status];
	const UnsignedInt removeFrame = TheGameLogic->getFrame() + duration;
	const Bool wasActive = slot.m_active;

	// Re-applying the same condition only ever EXTENDS it, so a shorter refresh from a second source
	// can't cut short a longer one already running.
	if( !wasActive || removeFrame > slot.m_frameToRemove )
		slot.m_frameToRemove = removeFrame;

	// Projectile-only: a fresh slot takes the caller's setting; if the same condition is also applied by a
	// normal (all weapons) source while active, the normal source wins until the slot expires.
	if( !wasActive )
		slot.m_projectileOnly = projectileOnly;
	else if( !projectileOnly )
		slot.m_projectileOnly = FALSE;

	// Tint contribution of this condition (last writer wins for the same condition).
	if( tintColorOverride )
	{
		slot.m_tintMode = TINT_MODE_CUSTOM;
		slot.m_tint = *tintColorOverride;
	}
	else
	{
		slot.m_tintMode = suppressDefaultTint ? TINT_MODE_NONE : TINT_MODE_DEFAULT_FRENZY;
	}

	slot.m_active = TRUE;
	getObject()->setWeaponBonusCondition( status );

	rebuildProjectileOnlyMask();
	refreshTint();

	setWakeFrame( getObject(), calcSleepTime() );
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void TempWeaponBonusHelper::crc( Xfer *xfer )
{

	// object helper crc
	ObjectHelper::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info;
	* 1: Initial version
	* 2: GeneralsMod @feature Dimitar 22/10/2026: per-condition slots (stacking, projectile-only, tint mode/color) */
// ------------------------------------------------------------------------------------------------
void TempWeaponBonusHelper::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 2;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// object helper base class
	ObjectHelper::xfer( xfer );

	if( version < 2 )
	{
		// old single-slot format (only ever loaded, never saved)
		WeaponBonusConditionType currentBonus = WEAPONBONUSCONDITION_INVALID;
		UnsignedInt frameToRemove = 0;
		xfer->xferUser( &currentBonus, sizeof(WeaponBonusConditionType) );// an enum
		xfer->xferUnsignedInt( &frameToRemove );

		if( xfer->getXferMode() == XFER_LOAD && currentBonus >= 0 && currentBonus < WEAPONBONUSCONDITION_COUNT )
		{
			BonusSlot &slot = m_slots[currentBonus];
			slot.m_active = TRUE;
			slot.m_frameToRemove = frameToRemove;
			slot.m_projectileOnly = FALSE;
			slot.m_tintMode = TINT_MODE_DEFAULT_FRENZY;	// what a v1 save could show (it can't tell V2 apart)
		}
		return;
	}

	if( xfer->getXferMode() == XFER_LOAD )
	{
		for( Int i = 0; i < MAX_BONUS_SLOTS; ++i )
			m_slots[i].m_active = FALSE;
	}

	UnsignedByte count = 0;
	if( xfer->getXferMode() != XFER_LOAD )
	{
		for( Int i = 0; i < WEAPONBONUSCONDITION_COUNT; ++i )
			if( m_slots[i].m_active )
				++count;
	}
	xfer->xferUnsignedByte( &count );

	if( xfer->getXferMode() == XFER_LOAD )
	{
		for( UnsignedByte n = 0; n < count; ++n )
		{
			Int index = 0;
			BonusSlot tmp;
			xfer->xferInt( &index );
			xfer->xferUnsignedInt( &tmp.m_frameToRemove );
			xfer->xferBool( &tmp.m_projectileOnly );
			xfer->xferInt( &tmp.m_tintMode );
			xfer->xferReal( &tmp.m_tint.red );
			xfer->xferReal( &tmp.m_tint.green );
			xfer->xferReal( &tmp.m_tint.blue );

			if( index < 0 || index >= WEAPONBONUSCONDITION_COUNT )
			{
				DEBUG_CRASH(( "TempWeaponBonusHelper::xfer - invalid condition index %d in save", index ));
				continue;
			}
			tmp.m_active = TRUE;
			m_slots[index] = tmp;
		}
	}
	else
	{
		for( Int i = 0; i < WEAPONBONUSCONDITION_COUNT; ++i )
		{
			BonusSlot &slot = m_slots[i];
			if( !slot.m_active )
				continue;
			Int index = i;
			xfer->xferInt( &index );
			xfer->xferUnsignedInt( &slot.m_frameToRemove );
			xfer->xferBool( &slot.m_projectileOnly );
			xfer->xferInt( &slot.m_tintMode );
			xfer->xferReal( &slot.m_tint.red );
			xfer->xferReal( &slot.m_tint.green );
			xfer->xferReal( &slot.m_tint.blue );
		}
	}

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void TempWeaponBonusHelper::loadPostProcess()
{

	// object helper base class
	ObjectHelper::loadPostProcess();

	// GeneralsMod @feature Dimitar 22/10/2026: the mask is derived (not saved), and the Drawable's tint color
	// override is deliberately not saved by the Drawable, so rebuild both from the restored slots. This also
	// fixes the old behavior where a V2-tinted unit showed the default FRENZY color after a load.
	rebuildProjectileOnlyMask();
	refreshTint();

}
