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

// FILE: ShieldedBody.cpp //////////////////////////////////////////////////////////////////////////
// Desc:   See ShieldedBody.h.
///////////////////////////////////////////////////////////////////////////////////////////////////

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/Xfer.h"
#include "GameClient/Drawable.h"
#include "GameLogic/Object.h"
#include "GameLogic/Module/ShieldedBody.h"

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
ShieldedBody::ShieldedBody( Thing *thing, const ModuleData* moduleData )
						 : ActiveBody( thing, moduleData )
{
	m_damageStateFrozen = FALSE;
	m_frozenDamageState = BODY_PRISTINE;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
ShieldedBody::~ShieldedBody()
{
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
BodyDamageType ShieldedBody::getDamageState() const
{
	if( m_damageStateFrozen )
		return m_frozenDamageState;

	return ActiveBody::getDamageState();
}

// ------------------------------------------------------------------------------------------------
/** Same as ActiveBody::evaluateVisualCondition(), except it reports getDamageState() -- which may
	* be a frozen snapshot rather than the real live value, see the header comment -- to the Drawable,
	* instead of reading the real damage state directly. */
// ------------------------------------------------------------------------------------------------
void ShieldedBody::evaluateVisualCondition()
{
	Drawable *draw = getObject()->getDrawable();
	if( draw )
		draw->reactToBodyDamageStateChange( getDamageState() );

	// destroy any particle systems that were attached to our body for the old state
	// and create new particle systems for the new state
	updateBodyParticleSystems();
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void ShieldedBody::freezeDamageState()
{
	if( m_damageStateFrozen )
		return;	// already frozen -- don't overwrite the existing snapshot with a possibly-different value

	m_frozenDamageState = ActiveBody::getDamageState();	// capture the TRUE state, not whatever we might currently be reporting
	m_damageStateFrozen = TRUE;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void ShieldedBody::unfreezeDamageState()
{
	if( !m_damageStateFrozen )
		return;	// nothing to do

	m_damageStateFrozen = FALSE;

	// Force one fresh sync now: ActiveBody's own internal logic only calls evaluateVisualCondition()
	// when ITS raw state changes as a direct result of a health change, which may not coincide with
	// this exact moment -- so without this, the Drawable could keep showing the stale frozen value
	// until the next unrelated health change happens to trigger a recheck.
	evaluateVisualCondition();
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void ShieldedBody::crc( Xfer *xfer )
{
	// extend base class
	ActiveBody::crc( xfer );
}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void ShieldedBody::xfer( Xfer *xfer )
{
	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	ActiveBody::xfer( xfer );

	xfer->xferBool( &m_damageStateFrozen );
	xfer->xferUser( &m_frozenDamageState, sizeof( BodyDamageType ) );
}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void ShieldedBody::loadPostProcess()
{
	// extend base class
	ActiveBody::loadPostProcess();
}
