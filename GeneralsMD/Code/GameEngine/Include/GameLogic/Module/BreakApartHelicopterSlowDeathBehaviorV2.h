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

// FILE: BreakApartHelicopterSlowDeathBehaviorV2.h ////////////////////////////////////////////////
// GeneralsMod @feature Dimitar 10/11/2026
// Desc: HelicopterSlowDeathBehavior + partial break-apart. Helicopter counterpart of
//       BreakApartJetSlowDeathBehaviorV2. Everything HelicopterSlowDeathBehavior does is kept
//       unchanged (spiral orbit, self spin, FXBlade/OCLBlade/pilot ejection at the blade fly-off
//       moment, hit ground, final blow-up, FinalRubbleObject, and SlowDeathBehavior's own
//       ProbabilityModifier lottery/DeathTypes/FX/OCL/Weapon phases). On top of that, parts of
//       BreakApartModel tear off the live helicopter in TWO events:
//
//       1. RANDOM PARTS, at the moment the slow death BEGINS -- identical to the jet module: the
//          subtree of every root in this tier's list (BreakApartSubObject /
//          OverkillBreakApartSubObject / ExtremeOverkillBreakApartSubObject; an empty overkill/extreme
//          list falls back to BreakApartSubObject) is walked deepest-first and EVERY part with a mesh
//          rolls on its own against this tier's chance. A part whose roll passes breaks off as ONE
//          rigid piece carrying every part still attached below it that didn't break off on its own
//          (so a broken parent takes its unbroken children with it).
//
//       2. GUARANTEED PARTS (AlwaysBreakApartSubObject, e.g. the rotors), always, in every tier, at
//          the vanilla blade fly-off moment (MinBladeFlyOffDelay..MaxBladeFlyOffDelay after death,
//          the same frame FXBlade/OCLBlade/pilot ejection fire). If both delays are 0 they break
//          right at death, together with the random parts. If the helicopter hits the ground before
//          the delay runs out (vanilla then skips the blade event entirely), they break on impact
//          instead -- so they always come off. Each listed part breaks off as one piece with its
//          whole subtree; a listed bone WITHOUT a mesh (e.g. a bare rotor hub bone) instead sends
//          each of its top-most mesh children flying as its own piece (one piece per blade).
//          Everything inside a guaranteed part's subtree is RESERVED: it never rolls in event 1,
//          but if a random part above it breaks it is carried along with that piece, and is then
//          skipped in event 2.
//
//       Pieces use the same generic BreakApartPieceOCL + W3DBreakApartPieceDraw mechanism, live-bone
//       alignment, Force/ForceSpreadAngle and InheritVelocity as the jet module (force of the tier
//       picked at death). Broken parts are HIDDEN on the helicopter's live model via
//       Drawable::showSubObject() -- the helicopter itself is the remainder and keeps spiralling down.
//
//       Leave BladeBoneName/BladeObjectName as you like (BladeObjectName is unused by the engine;
//       BladeBoneName only positions FXBlade/OCLBlade), but do NOT spawn blade debris from OCLBlade
//       when the blades are in AlwaysBreakApartSubObject, or you get the blades twice.
//
//       ASSET RULES (same as the jet module):
//         - the live model must contain the same subobject (mesh) names as BreakApartModel for every
//           part that can break off (short name after the "MODEL." prefix). The models that matter
//           are the ones shown AFTER death: RUBBLE (the killing blow sets it before beginSlowDeath())
//           and SPECIAL_DAMAGED (vanilla sets it on ground impact). Hidden parts are re-applied
//           after every model swap, so a missing name hits "ASSET ERROR: SubObject X not found" in
//           debug builds.
//         - do NOT hide the rotors in those condition states yourself (HideSubObject / a rotorless
//           model), or they vanish before they can fly off.
//         - the two rotors should be siblings in the hierarchy (not one parented under the other),
//           or the child rotor always leaves with its parent.
//
// Example:
//   Behavior = BreakApartHelicopterSlowDeathBehaviorV2 ModuleTag_Death01
//     ; ...all the usual HelicopterSlowDeathBehavior fields (no blade OCL)...
//     DestructionDelay = 99999   ; REQUIRED, like every vanilla helicopter death: SlowDeathBehavior's
//                                ; default of 0 destroys the helicopter on the first frame of death
//     MinBladeFlyOffDelay = 300
//     MaxBladeFlyOffDelay = 900
//     BreakApartModel                    = AVChinook_BA
//     AlwaysBreakApartSubObject          = ROTORFRONT ROTORBACK
//     BreakApartSubObject                = TAILBOOM GEARL GEARR
//     OverkillBreakApartSubObject        = TAILBOOM GEARL GEARR DOORL DOORR
//     ExtremeOverkillBreakApartSubObject = TAILBOOM GEARL GEARR DOORL DOORR NOSE
//     RemainSubObject                    = CHASSIS
//     BreakApartChance                 = 20%
//     OverkillBreakApartChance         = 50%
//     ExtremeOverkillBreakApartChance  = 90%
//     OverkillPercentageFromMaxHealth        = 40%
//     OverkillDamageGreaterOrEqThan          = 50
//     ExtremeOverkillPercentageFromMaxHealth = 80%
//     ExtremeOverkillDamageGreaterOrEqThan   = 150
//     Force                = 4.0
//     OverkillForce        = 6.0
//     ExtremeOverkillForce = 9.0
//     ForceSpreadAngle   = 60
//     InheritVelocity    = 100%
//     BreakApartPieceOCL = OCL_BreakApartPiece
//     BreakApartPieceFX  = FX_RotorSnap
//   End
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "GameLogic/Module/HelicopterSlowDeathUpdate.h"
#include "GameLogic/BreakApartDebrisHelper.h"

