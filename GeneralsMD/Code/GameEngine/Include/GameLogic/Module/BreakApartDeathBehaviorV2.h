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

// FILE: BreakApartDeathBehaviorV2.h ///////////////////////////////////////////////////////////
// GeneralsMod @feature Dimitar 19/09/2026, redesigned 19/09/2026
// Desc: Standalone DieModule that breaks a unit apart into its own real subobjects as debris,
//       recursively -- naming a bone/subobject in BreakApartSubObject (e.g. "TURRET") walks that
//       bone's own subtree on a dedicated, never-rendered reference W3D model (BreakApartModel;
//       see BreakApartModelHelper.h) and spawns one debris piece per descendant bone that actually
//       has geometry on it (e.g. "BARREL01"/"BARREL02" hanging off "TURRET"), children first, then
//       the bone itself last. No per-part authoring is needed: every piece is spawned from the
//       SAME modder-supplied BreakApartPieceOCL, and at spawn time this module tells that piece's
//       Draw module (W3DBreakApartPieceDraw) which single subobject of BreakApartModel to show,
//       via DrawModule::setBreakApartPiece() -- a fresh clone of the reference model with every
//       OTHER subobject hidden, positioned/oriented exactly like the dying object itself, so the
//       one visible piece lines up with where it looked on the original unit. Deliberately NOT
//       layered on SlowDeathBehavior -- this module owns its own DestructionDelay countdown and
//       its own DieMuxData (DeathTypes/ExemptStatus/RequiredStatus) filter, exactly like
//       SlowDeathBehavior does, so partition DeathTypes between the two on any object that has
//       both (Object::onDie() calls every DieModuleInterface unconditionally -- there's no
//       "only one die module handles this" arbitration outside SlowDeathBehavior's own internal
//       lottery between SlowDeathBehavior instances).
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "GameLogic/Module/BehaviorModule.h"
#include "GameLogic/Module/DieModule.h"
#include "GameLogic/Module/UpdateModule.h"

#include <vector>

class FXList;
class ObjectCreationList;
class WeaponTemplate;
class DamageInfo;

//-------------------------------------------------------------------------------------------------
typedef std::vector<const FXList*> BADB_FXListVec;
typedef std::vector<const ObjectCreationList*> BADB_OCLVec;
typedef std::vector<const WeaponTemplate*> BADB_WeaponTemplateVec;
typedef std::vector<AsciiString> BADB_BoneNameVec;

//-------------------------------------------------------------------------------------------------
// Own, independent 3-phase enum -- deliberately NOT reusing SlowDeathPhaseType/SDPHASE_* (this
// module is standalone, not layered on SlowDeathBehavior; see the header comment above).
enum BreakApartPhaseType CPP_11(: Int)
{
	BAPHASE_INITIAL = 0,
	BAPHASE_MIDPOINT,
	BAPHASE_FINAL,

	BAPHASE_COUNT
};

#ifdef DEFINE_BREAKAPARTPHASE_NAMES
static const char *const TheBreakApartPhaseNames[] =
{
	"INITIAL",
	"MIDPOINT",
	"FINAL",

	nullptr
};
static_assert(ARRAY_SIZE(TheBreakApartPhaseNames) == BAPHASE_COUNT + 1, "Incorrect array size");
#endif

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
class BreakApartDeathBehaviorV2ModuleData : public UpdateModuleData
{
public:
	DieMuxData					m_dieMuxData;
	UnsignedInt					m_destructionDelay;
	UnsignedInt					m_destructionDelayVariance;

	// GeneralsMod @feature Dimitar 21/09/2026: per-tier overrides for DestructionDelay/
	// DestructionDelayVariance above -- e.g. a normal death lingers a while before the dying object
	// is actually removed, but an overkill/extreme-overkill death (already violent enough that the
	// pieces flying apart ARE the spectacle) destroys it faster, or slower, or with wildly different
	// variance, independent of the normal-tier timing. Each defaults to a sentinel (0xFFFFFFFF,
	// impossible to reach via INI::parseDurationUnsignedInt's own msec->frame conversion) meaning
	// "not configured -- inherit DestructionDelay/DestructionDelayVariance above" -- so a modder who
	// only cares about retiming the normal tier doesn't have to also restate it twice more for the
	// other two tiers; the fields only diverge from the base timing when explicitly set. Which pair
	// applies is picked in onDie() from the SAME tier index (m_dieTier: 0/1/2) that already selects
	// which BreakApartSubObject root list and which Force field apply to this death.
	UnsignedInt					m_overkillDestructionDelay;
	UnsignedInt					m_overkillDestructionDelayVariance;
	UnsignedInt					m_extremeOverkillDestructionDelay;
	UnsignedInt					m_extremeOverkillDestructionDelayVariance;

