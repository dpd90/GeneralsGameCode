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

// FILE: ProjectileClipFeedbackUpdateV2.cpp ///////////////////////////////////////////////////////
// Desc:   See ProjectileClipFeedbackUpdateV2.h.
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/Xfer.h"

#include "GameClient/Drawable.h"

#include "GameLogic/Object.h"
#include "GameLogic/Weapon.h"
#include "GameLogic/Module/ProjectileClipFeedbackUpdateV2.h"

static const UnsignedInt PCFU2_INVALID_AMMO = 0xFFFFFFFF;
static const Int PCFU2_INVALID_CLIP = -1;

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ProjectileClipFeedbackUpdateV2ModuleData::ProjectileClipFeedbackUpdateV2ModuleData()
{
	m_checkIntervalFrames			= LOGICFRAMES_PER_SECOND / 4;	// ~250ms
	m_onlyWhenContained				= TRUE;
	m_updateWeaponConditions	= FALSE;
}

//-------------------------------------------------------------------------------------------------
/*static*/ void ProjectileClipFeedbackUpdateV2ModuleData::buildFieldParse(MultiIniFieldParse& p)
{
	UpdateModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "CheckInterval",					INI::parseDurationUnsignedInt,	nullptr, offsetof( ProjectileClipFeedbackUpdateV2ModuleData, m_checkIntervalFrames ) },
		{ "OnlyWhenContained",			INI::parseBool,									nullptr, offsetof( ProjectileClipFeedbackUpdateV2ModuleData, m_onlyWhenContained ) },
		{ "UpdateWeaponConditions",	INI::parseBool,									nullptr, offsetof( ProjectileClipFeedbackUpdateV2ModuleData, m_updateWeaponConditions ) },
		{ nullptr, nullptr, nullptr, 0 }
	};
	p.add(dataFieldParse);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ProjectileClipFeedbackUpdateV2::ProjectileClipFeedbackUpdateV2( Thing *thing, const ModuleData* moduleData ) : UpdateModule( thing, moduleData )
{
	invalidateCache();

	// First check on the next update pass, so an object that starts out contained gets synced right away.
	setWakeFrame( getObject(), UPDATE_SLEEP_NONE );
}

//-------------------------------------------------------------------------------------------------
ProjectileClipFeedbackUpdateV2::~ProjectileClipFeedbackUpdateV2()
{

}

//-------------------------------------------------------------------------------------------------
void ProjectileClipFeedbackUpdateV2::invalidateCache()
{
	for( Int i = 0; i < WEAPONSLOT_COUNT; ++i )
	{
		m_lastRemaining[i] = PCFU2_INVALID_AMMO;
		m_lastClipSize[i] = PCFU2_INVALID_CLIP;
	}
}

//-------------------------------------------------------------------------------------------------
void ProjectileClipFeedbackUpdateV2::pushChangedClipStatus()
{
	Object *obj = getObject();
	Drawable *draw = obj->getDrawable();
	if( draw == nullptr )
		return;

	for( Int i = 0; i < WEAPONSLOT_COUNT; ++i )
	{
		const Weapon *w = obj->getWeaponInWeaponSlot( (WeaponSlotType)i );
		if( w == nullptr )
		{
			// Slot empty (or emptied by a weapon set change) -- forget it so a new weapon here is pushed.
			m_lastRemaining[i] = PCFU2_INVALID_AMMO;
			m_lastClipSize[i] = PCFU2_INVALID_CLIP;
			continue;
		}

		// Note: getRemainingAmmo() goes through Weapon::getStatus(), which lazily updates the weapon's
		// status -- fine here since we run in the logic update, same as ObjectWeaponStatusHelper.
		UnsignedInt remaining = w->getRemainingAmmo();
		Int clipSize = w->getClipSize();

		if( remaining != m_lastRemaining[i] || clipSize != m_lastClipSize[i] )
		{
			draw->updateDrawableClipStatus( remaining, clipSize, (WeaponSlotType)i );
			m_lastRemaining[i] = remaining;
			m_lastClipSize[i] = clipSize;
		}
	}
}

//-------------------------------------------------------------------------------------------------
UpdateSleepTime ProjectileClipFeedbackUpdateV2::update()
{
	const ProjectileClipFeedbackUpdateV2ModuleData *data = getProjectileClipFeedbackUpdateV2ModuleData();
	UnsignedInt interval = data->m_checkIntervalFrames > 0 ? data->m_checkIntervalFrames : 1;

	Object *obj = getObject();

	if( data->m_onlyWhenContained && obj->getContainedBy() == nullptr )
	{
		// Outside a container the built-in ObjectWeaponStatusHelper handles this every frame. Drop the
		// cache so the first check after entering a container always pushes.
		invalidateCache();
		return UPDATE_SLEEP( interval );
	}

	if( data->m_updateWeaponConditions )
	{
		// Pushes clip status for every slot AND refreshes the weapon model-condition flags --
		// exactly what ObjectWeaponStatusHelper would do if it weren't skipped while held.
		obj->adjustModelConditionForWeaponStatus();
	}
	else
	{
		pushChangedClipStatus();
	}

	return UPDATE_SLEEP( interval );
}

//-------------------------------------------------------------------------------------------------
/** CRC */
//-------------------------------------------------------------------------------------------------
void ProjectileClipFeedbackUpdateV2::crc( Xfer *xfer )
{

	// extend base class
	UpdateModule::crc( xfer );

}

//-------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
//-------------------------------------------------------------------------------------------------
void ProjectileClipFeedbackUpdateV2::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpdateModule::xfer( xfer );

	// m_lastRemaining / m_lastClipSize are a visual-only cache, not saved (see loadPostProcess()).

}

//-------------------------------------------------------------------------------------------------
/** Load post process */
//-------------------------------------------------------------------------------------------------
void ProjectileClipFeedbackUpdateV2::loadPostProcess()
{

	// extend base class
	UpdateModule::loadPostProcess();

	// Force a full push on the first check after loading, since the Drawable is rebuilt fresh.
	invalidateCache();

}
