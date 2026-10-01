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

// FILE: W3DBreakApartPieceDraw.cpp /////////////////////////////////////////////////////////////
// GeneralsMod @feature Dimitar 19/09/2026
// Desc: See W3DBreakApartPieceDraw.h. This file lives under GameEngineDevice, so per this mod's
//       own convention it does NOT include "PreRTS.h".
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////

#include "W3DDevice/GameClient/Module/W3DBreakApartPieceDraw.h"

#include "Common/AsciiString.h"
#include "GameClient/Drawable.h"

#include "WW3D2/htree.h"
#include "WW3D2/rendobj.h"
#include "WWMath/aabox.h"
#include "WWMath/matrix3d.h"
#include "W3DDevice/GameClient/W3DAssetManager.h"
#include "W3DDevice/GameClient/W3DDisplay.h"
#include "W3DDevice/GameClient/W3DScene.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
W3DBreakApartPieceDraw::W3DBreakApartPieceDraw(Thing *thing, const ModuleData* moduleData) : DrawModule(thing, moduleData)
{
	m_renderObject = nullptr;
	m_pieceOffset.Make_Identity();
	m_particleSystemID = INVALID_PARTICLE_SYSTEM_ID;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
W3DBreakApartPieceDraw::~W3DBreakApartPieceDraw()
{
	// GeneralsMod @feature Dimitar 01/10/2026: explicitly destroy() the particle system this
	// instance created (if any) via attachConfiguredParticleSystem() -- otherwise it would keep
	// emitting/attached-nowhere indefinitely once this Drawable/DrawModule goes away, since nothing
	// else in the engine tracks or cleans up a particle system this module itself created.
	if (m_particleSystemID != INVALID_PARTICLE_SYSTEM_ID)
	{
		if (TheParticleSystemManager != nullptr)
		{
			ParticleSystem* sys = TheParticleSystemManager->findParticleSystem(m_particleSystemID);
			if (sys != nullptr)
				sys->destroy();
		}
		m_particleSystemID = INVALID_PARTICLE_SYSTEM_ID;
	}

	if (m_renderObject)
	{
		if (W3DDisplay::m_3DScene != nullptr)
			W3DDisplay::m_3DScene->Remove_Render_Object(m_renderObject);
		REF_PTR_RELEASE(m_renderObject);
		m_renderObject = nullptr;
	}
}

//-------------------------------------------------------------------------------------------------
// GeneralsMod @feature Dimitar 01/10/2026: custom two-token parser (bone name, then particle system
// template name) -- same shape as W3DModelDraw.cpp's own parseParticleSysBone(), which this mirrors
// deliberately for consistency, except it writes directly into this module's own two named fields
// rather than pushing onto a per-ConditionState vector (this module only ever has one active piece,
// so one field pair is enough). ini->parseParticleSystemTemplate() (INI.cpp) reads the SECOND token
// itself and DEBUG_ASSERTCRASH()s (does not hard-fail) if it's neither a real template name nor the
// literal "None".
//-------------------------------------------------------------------------------------------------
static void parseBreakApartParticleSystem(INI* ini, void* instance, void* /*store*/, const void* /*userData*/)
{
	W3DBreakApartPieceDrawModuleData* self = (W3DBreakApartPieceDrawModuleData*)instance;
	self->m_particleSystemBoneName = ini->getNextAsciiString();
	self->m_particleSystemBoneName.toLower();
	ini->parseParticleSystemTemplate(ini, instance, &(self->m_particleSystemTemplate), nullptr);
}

//-------------------------------------------------------------------------------------------------
/*static*/ void W3DBreakApartPieceDrawModuleData::buildFieldParse(MultiIniFieldParse& p)
{
	ModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "ParticleSystem", parseBreakApartParticleSystem, nullptr, 0 },
		{ nullptr, nullptr, nullptr, 0 }
	};
	p.add(dataFieldParse);
}

