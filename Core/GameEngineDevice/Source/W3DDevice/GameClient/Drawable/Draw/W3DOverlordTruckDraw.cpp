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

// FILE: W3DOverlordTruckDraw.cpp ////////////////////////////////////////////////////////////////////////////
// Author: Graham Smallwood, October 2002
// Desc: The Overlord has a super specific special need.  He needs his rider to draw explicitly after him,
// and he needs direct access to get that rider when everyone else can't see it because of the OverlordContain.
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////

#include <vector>
#include "Common/Xfer.h"
#include "GameClient/Drawable.h"
#include "GameLogic/Object.h"
#include "GameLogic/Module/ContainModule.h"
#include "W3DDevice/GameClient/Module/W3DOverlordTruckDraw.h"

//-------------------------------------------------------------------------------------------------
W3DOverlordTruckDrawModuleData::W3DOverlordTruckDrawModuleData()
{
}

//-------------------------------------------------------------------------------------------------
W3DOverlordTruckDrawModuleData::~W3DOverlordTruckDrawModuleData()
{
}

//-------------------------------------------------------------------------------------------------
void W3DOverlordTruckDrawModuleData::buildFieldParse(MultiIniFieldParse& p)
{
  W3DTruckDrawModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ nullptr, nullptr, nullptr, 0 }
	};
  p.add(dataFieldParse);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
W3DOverlordTruckDraw::W3DOverlordTruckDraw( Thing *thing, const ModuleData* moduleData )
: W3DTruckDraw( thing, moduleData )
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
W3DOverlordTruckDraw::~W3DOverlordTruckDraw()
{
}

//-------------------------------------------------------------------------------------------------
void W3DOverlordTruckDraw::doDrawModule(const Matrix3D* transformMtx)
{
	W3DTruckDraw::doDrawModule(transformMtx);

	// Our big thing is that we get our passenger(s) (the turret thing(s)) and then wake them up and
	// make them draw. Ordinary single-rider OverlordContain hands us back just one; OverlordContainV2
	// can hand us back several, one per occupied slot.
	// It depends on us because our renderObject is only made correct in the act of drawing.
	Object *me = getDrawable()->getObject();
	if( me && me->getContain() )
	{
		std::vector<const Object*> riders;
		me->getContain()->friend_getVisibleRiders( riders );

		for( std::vector<const Object*>::const_iterator it = riders.begin(); it != riders.end(); ++it )
		{
			const Object *rider = *it;
			Drawable *riderDraw = rider ? rider->getDrawable() : nullptr;
			if( !riderDraw )
				continue;

			riderDraw->setColorTintEnvelope( *getDrawable()->getColorTintEnvelope() );

			riderDraw->notifyDrawableDependencyCleared();
			riderDraw->draw();
		}
	}
}

//-------------------------------------------------------------------------------------------------
void W3DOverlordTruckDraw::setHidden(Bool h)
{
	W3DTruckDraw::setHidden(h);

	// We need to hide our rider(s), since they won't realize they're being contained in a contained container
	Object *me = getDrawable()->getObject();
	if( me && me->getContain() )
	{
		std::vector<const Object*> riders;
		me->getContain()->friend_getVisibleRiders( riders );

		for( std::vector<const Object*>::const_iterator it = riders.begin(); it != riders.end(); ++it )
		{
			const Object *rider = *it;
			if( rider && rider->getDrawable() )
				rider->getDrawable()->setDrawableHidden(h);
		}
	}
}

//-------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void W3DOverlordTruckDraw::crc( Xfer *xfer )
{

	// extend base class
	W3DTruckDraw::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void W3DOverlordTruckDraw::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	W3DTruckDraw::xfer( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void W3DOverlordTruckDraw::loadPostProcess()
{

	// extend base class
	W3DTruckDraw::loadPostProcess();

}
