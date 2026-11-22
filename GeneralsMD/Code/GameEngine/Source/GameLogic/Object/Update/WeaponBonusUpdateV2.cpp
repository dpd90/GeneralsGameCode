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

// FILE: WeaponBonusUpdateV2.cpp /////////////////////////////////////////////////
//-----------------------------------------------------------------------------
//
//	GeneralsMod @feature Dimitar 08/09/2026
//	See WeaponBonusUpdateV2.h. This is a copy of WeaponBonusUpdate.cpp with a
//	single behavioral difference: the tint applied while the bonus is active
//	comes from the "Tint" INI field instead of the hardcoded FRENZY_COLOR /
//	FRENZY_COLOR_INFANTRY constants, and is not varied by kindof. The custom
//	color is scaled down by WEAPON_BONUS_V2_TINT_SCALE before being handed to
//	the Drawable, because the tint is added directly to the scene lights'
//	diffuse color (see Drawable::getTintColor / W3DScene.cpp), so a raw
//	0-255 -> 0.0-1.0 color would blow out the model instead of just tinting
//	it. The scale keeps the visual intensity in line with the stock
//	FRENZY_COLOR magnitude no matter what hue is chosen.
//
//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// USER INCLUDES //////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "GameLogic/Module/WeaponBonusUpdateV2.h"

#define DEFINE_WEAPONBONUSCONDITION_NAMES///< GeneralsMod @feature Dimitar 08/09/2026: TheWeaponBonusNames is `static`, so each TU that uses it needs this, same as WeaponBonusUpdate.cpp does

#include "Common/ThingTemplate.h"///< GeneralsMod @feature Dimitar 22/10/2026: getTemplate()->getName() for the detonation-effect conflict message
#include "GameLogic/Module/ContainModule.h"
#include "GameLogic/Object.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/Weapon.h"

// GeneralsMod @feature Dimitar 08/09/2026: uniform dampening applied to any
// modder-specified Tint color, regardless of hue, so it stays an additive
// light tint (like the stock FRENZY_COLOR ~0.3 magnitude) rather than
// blowing out the model when a full-saturation color is specified in INI.
static const Real WEAPON_BONUS_V2_TINT_SCALE = 0.3f;

//-----------------------------------------------------------------------------
WeaponBonusUpdateV2ModuleData::WeaponBonusUpdateV2ModuleData()
{
	m_requiredAffectKindOf.clear();
	m_forbiddenAffectKindOf.clear();
	m_bonusDuration = 0;
	m_bonusDelay = 0;
	m_bonusRange = 0;
	m_bonusConditionType = WEAPONBONUSCONDITION_INVALID;
	m_tintColor.red = 0.0f;
	m_tintColor.green = 0.0f;
	m_tintColor.blue = 0.0f;
	m_hasTint = FALSE;							///< GeneralsMod @feature Dimitar 22/10/2026
	m_affectOnlyProjectiles = FALSE;
	m_detonationFX = nullptr;
	m_detonationOCL = nullptr;
}

//-----------------------------------------------------------------------------
// GeneralsMod @feature Dimitar 22/10/2026: a normal RGB parse that also records that a Tint line was present,
// so a module with no Tint line contributes no tint instead of a black override / the default FRENZY red.
/*static*/ void WeaponBonusUpdateV2ModuleData::parseTint( INI* ini, void *instance, void *store, const void* userData )
{
	INI::parseRGBColor( ini, instance, store, userData );
	WeaponBonusUpdateV2ModuleData *self = (WeaponBonusUpdateV2ModuleData*)( (char*)store - offsetof( WeaponBonusUpdateV2ModuleData, m_tintColor ) );
	self->m_hasTint = TRUE;
}

