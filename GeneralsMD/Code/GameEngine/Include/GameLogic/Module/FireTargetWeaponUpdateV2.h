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

// FILE: FireTargetWeaponUpdateV2.h ///////////////////////////////////////////////////////////////
// Desc:   GeneralsMod @feature Dimitar 07/10/2026: offensive sibling of PointDefenseUpdateV2.
//         Fires multi-target volleys: several shots per volley, at different targets, spread around
//         the unit. Shares PointDefenseUpdateV2's skeleton (cached scan, Anti* enforcement,
//         charges/cooldown, condition flags, SwitchStateV2 link) but has NO shield / interference
//         features, and is a separate class on purpose so PointDefenseUpdateV2 stays simple.
//
//         TWO MODES
//           - Self-targeting (TriggerWeaponSlot omitted): the module scans for enemies itself and fires
//             its own WeaponTemplate on its own schedule (the weapon's DelayBetweenShots). The unit can't
//             be ordered to attack with it.
//           - Companion (GeneralsMod @feature Dimitar 09/10/2026, TriggerWeaponSlot = PRIMARY/SECONDARY/
//             TERTIARY): give the unit the weapon as a normal WeaponSet weapon. Attack orders, auto-acquire,
//             turning and reloading are the engine's. Every time that slot's weapon ACTUALLY fires, its shot
//             counts as shot 1 of the volley and the module adds ShotsPerVolley - 1 extra shots at other
//             enemies in range, then sleeps until the next real shot (no scanning while idle). The ordered
//             target counts as already picked for the spread; if nobody else is in range the extras hit it
//             too. Ground shots (force-fire at a position) get extras at enemies near the unit. The extras
//             use the weapon that just fired (so weapon-set upgrades follow) unless WeaponTemplate is set.
//             Hooked through Object::registerShotFiredListener() / BehaviorModule::onOwnerShotFired().
//             IMPORTANT: a shot that arrives while the previous volley's extras are still pending gets NO
//             extras (and isn't counted). Make the weapon's reload (DelayBetweenShots, and ClipReloadTime
//             for clip weapons) longer than (ShotsPerVolley - 1) x VolleyShotDelay. Every shot of a clip
//             triggers its own volley.
//
//         TARGETS
//           - PrimaryTargetTypes / SecondaryTargetTypes / TertiaryTargetTypes: three priority tiers.
//             A lower tier only gets shots the tiers above it can't fill.
//           - TargetPriority: ordering within a tier. NEAREST (default), FARTHEST, LOWEST_HEALTH,
//             HIGHEST_HEALTH, LOWEST_HEALTH_PERCENT, HIGHEST_HEALTH_PERCENT, MOST_EXPENSIVE (template
//             build cost), RANDOM (synced logic RNG). Ties go to the nearest.
//           - MaxTrackedTargets (1..16): how many candidates each scan keeps. Keep it comfortably above
//             ShotsPerVolley (e.g. 8 for 3 shots) so the spread has choice.
//           - MinimumTargetRange: never shoot at anything closer than this (the weapon's own
//             MinimumAttackRange is folded in automatically, since fireWeapon() silently refuses those).
//           - ScanRange: self-targeting mode: search radius, must be larger than the weapon's AttackRange.
//             Companion mode: optional (the weapon's range is used if it's smaller).
//
//         VOLLEYS
//           - ShotsPerVolley (1..16): shots per volley. In companion mode this INCLUDES the unit's own
//             shot, so 3 = the real shot + 2 extras.
//           - MinTargetSeparationAngle (degrees): spread. A target is skipped if its direction from us is
//             closer than this angle to a target already picked (3 shots + 120 = a triangle). Leftover
//             shots go to the best remaining targets regardless of angle; with fewer targets than shots,
//             shots cycle over the targets. The spread is cosmetic -- it never costs a shot.
//           - VolleyShotDelay (ms, default 0): gap between the module's shots (in companion mode: from the
//             real shot to the first extra, and between extras). A shot whose target died re-aims at
//             another live target of the volley, then at any valid target in range, and is only dropped
//             if there is nothing at all to shoot.
//           - Self-targeting mode: the weapon's DelayBetweenShots (with the unit's real ROF bonus) is the
//             gap between volley starts. ClipSize / ClipReloadTime are ignored (fresh weapon per shot).
//
//         WHERE SHOTS START
//           - WeaponSlot = PRIMARY / SECONDARY / TERTIARY: the module's shots are created in that slot, so
//             the Draw module's bones for that slot are used. Default: TriggerWeaponSlot in companion mode,
//             else PRIMARY. Only the bones are borrowed -- the unit's own weapon in that slot is untouched.
//             Every module shot uses barrel 01 of that slot.
//
//         CHARGES / COOLDOWN (counted in volleys)
//           - VolleysBeforeCooldown: after this many volleys, cooldown for CooldownDuration. 0 = never.
//             The cooldown starts once the last volley's delayed shots are done. In companion mode a
//             cooldown only holds the EXTRA shots -- the unit's own weapon keeps firing normally.
//           - VolleyCounterResetTime: a gap this long with no volley resets the partial count. 0 = never.
//           - ActiveConditionState / CooldownConditionState, SwitchStateTemplate,
//             AlteredStateWhenOnCooldown: exactly as on PointDefenseUpdateV2.
//
//         ; companion mode: the unit's PRIMARY weapon is TeslaArcWeapon (DelayBetweenShots = 1500)
//         Behavior = FireTargetWeaponUpdateV2 ModuleTag_Lightning
//           TriggerWeaponSlot         = PRIMARY
//           PrimaryTargetTypes        = VEHICLE INFANTRY
//           SecondaryTargetTypes      = STRUCTURE
//           MaxTrackedTargets         = 8
//           ShotsPerVolley            = 3          ; real shot + 2 extras
//           MinTargetSeparationAngle  = 100
//           VolleyShotDelay           = 150        ; 2 extras = 300 ms < 1500 ms reload
//           TargetPriority            = LOWEST_HEALTH_PERCENT
//         End
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "Common/KindOf.h"
#include "GameLogic/Module/UpdateModule.h"

