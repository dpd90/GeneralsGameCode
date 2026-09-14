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

// FILE: ShieldGeneratorActivateV2.cpp ///////////////////////////////////////////////////////
// Desc:   See ShieldGeneratorActivateV2.h.
///////////////////////////////////////////////////////////////////////////////////////////////////

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/Xfer.h"
#include "GameLogic/Object.h"
#include "GameLogic/Module/ShieldGeneratorActivateV2.h"

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
ShieldGeneratorActivateV2::ShieldGeneratorActivateV2( Thing *thing, const ModuleData *moduleData )
										 : SpecialPowerModule( thing, moduleData )
{
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
ShieldGeneratorActivateV2::~ShieldGeneratorActivateV2()
{
}

// ------------------------------------------------------------------------------------------------
/** Nothing special: the base implementation validates readiness, starts the recharge, and --
	* critically -- calls initiateIntentToDoSpecialPower(), which is what finds our companion
	* ShieldGeneratorUpdateV2 module (matched by SpecialPowerTemplate) and asks it to (re-)shield. */
// ------------------------------------------------------------------------------------------------
void ShieldGeneratorActivateV2::doSpecialPower( UnsignedInt commandOptions )
{
	if( getObject()->isDisabled() )
		return;

	// call the base class action -- this is what actually finds and triggers our companion
	// ShieldGeneratorUpdateV2 update module via initiateIntentToDoSpecialPower().
	SpecialPowerModule::doSpecialPower( commandOptions );
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void ShieldGeneratorActivateV2::crc( Xfer *xfer )
{
	// extend base class
	SpecialPowerModule::crc( xfer );
}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void ShieldGeneratorActivateV2::xfer( Xfer *xfer )
{
	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	SpecialPowerModule::xfer( xfer );
}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void ShieldGeneratorActivateV2::loadPostProcess()
{
	// extend base class
	SpecialPowerModule::loadPostProcess();
}