class FXList;
class ObjectCreationList;

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
class BreakApartHelicopterSlowDeathBehaviorV2ModuleData : public HelicopterSlowDeathBehaviorModuleData
{
public:

	BreakApartHelicopterSlowDeathBehaviorV2ModuleData();
	static void buildFieldParse( MultiIniFieldParse &p );

	AsciiString									m_breakApartModel;					///< never-rendered reference W3D (see BreakApartModelHelper.h)

	BreakApartBoneNameVec				m_alwaysBreakApartSubObject;	///< always break off, every tier, at the blade fly-off moment

	// Roots whose subtrees are rolled part-by-part against the tier's chance at death. Exactly one
	// list is used per death, picked by tier; an EMPTY overkill/extreme list falls back to
	// BreakApartSubObject.
	BreakApartBoneNameVec				m_breakApartSubObject;
	BreakApartBoneNameVec				m_overkillBreakApartSubObject;
	BreakApartBoneNameVec				m_extremeOverkillBreakApartSubObject;
	BreakApartBoneNameVec				m_remainSubObject;					///< subtrees that never roll on their own (carried if a part above breaks)

	Real												m_breakApartChance;									///< per-part chance, normal death (0..1)
	Real												m_overkillBreakApartChance;					///< per-part chance, overkill death
	Real												m_extremeOverkillBreakApartChance;	///< per-part chance, extreme overkill death

	// same AND-of-two-checks tier selection as BreakApartDeathBehaviorV2; defaults to
	// HUGE_DAMAGE_AMOUNT so a tier is disabled unless BOTH of its fields are set
	Real												m_overkillPercentageFromMaxHealth;
	Real												m_overkillDamageGreaterOrEqThan;
	Real												m_extremeOverkillPercentageFromMaxHealth;
	Real												m_extremeOverkillDamageGreaterOrEqThan;

	Real												m_force;
	Real												m_overkillForce;
	Real												m_extremeOverkillForce;
	Real												m_forceSpreadAngle;					///< radians (degrees in INI)
	Real												m_inheritVelocity;					///< fraction of the helicopter's velocity each piece starts with (default 100%)

	const ObjectCreationList*		m_breakApartPieceOCL;				///< ONE generic piece OCL (needs a W3DBreakApartPieceDraw)
	const FXList*								m_breakApartPieceFX;				///< played once per spawned piece
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
class BreakApartHelicopterSlowDeathBehaviorV2 : public HelicopterSlowDeathBehavior
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( BreakApartHelicopterSlowDeathBehaviorV2, "BreakApartHelicopterSlowDeathBehaviorV2" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( BreakApartHelicopterSlowDeathBehaviorV2, BreakApartHelicopterSlowDeathBehaviorV2ModuleData )

public:

	BreakApartHelicopterSlowDeathBehaviorV2( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	virtual void beginSlowDeath( const DamageInfo *damageInfo ) override;
	virtual UpdateSleepTime update() override;

protected:

	void breakApartRandomParts();				///< event 1: per-part rolls at death
	void breakApartAlwaysParts();				///< event 2: AlwaysBreakApartSubObject

	/// Spawn one piece per bone in pieceBones (deepest first) -- with carryAttached, each also takes
	/// the parts still attached below it -- record them in m_removedBones and sync the live model.
	void detachPieces( const BreakApartBoneNameVec& pieceBones, Bool carryAttached );

	/// Hide every removed part on the live model and re-show the still-attached parts below them.
	void syncLiveSubObjects();

	Real getTierForce();

	Int										m_deathTier;					///< overkill tier picked at death (0/1/2)
	Bool									m_alwaysPartsDone;		///< event 2 has happened
	BreakApartBoneNameVec	m_removedBones;				///< every bone already gone from the live model (broken or carried)

};