// FORWARD REFERENCES /////////////////////////////////////////////////////////////////////////////
class Weapon;
class WeaponTemplate;
class WeaponBonus;
class SpecialPowerTemplate;
enum ModelConditionFlagType CPP_11(: Int);

enum
{
	FTWU2_MAX_TRACKED_TARGETS	= 16,		///< cap for MaxTrackedTargets
	FTWU2_MAX_SHOTS_PER_VOLLEY	= 16,		///< cap for ShotsPerVolley
	FTWU2_NUM_TIERS						= 3,		///< Primary / Secondary / Tertiary
	FTWU2_NO_SLOT							= -1		///< WeaponSlot / TriggerWeaponSlot not set
};

/// TargetPriority values (order matches the lookup list in the .cpp)
enum FireTargetPriorityType CPP_11(: Int)
{
	FTW_PRIORITY_NEAREST = 0,
	FTW_PRIORITY_FARTHEST,
	FTW_PRIORITY_LOWEST_HEALTH,
	FTW_PRIORITY_HIGHEST_HEALTH,
	FTW_PRIORITY_LOWEST_HEALTH_PERCENT,
	FTW_PRIORITY_HIGHEST_HEALTH_PERCENT,
	FTW_PRIORITY_MOST_EXPENSIVE,
	FTW_PRIORITY_RANDOM,

	FTW_PRIORITY_COUNT
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
class FireTargetWeaponUpdateV2ModuleData : public ModuleData
{
public:
	WeaponTemplate	*m_weaponTemplate;				///< WeaponTemplate: required in self-targeting mode; optional override in companion mode
	Int							m_triggerWeaponSlot;			///< TriggerWeaponSlot: WeaponSlotType that triggers volleys (companion mode). FTWU2_NO_SLOT = self-targeting.
	Int							m_weaponSlot;							///< WeaponSlot: slot the module's shots are created in (fire bones). FTWU2_NO_SLOT = trigger slot / PRIMARY.
	KindOfMaskType	m_primaryTargetKindOf;
	KindOfMaskType	m_secondaryTargetKindOf;
	KindOfMaskType	m_tertiaryTargetKindOf;
	UnsignedInt			m_scanFrames;							///< ScanRate (self-targeting mode)
	UnsignedInt			m_idleScanFrames;					///< IdleScanRate (self-targeting mode)
	Real						m_scanRange;
	Real						m_velocityFactor;					///< PredictTargetVelocityFactor
	UnsignedInt			m_maxTrackedTargets;			///< MaxTrackedTargets (clamped to FTWU2_MAX_TRACKED_TARGETS)
	Real						m_minimumTargetRange;			///< MinimumTargetRange
	Int							m_targetPriority;					///< TargetPriority (FireTargetPriorityType)
	UnsignedInt			m_shotsPerVolley;					///< ShotsPerVolley (clamped to FTWU2_MAX_SHOTS_PER_VOLLEY; includes the real shot in companion mode)
	Real						m_minTargetSeparationAngle;	///< MinTargetSeparationAngle (INI degrees, stored radians). 0 = no spread.
	UnsignedInt			m_volleyShotDelayFrames;	///< VolleyShotDelay (ms -> frames)
	UnsignedInt			m_volleysBeforeCooldown;	///< VolleysBeforeCooldown. 0 = never cool down.
	UnsignedInt			m_cooldownFrames;					///< CooldownDuration
	UnsignedInt			m_volleyCounterResetFrames;	///< VolleyCounterResetTime. 0 = count never decays.
	ModelConditionFlagType m_activeConditionState;		///< ActiveConditionState
	ModelConditionFlagType m_cooldownConditionState;	///< CooldownConditionState
	const SpecialPowerTemplate* m_switchStateTemplate;	///< SwitchStateTemplate (SwitchStateV2 on the same object)
	Bool						m_alteredStateWhenOnCooldown;	///< AlteredStateWhenOnCooldown