//-------------------------------------------------------------------------------------------------
void W3DBreakApartPieceDraw::setBreakApartPiece(const AsciiString& modelName, const AsciiString& boneName, const Matrix3D* liveBoneWorldTransform)
{
	// one-shot, same re-entrancy guard shape as W3DDebrisDraw::setModelName() -- a second call
	// (there should never be one) is silently ignored rather than leaking/replacing the first clone.
	if (m_renderObject != nullptr || modelName.isEmpty())
		return;

	m_renderObject = W3DDisplay::m_assetManager->Create_Render_Obj(modelName.str(), getDrawable()->getScale(), 0);
	DEBUG_ASSERTCRASH(m_renderObject, ("BreakApart piece model %s not found!", modelName.str()));
	if (m_renderObject == nullptr)
		return;

	if (W3DDisplay::m_3DScene != nullptr)
		W3DDisplay::m_3DScene->Add_Render_Object(m_renderObject);

	m_renderObject->Set_User_Data(getDrawable()->getDrawableInfo());

	// Hide every subobject except the one(s) attached to the requested bone -- this is what makes
	// one spawned clone of the reference model visually look like just a single broken-off part.
	// WW3D uses bone index 0 for both "the skeleton's true root bone" and "bone not found" (see
	// BreakApartModelHelper.h's own comment on the same ambiguity) -- if boneName doesn't resolve,
	// targetBoneIndex ends up 0, which in practice means every subobject gets hidden (no subobject
	// in a normal HLOD is attached directly to the true root), a safe, visually-inert failure mode
	// rather than a crash.
	const HTreeClass* htree = m_renderObject->Get_HTree();
	int targetBoneIndex = (htree != nullptr) ? htree->Get_Bone_Index(boneName.str()) : 0;
	// GeneralsMod @feature Dimitar 19/09/2026: align this piece with the bone's ACTUAL live world
	// transform at the moment of death (turret/barrel aim rotation etc -- see the
	// liveBoneWorldTransform comment on DrawModule::setBreakApartPiece() in Common/DrawModule.h),
	// not just the reference model's own bind pose. Default/fallback: mirror the debris Drawable's
	// own plain transform, same as before this feature was added -- m_pieceOffset stays identity
	// unless one of the two corrections below actually fires, so doDrawModule()/
	// reactToTransformChange() behave exactly as before this feature existed whenever neither does.
	m_pieceOffset.Make_Identity();
	Matrix3D pieceRootXform = *getDrawable()->getTransformMatrix();
	Bool pieceRootXformChanged = false;

	if (liveBoneWorldTransform != nullptr && targetBoneIndex != 0)
	{
		// Force a known baseline (identity) before reading the bone's BIND-pose transform, so this
		// doesn't depend on whatever transform Create_Render_Obj happened to leave the clone at.
		Matrix3D identityXform(1);
		m_renderObject->Set_Transform(identityXform);
		Matrix3D cloneBoneBindXform = m_renderObject->Get_Bone_Transform(targetBoneIndex);

		// Solve for the root transform that makes THIS clone's own copy of targetBoneIndex land
		// exactly at liveBoneWorldTransform: since Get_Bone_Transform(idx) == root * bindChain,
		// and cloneBoneBindXform (captured above, root==identity) IS that bindChain, the desired
		// root is liveBoneWorldTransform * Inverse(bindChain). This assumes both the reference
		// model and the live model are unscaled (or scaled identically) -- Get_Orthogonal_Inverse
		// assumes a pure rotation+translation matrix, same assumption W3DModelDraw::
		// getCurrentBonePositions() already makes for its own analogous inverse.
		Matrix3D bindInverse;
		cloneBoneBindXform.Get_Orthogonal_Inverse(bindInverse);
		Matrix3D::Multiply(*liveBoneWorldTransform, bindInverse, &pieceRootXform);
		pieceRootXformChanged = true;
	}

	// GeneralsMod @fix Dimitar 21/09/2026: correct for the target subobject's own visible mesh
	// sitting away from its bone's pivot -- e.g. a barrel bone rigged up at its mount point on
	// the turret, with the actual barrel geometry authored well below (or above) that pivot in
	// the bone's own local space. PhysicsBehavior's ground-clamp only ever knows about this
	// Object's single tracked point (the bone pivot, when one was resolved above) -- it has no
	// idea the visible mesh extends away from that point, so it settles the PIVOT at true ground
	// height and leaves the geometry floating/sunk by however far it sits from that pivot.
	//
	// Root-caused 21/09/2026 ("BARREL still floats even though TURRET lands correctly"): this
	// correction used to live INSIDE the `liveBoneWorldTransform != nullptr` branch above, so any
	// bone that fell back to the plain-transform path never got its own mesh-to-pivot offset
	// removed at all -- it just hovered at its authored bind-pose height above wherever
	// PhysicsBehavior settled the piece's root. That fallback path is the COMMON case, not a rare
	// edge case: BreakApartModel is typically subdivided more finely than the live unit's own
	// animated skeleton (see spawnBreakApartDebris()'s own comment -- a bone that exists purely to
	// give one subtree several independent debris chunks, e.g. splitting a barrel into a tube +
	// muzzle brake + mount, has no reason to also exist as a separately-named bone on the LIVE,
	// rendered model), so most resolved bones legitimately have targetBoneIndex != 0 (found on the
	// reference model) while liveBoneWorldTransform is nullptr (no same-named bone on the live
	// model to align against) -- TURRET/BARREL01 in the original report happened to still be real,
	// individually-posed live bones; the rest of that unit's pieces weren't. This correction is
	// therefore gated on targetBoneIndex alone: it only needs the CLONE's own resolved bone index
	// to query that subobject's local bounding box, independent of whether a live transform was
	// also available.
	//
	// Query the target subobject's own local-space bounding box for its lowest point (Center.Z -
	// Extent.Z, i.e. how far the mesh extends below its own pivot; a NEGATIVE box-relative value
	// here means the mesh actually sits ABOVE its pivot, which is what pushes it up into the air
	// once the pivot lands) and shift pieceRootXform's world Z by the negation of that, before it's
	// captured into m_pieceOffset below -- so the correction is baked into the bone-local frame
	// same as the bindChain correction above (when that also ran), and keeps rotating correctly
	// with the piece as applyRandomRotation() tumbles it, rather than being a one-time world-space
	// nudge that would drift out of alignment the moment the piece starts spinning. Best-effort
	// only: for a piece that lands in a wildly different final orientation than its spawn pose,
	// this can't perfectly zero out the gap (that would need per-orientation collision, out of
	// scope) -- it just removes the fixed, always-wrong offset that made every landing look
	// identically bad.
	if (targetBoneIndex != 0)
	{
		RenderObjClass* targetSub = m_renderObject->Get_Sub_Object_On_Bone(0, targetBoneIndex);
		if (targetSub != nullptr)
		{
			AABoxClass subBox;
			targetSub->Get_Obj_Space_Bounding_Box(subBox);
			Real groundOffset = subBox.Center.Z - subBox.Extent.Z;
			// GeneralsMod @fix Dimitar 21/09/2026: root-caused (2nd pass) -- Adjust_Z_Translation()
			// is a raw `Row[2][3] += z`, i.e. it nudges the matrix's WORLD-space Z translation
			// directly, with no regard for pieceRootXform's own rotation. groundOffset, however, was
			// measured along the target subobject's own OBJECT-space Z axis (Get_Obj_Space_Bounding_Box),
			// which only coincides with world-up when that bone's local Z happens to still point
			// straight up -- true for TURRET (which only ever yaws about world Z) but false for
			// anything pitched or mounted at another angle (BARREL01-03's aim pitch, TURRET01-04's
			// own mount angles) -- which is exactly why TURRET landed correctly while everything
			// hanging off it kept floating/sinking by the wrong amount in the wrong direction.
			// Matrix3D::Translate() (unlike Adjust_Z_Translation) post-multiplies -- it rotates the
			// given LOCAL offset by the matrix's own current orientation before adding it to the
			// world translation, which is what actually implements "push this piece up along its own
			// local Z axis" for a piece at any orientation, not just one whose local Z is world-up.
			pieceRootXform.Translate(Vector3(0.0f, 0.0f, -groundOffset));
			pieceRootXformChanged = true;
			REF_PTR_RELEASE(targetSub);
		}
	}

	if (pieceRootXformChanged)
	{
		// Capture the (now fixed) delta between the debris Object's own spawn-time Drawable
		// transform and this corrected root, so later frames can keep reproducing the same
		// alignment as the piece is subsequently flung/tumbled by PhysicsBehavior, instead of
		// snapping back to a bare Drawable-transform copy the instant it first moves.
		Matrix3D drawableInverse;
		getDrawable()->getTransformMatrix()->Get_Orthogonal_Inverse(drawableInverse);
		Matrix3D::Multiply(drawableInverse, pieceRootXform, &m_pieceOffset);
	}

	m_renderObject->Set_Transform(pieceRootXform);

	int numSub = m_renderObject->Get_Num_Sub_Objects();
	for (int i = 0; i < numSub; ++i)
	{
		RenderObjClass* sub = m_renderObject->Get_Sub_Object(i);	// Add_Ref()'d by this call
		if (sub != nullptr)
		{
			int subBoneIndex = m_renderObject->Get_Sub_Object_Bone_Index(sub);
			sub->Set_Hidden(subBoneIndex != targetBoneIndex ? 1 : 0);
			REF_PTR_RELEASE(sub);
		}
	}

	attachConfiguredParticleSystem();
}

