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
//
//         GeneralsMod @feature Dimitar 02/10/2026: optional block limit + cooldown ("charges").
//           - BlocksBeforeCooldown: after this many intercepts (shots that actually fired) the module
//             goes on cooldown for CooldownDuration -- a real engine sleep, zero per-frame cost --
//             then comes back fully recharged. 0 (default) = block forever, exactly as before. Both
//             fields must be > 0 for cooldown to ever happen.
//           - BlockCounterResetTime: optional. If this long passes with no intercept, the partial
//             count starts over from 0 (so 5 blocks spread over 10 minutes don't trigger a cooldown).
//             0 (default) = the count never decays.
//           - The cooldown timer is absolute (frame-based), so it keeps running while the unit is
//             disabled (EMP, underpowered...). If it expires while disabled, the module comes back
//             on the first frame the unit is no longer disabled (a disabled unit can't block anyway).
//           - Visual feedback: ActiveConditionState / CooldownConditionState (optional model
//             condition flags, e.g. USER_1 / USER_2) are set during the matching phase and cleared
//             during the other.
//           - SwitchStateTemplate (optional): drives a SwitchStateV2 module on the same object (matched
//             by its SpecialPowerTemplate) on every transition, via forceSwitchState().
//             AlteredStateWhenOnCooldown picks which phase maps to AlteredState: Yes = Altered while
//             on cooldown, No (default) = Altered while active/charged. In/Out effects of the
//             SwitchStateV2 fire on real transitions only, not on the silent initial sync at spawn.
//             Leave Lifetime = 0 on that SwitchStateV2 -- this module owns its state.
//
//         GeneralsMod @feature Dimitar 05/10/2026: ShieldHealth -- a health pool instead of counting blocks.
//           - ShieldHealth > 0: the shield has this much health. Every projectile our shot destroys drains
//             it by that projectile's OWN damage (the PrimaryDamage of the weapon it would have detonated
//             with) x DrainMultiplier -- so an aurora bomb drains far more than an infantry missile, with
//             no INI changes on the projectiles. Projectiles whose weapon can't be read drain DefaultDrain.
//             It's charged once, on the shot that destroys the projectile.
//           - "Last stand": the shield always blocks what it shoots, even if that projectile is worth more
//             than the health left -- the health just hits 0. At 0 it goes on cooldown for
//             CooldownDuration, exactly like running out of blocks, and comes back at full health.
//           - ShieldRegenRate (health per second, default 0): gradual regen while the shield is up and not
//             depleted, starting ShieldRegenDelay after the last drain (default 0 = immediately). No regen
//             during a cooldown -- a broken shield always waits out the full CooldownDuration.
//           - ShieldHealth and BlocksBeforeCooldown are alternatives: use one or the other. If both are
//             set, ShieldHealth wins and BlocksBeforeCooldown / BlockCounterResetTime are ignored.
//
//         GeneralsMod @feature Dimitar 02/10/2026: shield interference ("clash").
//           - InterferenceGroup: any name you invent (NOT the ModuleTag). Off when omitted (default).
//             Two units clash only if both carry a PointDefenseUpdateV2 with the SAME group name --
//             any players, own/allied/enemy alike.
//           - Every InterferenceScanRate (default 500 ms), an active unit looks for other active units
//             of its group within InterferenceRange (center to center, 2D). If it finds any, it and all
//             of them go on cooldown on the same frame -- instantly, no grace delay. The block count
//             resets, exactly like a normal cooldown, so stacked units keep knocking each other out.
//           - InterferenceCooldownDuration: cooldown after a clash; 0 (default) = use CooldownDuration.
//             Interference works even with BlocksBeforeCooldown = 0, as long as one of the two
//             durations is > 0.
//           - HasInterference (default Yes): No = this unit is never knocked down by a clash, but it
//             still knocks down every other active unit of its group it meets (a "superpower" shield).
//             Two HasInterference = No units ignore each other. Such a unit doesn't need any cooldown
//             duration for interference to work.
//           - InterferenceFX / InterferenceOCL (optional): played/created at a unit's own position when
//             IT is knocked down by a clash, in addition to the normal cooldown visuals (condition flags,
//             SwitchStateV2 In/Out effects) -- so a clash can look different from running out of blocks.
//             Each unit uses its own INI values. An immune (HasInterference = No) unit never plays them.
//           - SwitchStateEffectsOnInterference (default Yes): No = when a clash knocks this unit down, the
//             linked SwitchStateV2 still switches state but does NOT fire its In/Out effects (FXList/OCL/
//             Weapon) for that switch -- so those effects only play when blocks run out, and the clash
//             only shows InterferenceFX/OCL. The switch back at the end of the cooldown (recharge) is
//             the same either way and plays its effects normally.
//           - "Shield not up" -- never clashes, and is never knocked down: already on cooldown, dead,
//             disabled (EMP/underpowered/...), contained (transport/building), under construction.
//           - Visuals: same path as a normal cooldown (condition flags + SwitchStateV2 link), so a clash
//             and running out of blocks look the same.
//
//         Behavior = PointDefenseUpdateV2 ModuleTag_PD
//           ...
//           BlocksBeforeCooldown       = 6
//           CooldownDuration           = 8000
//           BlockCounterResetTime      = 15000
//           CooldownConditionState     = USER_2
//           SwitchStateTemplate        = SpecialPower_PDCharge
//           AlteredStateWhenOnCooldown = No       ; charged = Altered (e.g. fast locomotor)
//           InterferenceGroup          = ShieldCrash
//           InterferenceRange          = 120
//           InterferenceScanRate       = 500
//         End
//         Behavior = SwitchStateV2 ModuleTag_PDSwitch
//           SpecialPowerTemplate = SpecialPower_PDCharge
//           DefaultState = ...                     ; while on cooldown
//           AlteredState = ...                     ; while charged
//           Lifetime     = 0
//         End
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "Common/KindOf.h"
#include "GameLogic/Module/UpdateModule.h"