//-----------------------------------------------------------------------------
void WeaponBonusUpdateV2ModuleData::buildFieldParse(MultiIniFieldParse& p)
{
  UpdateModuleData::buildFieldParse(p);
	static const FieldParse dataFieldParse[] =
	{
		{ "RequiredAffectKindOf",		KindOfMaskType::parseFromINI,		nullptr, offsetof( WeaponBonusUpdateV2ModuleData, m_requiredAffectKindOf ) },
		{ "ForbiddenAffectKindOf",	KindOfMaskType::parseFromINI,		nullptr, offsetof( WeaponBonusUpdateV2ModuleData, m_forbiddenAffectKindOf ) },
		{ "BonusDuration",					INI::parseDurationUnsignedInt,	nullptr, offsetof( WeaponBonusUpdateV2ModuleData, m_bonusDuration ) },
		{ "BonusDelay",							INI::parseDurationUnsignedInt,	nullptr, offsetof( WeaponBonusUpdateV2ModuleData, m_bonusDelay ) },
		{ "BonusRange",							INI::parseReal,									nullptr, offsetof( WeaponBonusUpdateV2ModuleData, m_bonusRange ) },
		{ "BonusConditionType",			INI::parseIndexList,	TheWeaponBonusNames, offsetof( WeaponBonusUpdateV2ModuleData, m_bonusConditionType ) },
		{ "Tint",										WeaponBonusUpdateV2ModuleData::parseTint,	nullptr, offsetof( WeaponBonusUpdateV2ModuleData, m_tintColor ) },
		// GeneralsMod @feature Dimitar 22/10/2026:
		{ "AffectOnlyProjectiles",	INI::parseBool,									nullptr, offsetof( WeaponBonusUpdateV2ModuleData, m_affectOnlyProjectiles ) },
		{ "DetonationFX",						INI::parseFXList,								nullptr, offsetof( WeaponBonusUpdateV2ModuleData, m_detonationFX ) },
		{ "DetonationOCL",					INI::parseObjectCreationList,		nullptr, offsetof( WeaponBonusUpdateV2ModuleData, m_detonationOCL ) },
		{ nullptr, nullptr, nullptr, 0 }
	};
  p.add(dataFieldParse);
}