	// generic decorative effects -- identical shape/semantics to SlowDeathBehavior's own FX/OCL/
	// Weapon fields, fired at the object's own position at each phase. Fully independent of the
	// real per-subobject debris pieces below -- these are just ambient FX/OCL/Weapon triggers, the
	// same way SlowDeathBehavior uses them.
	BADB_FXListVec			m_fx[BAPHASE_COUNT];
	BADB_OCLVec					m_ocls[BAPHASE_COUNT];
	BADB_WeaponTemplateVec	m_weapons[BAPHASE_COUNT];

	AsciiString					m_breakApartModel;

	// Root bone/subobject names to recursively break apart -- naming a bone here also breaks apart
	// every descendant bone under it in BreakApartModel's own hierarchy (see
	// BreakApartModelHelper.h's GetBreakApartSubtreeBones()). Exactly one of these three lists is
	// used per death, chosen by OverkillPercentage/ExtremeOverkillPercentage below.
	BADB_BoneNameVec		m_breakApartSubObject;
	BADB_BoneNameVec		m_overkillBreakApartSubObject;
	BADB_BoneNameVec		m_extremeOverkillBreakApartSubObject;

	// GeneralsMod @feature Dimitar 19/09/2026: root bone/subobject names to PROTECT from the
	// recursive break-apart walk, whichever tier's root list would otherwise have pulled them in --
	// e.g. a small decorative antenna hanging off TURRET01 that shouldn't fly off as its own tiny
	// piece even though it's technically part of TURRET01's subtree. Deliberately a single,
	// non-tiered list (same convention as RemainPieceOCL/RemainPieceFX below, as opposed to the
	// three tiered lists above) -- "don't break this specific part apart" is treated as a property
	// of the part itself, not something that should vary by how hard the unit was overkilled.
	// SUBTREE-WIDE protection: naming a bone here protects it AND every one of its own descendants
	// (walked via GetBreakApartSubtreeBones() the exact same way a BreakApartSubObject root is --
	// see spawnBreakApartDebris()'s own comment), the mirror image of how a BreakApartSubObject root
	// pulls its whole subtree IN. Every protected bone is left out of both the individual-piece
	// spawn and allBrokenBones, so it stays visible on the RemainPieceOCL/RemainPieceFX remainder
	// piece instead of vanishing -- same as any subobject that was never named under a
	// BreakApartSubObject root to begin with.
	BADB_BoneNameVec		m_remainSubObject;

	// GeneralsMod @fix Dimitar 21/09/2026, redesigned same day to an AND of two independent checks
	// after the user identified a real gap in EACH check alone:
	//   - Percentage-of-max-health ALONE (the first fix) misfires the other way: a moderate hit that
	//     lands while the target still has a lot of health left can be a big fraction of max health
	//     and yet barely finish the target off (e.g. 100 max health, 50 remaining, 51 damage dealt --
	//     51% of max health, but only 1 point of damage was actually WASTED). That's a real kill, not
	//     really an "overkill" in the sense this field is meant to capture, and worse, it makes
	//     certain weapon/target matchups "hardcoded" to always-or-never overkill regardless of the
	//     situation the hit actually happened in.
	//   - Wasted-damage-alone (damage beyond what was needed to kill, i.e. this hit's raw damage
	//     minus the health the object had right before it -- the ORIGINAL 19/09/2026 design's own
	//     m_healthBeforeDamage, see the .cpp) has the opposite gap: it misfires low, since any
	//     finishing blow on an already-worn-down object wastes nearly its full damage almost by
	//     definition, so even a basic tank's ordinary shot looks like a huge overkill by this metric
	//     alone whenever it happens to land the killing blow.
	// So BOTH must hold for a tier to trigger: the hit must be big RELATIVE TO THIS UNIT'S OWN
	// TOUGHNESS (percentage of max health -- rules out a weak weapon ever qualifying, no matter how
	// little health the target had left), AND the hit must have genuinely WASTED damage beyond what
	// was needed to kill (rules out a merely-large hit that happened to land on a still-healthy
	// target with little to no actual excess). Real overkill, in both senses of the word, has to
	// actually occur.
	Real								m_overkillPercentageFromMaxHealth;				///< e.g. 0.4 for "hit must be >= 40% of max health"; default disables the tier
	Real								m_overkillDamageGreaterOrEqThan;					///< flat wasted-damage floor (dealt damage minus health-before-hit); default disables the tier
	Real								m_extremeOverkillPercentageFromMaxHealth;	///< same idea, extreme-overkill tier; default disables the tier
	Real								m_extremeOverkillDamageGreaterOrEqThan;		///< same idea, extreme-overkill tier; default disables the tier

	Real								m_force;
	Real								m_overkillForce;
	Real								m_extremeOverkillForce;

