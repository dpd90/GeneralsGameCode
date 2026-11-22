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

// FILE: BreakApartJetSlowDeathBehaviorV2.h ///////////////////////////////////////////////////////
// GeneralsMod @feature Dimitar 26/10/2026
// Desc: JetSlowDeathBehavior + partial break-apart. Everything JetSlowDeathBehavior does is kept
//       unchanged (roll, fall, FXInitialDeath/OCLInitialDeath, secondary event, hit-ground, final
//       blow-up, death loop sound, the on-ground/deck instant death path, and SlowDeathBehavior's
//       own ProbabilityModifier lottery/DeathTypes/FX/OCL/Weapon phases). On top of that, at the
//       moment the slow death BEGINS (in the air only -- the on-ground path never reaches
//       beginSlowDeath()), the subtree of every root in this tier's list (BreakApartSubObject /
//       OverkillBreakApartSubObject / ExtremeOverkillBreakApartSubObject) is walked deepest-first and
//       EVERY part with a mesh rolls on its own against this tier's chance (BreakApartChance /
//       OverkillBreakApartChance / ExtremeOverkillBreakApartChance; tier picked by the same
//       AND-of-two-checks overkill formula as BreakApartDeathBehaviorV2). Bones without a mesh never
//       roll. A part whose roll passes breaks off as ONE rigid piece carrying every part still
//       attached below it that didn't break off on its own roll:
//         - one BreakApartPieceOCL piece per broken part (same generic piece +
//           W3DBreakApartPieceDraw mechanism as BreakApartDeathBehaviorV2), aligned to where the
//           bone currently is on the live, rolling jet, flung outward by Force and carrying
//           InheritVelocity of the jet's own velocity so it keeps flying forward;
//         - the matching subobjects are HIDDEN on the jet's own live model via
//           Drawable::showSubObject() -- the jet itself is the "remainder", it keeps crashing.
//
//       ASSET RULE: the live jet model must contain the same subobject (mesh) names as
//       BreakApartModel for every part that can break off (matched by the short name after the
//       "MODEL." prefix, the same way ShowSubObjects/HideSubObjects match). A missing name triggers
//       the engine's own "*** ASSET ERROR: SubObject X not found" DEBUG_CRASH in debug builds, the
//       same as a misnamed HideSubObject would. The model that matters is whichever one the jet shows
//       AFTER death: the killing blow puts it in BODY_RUBBLE (MODELCONDITION_RUBBLE) before
//       beginSlowDeath() runs, so that is the RUBBLE ConditionState, or its best-match fallback.
//
// Example:
//   Behavior = BreakApartJetSlowDeathBehaviorV2 ModuleTag_Death01
//     ; ...all the usual JetSlowDeathBehavior fields...
//     BreakApartModel                  = AVRaptor_BA
//     BreakApartSubObject              = WINGL WINGR
//     OverkillBreakApartSubObject      = WINGL WINGR TAILFIN
//     ExtremeOverkillBreakApartSubObject = WINGL WINGR TAILFIN NOSE
//     BreakApartChance                 = 25%
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
//     BreakApartPieceFX  = FX_WingSnap
//     OCLSpecificHitGround   = OCL_AVRaptor_HitGround      ; fired IN ADDITION to OCLHitGround
//     OCLSpecificFinalBlowUp = OCL_AVRaptor_FinalBlowUp    ; fired IN ADDITION to OCLFinalBlowUp
//     OCLSpecificOnGroundDeath = OCL_AVRaptor_OnGroundDeath ; fired IN ADDITION to OCLOnGroundDeath
//     AirPitchRate      = 0.02   ; nose-down tip while falling, radians per frame (0 = off)
//     AirPitchRateDelta = 100%   ; each frame, airPitchRate = airPitchRate * airPitchRateDelta
//     MaxAirPitch       = 45     ; degrees nose-down at which the tip stops (0 = no limit)
//   End
//
// GeneralsMod @feature Dimitar 11/11/2026: OCLSpecificHitGround / OCLSpecificFinalBlowUp /
// OCLSpecificOnGroundDeath fire at the exact same moments as JetSlowDeathBehavior's OCLHitGround /
// OCLFinalBlowUp / OCLOnGroundDeath, on top of them, so a
// shared (generic, e.g. via a common parent/ini macro) OCL can stay on every plane while each plane
// adds its own specific one (e.g. its own wreck/hulk).
//
// GeneralsMod @feature Dimitar 13/11/2026: AirPitchRate / AirPitchRateDelta / MaxAirPitch. Vanilla
// JetSlowDeathBehavior only ROLLS the jet in the air (PitchRate starts after it hits the ground).
// These add an optional in-air nose-down pitch, mirroring RollRate/RollRateDelta: positive =
// nose down, stopped once the nose reaches MaxAirPitch. Hitting the ground hands over to the
// normal PitchRate as before.
// @fix 13/11/2026: the first version set PhysicsBehavior's pitch RATE, which never accumulated:
// the dying jet's locomotor keeps running (AIUpdate::doLocomotor() ignores death), and with
// TurnPivotOffset == 0 Locomotor::rotateObjAroundLocoPivot() calls Thing::setOrientation() every
// frame, rebuilding the jet upright (PHASE_INITIAL), so physics (PHASE_PHYSICS) only ever showed one
// frame's step. Now the module accumulates its own pitch ANGLE and writes it onto the jet's
// transform itself in update() (PHASE_NORMAL, i.e. after AI and physics in the same frame).
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "GameLogic/Module/JetSlowDeathBehavior.h"
#include "GameLogic/BreakApartDebrisHelper.h"

