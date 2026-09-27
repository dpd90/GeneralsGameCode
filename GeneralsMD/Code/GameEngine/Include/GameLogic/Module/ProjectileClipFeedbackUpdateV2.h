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

// FILE: ProjectileClipFeedbackUpdateV2.h /////////////////////////////////////////////////////////
// Desc:   Keeps ProjectileBoneFeedbackEnabledSlots (and optionally the weapon FIRING/RELOADING/
//         BETWEEN/PREATTACK model condition flags) in sync for objects sitting inside a container.
//
//         Why this exists: every weapon-carrying Object has a built-in ObjectWeaponStatusHelper that
//         calls Object::adjustModelConditionForWeaponStatus() every frame -- that is what pushes the
//         current ammo count to the Drawable (Drawable::updateDrawableClipStatus) so launch-bone
//         projectiles hide/show. But TransportContain (and therefore OverlordContain, Garrison,
//         Helix, Cave...) sets DISABLED_HELD on its riders, and ObjectWeaponStatusHelper uses the
//         default UpdateModule disabled mask (DISABLEDMASK_NONE), so it is skipped for the whole time
//         the rider is held. Firing still hides projectiles (AIAttackState calls the same function
//         itself), but a reload that completes outside of an attack -- clip reload, or the
//         FiringTracker AutoReloadWhenIdle refill -- never reaches the Drawable.
//
//         This module runs while disabled and re-pushes that state on a configurable interval.
//         Opt-in per object; no existing engine module is changed.
//
// INI:
//   Behavior = ProjectileClipFeedbackUpdateV2 ModuleTag_XX
//     CheckInterval          = 250   ; ms between checks (min 1 frame)
//     OnlyWhenContained      = Yes   ; do nothing unless getContainedBy() != null
//     UpdateWeaponConditions = No    ; Yes = call adjustModelConditionForWeaponStatus() (ammo feedback
//                                    ;       AND weapon model-condition flags); No = ammo feedback only
//   End
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "Common/GameType.h"
#include "GameLogic/Module/UpdateModule.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
class ProjectileClipFeedbackUpdateV2ModuleData : public UpdateModuleData
{
public:
	UnsignedInt		m_checkIntervalFrames;		///< CheckInterval: frames between checks.
	Bool					m_onlyWhenContained;			///< OnlyWhenContained: skip unless inside a container.
	Bool					m_updateWeaponConditions;	///< UpdateWeaponConditions: also refresh weapon model-condition flags.

	ProjectileClipFeedbackUpdateV2ModuleData();
	static void buildFieldParse(MultiIniFieldParse& p);
};

//-------------------------------------------------------------------------------------------------
/** Re-pushes ammo/clip state to the Drawable for held (contained) objects. */
//-------------------------------------------------------------------------------------------------
class ProjectileClipFeedbackUpdateV2 : public UpdateModule
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( ProjectileClipFeedbackUpdateV2, "ProjectileClipFeedbackUpdateV2" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( ProjectileClipFeedbackUpdateV2, ProjectileClipFeedbackUpdateV2ModuleData );

public:

	ProjectileClipFeedbackUpdateV2( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	virtual UpdateSleepTime update() override;

	// Must keep running while the object is DISABLED_HELD (that is the whole point of this module),
	// and also while held + EMP/subdued/etc, since the non-retail update path requires the mask to
	// cover EVERY disabled bit the object has (testForAll).
	virtual DisabledMaskType getDisabledTypesToProcess() const override { return DISABLEDMASK_ALL; }

protected:

	void invalidateCache();			///< Forces the next check to push every slot unconditionally.
	void pushChangedClipStatus();	///< Ammo-only mode: push slots whose remaining/clip size changed.

	// Client-visual cache only -- deliberately NOT xfer'd/crc'd (see loadPostProcess()).
	UnsignedInt	m_lastRemaining[WEAPONSLOT_COUNT];
	Int					m_lastClipSize[WEAPONSLOT_COUNT];
};
