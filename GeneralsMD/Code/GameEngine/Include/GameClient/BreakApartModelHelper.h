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

// FILE: BreakApartModelHelper.h /////////////////////////////////////////////////////////////////
// GeneralsMod @feature Dimitar 19/09/2026, redesigned 19/09/2026
// Desc: Thin, GameLogic-callable entry point into a client-side reference W3D model used by
//       BreakApartDeathBehaviorV2 to figure out WHICH bones/subobjects should break off a dying
//       unit, given only a root bone/subobject name -- it does not place anything itself; the
//       actual per-piece visual (spawning a debris Object whose Draw module clones this same
//       model and hides every subobject except one) is a separate cross-boundary hand-off, done
//       via DrawModule::setBreakApartPiece() (see Common/DrawModule.h) rather than through this
//       helper, since that part needs to happen on an already-spawned Drawable/DrawModule, not a
//       standalone lookup.
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>

class AsciiString;
class Matrix3D;
class Vector3;

//-------------------------------------------------------------------------------------------------
/** GetBreakApartSubtreeBones
	*
	* Declared here, under GameEngine/Include (on both the z_gameengine AND z_gameenginedevice
	* include paths), and DEFINED in
	* GeneralsMD/Code/GameEngineDevice/Source/W3DDevice/GameClient/W3DBreakApartModelHelper.cpp --
	* the concrete W3D/WWLib implementation. z_gameengine (where BreakApartDeathBehaviorV2 itself
	* lives) must never #include anything from GameEngineDevice directly -- same Logic/Client
	* boundary DrawModule/Drawable exist to enforce for per-Object bone queries, just expressed as a
	* plain extern free function here instead of a virtual interface, since walking a reference
	* model's own skeleton isn't per-Object state.
	*
	* modelName is a W3D model file's base name exactly as BreakApartModel = ... names it in INI (no
	* extension, same convention as every other W3D model-name field in this engine). rootBoneName
	* is the name of a bone/subobject in that model's own skeleton (e.g. "TURRET") -- exactly one of
	* the names a modder would write into BreakApartSubObject / OverkillBreakApartSubObject /
	* ExtremeOverkillBreakApartSubObject.
	*
	* The named model is loaded (via WW3DAssetManager::Create_Render_Obj) once on first request and
	* cached for the rest of the process, purely as a geometry/skeleton source -- never added to a
	* Scene, never Render()'d, never animated -- independent of any real unit's Drawable,
	* ModelConditionState, upgrades or damage-state variants (see the design discussion this module
	* was built from: walking a live unit's own Drawable hierarchy is unreliable, since
	* W3DModelDraw::getPristineBonePositionsForConditionState() resolves bones only against whichever
	* ConditionState model variant happens to be active).
	*
	* On success, *outBoneNames is filled with the name of rootBoneName's own bone plus every
	* descendant bone in its subtree (walked via the model's own HTreeClass parent/child chain) that
	* has at least one subobject actually attached to it (a pure joint/pivot bone with no mesh is
	* silently skipped, though its own children are still walked) -- ordered so every bone appears
	* strictly before its own ancestors (a post-order/"children fully listed before their parent"
	* walk), matching the recursive break-apart behavior this was built for: "TURRET" with
	* BARREL01/BARREL02 attached underneath it in the model's own hierarchy yields
	* [BARREL01, BARREL02, TURRET] (or the reverse sibling order -- sibling order is whatever the
	* model's own bone array gives, not independently meaningful), never the parent before a child.
	*
	* Returns false (and leaves *outBoneNames untouched) if the named model failed to load, or if
	* rootBoneName doesn't exist in it. NOTE: WW3D's own bone lookup (HTreeClass::Get_Bone_Index)
	* returns index 0 for BOTH "the skeleton's true root bone" and "lookup failed" -- there is no way
	* to tell these apart from the return value alone, so this function treats index 0 as "not
	* found" exactly like every other bone-by-name lookup already in this engine does (see
	* W3DModelDraw::clientOnly_getRenderObjBoneTransform) -- a genuine reference to the model's own
	* root bone is not reachable through this call. Name your BreakApartSubObject entries after real,
	* non-root sub-object bones (which is what every practical case looks like anyway -- the true
	* skeleton root is never itself a paintable subobject).
	*/
extern Bool GetBreakApartSubtreeBones( const AsciiString& modelName, const AsciiString& rootBoneName, std::vector<AsciiString>* outBoneNames );

