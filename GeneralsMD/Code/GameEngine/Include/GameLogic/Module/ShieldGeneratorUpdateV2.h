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

// FILE: ShieldGeneratorUpdateV2.h //////////////////////////////////////////////////////////////////
//
//	GeneralsMod @feature Dimitar 13/09/2026, redesigned 13/09/2026
//	Temporary "shield" module: on activation (via the companion ShieldGeneratorActivateV2
//	special power button, matched by SpecialPowerTemplate -- same pairing convention SwitchStateV2 /
//	SwitchStateV2Activate use), grants a temporary pool of absorbing health -- ShieldAmount (flat)
//	plus ShieldAmount% (a percentage of max health at the moment of activation, cumulative with the
//	flat amount) -- sets an optional ConditionStateType model condition flag for the duration, and
//	fires optional WeaponIn/OCLIn/FXListIn effects once. The pool is removed again -- either after
//	Lifetime frames, or earlier if RevertEarlyWhenDepleted is Yes and the pool is used up -- firing
//	WeaponOut/OCLOut/FXListOut (optionally only on the depleted case, see RunOutLogicOnlyWhenDepleted)
//	and clearing ConditionStateType.
//
//	SECOND REVISION -- this module does NOT touch the object's real max health at all (an earlier
//	version did, via ActiveBody::setMaxHealth(); see the project's module checklist doc for the full
//	postmortem on why that approach was abandoned). Instead it tracks its own private absorption pool
//	(m_shieldPool) and, in onDamage(), immediately refunds back whatever portion of each qualifying
//	hit the remaining pool can cover (via BodyModuleInterface::internalChangeHealth(), which bypasses
//	armor/DamageFX/callbacks -- appropriate here since this is a correction, not a hit). Only the
//	overflow beyond the pool ever survives as real, permanent damage. Reasons this is better:
//
//	  1. DamageTypes filtering falls out for free. A flat max-health boost has no way to say "only
//	     protect against EXPLOSION" -- every hit reduces the same single health number regardless of
//	     type. An absorption pool just skips the refund for a non-matching DamageInfo::in.m_damageType,
//	     leaving that hit to deal full, ordinary damage untouched. See DamageTypes below.
//	  2. Real max health never changes, so BodyDamageType (PRISTINE/DAMAGED/REALLYDAMAGED/RUBBLE) --
//	     which Locomotor reads every frame for movement penalties, and which drives the DAMAGED/
//	     REALLYDAMAGED model conditions -- is only ever affected by genuine, permanent damage, never
//	     by the shield itself. (For extra safety this module also opportunistically freezes/unfreezes
//	     the object's reported damage state via a companion ShieldedBody module, if the object's Body
//	     happens to be one -- see ShieldedBody.h. With the pool design this is normally a no-op safety
//	     net rather than load-bearing, since max health is never mutated, but it costs nothing to wire
//	     in and protects against any future variant of this module that DOES touch health again.)
//	  3. The health bar only ever reflects real, permanent changes -- no more heal-then-unheal look on
//	     activation, and no "restore max health" math or edge cases on reversion at all.
//
//	This is a SEPARATE module from SwitchStateV2 rather than an addition to it, specifically so the
//	shield can end (and change the model condition back) the moment its pool is depleted, without
//	that also having to mean "the Lifetime timer expired" -- SwitchStateV2's single revert path
//	conflates those two ideas, which doesn't fit a shield that needs a visible "shield broke" moment
//	distinct from "shield duration ran out". Being a separate module (with its own SpecialPowerTemplate)
//	also means an object can carry more than one of these independently (e.g. two different shield
//	abilities with different ConditionStateType flags) without them interfering with each other.
//
//	Usage -- give the target object both modules, sharing the same SpecialPowerTemplate:
//
//	Behavior = ShieldGeneratorActivateV2 ModuleTag_01
//	  SpecialPowerTemplate = SpecialPower_Shield
//	End
//	Behavior = ShieldGeneratorUpdateV2 ModuleTag_02
//	  SpecialPowerTemplate      = SpecialPower_Shield
//	  Lifetime                  = 10000            ; 0 = shield never expires on its own (still ends
//	                                                ; early on depletion if RevertEarlyWhenDepleted)
//	  WeaponIn                  = ...               ; optional; fired once (at self) on activation
//	  WeaponOut                 = ...               ; optional; fired once (at self) on reversion
//	  OCLIn                     = ...               ; optional; created once (at self) on activation
//	  OCLOut                    = ...               ; optional; created once (at self) on reversion
//	  FXListIn                  = ...               ; optional; played once (at self) on activation
//	  FXListOut                 = ...               ; optional; played once (at self) on reversion
//	  ShieldAmount              = 500.0             ; flat size of the absorption pool
//	  ShieldAmount%             = 0%                ; percentage of max health at activation time,
//	                                                ; ADDED to ShieldAmount above (not multiplicative)
//	  DamageTypes               = ALL               ; which damage types the pool absorbs -- same
//	                                                ; NONE/ALL/+X/-X token syntax as every other
//	                                                ; DamageTypeFlags field in the engine (see
//	                                                ; INI::parseDamageTypeFlags); e.g.
//	                                                ; "DamageTypes = NONE +ARMOR_PIERCING +EXPLOSION"
//	                                                ; to only shield against those two. Damage types
//	                                                ; NOT in this list pass straight through to real
//	                                                ; health, completely unaffected by the pool.
//	  ConditionStateType        = USER_1            ; optional; ModelConditionFlagType set while
//	                                                ; shielded, cleared on reversion. MODELCONDITION_
//	                                                ; prefix omitted, same as SwitchStateV2's states.
//	  RevertEarlyWhenDepleted   = Yes               ; No = pool still stops absorbing once used up,
//	                                                ; but ConditionStateType/the "active" state stays
//	                                                ; on until Lifetime expires (purely cosmetic once
//	                                                ; depleted); Yes = revert immediately once depleted
//	  RunOutLogicOnlyWhenDepleted = No              ; Yes = only fire WeaponOut/OCLOut/FXListOut when
//	                                                ; reverting because the pool was depleted, not when
//	                                                ; Lifetime simply runs out or the button is pressed
//	                                                ; again to refresh. Ignored (out logic always fires)
//	                                                ; if RevertEarlyWhenDepleted is No, since there is
//	                                                ; then no depleted-revert for it to restrict to.
//	End
//
//	Re-pressing the button while already shielded reverts the current shield and immediately grants a
//	fresh pool -- i.e. it's a refresh/recast, never a stacking bonus.
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "GameLogic/Module/SpecialPowerUpdateModule.h"
#include "GameLogic/Module/DamageModule.h"