	FireTargetWeaponUpdateV2ModuleData();
	static void buildFieldParse(MultiIniFieldParse& p);

	Bool isCompanionMode() const { return m_triggerWeaponSlot != FTWU2_NO_SLOT; }
};

//-------------------------------------------------------------------------------------------------
/** Fires multi-target volleys, either on its own (self-targeting) or as extras to the unit's own shots (companion). */
//-------------------------------------------------------------------------------------------------
class FireTargetWeaponUpdateV2 : public UpdateModule
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( FireTargetWeaponUpdateV2, "FireTargetWeaponUpdateV2" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( FireTargetWeaponUpdateV2, FireTargetWeaponUpdateV2ModuleData );

public:

	FireTargetWeaponUpdateV2( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	virtual void onObjectCreated() override;
	virtual UpdateSleepTime update() override;

	/// companion mode: the owner just fired one of its own WeaponSet weapons (see Object::notifyShotFiredListeners)
	virtual void onOwnerShotFired( const Weapon* weaponFired, ObjectID victimID ) override;

protected:

	// --- per-volley range info (uses the unit's real weapon bonus) ---
	struct RangeInfo
	{
		Real fireRangeSq;
		Real minRangeSq;
	};
	void computeRanges( RangeInfo &ri, const WeaponTemplate *tmpl ) const;
	void computeShooterBonus( WeaponBonus &bonus, const WeaponTemplate *tmpl ) const;	///< same as Weapon::computeBonus() (protected there)
	Int  getShotSlot() const;		///< WeaponSlot, else TriggerWeaponSlot, else PRIMARY

	// --- targeting ---
	Bool scanForTargets( const RangeInfo &ri, const WeaponTemplate *tmpl, Real scanRange );	///< fills m_trackedTargets[]. TRUE if any enemy was in range.
	void pruneTrackedTargets();
	Bool isValidTarget( Object *target ) const;							///< alive, still an enemy, not hidden by stealth
	Bool isInFireRange( Object *target, const RangeInfo &ri ) const;
	Real getPriorityKey( Object *target, Real distSq ) const;	///< lower = better (ties broken by distance)
	Int  getCap( UnsignedInt value, Int maxValue ) const;
	Bool isShooterBusy() const;															///< under construction / being sold: hold fire

	// --- volleys ---
	/// Picks the volley's targets (tiers, priority, spread) from the tracked list and arms the volley.
	/// primaryID: companion mode's real-shot victim (excluded from the picks, but counts for the spread and
	/// is cycled back to when there aren't enough other targets). Returns FALSE if there is nothing to shoot.
	Bool startVolley( const RangeInfo &ri, const WeaponTemplate *tmpl, ObjectID primaryID, Int shots, UnsignedInt firstShotFrame );
	void continueVolley( const RangeInfo &ri );							///< fires every shot of the running volley that's due this frame
	Object* resolveShotTarget( Int shotIndex, const RangeInfo &ri );
	Bool fireShotAt( Object *target, const WeaponTemplate *tmpl );	///< TRUE if the shot actually went out
	Bool isVolleyInProgress() const { return m_volleyNextShot < m_volleyShotCount; }
	void endVolley();

	// --- update paths ---
	UpdateSleepTime updateSelfTargeting();
	UpdateSleepTime updateCompanion();

	// --- charges / cooldown ---
	void registerVolley();
	Bool isVolleyLimitReached() const;
	UpdateSleepTime enterCooldown();
	void exitCooldown();
	void applyPhaseVisuals( Bool onCooldown, Bool fireEffects );

	// tracked candidates (scan output), best first: in-range Primary, Secondary, Tertiary, then out-of-range
	ObjectID		m_trackedTargets[FTWU2_MAX_TRACKED_TARGETS];
	Int					m_trackedTier[FTWU2_MAX_TRACKED_TARGETS];
	Int					m_numTracked;
	Int					m_nextScanFrames;				///< self-targeting: countdown (in update() calls) to the next full scan

	// volley in progress
	ObjectID		m_volleyTargets[FTWU2_MAX_SHOTS_PER_VOLLEY];
	Int					m_volleyShotCount;			///< shots in the running volley (0 = none)
	Int					m_volleyNextShot;				///< index of the next shot to fire
	UnsignedInt	m_volleyNextShotFrame;	///< absolute frame the next shot is due
	Bool				m_volleyCounted;				///< TRUE once the volley was counted (self-targeting: first shot out; companion: at trigger)
	UnsignedInt	m_nextVolleyFrame;			///< self-targeting: absolute frame a new volley may start (DelayBetweenShots)
	const WeaponTemplate*	m_volleyWeaponTemplate;	///< weapon the running volley fires (saved by name)

	// charges / cooldown
	Int					m_volleysThisCycle;
	UnsignedInt	m_lastVolleyFrame;
	UnsignedInt	m_cooldownEndFrame;			///< 0 = active
	Bool				m_phaseVisualsApplied;	///< not saved: initial visual sync happens on the first update / loadPostProcess
};
