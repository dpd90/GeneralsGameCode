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

// FILE: BreakApartDebrisHelper.cpp ///////////////////////////////////////////////////////////////
// GeneralsMod @feature Dimitar 26/10/2026
// Desc: see BreakApartDebrisHelper.h. Everything below the parse/tier helpers was moved verbatim
//       out of BreakApartDeathBehaviorV2.cpp (comments included, dates preserved) so the vehicle
//       and jet break-apart modules share one implementation.
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine
#include "Common/DrawModule.h"
#include "Common/INI.h"
#include "Common/RandomValue.h"
#include "GameClient/BreakApartModelHelper.h"
#include "GameClient/Drawable.h"
#include "GameClient/FXList.h"
#include "GameLogic/BreakApartDebrisHelper.h"
#include "GameLogic/Damage.h"
#include "GameLogic/Module/BodyModule.h"
#include "GameLogic/Module/PhysicsUpdate.h"
#include "GameLogic/Object.h"
#include "GameLogic/ObjectCreationList.h"
#include "WWMath/matrix3d.h"

//-------------------------------------------------------------------------------------------------
/** BreakApartSubObject-style field parser -- see the header. Moved here from
	* BreakApartDeathBehaviorV2.cpp's own static parseBoneNameList(). */
//-------------------------------------------------------------------------------------------------
void BreakApartParseBoneNameList( INI* ini, void * /*instance*/, void *store, const void* /*userData*/ )
{
	BreakApartBoneNameVec* v = (BreakApartBoneNameVec*)store;
	for (const char* token = ini->getNextTokenOrNull(); token; token = ini->getNextTokenOrNull())
	{
		v->push_back(AsciiString(token));
	}
}

//-------------------------------------------------------------------------------------------------
/** Moved here from BreakApartDeathBehaviorV2::onDie() (21/09/2026 AND-of-two-checks design):
	*   1) percentOfMaxHealth = this hit's raw damage / the object's own max health -- rules out a
	*      weak weapon ever qualifying, no matter how little health the target had left.
	*   2) wastedDamage = this hit's raw damage - the health the object had immediately before it --
	*      rules out a merely-large hit that barely finished off a still-healthy target.
	* Both use m_actualDamageDealt (UNCLIPPED), not m_actualDamageClipped (which can never exceed
	* m_healthBeforeDamage and so could never show any waste). getMaxHealth() is the CURRENT health
	* cap, including max-health upgrades. */
//-------------------------------------------------------------------------------------------------
Int BreakApartCalcOverkillTier( Object* obj, const DamageInfo* damageInfo,
	Real overkillPercentageFromMaxHealth, Real overkillDamageGreaterOrEqThan,
	Real extremeOverkillPercentageFromMaxHealth, Real extremeOverkillDamageGreaterOrEqThan )
{
	if (obj == nullptr || damageInfo == nullptr)
		return 0;

	Real maxHealth = obj->getBodyModule() ? obj->getBodyModule()->getMaxHealth() : 0.0f;
	if (maxHealth <= 0.0f)
		return 0;

	Real dealt = damageInfo->out.m_actualDamageDealt;
	Real percentOfMaxHealth = dealt / maxHealth;
	Real wastedDamage = dealt - damageInfo->out.m_healthBeforeDamage;

	if (percentOfMaxHealth >= extremeOverkillPercentageFromMaxHealth &&
		wastedDamage >= extremeOverkillDamageGreaterOrEqThan)
		return 2;

	if (percentOfMaxHealth >= overkillPercentageFromMaxHealth &&
		wastedDamage >= overkillDamageGreaterOrEqThan)
		return 1;

	return 0;
}