//-------------------------------------------------------------------------------------------------
/** GetBreakApartBoneBindTransform
	*
	* GeneralsMod @feature Dimitar 21/09/2026: resolves boneName's BIND-POSE (rest-pose) world
	* transform on the SAME standalone BreakApartModel reference model GetBreakApartSubtreeBones()
	* above reads (root bone treated as identity) -- exactly the "force the render object's own
	* transform to identity, then read Get_Bone_Transform()" trick
	* W3DBreakApartPieceDraw::setBreakApartPiece() already performs on its own PER-PIECE clone of
	* this same model when it has a live bone transform to align against (see that function's own
	* comment in W3DBreakApartPieceDraw.cpp) -- exposed here as a free function so
	* BreakApartDeathBehaviorV2::spawnBreakApartDebris() can get a *reasonable* transform for a bone
	* that has NO live counterpart on the dying unit's own model at all, which is the common case
	* whenever BreakApartModel is subdivided more finely than the live unit's own animated skeleton
	* (see GetBreakApartSubtreeBones()'s own comment above, and spawnBreakApartDebris()'s comment on
	* why haveLiveXform/haveBoneXform legitimately ends up false for most bones).
	*
	* Composing *outBindTransform with the dying Object's own current world transform
	* (dyingObjectTransform * *outBindTransform) gives an approximate world transform for the bone:
	* the ANCHOR point (the dying Object's own current position/orientation) is exact, but the
	* ROTATION contribution from the bind pose is only an approximation of the unit's actual live
	* pose at time of death (there is no live bone to read, so this is the best available stand-in --
	* for a mostly-rigid vehicle hull this is usually a very close approximation; for a bone on a
	* limb/turret that could be posed far from its rest angle, it is a rougher one). Still far closer
	* than leaving the debris Object's own physics-tracked point sitting at the dying Object's own
	* root, which is what happens if no fallback at all is used.
	*
	* Returns false (and leaves *outBindTransform untouched) exactly like GetBreakApartSubtreeBones
	* above -- the named model failed to load, or boneName doesn't exist in it (same bone-index-0
	* "true root vs. not found" ambiguity, handled the same way). */
extern Bool GetBreakApartBoneBindTransform( const AsciiString& modelName, const AsciiString& boneName, Matrix3D* outBindTransform );

//-------------------------------------------------------------------------------------------------
/** GetBreakApartSubObjectLocalCenter
	*
	* GeneralsMod @fix Dimitar 23/09/2026: resolves boneName's own attached subobject's LOCAL-SPACE
	* bounding-box CENTER on the SAME standalone BreakApartModel reference model the other two
	* functions above read -- i.e. "roughly where this piece's own visible geometry actually sits,
	* relative to its own bone pivot" (the bone pivot is boneName's own local origin; a real W3D
	* subobject's mesh is very often NOT centered on its own bone -- e.g. a barrel's bone pivot sits
	* at its mount point, with the tube extending well past it in one direction). Unlike
	* GetBreakApartBoneBindTransform(), this does NOT need the reference model's own transform forced
	* to identity first -- RenderObjClass::Get_Obj_Space_Bounding_Box() is already expressed in the
	* subobject's own local/object space, independent of whatever world transform the model
	* currently has (same box W3DBreakApartPieceDraw::setBreakApartPiece() itself already queries,
	* off its own separate per-piece clone, for its ground-offset correction).
	*
	* Exists so BreakApartDeathBehaviorV2::spawnBreakApartDebris() can re-anchor a freshly-spawned
	* debris Object's own SIM/physics transform (the point PhysicsBehavior actually rotates/tumbles
	* the piece around) onto this piece's own approximate visual center of mass, rather than leaving
	* it at the bone's raw pivot -- otherwise a piece whose mesh sits well off its own bone (a barrel,
	* say) visibly tumbles around a point nowhere near its own geometry (looks like it's spinning
	* around an invisible point off in space) instead of rotating naturally about roughly its own
	* middle, once it's a free-flying piece of debris with no bone attachment left to justify pivoting
	* on the original mount point. The bone's own TRUE world transform is still what's passed to
	* setBreakApartPiece() for the actual render alignment -- only the debris Object's OWN transform
	* (read back out by setBreakApartPiece() via getDrawable()->getTransformMatrix() as ITS OWN
	* alignment baseline before being overwritten) shifts to this centroid, so setBreakApartPiece()'s
	* existing m_pieceOffset math (Inverse(drawable transform at spawn) * pieceRootXform) ends up
	* capturing the fixed local offset from the centroid back to the true mesh position automatically
	* -- no changes needed inside setBreakApartPiece() itself, that offset already rotates correctly
	* with the piece as it tumbles (same reason m_pieceOffset was introduced in the first place).
	*
	* Returns false (and leaves *outLocalCenter untouched) if the named model failed to load,
	* boneName doesn't exist in it (same bone-index-0 ambiguity as the other two functions), or
	* boneName has no subobject actually attached to it (a pure joint/pivot bone) -- in any of these
	* cases the caller should just fall back to no recentering at all (leave the debris Object
	* anchored at the bone's own raw position, the old behavior). */
extern Bool GetBreakApartSubObjectLocalCenter( const AsciiString& modelName, const AsciiString& boneName, Vector3* outLocalCenter );