//-------------------------------------------------------------------------------------------------
void W3DBreakApartPieceDraw::setBreakApartRemainder(const AsciiString& modelName, const std::vector<AsciiString>& hiddenBoneNames)
{
	// Same one-shot guard as setBreakApartPiece() above.
	if (m_renderObject != nullptr || modelName.isEmpty())
		return;

	m_renderObject = W3DDisplay::m_assetManager->Create_Render_Obj(modelName.str(), getDrawable()->getScale(), 0);
	DEBUG_ASSERTCRASH(m_renderObject, ("BreakApart remainder piece model %s not found!", modelName.str()));
	if (m_renderObject == nullptr)
		return;

	if (W3DDisplay::m_3DScene != nullptr)
		W3DDisplay::m_3DScene->Add_Render_Object(m_renderObject);

	m_renderObject->Set_User_Data(getDrawable()->getDrawableInfo());

	// GeneralsMod @feature Dimitar 19/09/2026: no live-bone alignment here -- unlike a single
	// broken-off part, the remainder isn't tied to one bone, so it's simply placed at the dying
	// object's own plain transform (m_pieceOffset stays identity, same fallback setBreakApartPiece()
	// itself uses when no live transform is available).
	m_renderObject->Set_Transform(*getDrawable()->getTransformMatrix());

	// Resolve every hidden bone name to an index up front. Same bone-index-0 ambiguity as
	// setBreakApartPiece()/GetBreakApartSubtreeBones() (WW3D's Get_Bone_Index() returns 0 both for
	// "not found" and for the skeleton's own true root bone) -- disambiguated the same way: only
	// treat a 0 result as a real hit if the name actually matches the root bone's own name. Getting
	// this wrong in THIS direction (show-many/hide-few) would mean a bone that should be hidden
	// stays visible instead -- worth the extra check, unlike setBreakApartPiece()'s own "index 0 ==
	// hide everything" safe failure mode, which doesn't apply here.
	std::vector<int> hiddenIndices;
	const HTreeClass* htree = m_renderObject->Get_HTree();
	if (htree != nullptr)
	{
		hiddenIndices.reserve(hiddenBoneNames.size());
		for (std::vector<AsciiString>::const_iterator it = hiddenBoneNames.begin(); it != hiddenBoneNames.end(); ++it)
		{
			int idx = htree->Get_Bone_Index(it->str());
			if (idx != 0 || it->compareNoCase(htree->Get_Bone_Name(0)) == 0)
				hiddenIndices.push_back(idx);
		}
	}

	int numSub = m_renderObject->Get_Num_Sub_Objects();
	for (int i = 0; i < numSub; ++i)
	{
		RenderObjClass* sub = m_renderObject->Get_Sub_Object(i);	// Add_Ref()'d by this call
		if (sub != nullptr)
		{
			int subBoneIndex = m_renderObject->Get_Sub_Object_Bone_Index(sub);
			Bool isHidden = false;
			for (std::vector<int>::const_iterator hIt = hiddenIndices.begin(); hIt != hiddenIndices.end() && !isHidden; ++hIt)
				isHidden = (*hIt == subBoneIndex);
			sub->Set_Hidden(isHidden ? 1 : 0);
			REF_PTR_RELEASE(sub);
		}
	}

	attachConfiguredParticleSystem();
}

