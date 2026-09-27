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

// FILE: W3DOverlordWheeledTankDraw.h /////////////////////////////////////////////////////////////
//
//	GeneralsMod @feature Dimitar 23/09/2026
//	W3DWheeledTankDraw + W3DOverlordTankDraw in one module: everything W3DWheeledTankDraw does
//	(all of W3DTankDraw -- turret, tread debris, tread UV scrolling -- plus the differential
//	LeftTireBones/RightTireBones spin), and everything W3DOverlordTankDraw does (explicitly drawing
//	the contained rider(s) right after ourselves, and propagating setHidden() to them).
//
//	Built by subclassing W3DWheeledTankDraw (not W3DOverlordTankDraw): the wheel logic owns real
//	per-instance state and INI fields, while the Overlord part is ~two stateless overrides, so it is
//	far cheaper to re-add the Overlord behavior here than to duplicate the wheel code. The Overlord
//	part is a verbatim copy of W3DOverlordTankDraw's doDrawModule()/setHidden() rider handling,
//	including OverlordContainV2's multi-rider friend_getVisibleRiders() support.
//
//	Usage -- use on an Overlord-style unit (OverlordContain / OverlordContainV2) instead of
//	W3DOverlordTankDraw or W3DWheeledTankDraw:
//
//	Draw = W3DOverlordWheeledTankDraw ModuleTag_01
//	  ... every W3DTankDraw field (Model, ConditionState, TreadDebrisLeft/Right, ...) ...
//	  TireRotationMultiplier = 0.2
//	  LeftTireBones  = Tire01 Tire02 Tire03
//	  RightTireBones = Tire04 Tire05 Tire06
//	End
//
//	No new INI fields of its own -- the module data is just W3DWheeledTankDrawModuleData, reused
//	via the plain MAKE_STANDARD_MODULE_MACRO (same pattern ImmortalBody uses for ActiveBodyModuleData).
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "W3DDevice/GameClient/Module/W3DWheeledTankDraw.h"

//-------------------------------------------------------------------------------------------------
class W3DOverlordWheeledTankDraw : public W3DWheeledTankDraw
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( W3DOverlordWheeledTankDraw, "W3DOverlordWheeledTankDraw" )
	MAKE_STANDARD_MODULE_MACRO( W3DOverlordWheeledTankDraw )

public:

	W3DOverlordWheeledTankDraw( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	virtual void setHidden(Bool h) override;
	virtual void doDrawModule(const Matrix3D* transformMtx) override;

};