// FORWARD REFERENCES /////////////////////////////////////////////////////////////////////////////
class ThingTemplate;
class WeaponTemplate;
class SpecialPowerTemplate;
class FXList;
class ObjectCreationList;
enum ModelConditionFlagType CPP_11(: Int);

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
	UnsignedInt			m_blocksBeforeCooldown;		///< BlocksBeforeCooldown: intercepts per charge before going on cooldown. 0 = never (block forever).
	UnsignedInt			m_cooldownFrames;					///< CooldownDuration: how long the cooldown sleep lasts (parsed from ms).
	UnsignedInt			m_blockCounterResetFrames;	///< BlockCounterResetTime: a gap this long with no intercept resets the partial count. 0 = never resets.
	Real						m_shieldHealth;							///< ShieldHealth: health pool drained by intercepted projectiles' damage. 0 = off (use BlocksBeforeCooldown instead).
	Real						m_drainMultiplier;					///< DrainMultiplier: scales the drain (default 1.0)
	Real						m_defaultDrain;							///< DefaultDrain: drain for projectiles whose detonation weapon can't be read (default 0)
	Real						m_shieldRegenRate;					///< ShieldRegenRate: health per second regained while up (default 0 = none)
	UnsignedInt			m_shieldRegenDelayFrames;		///< ShieldRegenDelay: wait this long after the last drain before regen starts (default 0)
	ModelConditionFlagType m_activeConditionState;		///< ActiveConditionState: optional flag set while active (charged). MODELCONDITION_INVALID = unused.
	ModelConditionFlagType m_cooldownConditionState;	///< CooldownConditionState: optional flag set while on cooldown. MODELCONDITION_INVALID = unused.
	const SpecialPowerTemplate* m_switchStateTemplate;	///< SwitchStateTemplate: optional SpecialPowerTemplate of a SwitchStateV2 on the same object to drive. nullptr = none.
	Bool						m_alteredStateWhenOnCooldown;	///< AlteredStateWhenOnCooldown: Yes = SwitchStateV2 Altered while on cooldown; No (default) = Altered while active.
	NameKeyType			m_interferenceGroup;				///< InterferenceGroup: free name, hashed at INI load. NAMEKEY_INVALID (omitted) = interference off.
	Real						m_interferenceRange;				///< InterferenceRange: center-to-center 2D distance that counts as a clash.
	UnsignedInt			m_interferenceScanFrames;		///< InterferenceScanRate: frames between interference checks (parsed from ms; default 500 ms).
	UnsignedInt			m_interferenceCooldownFrames;	///< InterferenceCooldownDuration: cooldown after a clash (parsed from ms). 0 = use CooldownDuration.
	Bool						m_hasInterference;					///< HasInterference: Yes (default) = can be knocked down by a clash; No = immune, but still knocks down others.
	const FXList*						m_interferenceFX;		///< InterferenceFX: played at this unit when a clash knocks it down (nullptr = none)
	const ObjectCreationList*	m_interferenceOCL;	///< InterferenceOCL: created at this unit when a clash knocks it down (nullptr = none)
	Bool						m_switchStateEffectsOnInterference;	///< SwitchStateEffectsOnInterference: Yes (default) = linked SwitchStateV2 fires its effects on a clash too; No = silent switch on a clash
	Real					m_minimumInterceptRange;	///< MinimumInterceptRange: never intercept a target closer than this (0 = disabled). Lets a projectile through once its shooter has already breached this inner radius, instead of relying on the Weapon's own MinimumAttackRange (which fireWeapon() enforces silently and would otherwise jam our reload timer -- see .cpp).

	PointDefenseUpdateV2ModuleData();
	static void buildFieldParse(MultiIniFieldParse& p);
	static void parseInterferenceGroup( INI* ini, void *instance, void *store, const void* userData );

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

	// GeneralsMod @feature Dimitar 02/10/2026: interference, called on OTHER units' instances by the one that detects the clash
	NameKeyType getInterferenceGroup() const;	///< NAMEKEY_INVALID = interference off
	Bool isInterferenceShieldUp() const;			///< can this instance take part in a clash right now? (see header comment)
	Bool hasInterference() const;							///< HasInterference: can a clash knock this instance down?
	void startInterferenceCooldown();					///< knocked down by someone else's clash: enter cooldown and reschedule our own update

