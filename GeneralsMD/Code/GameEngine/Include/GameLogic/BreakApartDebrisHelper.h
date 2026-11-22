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

// FILE: BreakApartDebrisHelper.h /////////////////////////////////////////////////////////////////
// GeneralsMod @feature Dimitar 26/10/2026
// Desc: Shared GameLogic-side break-apart machinery used by BOTH BreakApartDeathBehaviorV2
//       (vehicles: the whole object is replaced by pieces) and BreakApartJetSlowDeathBehaviorV2
//       (jets: some parts detach while the jet itself keeps crashing via JetSlowDeathBehavior).
//       Moved verbatim out of BreakApartDeathBehaviorV2.cpp so the fixes made there (live-bone
//       alignment, bind-pose fallback for bones missing on the live model, SIM-transform
//       relocation, pivot recentering, directional outward force) live in exactly ONE place.
//       Plain free functions, not a module -- no CMake/ModuleFactory/memory-pool registration
//       beyond adding the .h/.cpp to CMakeLists.txt.
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Lib/BaseType.h"
#include "Common/AsciiString.h"

#include <vector>

class Object;
class DamageInfo;
class ObjectCreationList;
class FXList;
class INI;

typedef std::vector<AsciiString> BreakApartBoneNameVec;

//-------------------------------------------------------------------------------------------------
/** INI field parser for a plain space-separated list of bone/subobject names. Multiple statements
	* accumulate (BreakApartSubObject = A B, then BreakApartSubObject = C -> [A, B, C]). store must
	* point at a BreakApartBoneNameVec. */
//-------------------------------------------------------------------------------------------------
void BreakApartParseBoneNameList( INI* ini, void *instance, void *store, const void* userData );

//-------------------------------------------------------------------------------------------------
/** Overkill tier for a killing blow: 0 = normal, 1 = overkill, 2 = extreme overkill. A tier needs
	* BOTH its percentage-of-max-health AND its flat wasted-damage threshold to be met (see
	* BreakApartDeathBehaviorV2.h's m_overkillPercentageFromMaxHealth comment for the full reasoning).
	* Pass HUGE_DAMAGE_AMOUNT thresholds to disable a tier. Only meaningful for the DamageInfo of the
	* killing blow (reads m_healthBeforeDamage). */
//-------------------------------------------------------------------------------------------------
Int BreakApartCalcOverkillTier( Object* obj, const DamageInfo* damageInfo,
	Real overkillPercentageFromMaxHealth, Real overkillDamageGreaterOrEqThan,
	Real extremeOverkillPercentageFromMaxHealth, Real extremeOverkillDamageGreaterOrEqThan );

//-------------------------------------------------------------------------------------------------
/** Case-insensitive membership test. */
//-------------------------------------------------------------------------------------------------
Bool BreakApartBoneNameListContains( const BreakApartBoneNameVec& list, const AsciiString& name );

//-------------------------------------------------------------------------------------------------
/** Appends rootBone's whole subtree on modelName (children before parent, see
	* GetBreakApartSubtreeBones()) to *inOutBones, skipping any bone listed in protectedBones and any
	* bone already present in *inOutBones (so overlapping roots -- e.g. WING and its own WINGTIP both
	* listed -- never spawn the same piece twice). Pass an empty protectedBones for no protection.
	* Silently does nothing if the model or root bone can't be resolved. */
//-------------------------------------------------------------------------------------------------
void BreakApartAppendSubtree( const AsciiString& modelName, const AsciiString& rootBone,
	const BreakApartBoneNameVec& protectedBones, BreakApartBoneNameVec* inOutBones );

//-------------------------------------------------------------------------------------------------
/** Fling + tumble a freshly-spawned debris piece. biasDirXY (may be nullptr) is the horizontal
	* direction the piece should fly roughly toward, +/- spreadAngle of random jitter. */
//-------------------------------------------------------------------------------------------------
void BreakApartApplyDebrisForce( Object* debris, Real force, const Coord3D* biasDirXY, Real spreadAngle );

//-------------------------------------------------------------------------------------------------
/** Spawns ONE debris piece for boneName from pieceOCL, aligned to where that bone currently is on
	* obj (live bone transform if obj's Drawable has it, otherwise a bind-pose approximation from
	* modelName), recentered on the subobject's own visual center, told via
	* DrawModule::setBreakApartPiece() which subobject of modelName to show, flung outward with force,
	* and given pieceFX. inheritedVelocity (may be nullptr) is added on top as-is -- used by the jet
	* version so a detached wing keeps the jet's own forward speed instead of stopping dead in the
	* air. attachedBones (may be nullptr) are extra bones whose meshes stay visible on this same piece
	* (parts still attached below boneName that didn't break off on their own). Returns the spawned
	* piece, or nullptr. */
//-------------------------------------------------------------------------------------------------
Object* BreakApartSpawnPiece( Object* obj, const AsciiString& modelName, const AsciiString& boneName,
	const ObjectCreationList* pieceOCL, const FXList* pieceFX, Real force, Real spreadAngle,
	const Coord3D* inheritedVelocity, const BreakApartBoneNameVec* attachedBones = nullptr );