class FXList;
class ObjectCreationList;

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
class BreakApartJetSlowDeathBehaviorV2ModuleData : public JetSlowDeathBehaviorModuleData
{
public:

	BreakApartJetSlowDeathBehaviorV2ModuleData();
	static void buildFieldParse( MultiIniFieldParse &p );

	AsciiString									m_breakApartModel;					///< never-rendered reference W3D (see BreakApartModelHelper.h)

	// Roots whose subtrees are rolled part-by-part against the tier's chance. Like
	// BreakApartDeathBehaviorV2, exactly one list is used per death, picked by tier -- except that an
	// EMPTY overkill/extreme list falls back to BreakApartSubObject, so a modder who only wants
	// higher chances (not different parts) on overkill doesn't have to repeat the list.
	BreakApartBoneNameVec				m_breakApartSubObject;
	BreakApartBoneNameVec				m_overkillBreakApartSubObject;
	BreakApartBoneNameVec				m_extremeOverkillBreakApartSubObject;
	BreakApartBoneNameVec				m_remainSubObject;					///< subtrees that never break off, even under a passing root

	Real												m_breakApartChance;									///< per-root chance, normal death (0..1)
	Real												m_overkillBreakApartChance;					///< per-root chance, overkill death
	Real												m_extremeOverkillBreakApartChance;	///< per-root chance, extreme overkill death

	// same AND-of-two-checks tier selection as BreakApartDeathBehaviorV2 (see its .h); defaults to
	// HUGE_DAMAGE_AMOUNT so a tier is disabled unless BOTH of its fields are set
	Real												m_overkillPercentageFromMaxHealth;
	Real												m_overkillDamageGreaterOrEqThan;
	Real												m_extremeOverkillPercentageFromMaxHealth;
	Real												m_extremeOverkillDamageGreaterOrEqThan;

	Real												m_force;
	Real												m_overkillForce;
	Real												m_extremeOverkillForce;
	Real												m_forceSpreadAngle;					///< radians (degrees in INI)
	Real												m_inheritVelocity;					///< fraction of the jet's velocity each piece starts with (default 100%)

	const ObjectCreationList*		m_breakApartPieceOCL;				///< ONE generic piece OCL (needs a W3DBreakApartPieceDraw)
	const FXList*								m_breakApartPieceFX;				///< played once per spawned piece

	const ObjectCreationList*		m_oclSpecificHitGround;			///< extra per-plane OCL, fired together with OCLHitGround
	const ObjectCreationList*		m_oclSpecificFinalBlowUp;		///< extra per-plane OCL, fired together with OCLFinalBlowUp
	const ObjectCreationList*		m_oclSpecificOnGroundDeath;	///< extra per-plane OCL, fired together with OCLOnGroundDeath

	Real												m_airPitchRate;							///< initial in-air pitch rate, radians per frame (+ = nose down, 0 = off)
	Real												m_airPitchRateDelta;				///< per-frame multiplier on the in-air pitch rate (default 100%)
	Real												m_maxAirPitch;							///< radians (degrees in INI) nose-down where the tip stops; 0 = no limit
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
class BreakApartJetSlowDeathBehaviorV2 : public JetSlowDeathBehavior
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( BreakApartJetSlowDeathBehaviorV2, "BreakApartJetSlowDeathBehaviorV2" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( BreakApartJetSlowDeathBehaviorV2, BreakApartJetSlowDeathBehaviorV2ModuleData )

public:

	BreakApartJetSlowDeathBehaviorV2( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	virtual void onDie( const DamageInfo *damageInfo ) override;
	virtual void beginSlowDeath( const DamageInfo *damageInfo ) override;
	virtual UpdateSleepTime update() override;

protected:

	void breakApartParts( Int tier );

	Real m_airPitchRate;		///< current in-air pitch rate (decays by AirPitchRateDelta), saved in xfer v2
	Real m_airPitchAngle;		///< accumulated in-air nose-down angle (radians), saved in xfer v3

};