protected:

	Bool scanForTargets();		///< Full scan. Populates m_trackedTargets[]. Returns true if anything was found in ScanRange at all (friend or foe -- used only to decide whether to idle-sleep).
	void fireWhenReady();			///< Walks m_trackedTargets[], drops dead/stale entries, fires at whichever tracked target is currently in range and off cooldown.
	void mergeBucket( const ObjectID *ids, Int count );	///< helper: appends up to the configured cap into m_trackedTargets[]

	// GeneralsMod @feature Dimitar 02/10/2026: block limit / cooldown
	void registerBlock();								///< called for every shot that actually fired; applies BlockCounterResetTime and counts it
	Bool isBlockLimitReached() const;		///< true when the shield is used up: BlocksBeforeCooldown reached, or ShieldHealth at 0 (needs CooldownDuration)
	Bool usesShieldHealth() const;			///< ShieldHealth mode (overrides BlocksBeforeCooldown)
	Real getProjectileDrain( Object *projectile ) const;	///< that projectile's detonation damage x DrainMultiplier, or DefaultDrain
	void regenShield();									///< ShieldRegenRate, by elapsed frames since the last call (works across idle sleeps)
	UpdateSleepTime enterCooldown( UnsignedInt cooldownFrames, Bool fromInterference );	///< drops targets, flips visuals/switch state, returns the cooldown sleep
	UnsignedInt getInterferenceCooldownFrames() const;	///< InterferenceCooldownDuration, or CooldownDuration if that's 0
	void fireInterferenceEffects();			///< InterferenceFX / InterferenceOCL at our own position
	Bool checkInterference();						///< scans for same-group active units in range; knocks them all down and returns TRUE on a clash
	void exitCooldown();								///< recharges: resets count/reload/scan, flips visuals/switch state back
	void applyPhaseVisuals( Bool onCooldown, Bool fireEffects );	///< condition flags + SwitchStateV2 link for the given phase

	ObjectID m_trackedTargets[PDU2_MAX_TRACKED_TARGETS];
	Int      m_numTracked;
	Bool     m_inRange;
	Int      m_nextScanFrames;
	Int      m_nextShotAvailableInFrames;

	// GeneralsMod @feature Dimitar 02/10/2026: block limit / cooldown
	Int					m_blocksThisCycle;			///< intercepts since the last recharge (or counter reset)
	UnsignedInt	m_lastBlockFrame;				///< frame of the most recent intercept (for BlockCounterResetTime)
	Real				m_currentShieldHealth;	///< ShieldHealth mode: current pool
	UnsignedInt	m_lastDrainFrame;				///< ShieldHealth mode: frame of the last drain (for ShieldRegenDelay)
	UnsignedInt	m_lastRegenFrame;				///< ShieldHealth mode: frame regen was last applied up to
	UnsignedInt	m_cooldownEndFrame;			///< absolute frame the cooldown ends; 0 = active (not on cooldown)
	UnsignedInt	m_nextInterferenceCheckFrame;	///< absolute frame of the next interference check
	Bool				m_phaseVisualsApplied;	///< not saved: the initial visual/switch sync is done on the first update (Drawable isn't bound in onObjectCreated) and re-done in loadPostProcess
};