// FORWARD REFERENCES /////////////////////////////////////////////////////////////////////////////
enum ModelConditionFlagType CPP_11(: Int);
class WeaponTemplate;
class Weapon;
class FXList;
class ObjectCreationList;
class ShieldedBody;

//-------------------------------------------------------------------------------------------------
class ShieldGeneratorUpdateV2ModuleData : public UpdateModuleData
{
public:

	const SpecialPowerTemplate*	m_specialPowerTemplate;	///< must match the SpecialPowerTemplate on the companion ShieldGeneratorActivateV2 module
	UnsignedInt										m_lifetimeFrames;				///< 0 = shield never expires on its own; otherwise, frames (parsed from milliseconds) it lasts before auto-reverting
	const WeaponTemplate*				m_weaponInTemplate;			///< optional; fired once (at our own position) on activation. nullptr if not given in INI.
	const WeaponTemplate*				m_weaponOutTemplate;		///< optional; fired once (at our own position) on reversion. nullptr if not given in INI.
	const ObjectCreationList*		m_oclIn;								///< optional; created once (at our own position) on activation
	const ObjectCreationList*		m_oclOut;								///< optional; created once (at our own position) on reversion
	const FXList*									m_fxListIn;							///< optional; played once (at our own position) on activation
	const FXList*									m_fxListOut;						///< optional; played once (at our own position) on reversion
	Real													m_shieldAmount;					///< flat size of the absorption pool
	Real													m_shieldAmountPercent;	///< percentage (as a fraction, e.g. 0.5 for "50%") of max health at activation time, ADDED to m_shieldAmount
	DamageTypeFlags								m_damageTypes;					///< which damage types the pool absorbs; non-matching types pass straight through to real health
	ModelConditionFlagType				m_conditionState;				///< optional model condition flag set while shielded, cleared on reversion. MODELCONDITION_INVALID if not given in INI.
	Bool													m_revertEarlyWhenDepleted;	///< Yes (default): revert as soon as the pool is used up, ahead of Lifetime
	Bool													m_runOutLogicOnlyWhenDepleted;	///< Yes: only fire WeaponOut/OCLOut/FXListOut on a depleted revert, not a Lifetime-expiry or refresh revert. Meaningless (ignored) if m_revertEarlyWhenDepleted is No.

