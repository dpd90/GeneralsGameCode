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

// FILE: W3DOverlordWheeledTankDraw.cpp ///////////////////////////////////////////////////////////
// Desc:   See W3DOverlordWheeledTankDraw.h.
///////////////////////////////////////////////////////////////////////////////////////////////////

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
// GeneralsMod @feature Dimitar 23/09/2026: GameEngineDevice/.cpp files do NOT start with "PreRTS.h".
#include <vector>

#include "Common/Xfer.h"

#include "GameClient/Drawable.h"

#include "GameLogic/Object.h"
#include "GameLogic/Module/ContainModule.h"

#include "W3DDevice/GameClient/Module/W3DOverlordWheeledTankDraw.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
W3DOverlordWheeledTankDraw::W3DOverlordWheeledTankDraw( Thing *thing, const ModuleData* moduleData )
: W3DWheeledTankDraw( thing, moduleData )
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
W3DOverlordWheeledTankDraw::~W3DOverlordWheeledTankDraw()
{
	// nothing owned here -- riders are owned by the contain module, tire/tread state by the bases.
}

//-------------------------------------------------------------------------------------------------
/** Wheeled-tank drawing first (turret, treads, tire bones), then -- exactly like
	* W3DOverlordTankDraw -- wake up and draw our visible rider(s), since their render objects are
	* only positioned correctly once ours has been drawn this frame. */
//-------------------------------------------------------------------------------------------------
void W3DOverlordWheeledTankDraw::doDrawModule(const Matrix3D* transformMtx)
{
	W3DWheeledTankDraw::doDrawModule(transformMtx);

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
void W3DOverlordWheeledTankDraw::setHidden(Bool h)
{
	W3DWheeledTankDraw::setHidden(h);

	// Hide our rider(s) too, since they won't realize they're being contained in a contained container.
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

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void W3DOverlordWheeledTankDraw::crc( Xfer *xfer )
{

	// extend base class
	W3DWheeledTankDraw::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void W3DOverlordWheeledTankDraw::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	W3DWheeledTankDraw::xfer( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void W3DOverlordWheeledTankDraw::loadPostProcess()
{

	// extend base class
	W3DWheeledTankDraw::loadPostProcess();

}
