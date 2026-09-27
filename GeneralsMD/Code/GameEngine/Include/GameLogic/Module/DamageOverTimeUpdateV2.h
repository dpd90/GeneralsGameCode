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

// FILE: DamageOverTimeUpdateV2.h ///////////////////////////////////////////////////////////////////
//
//	GeneralsMod @feature Dimitar 15/09/2026, extended 16/09/2026
//	Generic "damage over time" module: reacts to being hit by a qualifying DamageType
//	(TriggerDamageTypes below, default NONE +OVERTIME1 -- see the new DAMAGE_OVERTIME1..8 channels in
//	GameLogic/Damage.h) by dealing Amount (+ Amount% of the triggering hit's own dealt damage) again,
//	once per DamageType flagged in DealDamageTypes, every Rate frames, for a total of Lifetime frames,
//	optionally firing Weapon/OCL/FXList (all at our own position) on every one of those ticks. Modeled
//	directly on the vanilla PoisonedBehavior (same trigger-by-DamageType-in-onDamage / periodic-
//	UPDATE_SLEEP idiom) crossed with ShieldGeneratorUpdateV2's Weapon/OCL/FXList-fired-at-self
//	convention, generalized so a modder can point ANY weapon at this effect via TriggerDamageTypes
//	instead of being hardcoded to one specific flavor (flame/poison) the way the two vanilla modules
//	are.
//
//	16/09/2026: split what used to be a single dual-purpose "DamageTypes" field into two independent
//	fields -- TriggerDamageTypes (what starts/refreshes the effect) and DealDamageTypes (what tick
//	damage is dealt as) -- so the two no longer have to be the same list. Listing more than one type in
//	DealDamageTypes (e.g. "NONE +ARMOR_PIERCING +FLAME") deals the FULL Amount/Amount% separately as
//	EACH listed type, every tick -- not split across them -- so a single instance can deal a "mixed"
//	effect that's independently mitigated by a unit's AP Percent AND its FLAME Percent at once, while
//	still being triggered by whatever TriggerDamageTypes says (which need not overlap with
//	DealDamageTypes at all -- e.g. triggered only by OVERTIME1, but always deals ARMOR_PIERCING +
//	FLAME). Previously tick damage was dealt as a single captured m_triggerType (just whichever type
//	happened to trigger the effect); that capture is gone now that ticks iterate DealDamageTypes
//	directly.
//
//	Also 16/09/2026: added FireInitially (default No). When Yes, every qualifying hit -- a fresh start
//	AND a refresh of an already-active effect -- ALSO deals one tick's worth of damage/Weapon/OCL/
//	FXList immediately, right when it (re)triggers, on top of (not instead of) the normal Rate-cadence
//	tick already scheduled. Without it, the very first tick doesn't land until Rate frames after the
//	triggering hit, same as before this field existed. See startOrRefresh()'s own comment for the
//	one nuance worth knowing: the immediate tick runs synchronously from inside onDamage(), which
//	itself runs from inside the ORIGINAL hit's own attemptDamage() call -- a nested/reentrant
//	attemptDamage(), not a new top-level one like a normal scheduled tick gets from update(). This is
//	safe (ActiveBody::attemptDamage() holds no static/reentrancy-unsafe state and the object's module
//	list doesn't change during combat), but it does mean any sibling module later in the object's own
//	module list gets notified of OUR immediate tick's hit before it gets notified of the ORIGINAL
//	triggering hit that caused it, since our nested call's full module-notify pass completes before
//	the outer one resumes.
//
//	Why new DamageTypes instead of reusing DAMAGE_FLAME/DAMAGE_POISON: reusing either ties this
//	effect to their existing ArmorSet/DamageFX resistance tuning and visual baggage (FlammableUpdate's
//	own "aflame" status/model-burn state, PoisonedBehavior's green tint) that has nothing to do with a
//	generic overtime strike. DAMAGE_OVERTIME1..8 are clean, mod-owned channels with none of that --
//	see GameLogic/Damage.h (enum) and GameLogic/System/Damage.cpp (DamageTypeFlags::s_bitNameList[]).
//
//	Usage -- give the TARGET object (the thing that should be able to suffer overtime damage) this
//	module, and give the ATTACKER's Weapon a DamageType matching this module's TriggerDamageTypes:
//
//	Behavior = DamageOverTimeUpdateV2 ModuleTag_01
//	  Lifetime    = 8000              ; total duration of the effect once triggered, in msec
//	  Rate        = 1000              ; how often (msec) the tick damage/Weapon/OCL/FXList re-fire
//	  Amount      = 5.0               ; flat damage dealt on every tick
//	  Amount%     = 10%               ; PLUS this percentage of the triggering hit's own dealt
//	                                  ; damage (damageInfo->out.m_actualDamageDealt), captured once
//	                                  ; when the effect starts/refreshes -- not re-measured per tick
//	  FireInitially = No              ; optional, default No; if Yes, ALSO deals one tick's worth of
//	                                  ; damage/Weapon/OCL/FXList immediately on every qualifying hit
//	                                  ; (fresh start or refresh), instead of waiting Rate frames for
//	                                  ; the first tick to land. See the header comment above for the
//	                                  ; one nuance (nested attemptDamage()) this implies.
//	  Weapon      = ...               ; optional; fired at self on every tick (subject to the
//	                                  ; weapon's own ReadyToFire/ammo/reload state, same as any
//	                                  ; other "fire at self" module in this mod)
//	  OCL         = ...               ; optional; created at self on every tick
//	  FXList      = ...               ; optional; played at self on every tick
//	  TriggerDamageTypes = NONE +OVERTIME1   ; which incoming DamageTypes start/refresh this effect.
//	                                  ; Default shown here IS the module's actual default. 8 dedicated
//	                                  ; channels exist (OVERTIME1..OVERTIME8, see GameLogic/Damage.h)
//	                                  ; so up to 8 independently-configured instances of this module
//	                                  ; can coexist on one object without refreshing/clobbering each
//	                                  ; other -- give each instance a different single channel and a
//	                                  ; different ModuleTag. Widen to more than one type (e.g.
//	                                  ; "NONE +OVERTIME1 +FLAME") to also trigger from an existing
//	                                  ; real DamageType.
//	  DealDamageTypes    = NONE +OVERTIME1   ; which DamageType(s) tick damage is dealt as -- every
//	                                  ; flagged type gets the FULL Amount/Amount% each tick (not split
//	                                  ; across them), each passing through the target's own ArmorSet
//	                                  ; Percent coefficient for that type independently. Default shown
//	                                  ; here IS the module's actual default; does NOT have to match
//	                                  ; TriggerDamageTypes -- e.g. TriggerDamageTypes = NONE +OVERTIME1
//	                                  ; with DealDamageTypes = NONE +ARMOR_PIERCING +FLAME is a valid,
//	                                  ; and common, setup (triggered by one channel, deals a mixed
//	                                  ; real-type effect).
//	  RequiredKindOf = INFANTRY       ; optional; if set, the TARGET must match at least one listed
//	                                  ; KindOf to be affected at all (default: no restriction)
//	  ForbiddenKindOf = INFANTRY      ; optional; if the TARGET matches any listed KindOf, this
//	                                  ; instance never triggers/refreshes on it (default: none
//	                                  ; excluded). Lets a DefaultThingTemplate-wide InheritableModule
//	                                  ; instance exclude whole unit categories (e.g. INFANTRY) without
//	                                  ; a RemoveModule on every excluded object individually.
//	End
//
//	Every tick is dealt once per DamageType flagged in DealDamageTypes (NOT DAMAGE_UNRESISTABLE) --
//	each flagged type gets the full Amount/Amount% independently, so it passes through the target's
//	own ArmorSet Percent coefficient for that type exactly like any other hit of that type would. This
//	is what makes overtime damage scale with armor: give a unit's Armor template a reduced (or
//	increased) Percent for OVERTIME3, say, and both the initial hit AND every subsequent tick are
//	affected by it identically -- and it's also what lets a single instance deal a genuinely mixed
//	effect (DealDamageTypes = NONE +ARMOR_PIERCING +FLAME deals full Amount as BOTH types, every tick,
//	each mitigated independently). The obvious risk -- a tick's own damage re-triggering/refreshing us
//	forever, if a type in DealDamageTypes also happens to be flagged in TriggerDamageTypes -- is closed
//	by m_dealingTick, a reentrancy flag set for the duration of the whole tick-dealing loop in
//	update(); onDamage() ignores anything that arrives while it's set (this guards THIS instance
//	against itself only -- a different DamageOverTimeUpdateV2 instance on the same object that also
//	watches one of our dealt types is a separate, deliberately out-of-scope case; the intended 3
//	KindOf x 8 OVERTIME-channel usage pattern, see the project checklist doc, never puts two instances
//	watching the same channel on one object in the first place). Getting hit again by a qualifying
//	DamageType (from an outer, non-self source) while already active is a REFRESH, not a stack:
//	Amount/Amount% is recaptured from the newest qualifying hit, Lifetime restarts from now, but the
//	in-progress Rate cadence is preserved (same "don't push the next tick further out" logic
//	PoisonedBehavior::startPoisonedEffects() uses). Being healed (onHealing) cancels the effect
//	outright, same as PoisonedBehavior.
//
//	Intended to be usable from DefaultThingTemplate (wrapped in InheritableModule) so any unit in the
//	game CAN be hit with this, while staying completely inert (no per-frame ticking at all -- see the
//	constructor's UPDATE_SLEEP_FOREVER) until something actually deals it OVERTIME-flagged damage.
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "GameLogic/Module/UpdateModule.h"
#include "GameLogic/Module/DamageModule.h"

