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

// FILE: PointDefenseUpdateV2.h ///////////////////////////////////////////////////////////////////
// Desc:   Reworked point-defense laser targeting update. Based on PointDefenseLaserUpdate, with:
//           - PredictTargetVelocityFactor actually used (V1 computed a predicted position and then
//             silently discarded it -- see PointDefenseUpdateV2.cpp for details).
//           - Distance comparisons done with squared distances (no sqrt in the hot path).
//           - The relationship (enemy) check is pushed into the partition-manager query itself via
//             a PartitionFilter, instead of pulling back every nearby object and rejecting most of
//             them one at a time afterward.
//           - Caches up to MaxTrackedTargets candidates per scan instead of exactly one, so losing
//             the current target doesn't require an emergency re-scan before the next shot.
//           - Optional idle sleep (IdleScanRate): when a scan finds absolutely nothing in ScanRange,
//             the module can tell the engine not to call update() again for a while, instead of
//             paying a per-frame call+branch for a unit with nothing nearby. Off (0) by default --
//             it trades a bit of first-contact latency for CPU, so enable it deliberately per unit.
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "Common/KindOf.h"
#include "GameLogic/Module/UpdateModule.h"

// FORWARD REFERENCES /////////////////////////////////////////////////////////////////////////////
class ThingTemplate;
class WeaponTemplate;

// How many "next best" targets we're willing to cache alongside the primary one.
// Kept small and fixed-size on purpose -- this is a per-frame hot path.
enum { PDU2_MAX_TRACKED_TARGETS = 4 };

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
class PointDefenseUpdateV2ModuleData : public ModuleData
{
public:
	WeaponTemplate	*m_weaponTemplate;
	KindOfMaskType	m_primaryTargetKindOf;
	KindOfMaskType  m_secondaryTargetKindOf;
	UnsignedInt			m_scanFrames;					///< ScanRate: frames between full re-scans while something is nearby.
	UnsignedInt			m_idleScanFrames;			///< IdleScanRate: frames to sleep once a scan finds nothing at all. 0 = never idle-sleep.
	Real						m_scanRange;
	Real						m_velocityFactor;			///< PredictTargetVelocityFactor -- now actually used, see .cpp.
	UnsignedInt			m_maxTrackedTargets;	///< MaxTrackedTargets: how many candidates to cache per scan (clamped to PDU2_MAX_TRACKED_TARGETS).
	Real					m_minimumInterceptRange;	///< MinimumInterceptRange: never intercept a target closer than this (0 = disabled). Lets a projectile through once its shooter has already breached this inner radius, instead of relying on the Weapon's own MinimumAttackRange (which fireWeapon() enforces silently and would otherwise jam our reload timer -- see .cpp).

	PointDefenseUpdateV2ModuleData();
	static void buildFieldParse(MultiIniFieldParse& p);

private:

};

//-------------------------------------------------------------------------------------------------
/** Reworked point-defense targeting/firing update. */
//-------------------------------------------------------------------------------------------------
class PointDefenseUpdateV2 : public UpdateModule
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( PointDefenseUpdateV2, "PointDefenseUpdateV2" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( PointDefenseUpdateV2, PointDefenseUpdateV2ModuleData );

public:

	PointDefenseUpdateV2( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	virtual void onObjectCreated() override;
	virtual UpdateSleepTime update() override;

protected:

	Bool scanForTargets();		///< Full scan. Populates m_trackedTargets[]. Returns true if anything was found in ScanRange at all (friend or foe -- used only to decide whether to idle-sleep).
	void fireWhenReady();			///< Walks m_trackedTargets[], drops dead/stale entries, fires at whichever tracked target is currently in range and off cooldown.
	void mergeBucket( const ObjectID *ids, Int count );	///< helper: appends up to the configured cap into m_trackedTargets[]

	ObjectID m_trackedTargets[PDU2_MAX_TRACKED_TARGETS];
	Int      m_numTracked;
	Bool     m_inRange;
	Int      m_nextScanFrames;
	Int      m_nextShotAvailableInFrames;
};