//-------------------------------------------------------------------------------------------------
// GeneralsMod @feature Dimitar 01/10/2026: creates and bone-attaches this instance's configured
// ParticleSystem (W3DBreakApartPieceDrawModuleData::m_particleSystemTemplate/m_particleSystemBoneName),
// if any -- shared by setBreakApartPiece()/setBreakApartRemainder() above, called once each, right
// after m_renderObject is fully set up (clone created, transform placed, subobjects hidden/shown) so
// Get_Bone_Index()/Get_Bone_Transform() below see the FINAL clone state. No-op if the field was never
// set in INI (m_particleSystemTemplate == nullptr, the ModuleData's own default) or
// TheParticleSystemManager isn't up yet.
//
// Position/orientation logic mirrors W3DModelDraw::recalcBonesForClientParticleSystems() (its own
// "ugh... kill the mtx so we get it in modelspace, not world space" comment, W3DModelDraw.cpp) --
// temporarily force m_renderObject to identity so Get_Bone_Transform() returns the bone's LOCAL
// (piece-relative) position/rotation rather than wherever this piece currently sits in the world,
// then restore the real transform immediately after reading it. sys->attachToDrawable(getDrawable())
// is what then keeps the particle system following this piece every frame (world placement handled
// entirely by the particle system's own attachment, not by anything in doDrawModule()/
// reactToTransformChange() above) -- so it rides along through landing and tumbling exactly like the
// render clone itself does via m_pieceOffset.
//
// "None" (or an unresolved bone name -- misspelled, or genuinely absent on this particular piece,
// e.g. a BreakApartModel bone with no matching name in THIS piece's own subtree) means "no specific
// bone" -- the particle system is left at local (0,0,0)/no extra rotation, i.e. attached at the
// piece's own root (which, since the 23/09/2026 recentering fix, is roughly this piece's own visual
// center already -- a reasonable default for "just put smoke somewhere on this piece").
//-------------------------------------------------------------------------------------------------
void W3DBreakApartPieceDraw::attachConfiguredParticleSystem()
{
	if (m_renderObject == nullptr || TheParticleSystemManager == nullptr)
		return;

	const W3DBreakApartPieceDrawModuleData* modData = getW3DBreakApartPieceDrawModuleData();
	if (modData == nullptr || modData->m_particleSystemTemplate == nullptr)
		return;

	ParticleSystem* sys = TheParticleSystemManager->createParticleSystem(modData->m_particleSystemTemplate);
	if (sys == nullptr)
		return;

	Coord3D pos = { 0.0f, 0.0f, 0.0f };
	Real rotationZ = 0.0f;

	Bool wantsBone = !modData->m_particleSystemBoneName.isEmpty() && modData->m_particleSystemBoneName.compareNoCase("none") != 0;
	const HTreeClass* htree = m_renderObject->Get_HTree();
	int boneIndex = (wantsBone && htree != nullptr) ? htree->Get_Bone_Index(modData->m_particleSystemBoneName.str()) : 0;
	if (wantsBone && boneIndex != 0)
	{
		Matrix3D savedXform = m_renderObject->Get_Transform();
		Matrix3D identityXform(true);
		m_renderObject->Set_Transform(identityXform);

		const Matrix3D& boneXform = m_renderObject->Get_Bone_Transform(boneIndex);
		Vector3 vpos = boneXform.Get_Translation();
		rotationZ = boneXform.Get_Z_Rotation();

		m_renderObject->Set_Transform(savedXform);

		pos.x = vpos.X;
		pos.y = vpos.Y;
		pos.z = vpos.Z;
	}

	sys->setPosition(&pos);
	sys->rotateLocalTransformZ(rotationZ);
	sys->attachToDrawable(getDrawable());
	sys->setSaveable(FALSE);	///< re-created fresh on load, same as W3DModelDraw's own bone-attached particle systems

	m_particleSystemID = sys->getSystemID();
}