//-------------------------------------------------------------------------------------------------
/** Outward force for a debris piece, with a fixed mostly-upward pitch band -- unlike
	* SlowDeathBehavior's FlingPitch/FlingPitchVariance, the user's own Force/OverkillForce/
	* ExtremeOverkillForce fields carry no pitch control of their own, so this module hardcodes a
	* reasonable "scatter outward and up" pitch range rather than exposing one. Revisit if per-tier
	* pitch control is ever wanted.
	*
	* GeneralsMod @feature Dimitar 21/09/2026: biasDirXY, when non-null, is the (not necessarily
	* normalized) horizontal direction the piece should fly roughly TOWARD -- typically the vector
	* from the dying object's own center to that piece's actual world position (see
	* spawnBreakApartDebris() below), so a part mounted on the tank's left flies off left-ish instead
	* of in a fully random direction that could just as easily send it clean across to the right.
	* The actual yaw is that bias direction's own angle plus a random +/- spreadAngle jitter, so it
	* still scatters naturally rather than every piece flying in a perfectly straight radial line.
	* biasDirXY == nullptr (or a degenerate near-zero-length vector) falls back to the old, fully
	* random -PI..PI behavior -- used for the RemainPieceOCL remainder piece, which has no off-center
	* position of its own to derive a direction from. */
//-------------------------------------------------------------------------------------------------
static void calcRandomOutwardForce(Real magnitude, const Coord3D* biasDirXY, Real spreadAngle, Coord3D& force)
{
	Real angle;
	if (biasDirXY != nullptr && (biasDirXY->x * biasDirXY->x + biasDirXY->y * biasDirXY->y) > 0.0001f)
		angle = (Real)atan2(biasDirXY->y, biasDirXY->x) + GameLogicRandomValueReal(-spreadAngle, spreadAngle);
	else
		angle = GameLogicRandomValueReal(-PI, PI);

	Real pitch = GameLogicRandomValueReal(0.15f * PI, 0.5f * PI);

	Matrix3D mtx(1);
	mtx.Scale(magnitude);
	mtx.Rotate_Z(angle);
	mtx.Rotate_Y(-pitch);

	Vector3 v = mtx.Get_X_Vector();

	force.x = v.X;
	force.y = v.Y;
	force.z = v.Z;
}

//-------------------------------------------------------------------------------------------------
/** Fling + tumble a freshly-spawned debris piece exactly like every other one -- shared by both the
	* per-bone pieces below and the single remainder piece, so the two spawn paths behave identically
	* aside from which subobject(s) end up visible on the clone. */
//-------------------------------------------------------------------------------------------------
void BreakApartApplyDebrisForce(Object* debris, Real force, const Coord3D* biasDirXY, Real spreadAngle)
{
	if (force <= 0.0f)
		return;

	PhysicsBehavior* physics = debris->getPhysics();
	if (physics == nullptr)
		return;

	Coord3D forceVec;
	calcRandomOutwardForce(force, biasDirXY, spreadAngle, forceVec);
	physics->setAllowToFall(true);
	physics->applyForce(&forceVec);
	// GeneralsMod @feature Dimitar 19/09/2026: give the piece some tumble too -- applyForce() alone
	// only ever produces straight-line motion, never rotation. applyRandomRotation() is the same
	// helper the engine's own disposition-based debris (SEND_IT_FLYING/SEND_IT_UP in
	// ObjectCreationList.cpp) effectively hand-rolls itself; it reads ShockMaxYaw/ShockMaxPitch/
	// ShockMaxRoll off the piece's OWN PhysicsBehavior ModuleData (nonzero engine defaults, but
	// overridable per BreakApartDebrisPiece template) and also flips on AllowBouncing.
	physics->applyRandomRotation();
}

//-------------------------------------------------------------------------------------------------
Bool BreakApartBoneNameListContains(const BreakApartBoneNameVec& list, const AsciiString& name)
{
	for (BreakApartBoneNameVec::const_iterator it = list.begin(); it != list.end(); ++it)
	{
		if (it->compareNoCase(name) == 0)
			return true;
	}
	return false;
}

