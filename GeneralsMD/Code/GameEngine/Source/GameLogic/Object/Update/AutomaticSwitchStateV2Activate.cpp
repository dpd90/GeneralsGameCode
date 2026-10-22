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

// FILE: AutomaticSwitchStateV2Activate.cpp ///////////////////////////////////////////////////////
// Desc:   See AutomaticSwitchStateV2Activate.h.
///////////////////////////////////////////////////////////////////////////////////////////////////

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/INI.h"
#include "Common/Xfer.h"

#include "GameLogic/Object.h"

#include "GameLogic/Module/AutomaticSwitchStateV2Activate.h"
#include "GameLogic/Module/SpecialPowerUpdateModule.h"

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
AutomaticSwitchStateV2ActivateModuleData::AutomaticSwitchStateV2ActivateModuleData()
{
	m_specialPowerTemplate = nullptr;
	m_initialDelayFrames = 0;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
/*static*/ void AutomaticSwitchStateV2ActivateModuleData::buildFieldParse(MultiIniFieldParse& p)
{
	UpdateModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "SpecialPowerTemplate",	INI::parseSpecialPowerTemplate,	nullptr, offsetof( AutomaticSwitchStateV2ActivateModuleData, m_specialPowerTemplate ) },
		{ "InitialDelay",					INI::parseDurationUnsignedInt,		nullptr, offsetof( AutomaticSwitchStateV2ActivateModuleData, m_initialDelayFrames ) },
		{ nullptr, nullptr, nullptr, 0 }
	};
	p.add(dataFieldParse);
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
AutomaticSwitchStateV2Activate::AutomaticSwitchStateV2Activate( Thing *thing, const ModuleData* moduleData ) : UpdateModule( thing, moduleData )
{
	m_hasFired = FALSE;

	// Sleep straight through the delay -- no per-frame cost while waiting. (On save/load the base
	// UpdateModule::xfer restores the real saved wake frame over this, so the remaining delay is kept.)
	UnsignedInt delay = getAutomaticSwitchStateV2ActivateModuleData()->m_initialDelayFrames;
	setWakeFrame( getObject(), delay > 0 ? UPDATE_SLEEP( delay ) : UPDATE_SLEEP_NONE );
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
AutomaticSwitchStateV2Activate::~AutomaticSwitchStateV2Activate()
{
}

// ------------------------------------------------------------------------------------------------
/** InitialDelay has elapsed: find the companion SwitchStateV2 (matched by SpecialPowerTemplate)
	* and drive it into AlteredState, using the same automatic-trigger hand-off
	* SwitchStateWhenDamagedBehaviorV2 uses. Fires exactly once. */
// ------------------------------------------------------------------------------------------------
UpdateSleepTime AutomaticSwitchStateV2Activate::update()
{
	if( m_hasFired )
		return UPDATE_SLEEP_FOREVER;

	m_hasFired = TRUE;

	const AutomaticSwitchStateV2ActivateModuleData *data = getAutomaticSwitchStateV2ActivateModuleData();
	if( data->m_specialPowerTemplate == nullptr )
		return UPDATE_SLEEP_FOREVER;

	Object *obj = getObject();
	if( obj->isEffectivelyDead() )
		return UPDATE_SLEEP_FOREVER;

	for( BehaviorModule** u = obj->getBehaviorModules(); *u; ++u )
	{
		SpecialPowerUpdateInterface *spu = (*u)->getSpecialPowerUpdateInterface();
		if( spu && spu->notifyQualifyingDamage( data->m_specialPowerTemplate ) )
			break;
	}

	return UPDATE_SLEEP_FOREVER;
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void AutomaticSwitchStateV2Activate::crc( Xfer *xfer )
{
	// extend base class
	UpdateModule::crc( xfer );
}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void AutomaticSwitchStateV2Activate::xfer( Xfer *xfer )
{
	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpdateModule::xfer( xfer );

	xfer->xferBool( &m_hasFired );
}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void AutomaticSwitchStateV2Activate::loadPostProcess()
{
	// extend base class
	UpdateModule::loadPostProcess();
}