	ShieldGeneratorUpdateV2ModuleData();

	static void buildFieldParse(MultiIniFieldParse& p);

};

//-------------------------------------------------------------------------------------------------
class ShieldGeneratorUpdateV2 : public SpecialPowerUpdateModule, public DamageModuleInterface
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( ShieldGeneratorUpdateV2, "ShieldGeneratorUpdateV2" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( ShieldGeneratorUpdateV2, ShieldGeneratorUpdateV2ModuleData )

public:

	ShieldGeneratorUpdateV2( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	// module methods
	static Int getInterfaceMask() { return SpecialPowerUpdateModule::getInterfaceMask() | (MODULEINTERFACE_DAMAGE); }

	// BehaviorModule
	virtual DamageModuleInterface* getDamage() override { return this; }

	// UpdateModuleInterface
	virtual UpdateSleepTime update() override;
	virtual DisabledMaskType getDisabledTypesToProcess() const override { return DISABLEDMASK_ALL; } ///< a scheduled Lifetime revert must still happen even if we get disabled/held

	// SpecialPowerUpdateInterface (reached via our companion ShieldGeneratorActivateV2's initiateIntentToDoSpecialPower)
	virtual SpecialPowerUpdateInterface* getSpecialPowerUpdateInterface() override { return this; }
	virtual Bool initiateIntentToDoSpecialPower( const SpecialPowerTemplate *specialPowerTemplate, const Object *targetObj, const Coord3D *targetPos, const Waypoint *way, UnsignedInt commandOptions ) override;
	virtual Bool isSpecialAbility() const override { return false; } ///< IMPORTANT: this is not cosmetic -- returning true here makes the engine blind-cast this SpecialPowerUpdateInterface* to a SpecialAbilityUpdate* in several places. ShieldGeneratorUpdateV2 is not a SpecialAbilityUpdate subclass, so that cast would be memory corruption.
	virtual Bool isSpecialPower() const override { return false; }
	virtual Bool isActive() const override { return false; }
	virtual CommandOption getCommandOption() const override { return (CommandOption)0; }
	virtual Bool doesSpecialPowerHaveOverridableDestinationActive() const override { return false; }
	virtual Bool doesSpecialPowerHaveOverridableDestination() const override { return false; }
	virtual void setSpecialPowerOverridableDestination( const Coord3D *loc ) override { }
	virtual Bool isPowerCurrentlyInUse( const CommandButton *command = nullptr ) const override { return false; }

	// DamageModuleInterface -- this is how the shield actually absorbs damage: refund the covered
	// portion of each qualifying hit back immediately, so only the overflow survives as real damage
	virtual void onDamage( DamageInfo *damageInfo ) override;
	virtual void onHealing( DamageInfo *damageInfo ) override { }
	virtual void onBodyDamageStateChange( const DamageInfo* damageInfo, BodyDamageType oldState, BodyDamageType newState ) override { }

protected:

	void enterShield();							///< grants the pool, sets ConditionStateType, freezes damage state (if ShieldedBody), fires WeaponIn/OCLIn/FXListIn, schedules the Lifetime revert
	void revertShield( Bool wasDepleted );	///< clears ConditionStateType, unfreezes damage state, and (unless RunOutLogicOnlyWhenDepleted says otherwise) fires WeaponOut/OCLOut/FXListOut

	ShieldedBody* findShieldedBody() const;	///< returns the object's Body module cast to ShieldedBody* if (and only if) that's genuinely what it is, else nullptr -- see ShieldedBody.h for why this lookup exists

private:

	Bool					m_isActive;			///< whether the shield is currently applied
	UnsignedInt		m_revertFrame;	///< absolute frame to auto-revert on; 0 = none scheduled
	Real					m_shieldPool;		///< remaining absorption capacity; meaningless while !m_isActive
	Weapon*				m_weaponIn;			///< allocated from m_weaponInTemplate in the constructor; nullptr if none given
	Weapon*				m_weaponOut;		///< allocated from m_weaponOutTemplate in the constructor; nullptr if none given

};