// FORWARD REFERENCES /////////////////////////////////////////////////////////////////////////////
class WeaponTemplate;
class Weapon;
class FXList;
class ObjectCreationList;

//-------------------------------------------------------------------------------------------------
class DamageOverTimeUpdateV2ModuleData : public UpdateModuleData
{
public:

	UnsignedInt										m_lifetimeFrames;			///< total duration of the effect once triggered/refreshed
	UnsignedInt										m_rateFrames;					///< how often the tick damage/Weapon/OCL/FXList re-fire
	Real													m_amount;							///< flat damage dealt on every tick
	Real													m_amountPercent;			///< PLUS this fraction (0.10 for "10%") of the triggering hit's own dealt damage, captured once per trigger/refresh
	Bool													m_fireInitially;			///< if TRUE, ALSO deal one tick's worth of damage/Weapon/OCL/FXList immediately on every qualifying hit (start or refresh), instead of waiting Rate frames for the first tick; default FALSE
	const WeaponTemplate*				m_weaponTemplate;			///< optional; fired at our own position on every tick. nullptr if not given in INI.
	const ObjectCreationList*		m_ocl;								///< optional; created at our own position on every tick
	const FXList*									m_fxList;							///< optional; played at our own position on every tick
	DamageTypeFlags								m_triggerDamageTypes;	///< which incoming DamageTypes start/refresh this effect; default NONE +OVERTIME1. Independent of m_dealDamageTypes -- does not have to overlap with it
	DamageTypeFlags								m_dealDamageTypes;		///< which DamageType(s) tick damage is dealt as -- each flagged type gets the full Amount/Amount%, independently, every tick; default NONE +OVERTIME1. Independent of m_triggerDamageTypes -- does not have to overlap with it
	KindOfMaskType								m_requiredKindOf;			///< if any bits set, the TARGET must match at least one to be affected; empty (default) = no restriction
	KindOfMaskType								m_forbiddenKindOf;		///< if the TARGET matches any bit set here, this effect never triggers/refreshes on it; empty (default) = no exclusion

