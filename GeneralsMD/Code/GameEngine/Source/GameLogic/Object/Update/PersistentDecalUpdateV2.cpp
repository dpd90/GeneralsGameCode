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

// FILE: PersistentDecalUpdateV2.cpp /////////////////////////////////////////////////////////////
//
//	GeneralsMod @feature Dimitar 14/09/2026
//	See PersistentDecalUpdateV2.h for the full design rationale.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/GameUtility.h"	// GeneralsMod @feature Dimitar 14/09/2026: rts::getObservedOrLocalPlayer(), for OnlyVisibleToOwningPlayer
#include "Common/Xfer.h"
#include "GameClient/Drawable.h"
#include "GameLogic/Module/PersistentDecalUpdateV2.h"
#include "GameLogic/Object.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PersistentDecalUpdateV2::PersistentDecalUpdateV2( Thing *thing, const ModuleData* moduleData ) : UpdateModule( thing, moduleData )
{
	setWakeFrame( getObject(), UPDATE_SLEEP_FOREVER );
}

//-------------------------------------------------------------------------------------------------
// GeneralsMod @bugfix Dimitar 14/09/2026: MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE only declares the
// virtual destructor prototype in the header -- it still has to be defined here, or every build
// fails at LINK time (not compile time) with LNK2019 unresolved external symbol on the scalar
// deleting destructor, the moment anything actually instantiates/destroys one of these (i.e. the
// first real INI usage). Matches SpyVisionUpdate::~SpyVisionUpdate()'s own trivial empty body --
// nothing to clean up here, W3DModelDraw's own destructor already releases m_terrainDecal.
//-------------------------------------------------------------------------------------------------
PersistentDecalUpdateV2::~PersistentDecalUpdateV2()
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
UpdateSleepTime PersistentDecalUpdateV2::update()
{
	// GeneralsMod @feature Dimitar 14/09/2026: this module never wakes itself once constructed --
	// upgradeImplementation() paints the decal once (below) and never touches the wake state, so
	// the object stays asleep forever. This should never actually run, but UpdateModule::update()
	// is pure virtual and must be defined.
	return UPDATE_SLEEP_FOREVER;
}

//-------------------------------------------------------------------------------------------------
/** CreateModuleInterface hook -- fires once per object, right after construction completes. Safe
	* to touch the Drawable here (unlike this module's own constructor): ThingFactory::newObject()
	* calls onCreate() only after Object::init() has already run TheGameLogic->sendObjectCreated(),
	* which is what creates and binds the Drawable -- see that function's own comment for the exact
	* same class of bug ("veterancy create module... chemical suits upgrade... terrain decal") this
	* mirrors and avoids. Calling giveSelfUpgrade() straight from the constructor instead would look
	* like it works (no crash -- getDrawable() is just null-checked away in upgradeImplementation()
	* below) but would silently paint no decal at all, every time, for StartsActive units. */
//-------------------------------------------------------------------------------------------------
void PersistentDecalUpdateV2::onCreate()
{
	if ( getPersistentDecalUpdateV2ModuleData()->m_startsActive )
		giveSelfUpgrade();
}

//-------------------------------------------------------------------------------------------------
/** UpgradeMux hook -- fires once, the first time a TriggeredBy upgrade completes (or immediately
	* from onCreate() above, if StartsActive). */
//-------------------------------------------------------------------------------------------------
void PersistentDecalUpdateV2::upgradeImplementation()
{
	const PersistentDecalUpdateV2ModuleData *data = getPersistentDecalUpdateV2ModuleData();

	if ( data->m_textureName.isEmpty() )
		return;

	// GeneralsMod @feature Dimitar 14/09/2026: OnlyVisibleToOwningPlayer -- same one-time,
	// checked-only-here semantics as RadiusDecalTemplate::createRadiusDecal()'s own field of the
	// same name (Core/GameEngine/Source/GameClient/RadiusDecal.cpp): if the observing client isn't
	// this object's own controlling player, this client's local Shadow* is simply never created at
	// all -- every other client still runs upgradeImplementation() identically (this doesn't touch
	// synced sim state, only local rendering), so this is purely per-client. Uses
	// rts::getObservedOrLocalPlayer() (not ThePlayerList->getLocalPlayer()) to behave correctly for
	// a spectator/replay observer following a specific player, matching Drawable::changedTeam()'s
	// own choice of helper for the (differently-scoped) KindOf=FS_FAKE relationship check. Like
	// RadiusDecalTemplate's field, this is NOT re-evaluated later -- a unit captured after this
	// decal is painted keeps whatever visibility was decided here.
	if ( data->m_onlyVisibleToOwningPlayer &&
			 getObject()->getControllingPlayer() != rts::getObservedOrLocalPlayer() )
		return;

	Drawable *draw = getObject()->getDrawable();
	if ( draw )
		draw->setPersistentDecal( data->m_textureName, data->m_sizeX, data->m_sizeY, data->m_style );
}

//-------------------------------------------------------------------------------------------------
/** ObjectModule hook -- fires on genuine capture only (Object::setTeam() calls this from its own
	* onCapture(), and only when oldTeam && team && !restoring -- never on initial team assignment).
	* GeneralsMod @feature Dimitar 14/09/2026: OnlyVisibleToOwningPlayer's own creation-time check in
	* upgradeImplementation() above is a one-shot decision and by itself goes stale the moment a unit
	* changes hands -- exactly the KindOf=FS_FAKE problem Drawable::changedTeam() solves for the
	* vanilla shadow-texture decal, except that's Drawable-level code with no idea this module or its
	* OnlyVisibleToOwningPlayer field exist. This re-runs the same per-client visibility decision from
	* the new ownership instead: if this client is now the owner and wasn't painting the decal (or
	* never had a reason to check because OnlyVisibleToOwningPlayer was off from another module's
	* implementation -- moot for us, we own our own decal state), (re)paint it; if this client no
	* longer owns the unit, clear it. Nothing to do when OnlyVisibleToOwningPlayer is off (the decal,
	* if any, was already visible to everyone and capture doesn't change that) or when the decal was
	* never actually granted to this unit at all (isAlreadyUpgraded() false -- StartsActive=No and its
	* TriggeredBy upgrade never completed). */
//-------------------------------------------------------------------------------------------------
void PersistentDecalUpdateV2::onCapture( Player *oldOwner, Player *newOwner )
{
	const PersistentDecalUpdateV2ModuleData *data = getPersistentDecalUpdateV2ModuleData();

	if ( !data->m_onlyVisibleToOwningPlayer || data->m_textureName.isEmpty() || !isAlreadyUpgraded() )
		return;

	Drawable *draw = getObject()->getDrawable();
	if ( !draw )
		return;

	if ( newOwner == rts::getObservedOrLocalPlayer() )
		draw->setPersistentDecal( data->m_textureName, data->m_sizeX, data->m_sizeY, data->m_style );
	else
		draw->setPersistentDecal( AsciiString::TheEmptyString, 0.0f, 0.0f, data->m_style );	// this client no longer owns it -- clear
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void PersistentDecalUpdateV2::crc( Xfer *xfer )
{

	// extend base class
	UpdateModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void PersistentDecalUpdateV2::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpdateModule::xfer( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void PersistentDecalUpdateV2::loadPostProcess()
{

	// extend base class
	UpdateModule::loadPostProcess();

}
