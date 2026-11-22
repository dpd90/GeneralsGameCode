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

// FILE: BuildTimeModifierPowerV2.cpp ///////////////////////////////////////////////////////////
//
//	GeneralsMod @feature Dimitar 10/11/2026
//	See BuildTimeModifierPowerV2.h. Activation logic is CashBountyPower.cpp's verbatim; the effect
//	is the build-time twin of CostModifierUpgrade (Player kindof/percent list, ref counted).
//	Identical (kindOf, percent) entries are ref-counted and multiplied only once, so several host
//	objects (several Command Centers) never stack. The entry is never removed on purpose
//	(permanent science bonus), so no onDelete()/onCapture() overrides.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/Player.h"
#include "Common/Xfer.h"
#include "GameLogic/Module/BuildTimeModifierPowerV2.h"
#include "GameLogic/Object.h"

//-------------------------------------------------------------------------------------------------
BuildTimeModifierPowerV2ModuleData::BuildTimeModifierPowerV2ModuleData()
{
	m_kindOf = KINDOFMASK_NONE;
	m_percentage = 0.0f;
}

//-------------------------------------------------------------------------------------------------
/* static */ void BuildTimeModifierPowerV2ModuleData::buildFieldParse(MultiIniFieldParse& p)
{
	SpecialPowerModuleData::buildFieldParse( p );

	static const FieldParse dataFieldParse[] =
	{
		{ "EffectKindOf",		KindOfMaskType::parseFromINI, nullptr, offsetof( BuildTimeModifierPowerV2ModuleData, m_kindOf ) },
		{ "Percentage",			INI::parsePercentToReal, nullptr, offsetof( BuildTimeModifierPowerV2ModuleData, m_percentage ) },
		{ nullptr, nullptr, nullptr, 0 }
	};
	p.add(dataFieldParse);
}

//-------------------------------------------------------------------------------------------------
BuildTimeModifierPowerV2::BuildTimeModifierPowerV2( Thing *thing, const ModuleData* moduleData ) :
							SpecialPowerModule( thing, moduleData )
{
}

//-------------------------------------------------------------------------------------------------
EMPTY_DTOR( BuildTimeModifierPowerV2 )

//-------------------------------------------------------------------------------------------------
void BuildTimeModifierPowerV2::applyToPlayer( Player *player )
{
	if( player == nullptr )
		return;

	const BuildTimeModifierPowerV2ModuleData *d = getBuildTimeModifierPowerV2ModuleData();
	player->addKindOfProductionTimeChange( d->m_kindOf, d->m_percentage );
}

//-------------------------------------------------------------------------------------------------
/** Host object created after the science was already bought (second Command Center, rebuilt
	* Command Center, object re-created on save load). Adding again only bumps the ref count. */
//-------------------------------------------------------------------------------------------------
void BuildTimeModifierPowerV2::onObjectCreated()
{
	Player *controller = getObject()->getControllingPlayer();
	if( controller && controller->hasScience( getRequiredScience() ) )
		applyToPlayer( controller );
}

//-------------------------------------------------------------------------------------------------
/** Called by Player::addScience() the moment RequiredScience is purchased/granted. */
//-------------------------------------------------------------------------------------------------
void BuildTimeModifierPowerV2::onSpecialPowerCreation()
{
	SpecialPowerModule::onSpecialPowerCreation();

	applyToPlayer( getObject()->getControllingPlayer() );
}

//-------------------------------------------------------------------------------------------------
/** CRC */
//-------------------------------------------------------------------------------------------------
void BuildTimeModifierPowerV2::crc( Xfer *xfer )
{
	SpecialPowerModule::crc( xfer );
}

//-------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version
	* No module state: the effect itself is saved by Player::xfer (version 9). */
//-------------------------------------------------------------------------------------------------
void BuildTimeModifierPowerV2::xfer( Xfer *xfer )
{
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	SpecialPowerModule::xfer( xfer );
}

//-------------------------------------------------------------------------------------------------
/** Load post process */
//-------------------------------------------------------------------------------------------------
void BuildTimeModifierPowerV2::loadPostProcess()
{
	SpecialPowerModule::loadPostProcess();
}