//-------------------------------------------------------------------------------------------------
void W3DBreakApartPieceDraw::reactToTransformChange( const Matrix3D *oldMtx,
																											const Coord3D *oldPos,
																											Real oldAngle )
{
	if( m_renderObject )
	{
		// GeneralsMod @feature Dimitar 19/09/2026: recompose with m_pieceOffset instead of a bare
		// copy, so the bone alignment captured in setBreakApartPiece() (see there) survives every
		// subsequent transform change, not just the very first one. Identity offset (the "no live
		// transform was available" case) makes this behave exactly like the old bare-copy code.
		Matrix3D worldXform;
		Matrix3D::Multiply( *getDrawable()->getTransformMatrix(), m_pieceOffset, &worldXform );
		m_renderObject->Set_Transform( worldXform );
	}
}

//-------------------------------------------------------------------------------------------------
void W3DBreakApartPieceDraw::doDrawModule(const Matrix3D* transformMtx)
{
	if (m_renderObject)
	{
		Matrix3D scaledTransform;
		if (getDrawable()->getInstanceScale() != 1.0f)
		{	//do custom scaling of the W3D model.
			scaledTransform = *transformMtx;
			scaledTransform.Scale(getDrawable()->getInstanceScale());
			transformMtx = &scaledTransform;
			m_renderObject->Set_ObjectScale(getDrawable()->getInstanceScale());
		}
		// GeneralsMod @feature Dimitar 19/09/2026: see reactToTransformChange() above -- same
		// m_pieceOffset recomposition instead of a bare Set_Transform(*transformMtx).
		Matrix3D worldXform;
		Matrix3D::Multiply( *transformMtx, m_pieceOffset, &worldXform );
		m_renderObject->Set_Transform(worldXform);
	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void W3DBreakApartPieceDraw::crc( Xfer *xfer )
{

	// extend base class
	DrawModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void W3DBreakApartPieceDraw::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	DrawModule::xfer( xfer );

	// GeneralsMod @feature Dimitar 19/09/2026: deliberately nothing else to persist here -- m_renderObject
	// is a pure rendering-side handle (never touches synced sim state), and it's fully re-derivable
	// on load the same way it was created in the first place. Since BreakApartDeathBehaviorV2 itself
	// calls setBreakApartPiece() only once, right at spawn time (not from xfer/loadPostProcess), a
	// reloaded save would need the model/bone names re-supplied some other way to look identical --
	// acceptable for a short-lived debris piece (see this module's own header comment), same
	// "not concerned with faithfully round-tripping through a save" posture W3DGhostObject-style
	// pure-render helpers already take in this engine.

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void W3DBreakApartPieceDraw::loadPostProcess()
{

	// extend base class
	DrawModule::loadPostProcess();

}