	DamageOverTimeUpdateV2ModuleData();

	static void buildFieldParse(MultiIniFieldParse& p);

};

//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
class DamageOverTimeUpdateV2 : public UpdateModule,
																public DamageModuleInterface
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( DamageOverTimeUpdateV2, "DamageOverTimeUpdateV2" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( DamageOverTimeUpdateV2, DamageOverTimeUpdateV2ModuleData )

public:

	DamageOverTimeUpdateV2( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	static Int getInterfaceMask() { return UpdateModule::getInterfaceMask() | (MODULEINTERFACE_DAMAGE); }

	// BehaviorModule
	virtual DamageModuleInterface* getDamage() override { return this; }

	// DamageModuleInterface
	virtual void onDamage( DamageInfo *damageInfo ) override;
	virtual void onHealing( DamageInfo *damageInfo ) override;
	virtual void onBodyDamageStateChange( const DamageInfo* damageInfo, BodyDamageType oldState, BodyDamageType newState ) override { }

	// UpdateModuleInterface
	virtual UpdateSleepTime update() override;
	virtual DisabledMaskType getDisabledTypesToProcess() const override { return DISABLEDMASK_ALL; } ///< keep ticking even if disabled/held -- an overtime strike shouldn't be shrugged off by a stun

protected:

	void startOrRefresh( const DamageInfo *damageInfo );
	void stopEffect();
	UpdateSleepTime calcSleepTime();
	void dealTickDamage();	///< deals one tick's worth of damage (once per DealDamageTypes flag); shared by update()'s normal cadence and startOrRefresh()'s FireInitially immediate tick
	void fireTickEffects();	///< fires Weapon/OCL/FXList at self; called once per tick from update() (and from startOrRefresh() when FireInitially)

private:

	UnsignedInt		m_tickFrame;		///< absolute frame of the next tick; 0 = not currently active
	UnsignedInt		m_stopFrame;		///< absolute frame the effect ends; 0 = not currently active
	Real					m_tickAmount;		///< damage dealt on every tick, captured at trigger/refresh time (Amount + Amount% * triggering hit's dealt damage)
	ObjectID			m_sourceID;			///< source of the triggering hit, credited on every tick (for XP/kill credit)
	DeathType			m_deathType;		///< death type of the triggering hit, used if a tick kills us
	Bool					m_dealingTick;	///< TRUE only during the tick-dealing loop in dealTickDamage() -- lets onDamage() tell a self-inflicted tick apart from a genuine new hit, in case a type in DealDamageTypes is also flagged in TriggerDamageTypes
	Weapon*				m_weapon;				///< allocated from m_weaponTemplate in the constructor; nullptr if none given

};

