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

// FILE: W3DWheeledTankDraw.cpp ///////////////////////////////////////////////////////////////////
// Desc:   See W3DWheeledTankDraw.h.
///////////////////////////////////////////////////////////////////////////////////////////////////

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
// GeneralsMod @feature Dimitar 15/09/2026: unlike GameEngine/.cpp files, GameEngineDevice/.cpp files
// do NOT start with "PreRTS.h" -- see W3DPersistentAnimModelDraw.cpp for the same note.
#include "Common/GlobalData.h"
#include "Common/INI.h"
#include "Common/Xfer.h"

#include "GameClient/Drawable.h"

#include "GameLogic/Object.h"
#include "GameLogic/Locomotor.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/PhysicsUpdate.h"

#include "W3DDevice/GameClient/W3DGameClient.h"
#include "W3DDevice/GameClient/Module/W3DWheeledTankDraw.h"

//-------------------------------------------------------------------------------------------------
W3DWheeledTankDrawModuleData::W3DWheeledTankDrawModuleData()
	: m_tireRotationMultiplier(0.0f)
{
}

//-------------------------------------------------------------------------------------------------
W3DWheeledTankDrawModuleData::~W3DWheeledTankDrawModuleData()
{
}

//-------------------------------------------------------------------------------------------------
void W3DWheeledTankDrawModuleData::buildFieldParse(MultiIniFieldParse& p)
{
	W3DTankDrawModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "TireRotationMultiplier", INI::parseReal, nullptr, offsetof(W3DWheeledTankDrawModuleData, m_tireRotationMultiplier) },
		{ "LeftTireBones", INI::parseAsciiStringVector, nullptr, offsetof(W3DWheeledTankDrawModuleData, m_leftTireBoneNames) },
		{ "RightTireBones", INI::parseAsciiStringVector, nullptr, offsetof(W3DWheeledTankDrawModuleData, m_rightTireBoneNames) },
		{ nullptr, nullptr, nullptr, 0 }
	};
	p.add(dataFieldParse);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
W3DWheeledTankDraw::W3DWheeledTankDraw( Thing *thing, const ModuleData* moduleData )
: W3DTankDraw( thing, moduleData )
, m_leftWheelRotation(0.0f)
, m_rightWheelRotation(0.0f)
, m_prevRenderObjForTires(nullptr)
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
W3DWheeledTankDraw::~W3DWheeledTankDraw()
{
	// nothing owned here beyond plain bone-index vectors -- no particle systems or sub-object
	// references of our own to release (tread debris, if any, is released by W3DTankDraw::~W3DTankDraw).
}

//-------------------------------------------------------------------------------------------------
/** Resolve LeftTireBones/RightTireBones names to bone indices on the current render object. */
//-------------------------------------------------------------------------------------------------
void W3DWheeledTankDraw::updateTireBones()
{
	m_leftTireBones.clear();
	m_rightTireBones.clear();

	RenderObjClass *robj = getRenderObject();
	m_prevRenderObjForTires = robj;

	if (robj == nullptr)
		return;

	const W3DWheeledTankDrawModuleData *moduleData = getW3DWheeledTankDrawModuleData();
	if (moduleData == nullptr)
		return;

	m_leftTireBones.reserve(moduleData->m_leftTireBoneNames.size());
	for (size_t i = 0; i < moduleData->m_leftTireBoneNames.size(); ++i)
	{
		const AsciiString &boneName = moduleData->m_leftTireBoneNames[i];
		const Int boneIndex = robj->Get_Bone_Index(boneName.str());
		DEBUG_ASSERTCRASH(boneIndex, ("Missing left tire bone %s in model %s", boneName.str(), robj->Get_Name()));
		m_leftTireBones.push_back(boneIndex);
	}

	m_rightTireBones.reserve(moduleData->m_rightTireBoneNames.size());
	for (size_t i = 0; i < moduleData->m_rightTireBoneNames.size(); ++i)
	{
		const AsciiString &boneName = moduleData->m_rightTireBoneNames[i];
		const Int boneIndex = robj->Get_Bone_Index(boneName.str());
		DEBUG_ASSERTCRASH(boneIndex, ("Missing right tire bone %s in model %s", boneName.str(), robj->Get_Name()));
		m_rightTireBones.push_back(boneIndex);
	}
}

//-------------------------------------------------------------------------------------------------
void W3DWheeledTankDraw::onRenderObjRecreated()
{
	// let the base class refresh its own tread bookkeeping (and its own m_prevRenderObj) first.
	W3DTankDraw::onRenderObjRecreated();
	updateTireBones();
}

//-------------------------------------------------------------------------------------------------
/** Spin LeftTireBones/RightTireBones -- same direction for forward/backward driving, opposite
	* directions while turning (added on top of, not instead of, the forward/backward spin so that
	* normal steering corrections don't cause a visible snap -- see the file header for why). */