//-------------------------------------------------------------------------------------------------
void BreakApartAppendSubtree( const AsciiString& modelName, const AsciiString& rootBone,
	const BreakApartBoneNameVec& protectedBones, BreakApartBoneNameVec* inOutBones )
{
	BreakApartBoneNameVec subtreeBones;
	if (!GetBreakApartSubtreeBones(modelName, rootBone, &subtreeBones))
		return;	// root bone missing from the reference model (or model failed to load) -- skip it

	for (BreakApartBoneNameVec::const_iterator it = subtreeBones.begin(); it != subtreeBones.end(); ++it)
	{
		// RemainSubObject protection (see BreakApartDeathBehaviorV2.h's m_remainSubObject comment)
		if (BreakApartBoneNameListContains(protectedBones, *it))
			continue;
		// GeneralsMod @fix Dimitar 26/10/2026: dedupe -- two listed roots where one is inside the
		// other's subtree used to spawn that shared bone's piece twice.
		if (BreakApartBoneNameListContains(*inOutBones, *it))
			continue;
		inOutBones->push_back(*it);
	}
}

//-------------------------------------------------------------------------------------------------
Object* BreakApartSpawnPiece( Object* obj, const AsciiString& modelName, const AsciiString& boneName,
	const ObjectCreationList* pieceOCL, const FXList* pieceFX, Real force, Real spreadAngle,
	const Coord3D* inheritedVelocity, const BreakApartBoneNameVec* attachedBones )
{
	if (obj == nullptr || pieceOCL == nullptr || modelName.isEmpty())
		return nullptr;

	// The dying object's OWN Drawable is still alive at this point (see the 19/09/2026 comment that
	// used to live at the top of BreakApartDeathBehaviorV2::spawnBreakApartDebris()):
	// Drawable::getCurrentWorldspaceClientBonePositions() is the existing GameLogic-safe accessor for
	// a bone's CURRENT world transform (LaserUpdate::updateStartPos() precedent). Purely a per-client
	// visual read feeding the piece's own Draw-module alignment.
	const Drawable* drawable = obj->getDrawable();

	Object* debris = ObjectCreationList::create( pieceOCL, obj, obj->getPosition(), nullptr, obj->getOrientation() );
	if (debris == nullptr)
		return nullptr;

	// Look up this bone's current world transform on the LIVE dying Drawable. May fail
	// (bone doesn't exist on the live model's current condition-state variant, or the
	// object has no Drawable at all for some reason) -- in that case, fall back below to
	// an APPROXIMATE world transform derived from the reference model's own bind pose.
	Matrix3D boneWorldXform;
	Bool haveBoneXform = (drawable != nullptr) &&
		drawable->getCurrentWorldspaceClientBonePositions( boneName.str(), boneWorldXform );
	// GeneralsMod @fix Dimitar 21/09/2026: root-caused -- a bone can legitimately resolve on
	// the standalone BreakApartModel reference skeleton (used above to enumerate the subtree)
	// while having no same-named bone on the LIVE dying unit's own model at all (e.g. a bone
	// that exists purely to give one subtree several independent debris chunks). That's the
	// common case, not an error: the live lookup above legitimately fails for most bones on
	// a finely-subdivided BreakApartModel.

	if (!haveBoneXform)
	{
		// GeneralsMod @fix Dimitar 21/09/2026: root-caused the "pieces float on landing"
		// bug down to THIS gap. Previously, when no live transform was found, boneWorldXform
		// was simply never set at all, and everything below degraded to nullptr/false --
		// which left this debris Object's own SIM/physics-tracked transform sitting at
		// wherever ObjectCreationList::create() spawned it above: the DYING OBJECT's own
		// root position (obj->getPosition()/getOrientation()), not the bone's position.
		// PhysicsBehavior then ground-clamps THAT (wrong) point -- the hull's own root
		// height, not the height the bone/mesh actually sits at (e.g. mounted up on a
		// turret) -- while the Draw-side ground-offset correction in
		// W3DBreakApartPieceDraw.cpp only ever compensated for the small mesh-to-its-own-
		// pivot offset, not this much larger root-to-bone-mount-height gap. Net effect: the
		// visible mesh renders however many units above the ground that the bone sits above
		// the hull root -- exactly the reported "floats when landing" symptom, and exactly
		// why TURRET/BARREL01 (the two bones that DID have a live transform) always worked
		// while every fallback bone (TURRET02, BARREL02/03, TURRET01/03/04) never did.
		//
		// Fix: approximate the bone's world transform as
		// dyingObjectTransform * boneBindPoseOnReferenceModel -- the ANCHOR (the dying
		// Object's own current position/orientation) is exact; only the ROTATION comes from
		// the reference model's REST pose rather than the unit's actual live pose (there's
		// no live bone to read, so this is the best available stand-in -- see
		// GetBreakApartBoneBindTransform()'s own comment in BreakApartModelHelper.h). This
		// still anchors the debris Object's physics point to roughly the right spot on the
		// dying unit instead of leaving it down at the root.
		Matrix3D boneBindXform;
		if (GetBreakApartBoneBindTransform(modelName, boneName, &boneBindXform))
		{
			Matrix3D::Multiply(*obj->getTransformMatrix(), boneBindXform, &boneWorldXform);
			haveBoneXform = true;
		}
	}

	// GeneralsMod @fix Dimitar 19/09/2026: re-anchor the debris Object's own SIM
	// position/orientation onto the bone's world transform (live if available, else the
	// bind-pose approximation just computed above), BEFORE telling the Draw module which
	// subobject to show. Without this, the Object stayed spawned at the DYING OBJECT's own
	// root transform (obj->getPosition()/getOrientation(), passed to
	// ObjectCreationList::create() above) -- fine for a piece that happens to BE the model
	// root, but wrong for anything mounted away from it (e.g. a barrel mounted up on a
	// turret): PhysicsBehavior moves/rotates the Object around THAT root point, not around
	// the bone's own position, while setBreakApartPiece()'s m_pieceOffset only visually
	// re-aligns the RENDER once, at spawn -- so as physics then evolves the (wrong) root-
	// anchored transform frame by frame, the visible piece swings around a pivot that isn't
	// where it geometrically is (looks like it falls from/rotates around the wrong height/
	// point entirely, or floats above the ground once landed). setTransformMatrix() here
	// (Thing::setTransformMatrix(), public, the same call the engine's own object-
	// placement code uses) updates the Object's real synced transform AND propagates to
	// its Drawable via Object::reactToTransformChange()'s existing
	// `m_drawable->setTransformMatrix(...)` call -- so by the time setBreakApartPiece()
	// below reads getDrawable()->getTransformMatrix(), it already reflects the bone's own
	// position, which makes its m_pieceOffset math collapse to exactly the fixed
	// root-to-bone geometric offset (no per-frame drift), and PhysicsBehavior now moves/
	// rotates the Object around the bone's own point instead of the model's root -- the
	// visible piece and the physics body become the same rigid point. Left at the old
	// root-anchored spawn position/orientation only in the (now rare) case where neither a
	// live transform NOR a bind-pose fallback could be resolved at all (model failed to
	// load, or the bone name itself is bad) -- setBreakApartPiece() still degrades
	// gracefully via its own targetBoneIndex-only ground-offset correction in that case.
	//
	// GeneralsMod @fix Dimitar 23/09/2026: root-caused "rotation looks weird, like the
	// pivot isn't on the part itself" -- anchoring the SIM transform at the bone's own raw
	// pivot (as above) means PhysicsBehavior tumbles/rotates the piece around THAT point,
	// but a bone's pivot is very often NOT anywhere near its subobject's own visual center
	// (a barrel's bone sits at its mount point, with the tube extending well past it in one
	// direction) -- once the piece is free-flying debris with no bone attachment left to
	// justify pivoting on the original mount point, rotating around it looks exactly like
	// what was reported: the mesh visibly swinging around an point off in space rather than
	// tumbling about roughly its own middle. Fix: shift the SIM anchor from the bone's raw
	// pivot to (approximately) the subobject's own local bounding-box center
	// (GetBreakApartSubObjectLocalCenter(), BreakApartModelHelper.h) BEFORE calling
	// setTransformMatrix() -- Translate() (not Adjust_*_Translation -- see the 21/09/2026
	// lesson elsewhere in this file) rotates that LOCAL offset by boneWorldXform's own
	// current orientation before adding it, so the recentered point is correct regardless
	// of this bone's own orientation. Deliberately does NOT change what's passed to
	// setBreakApartPiece() below (still the TRUE, non-recentered boneWorldXform) -- that
	// function reads getDrawable()->getTransformMatrix() (now the RECENTERED transform,
	// via reactToTransformChange()) purely as ITS OWN pre-overwrite baseline before
	// computing pieceRootXform from the true bone transform passed in, so its EXISTING
	// m_pieceOffset math (Inverse(drawable transform at spawn) * pieceRootXform) ends up
	// automatically capturing the fixed local offset from the new centroid back to the
	// true mesh position -- no changes needed inside setBreakApartPiece() itself, and that
	// offset already rotates correctly with the piece as it tumbles, for the exact same
	// reason m_pieceOffset was introduced in the first place (see its own 19/09/2026
	// comment in W3DBreakApartPieceDraw.cpp).
	if (haveBoneXform)
	{
		Matrix3D pivotWorldXform = boneWorldXform;
		Vector3 localCenter(0.0f, 0.0f, 0.0f);
		if (GetBreakApartSubObjectLocalCenter(modelName, boneName, &localCenter))
			pivotWorldXform.Translate(localCenter);
		debris->setTransformMatrix(&pivotWorldXform);
	}

	// Tell whichever Draw module on this fresh Object is ours which single subobject of
	// BreakApartModel to actually show -- a no-op on every other DrawModule (see
	// DrawModule::setBreakApartPiece()'s own comment in Common/DrawModule.h).
	for (DrawModule** dm = debris->getDrawable()->getDrawModules(); *dm; ++dm)
	{
		(*dm)->setBreakApartPiece(modelName, boneName, haveBoneXform ? &boneWorldXform : nullptr, attachedBones);
	}

	// GeneralsMod @feature Dimitar 21/09/2026: bias the outward force toward this piece's
	// own actual offset from the dying object's center (flat, XY only -- see
	// calcRandomOutwardForce()'s own comment) rather than a fully random direction, so a
	// piece mounted on the tank's left reliably flies off left-ish. Only possible when we
	// have SOME world position for the bone (haveBoneXform, live or bind-pose-approximate)
	// -- otherwise there's nothing to derive a direction from, and calcRandomOutwardForce()
	// itself already degrades to the old fully random behavior for a degenerate/absent
	// direction.
	Coord3D outwardDirXY = { 0.0f, 0.0f, 0.0f };
	if (haveBoneXform)
	{
		outwardDirXY.x = boneWorldXform.Get_X_Translation() - obj->getPosition()->x;
		outwardDirXY.y = boneWorldXform.Get_Y_Translation() - obj->getPosition()->y;
	}
	BreakApartApplyDebrisForce(debris, force, haveBoneXform ? &outwardDirXY : nullptr, spreadAngle);
	FXList::doFXObj(pieceFX, debris);

	// GeneralsMod @feature Dimitar 26/10/2026: carry the source object's own velocity over onto the
	// piece (BreakApartJetSlowDeathBehaviorV2: a wing torn off a jet in flight must keep flying
	// forward and arc down, not stop dead in mid-air). Applied as a one-frame force of
	// velocity * mass (PhysicsBehavior::applyForce() divides by mass, and the next physics update
	// adds that acceleration straight onto m_vel) rather than addVelocityTo(), because applyForce()
	// also wakes a sleeping PhysicsBehavior and addVelocityTo() does not.
	if (inheritedVelocity != nullptr)
	{
		PhysicsBehavior* physics = debris->getPhysics();
		if (physics != nullptr)
		{
			Real mass = physics->getMass();
			Coord3D velForce;
			velForce.x = inheritedVelocity->x * mass;
			velForce.y = inheritedVelocity->y * mass;
			velForce.z = inheritedVelocity->z * mass;
			physics->setAllowToFall(true);
			physics->applyForce(&velForce);
		}
	}

	return debris;
}