//-----------------------------------------------------------------------------
// PUBLIC FUNCTIONS ///////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
WeaponBonusUpdateV2::WeaponBonusUpdateV2( Thing *thing, const ModuleData* moduleData ) : UpdateModule( thing, moduleData )
{
	setWakeFrame(getObject(), UPDATE_SLEEP_NONE);

	// GeneralsMod @feature Dimitar 22/10/2026: projectiles only carry their launcher's bonus FLAGS, not which aura set
	// them, so DetonationFX / DetonationOCL are looked up per condition in TheWeaponStore. Registering from the
	// constructor also re-registers on save-load (objects are re-constructed), and entries live until the game resets,
	// so a projectile still in flight after this unit dies still plays its effect.
	const WeaponBonusUpdateV2ModuleData *data = getWeaponBonusUpdateV2ModuleData();
	if( TheWeaponStore && ( data->m_detonationFX || data->m_detonationOCL ) )
		TheWeaponStore->registerBonusDetonationEffect( data->m_bonusConditionType, data->m_detonationFX, data->m_detonationOCL, getObject()->getTemplate()->getName() );
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
WeaponBonusUpdateV2::~WeaponBonusUpdateV2()
{

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
struct tempWeaponBonusDataV2 // GeneralsMod @feature Dimitar 08/09/2026: V2 of tempWeaponBonusData, carries the custom tint
{
	WeaponBonusConditionType m_type;
	UnsignedInt m_duration;
	KindOfMaskType m_requiredMask;
	KindOfMaskType m_forbiddenMask;
	RGBColor m_tintColor;
	Bool m_hasTint;								///< GeneralsMod @feature Dimitar 22/10/2026
	Bool m_affectOnlyProjectiles;	///< GeneralsMod @feature Dimitar 22/10/2026
};
void containIteratingDoTempWeaponBonusV2( Object *passenger, void *voidData)
{
	tempWeaponBonusDataV2 *data = (tempWeaponBonusDataV2 *)voidData;

	if( passenger->isKindOfMulti(data->m_requiredMask, data->m_forbiddenMask) )
		passenger->doTempWeaponBonus(data->m_type, data->m_duration, data->m_hasTint ? &data->m_tintColor : nullptr,
			TRUE /*V2 never falls back to the default FRENZY red*/, data->m_affectOnlyProjectiles);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
UpdateSleepTime WeaponBonusUpdateV2::update()
{
	const WeaponBonusUpdateV2ModuleData * data = getWeaponBonusUpdateV2ModuleData();
	Object *me = getObject();

	// GeneralsMod @feature Dimitar 08/09/2026: dampen the configured tint uniformly, regardless of hue,
	// before it is added to the scene lights (see WEAPON_BONUS_V2_TINT_SCALE comment above).
	RGBColor scaledTintColor;
	scaledTintColor.red		= data->m_tintColor.red		* WEAPON_BONUS_V2_TINT_SCALE;
	scaledTintColor.green	= data->m_tintColor.green	* WEAPON_BONUS_V2_TINT_SCALE;
	scaledTintColor.blue	= data->m_tintColor.blue	* WEAPON_BONUS_V2_TINT_SCALE;

	PartitionFilterRelationship relationship( me, PartitionFilterRelationship::ALLOW_ALLIES );
	PartitionFilterSameMapStatus filterMapStatus(me);
	PartitionFilterAlive filterAlive;

	// Leaving this here commented out to show that I need to reach valid contents of invalid transports.
	// So these checks are on an individual basis, not in the Partition query
//	PartitionFilterAcceptByKindOf filterKindof(data->m_requiredAffectKindOf,data->m_forbiddenAffectKindOf);
	PartitionFilter *filters[] = { &relationship, &filterAlive, &filterMapStatus, nullptr };

	// scan objects in our region
	ObjectIterator *iter = ThePartitionManager->iterateObjectsInRange( me->getPosition(),
																																			data->m_bonusRange,
																																			FROM_CENTER_2D,
																																			filters );
	MemoryPoolObjectHolder hold( iter );
	tempWeaponBonusDataV2 weaponBonusData;
	weaponBonusData.m_type = data->m_bonusConditionType;
	weaponBonusData.m_duration = data->m_bonusDuration;
	weaponBonusData.m_requiredMask = data->m_requiredAffectKindOf;
	weaponBonusData.m_forbiddenMask = data->m_forbiddenAffectKindOf;
	weaponBonusData.m_tintColor = scaledTintColor;
	weaponBonusData.m_hasTint = data->m_hasTint;
	weaponBonusData.m_affectOnlyProjectiles = data->m_affectOnlyProjectiles;

	for( Object *currentObj = iter->first(); currentObj != nullptr; currentObj = iter->next() )
	{
		if( currentObj->isKindOfMulti(data->m_requiredAffectKindOf, data->m_forbiddenAffectKindOf) )
		{
			// GeneralsMod @feature Dimitar 22/10/2026: no Tint line = contribute no tint (never the default FRENZY red);
			// AffectOnlyProjectiles restricts the bonus to weapons with a ProjectileObject.
			currentObj->doTempWeaponBonus(data->m_bonusConditionType, data->m_bonusDuration, data->m_hasTint ? &scaledTintColor : nullptr,
				TRUE, data->m_affectOnlyProjectiles);
		}

		if( currentObj->getContain() )
		{
			currentObj->getContain()->iterateContained(containIteratingDoTempWeaponBonusV2, &weaponBonusData, FALSE);
		}
	}

	return UPDATE_SLEEP(data->m_bonusDelay); // Only need an internal timer if there are external hooks for wakning us up, or a second thing we can do
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void WeaponBonusUpdateV2::crc( Xfer *xfer )
{

	// extend base class
	UpdateModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void WeaponBonusUpdateV2::xfer( Xfer *xfer )
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
void WeaponBonusUpdateV2::loadPostProcess()
{

	// extend base class
	UpdateModule::loadPostProcess();

}