	// GeneralsMod @feature Dimitar 21/09/2026: half-angle (radians, INI::parseAngleReal -- write
	// degrees in INI) of random scatter allowed around each per-bone piece's own OUTWARD-FROM-
	// CENTER direction (the dying object's own position -> that bone's actual live world position,
	// projected flat onto XY) when force is applied in spawnBreakApartDebris() below, instead of a
	// fully random direction -- e.g. a part mounted on the tank's left side now reliably flies
	// off toward the left (plus this much random jitter either way), rather than occasionally flying
	// clean across to the right, which looked unnatural. Tier-agnostic (one field, like
	// RemainPieceOCL/RemainPieceFX/RemainSubObject above) -- how much a piece's flight direction is
	// allowed to wander isn't something that should vary by how hard the unit was overkilled. Only
	// meaningful for pieces with a resolved live bone position; the RemainPieceOCL remainder piece
	// has no off-center position of its own to derive a direction from, so it always keeps the old,
	// fully random behavior regardless of this field.
	Real								m_forceSpreadAngle;

	// GeneralsMod @feature Dimitar 19/09/2026: the ONE generic OCL used to spawn every debris
	// piece, whatever bone it ends up representing -- its Object must have a
	// W3DBreakApartPieceDraw Draw module; this module tells that module which single subobject of
	// BreakApartModel to actually display, per spawn, via DrawModule::setBreakApartPiece(). No
	// per-bone/per-part authoring is needed -- see the header comment above.
	const ObjectCreationList*	m_breakApartPieceOCL;

	// GeneralsMod @feature Dimitar 19/09/2026: the ONE OCL spawned, at most once per death, for
	// every subobject that did NOT get broken apart into its own piece -- i.e. everything left over
	// once every resolved BreakApartSubObject/OverkillBreakApartSubObject/
	// ExtremeOverkillBreakApartSubObject root's whole subtree has been removed from it. Optional --
	// leave unset (default nullptr, same convention as m_breakApartPieceOCL) to keep the old
	// behavior of the non-broken-apart remainder simply vanishing with the rest of the object.
	const ObjectCreationList*	m_remainPieceOCL;

	// GeneralsMod @feature Dimitar 19/09/2026: one-shot FX played at each spawned piece's own
	// position, right alongside BreakApartPieceOCL/RemainPieceOCL -- e.g. a small sparks/debris-
	// puff burst per broken-off part, independent of the phase-keyed FX[]/OCL[]/Weapon[] triples
	// above (those fire once at the DYING OBJECT's own position per phase; these fire once PER
	// SPAWNED PIECE, at that piece's own position). Both optional, default nullptr (disabled), same
	// convention as every other FXList/OCL field on this module.
	const FXList*							m_breakApartPieceFX;
	const FXList*							m_remainPieceFX;

	BreakApartDeathBehaviorV2ModuleData();
	static void buildFieldParse(MultiIniFieldParse& p);

private:

};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
class BreakApartDeathBehaviorV2 : public UpdateModule,
																	 public DieModuleInterface
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( BreakApartDeathBehaviorV2, "BreakApartDeathBehaviorV2" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( BreakApartDeathBehaviorV2, BreakApartDeathBehaviorV2ModuleData )

public:

	BreakApartDeathBehaviorV2( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	static Int getInterfaceMask() { return UpdateModule::getInterfaceMask() | (MODULEINTERFACE_DIE); }

	// BehaviorModule
	virtual DieModuleInterface* getDie() override { return this; }

	// UpdateModuleInterface
	virtual UpdateSleepTime update() override;
	// Disabled conditions to process -- all (matches SlowDeathBehavior: a dying/breaking-apart
	// object must still finish its own countdown regardless of disabled-status flags)
	virtual DisabledMaskType getDisabledTypesToProcess() const override { return DISABLEDMASK_ALL; }

	// DieModuleInterface
	virtual void onDie( const DamageInfo *damageInfo ) override;

protected:

	void doPhaseStuff( BreakApartPhaseType phase );
	void spawnBreakApartDebris( const BADB_BoneNameVec& rootBones, Real force );
	Bool isBreakApartActivated() const { return (m_flags & (1<<BREAK_APART_ACTIVATED)) != 0; }

private:

	enum
	{
		BREAK_APART_ACTIVATED,
		MIDPOINT_EXECUTED
	};

	UnsignedInt m_midpointFrame;
	UnsignedInt m_destructionFrame;
	UnsignedInt	m_flags;

	// GeneralsMod @feature Dimitar 19/09/2026: which tier this death selected (0 = normal,
	// 1 = overkill, 2 = extreme overkill; -1 = no death in progress) and its force, cached at
	// onDie() time and consumed later, right at actual destruction (see update()) -- debris pieces
	// now spawn in sync with DestructionDelay rather than instantly at death, so the still-standing
	// dying object and its own debris don't visibly overlap for the whole delay window. A tier
	// INDEX (not a raw BADB_BoneNameVec* into this module's own ModuleData) is what's persisted --
	// safe to xfer, unlike a pointer, and survives a save/load that lands mid-death.
	Int m_dieTier;
	Real m_dieForce;
};
