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

// FILE: W3DWheeledTankDraw.h /////////////////////////////////////////////////////////////////////
//
//	GeneralsMod @feature Dimitar 15/09/2026
//	Just like W3DTankDraw (turret handling, tread debris particles, tread-mesh UV scrolling -- all
//	of it, unchanged), plus a pair of wheel-bone lists that spin like a skid-steered wheeled vehicle
//	instead of (or alongside) scrolling tread textures: LeftTireBones and RightTireBones spin
//	together in the same direction when driving straight forward/backward, but counter-rotate
//	(one side forward, the other backward) while the unit is turning -- the same visual language as
//	real tank treads, just applied to wheel bones via Rotate_Y instead of tread-mesh UV offsets.
//
//	Why a new module instead of extending W3DTruckDraw: W3DTruckDraw's wheel bones always spin in
//	the same direction as each other (only flipping for forward/backward via
//	Locomotor::isMovingBackwards()) and rely on steering-angle/suspension data
//	(Drawable::getWheelInfo()) for a front-wheel-steer look. This module targets vehicles that steer
//	by differential wheel speed instead (like a tracked vehicle, just with wheels) -- no steering
//	angle, no suspension bounce, just per-side rotation direction driven by the object's actual
//	turn state.
//
//	How the differential is computed (see W3DWheeledTankDraw::doDrawModule): the turn contribution
//	is ADDED on top of the forward/backward contribution, not a mode switch between them. That
//	means (a) an in-place pivot (zero forward speed, but PhysicsBehavior::getTurning() != TURN_NONE)
//	still shows the wheels counter-rotating, and (b) normal driving with a minor steering
//	correction doesn't visibly snap between "speed-based" and "turn-based" spin every time
//	getTurning() flips on and off, since the turn term simply adds a small amount either way.
//	PhysicsTurningType's TURN_POSITIVE means turning left and TURN_NEGATIVE means turning right
//	(confirmed against Locomotor::rotateObjAroundLocoPivot, and matches the sign convention
//	W3DTankDraw's own tread-scroll code already uses for the identical turn query).
//
//	Usage -- give the target object this instead of W3DTankDraw or W3DTruckDraw:
//
//	Draw = W3DWheeledTankDraw ModuleTag_01
//	  ... every existing W3DTankDraw field still works here, unchanged (Model, ConditionState,
//	  TreadDebrisLeft/Right, TreadAnimationRate, etc.) ...
//	  TireRotationMultiplier = 0.2      ; radians of bone rotation added per frame per unit of speed;
//	                                    ; also reused (undivided by speed) as the flat per-frame
//	                                    ; counter-rotation magnitude while turning -- see header
//	                                    ; comment above. Same meaning/units as W3DTruckDraw's field
//	                                    ; of the same name.
//	  LeftTireBones  = Tire01 Tire02 Tire03 Tire04   ; space-separated bone names, any count
//	  RightTireBones = Tire05 Tire06 Tire07 Tire08   ; independent of LeftTireBones's count
//	End
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include <vector>

#include "W3DDevice/GameClient/Module/W3DTankDraw.h"

//-------------------------------------------------------------------------------------------------
class W3DWheeledTankDrawModuleData : public W3DTankDrawModuleData
{
public:
	std::vector<AsciiString> m_leftTireBoneNames;
	std::vector<AsciiString> m_rightTireBoneNames;
	Real m_tireRotationMultiplier;

	W3DWheeledTankDrawModuleData();
	virtual ~W3DWheeledTankDrawModuleData() override;
	static void buildFieldParse(MultiIniFieldParse& p);
};

//-------------------------------------------------------------------------------------------------
class W3DWheeledTankDraw : public W3DTankDraw
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( W3DWheeledTankDraw, "W3DWheeledTankDraw" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( W3DWheeledTankDraw, W3DWheeledTankDrawModuleData )

public:

	W3DWheeledTankDraw( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	virtual void doDrawModule(const Matrix3D* transformMtx) override;

protected:
	virtual void onRenderObjRecreated() override;

protected:
	std::vector<Int> m_leftTireBones;
	std::vector<Int> m_rightTireBones;

	Real m_leftWheelRotation;
	Real m_rightWheelRotation;

	// TheSuperHackers @info Tracked independently of W3DTankDraw's own m_prevRenderObj (which it
	// already updates for tread purposes before our doDrawModule code runs each frame) so that a
	// render-object change can still be detected here as a defensive fallback, mirroring the same
	// belt-and-suspenders manual check W3DTruckDraw/W3DTankTruckDraw make in their own doDrawModule
	// in addition to their onRenderObjRecreated() hook.
	RenderObjClass *m_prevRenderObjForTires;

	void updateTireBones(); ///< Resolve LeftTireBones/RightTireBones names to bone indices on the current render object.
};