//-------------------------------------------------------------------------------------------------
void W3DWheeledTankDraw::doDrawModule(const Matrix3D* transformMtx)
{
	// Handles turret, tread debris, and tread-mesh UV scrolling exactly as W3DTankDraw always has.
	W3DTankDraw::doDrawModule(transformMtx);

	if (!TheGlobalData->m_showClientPhysics)
		return;

	// TheSuperHackers @tweak Update the draw on every WW Sync only.
	// All calculations are originally catered to a 30 fps logic step.
	if (WW3D::Get_Sync_Frame_Time() == 0)
		return;

	RenderObjClass *robj = getRenderObject();
	if (robj == nullptr)
		return;

	// Defensive fallback in case onRenderObjRecreated() wasn't (yet) called for this render object --
	// same manual re-check W3DTruckDraw/W3DTankTruckDraw make for their own bone caches.
	if (robj != m_prevRenderObjForTires)
		updateTireBones();

	if (m_leftTireBones.empty() && m_rightTireBones.empty())
		return;

	const W3DWheeledTankDrawModuleData *moduleData = getW3DWheeledTankDrawModuleData();
	if (moduleData == nullptr)
		return; // shouldn't ever happen.

	Object *obj = getDrawable()->getObject();
	if (obj == nullptr)
		return;

	PhysicsBehavior *physics = obj->getPhysics();
	if (physics == nullptr)
		return;

	Real signedSpeed = physics->getVelocityMagnitude();
	if (AIUpdateInterface *ai = obj->getAI())
	{
		if (Locomotor *loco = ai->getCurLocomotor())
		{
			if (loco->isMovingBackwards())
				signedSpeed = -signedSpeed; // rotate wheels backwards.
		}
	}

	const Real rotationFactor = moduleData->m_tireRotationMultiplier;
	const Real forwardDelta = rotationFactor * signedSpeed;

	// PhysicsTurningType: TURN_POSITIVE = turning left, TURN_NEGATIVE = turning right (matches the
	// sign convention W3DTankDraw's own tread-scroll code already uses for the same query). The
	// turn term is a flat per-frame amount (not scaled by speed) so it still shows up during an
	// in-place pivot at zero forward speed, and it's ADDED to forwardDelta rather than replacing
	// it, so a minor steering correction while driving doesn't snap the spin rate/direction.
	Real turnDelta = 0.0f;
	switch (physics->getTurning())
	{
		case TURN_POSITIVE: // turning left: left tires rotate backward, right tires rotate forward
			turnDelta = rotationFactor;
			break;
		case TURN_NEGATIVE: // turning right: left tires rotate forward, right tires rotate backward
			turnDelta = -rotationFactor;
			break;
		case TURN_NONE:
		default:
			break;
	}

	m_leftWheelRotation = WWMath::Normalize_Angle(m_leftWheelRotation + forwardDelta - turnDelta);
	m_rightWheelRotation = WWMath::Normalize_Angle(m_rightWheelRotation + forwardDelta + turnDelta);

	Matrix3D wheelXfrm(1);

	for (size_t i = 0; i < m_leftTireBones.size(); ++i)
	{
		const Int boneIndex = m_leftTireBones[i];
		if (!boneIndex)
			continue;
		wheelXfrm.Make_Identity();
		wheelXfrm.Rotate_Y(m_leftWheelRotation);
		robj->Capture_Bone(boneIndex);
		robj->Control_Bone(boneIndex, wheelXfrm);
	}

	for (size_t i = 0; i < m_rightTireBones.size(); ++i)
	{
		const Int boneIndex = m_rightTireBones[i];
		if (!boneIndex)
			continue;
		wheelXfrm.Make_Identity();
		wheelXfrm.Rotate_Y(m_rightWheelRotation);
		robj->Capture_Bone(boneIndex);
		robj->Control_Bone(boneIndex, wheelXfrm);
	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void W3DWheeledTankDraw::crc( Xfer *xfer )
{

	// extend base class
	W3DTankDraw::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void W3DWheeledTankDraw::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	W3DTankDraw::xfer( xfer );

	// m_leftWheelRotation/m_rightWheelRotation are purely visual state re-derived every frame from
	// PhysicsBehavior -- nothing here needs to be saved, same as W3DTankDraw/W3DTruckDraw's own
	// per-frame visual state.

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void W3DWheeledTankDraw::loadPostProcess()
{

	// extend base class
	W3DTankDraw::loadPostProcess();

	// force tire bone indices to be re-resolved against whatever render object comes back after
	// load, rather than trusting stale indices from before the save.
	m_leftTireBones.clear();
	m_rightTireBones.clear();
	m_prevRenderObjForTires = nullptr;

}
