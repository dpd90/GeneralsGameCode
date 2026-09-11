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

// FILE: HealAIUpdateV2.h /////////////////////////////////////////////////////////////////////////
//
//	GeneralsMod @feature Dimitar 11/09/2026
//	HealAIUpdateV2 is a slimmed-down, non-Dozer copy of DozerAIUpdate's "bored" auto-scan pattern (see
//	DozerAIUpdate.h/.cpp), keeping only the repair/heal half of Dozer's behavior and dropping
//	build/fortify/dock-point/bridge-scaffolding/mine-clearing entirely. Differences from Dozer:
//	  - Does NOT require KindOf = DOZER on the object using this module.
//	  - Target selection is KindOf/ForbiddenKindOf-filtered (same field names/parser as
//	    FireOCLBehaviorV2's scan), not hardcoded to KindOf = STRUCTURE, and is not routed through
//	    ActionManager::canRepairObject() (which hard-requires KindOf(DOZER) on the healer and
//	    KindOf(STRUCTURE) on the target -- unusable here).
//	  - Healing is continuous weapon fire (NO_MAX_SHOTS_LIMIT), not a single dock-repair tick. Because
//	    the base engine's AIAttackState only ever stops attacking when the victim dies (it has no
//	    "target no longer needs attention" concept -- never needed for killing enemies), this module
//	    adds its own per-frame check in update() that force-stops the attack once the current victim
//	    is back to full health.
//	  - Turret-vs-body aiming needs no special handling at all: AIAttackAimAtTargetState already skips
//	    body rotation entirely whenever the current weapon's slot is on a turret with nonzero
//	    TurretTurnRate (see AIStates.cpp) -- purely a matter of this object's INI (Turret /
//	    ControlledWeaponSlots), not code.
//	The actual heal-per-shot amount belongs on the healing Weapon's damage (DAMAGE_HEALING_* /
//	IsHealingDamage(), see Damage.h) -- deliberately not duplicated as a field here.
//
//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "GameLogic/Module/AIUpdate.h"
#include "Common/KindOf.h"

//-------------------------------------------------------------------------------------------------
class HealAIUpdateV2ModuleData : public AIUpdateModuleData
{
public:

	Real						m_boredTime;				///< after this many frames of continuous idle, look for something to heal
	Real						m_boredRange;				///< range to search in when bored
	KindOfMaskType	m_kindof;						///< only objects matching these KindOfs are eligible heal targets -- defaults to none
	KindOfMaskType	m_forbiddenKindof;	///< objects matching these KindOfs are never eligible heal targets -- defaults to none

	HealAIUpdateV2ModuleData();

	static void buildFieldParse( MultiIniFieldParse &p );

};

//-------------------------------------------------------------------------------------------------
/** Idle units using this module periodically scan for a damaged, KindOf-filtered ally within
	* BoredRange and heal it with continuous weapon fire, stopping on their own once the target is
	* back to full health. See HealAIUpdateV2.cpp for the full scan / self-stop logic. */
//-------------------------------------------------------------------------------------------------
class HealAIUpdateV2 : public AIUpdateInterface
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( HealAIUpdateV2, "HealAIUpdateV2" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( HealAIUpdateV2, HealAIUpdateV2ModuleData )

public:

	HealAIUpdateV2( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	virtual UpdateSleepTime update() override;

private:

	static Object* findHealTarget( Object *self, const HealAIUpdateV2ModuleData *data );

	UnsignedInt m_idleTooLongTimestamp;	///< frame we started tracking our current stretch of idle time

};
