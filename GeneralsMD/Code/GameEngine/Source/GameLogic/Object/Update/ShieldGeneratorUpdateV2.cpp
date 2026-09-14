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

// FILE: ShieldGeneratorUpdateV2.cpp ///////////////////////////////////////////////////////////////
// Desc:   See ShieldGeneratorUpdateV2.h.
///////////////////////////////////////////////////////////////////////////////////////////////////

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/INI.h"
#include "Common/NameKeyGenerator.h"
#include "Common/Xfer.h"

#include "GameClient/FXList.h"

#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"
#include "GameLogic/ObjectCreationList.h"
#include "GameLogic/Weapon.h"
#include "GameLogic/WeaponStatus.h"

#include "GameLogic/Module/BodyModule.h"
#include "GameLogic/Module/ShieldedBody.h"
#include "GameLogic/Module/ShieldGeneratorUpdateV2.h"

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
ShieldGeneratorUpdateV2ModuleData::ShieldGeneratorUpdateV2ModuleData()
{
	m_specialPowerTemplate = nullptr;
	m_lifetimeFrames = 0;
	m_weaponInTemplate = nullptr;
	m_weaponOutTemplate = nullptr;
	m_oclIn = nullptr;
	m_oclOut = nullptr;
	m_fxListIn = nullptr;
	m_fxListOut = nullptr;
	m_shieldAmount = 0.0f;
	m_shieldAmountPercent = 0.0f;
	m_damageTypes = DAMAGE_TYPE_FLAGS_ALL;
	m_conditionState = MODELCONDITION_INVALID;
	m_revertEarlyWhenDepleted = TRUE;
	m_runOutLogicOnlyWhenDepleted = FALSE;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
/*static*/ void ShieldGeneratorUpdateV2ModuleData::buildFieldParse(MultiIniFieldParse& p)
{
	UpdateModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "SpecialPowerTemplate",					INI::parseSpecialPowerTemplate,	nullptr, offsetof( ShieldGeneratorUpdateV2ModuleData, m_specialPowerTemplate ) },
		{ "Lifetime",												INI::parseDurationUnsignedInt,		nullptr, offsetof( ShieldGeneratorUpdateV2ModuleData, m_lifetimeFrames ) },
		{ "WeaponIn",												INI::parseWeaponTemplate,				nullptr, offsetof( ShieldGeneratorUpdateV2ModuleData, m_weaponInTemplate ) },
		{ "WeaponOut",											INI::parseWeaponTemplate,				nullptr, offsetof( ShieldGeneratorUpdateV2ModuleData, m_weaponOutTemplate ) },
		{ "OCLIn",													INI::parseObjectCreationList,		nullptr, offsetof( ShieldGeneratorUpdateV2ModuleData, m_oclIn ) },
		{ "OCLOut",													INI::parseObjectCreationList,		nullptr, offsetof( ShieldGeneratorUpdateV2ModuleData, m_oclOut ) },
		{ "FXListIn",												INI::parseFXList,								nullptr, offsetof( ShieldGeneratorUpdateV2ModuleData, m_fxListIn ) },
		{ "FXListOut",											INI::parseFXList,								nullptr, offsetof( ShieldGeneratorUpdateV2ModuleData, m_fxListOut ) },
		{ "ShieldAmount",										INI::parseReal,									nullptr, offsetof( ShieldGeneratorUpdateV2ModuleData, m_shieldAmount ) },
		{ "ShieldAmount%",									INI::parsePercentToReal,				nullptr, offsetof( ShieldGeneratorUpdateV2ModuleData, m_shieldAmountPercent ) },
		{ "DamageTypes",										INI::parseDamageTypeFlags,			nullptr, offsetof( ShieldGeneratorUpdateV2ModuleData, m_damageTypes ) },
		{ "ConditionStateType",							INI::parseIndexList,	ModelConditionFlags::getBitNames(), offsetof( ShieldGeneratorUpdateV2ModuleData, m_conditionState ) },
		{ "RevertEarlyWhenDepleted",				INI::parseBool,									nullptr, offsetof( ShieldGeneratorUpdateV2ModuleData, m_revertEarlyWhenDepleted ) },
		{ "RunOutLogicOnlyWhenDepleted",		INI::parseBool,									nullptr, offsetof( ShieldGeneratorUpdateV2ModuleData, m_runOutLogicOnlyWhenDepleted ) },
		{ nullptr, nullptr, nullptr, 0 }
	};
	p.add(dataFieldParse);
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
ShieldGeneratorUpdateV2::ShieldGeneratorUpdateV2( Thing *thing, const ModuleData *moduleData ) : SpecialPowerUpdateModule( thing, moduleData )
{
	m_isActive = FALSE;
	m_revertFrame = 0;
	m_shieldPool = 0.0f;

	const ShieldGeneratorUpdateV2ModuleData *data = getShieldGeneratorUpdateV2ModuleData();

	m_weaponIn = nullptr;
	if( data->m_weaponInTemplate )
	{
		m_weaponIn = TheWeaponStore->allocateNewWeapon( data->m_weaponInTemplate, PRIMARY_WEAPON );
		m_weaponIn->loadAmmoNow( getObject() );
	}

	m_weaponOut = nullptr;
	if( data->m_weaponOutTemplate )
	{
		m_weaponOut = TheWeaponStore->allocateNewWeapon( data->m_weaponOutTemplate, PRIMARY_WEAPON );
		m_weaponOut->loadAmmoNow( getObject() );
	}
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
ShieldGeneratorUpdateV2::~ShieldGeneratorUpdateV2()
{
	deleteInstance( m_weaponIn );
	deleteInstance( m_weaponOut );
}

// ------------------------------------------------------------------------------------------------
/** Looks up this object's Body module and returns it as a ShieldedBody* if (and only if) that's
	* genuinely what it is -- see ShieldedBody.h for why this lookup goes through getBodyModule() +
	* getModuleNameKey() rather than Object::findModule() (which is protected and not callable from
	* here). Returns nullptr for any object whose Body is plain ActiveBody or another subclass --
	* that's an expected, harmless case, not an error: ShieldedBody is opt-in per unit template. */
// ------------------------------------------------------------------------------------------------
ShieldedBody* ShieldGeneratorUpdateV2::findShieldedBody() const
{
	BodyModuleInterface *bodyInterface = getObject()->getBodyModule();
	if( !bodyInterface )
		return nullptr;

	// BodyModuleInterface is only ever implemented by BodyModule in this engine, and ActiveBody /
	// ShieldedBody sit below it in a single, non-virtual inheritance chain (BodyModule -> ActiveBody
	// -> ShieldedBody), so these static_casts are well-defined once the name key confirms the
	// concrete type -- the same no-RTTI, cast-after-name-match convention findUpdateModule() /
	// findDamageModule() use internally.
	BodyModule *bodyModule = static_cast<BodyModule*>( bodyInterface );
	if( bodyModule->getModuleNameKey() != NAMEKEY( "ShieldedBody" ) )
		return nullptr;

	return static_cast<ShieldedBody*>( bodyModule );
}

// ------------------------------------------------------------------------------------------------
/** Grants the absorption pool (ShieldAmount + ShieldAmount% of current max health), sets
	* ConditionStateType, fires WeaponIn/OCLIn/FXListIn once, and schedules the Lifetime revert (if
	* configured). Real max health is never touched -- see the file header for why. If our Body is a
	* ShieldedBody, also freezes its reported damage state for the duration (defense-in-depth; see
	* ShieldedBody.h). */
// ------------------------------------------------------------------------------------------------
void ShieldGeneratorUpdateV2::enterShield()
{
	const ShieldGeneratorUpdateV2ModuleData *data = getShieldGeneratorUpdateV2ModuleData();
	Object *obj = getObject();
	BodyModuleInterface *body = obj->getBodyModule();
	if( !body )
		return;

	Real currentMax = body->getMaxHealth();
	Real pool = data->m_shieldAmount + data->m_shieldAmountPercent * currentMax;
	if( pool <= 0.0f )
		return;	// nothing configured -- don't bother toggling condition/timers/FX for a no-op shield

	m_shieldPool = pool;

	ShieldedBody *shielded = findShieldedBody();
	if( shielded )
		shielded->freezeDamageState();

	if( data->m_conditionState != MODELCONDITION_INVALID )
		obj->setModelConditionState( data->m_conditionState );

	if( m_weaponIn && m_weaponIn->getStatus() == READY_TO_FIRE )
		m_weaponIn->forceFireWeapon( obj, obj->getPosition() );

	if( data->m_oclIn )
		ObjectCreationList::create( data->m_oclIn, obj, nullptr );

	if( data->m_fxListIn )
		FXList::doFXObj( data->m_fxListIn, obj );

	m_isActive = TRUE;

	if( data->m_lifetimeFrames > 0 )
	{
		m_revertFrame = TheGameLogic->getFrame() + data->m_lifetimeFrames;
		setWakeFrame( obj, frameToSleepTime( m_revertFrame ) );
	}
	else
	{
		m_revertFrame = 0;
	}
}

// ------------------------------------------------------------------------------------------------
/** Drops the remaining pool, clears ConditionStateType, unfreezes the damage state (if our Body is
	* a ShieldedBody), and -- unless RunOutLogicOnlyWhenDepleted says this particular revert should be
	* silent -- fires WeaponOut/OCLOut/FXListOut once. No health/max-health restoration happens here
	* at all: under the pool model, real health was never mutated by enterShield() in the first place,
	* so there is nothing to restore. Safe to call when not currently active (no-op).
	*
	* wasDepleted should be TRUE only when this call is happening because the pool hit zero (from
	* onDamage()), and FALSE for every other reason (Lifetime expiry, or a refresh recast) -- this is
	* what RunOutLogicOnlyWhenDepleted filters on. */
// ------------------------------------------------------------------------------------------------
void ShieldGeneratorUpdateV2::revertShield( Bool wasDepleted )
{
	if( !m_isActive )
		return;

	const ShieldGeneratorUpdateV2ModuleData *data = getShieldGeneratorUpdateV2ModuleData();
	Object *obj = getObject();

	ShieldedBody *shielded = findShieldedBody();
	if( shielded )
		shielded->unfreezeDamageState();

	if( data->m_conditionState != MODELCONDITION_INVALID )
		obj->clearModelConditionFlags( MAKE_MODELCONDITION_MASK( data->m_conditionState ) );

	// RunOutLogicOnlyWhenDepleted only means something when RevertEarlyWhenDepleted is set -- without
	// an early-depletion revert path to restrict to, honoring it here would mean the out logic could
	// NEVER run (every revert would look like "not the depleted one"), which is exactly the trap the
	// user flagged when asking for this field. So it's only allowed to suppress anything when both
	// RevertEarlyWhenDepleted is on AND this particular revert isn't the depleted one.
	Bool suppressOutLogic = data->m_revertEarlyWhenDepleted && data->m_runOutLogicOnlyWhenDepleted && !wasDepleted;
	if( !suppressOutLogic )
	{
		if( m_weaponOut && m_weaponOut->getStatus() == READY_TO_FIRE )
			m_weaponOut->forceFireWeapon( obj, obj->getPosition() );

		if( data->m_oclOut )
			ObjectCreationList::create( data->m_oclOut, obj, nullptr );

		if( data->m_fxListOut )
			FXList::doFXObj( data->m_fxListOut, obj );
	}

	m_isActive = FALSE;
	m_revertFrame = 0;
	m_shieldPool = 0.0f;
}

// ------------------------------------------------------------------------------------------------
/** Called by our companion ShieldGeneratorActivateV2 module (via the base SpecialPowerModule's
	* doSpecialPower -> initiateIntentToDoSpecialPower hand-off) whenever the button is pressed.
	* Re-pressing while already shielded is a refresh, not a stack: revertShield(FALSE) first (this is
	* neither a Lifetime expiry nor a depletion, so out logic follows RunOutLogicOnlyWhenDepleted's
	* normal rule of firing), then enterShield() grants a brand new pool. */
// ------------------------------------------------------------------------------------------------
Bool ShieldGeneratorUpdateV2::initiateIntentToDoSpecialPower( const SpecialPowerTemplate *specialPowerTemplate,
																															 const Object * /*targetObj*/,
																															 const Coord3D * /*targetPos*/,
																															 const Waypoint * /*way*/,
																															 UnsignedInt /*commandOptions*/ )
{
	const ShieldGeneratorUpdateV2ModuleData *data = getShieldGeneratorUpdateV2ModuleData();

	// Make sure this call is actually meant for us -- an object can have more than one special power.
	if( specialPowerTemplate != data->m_specialPowerTemplate )
		return FALSE;

	if( m_isActive )
		revertShield( FALSE );

	enterShield();

	return TRUE;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
UpdateSleepTime ShieldGeneratorUpdateV2::update()
{
	if( m_revertFrame != 0 )
	{
		if( TheGameLogic->getFrame() >= m_revertFrame )
		{
			revertShield( FALSE );	// time's up -- this is a Lifetime expiry, not a depletion
			return UPDATE_SLEEP_FOREVER;
		}
		return frameToSleepTime( m_revertFrame );
	}
	return UPDATE_SLEEP_FOREVER;
}

// ------------------------------------------------------------------------------------------------
/** Damage has been dealt. If we're currently shielded, this damage type is one we shield against
	* (DamageTypes), and the pool still has anything left, refund whatever portion of this hit the
	* pool can cover straight back to real health -- immediately, via
	* BodyModuleInterface::internalChangeHealth(), which bypasses armor/DamageFX/callbacks, since this
	* is a correction to a hit that's already been applied, not a second hit. Only the overflow beyond
	* the pool (or any damage of a type this shield doesn't cover at all) ever survives as real,
	* permanent damage. If the pool hits zero and RevertEarlyWhenDepleted is set, revert immediately
	* instead of waiting for Lifetime to expire. */
// ------------------------------------------------------------------------------------------------
void ShieldGeneratorUpdateV2::onDamage( DamageInfo *damageInfo )
{
	if( !m_isActive || m_shieldPool <= 0.0f || !damageInfo )
		return;

	const ShieldGeneratorUpdateV2ModuleData *data = getShieldGeneratorUpdateV2ModuleData();

	if( !getDamageTypeFlag( data->m_damageTypes, damageInfo->in.m_damageType ) )
		return;	// this damage type isn't covered by the shield -- let it stand as real, permanent damage

	// m_actualDamageClipped is exactly how much this hit actually subtracted from real health (unlike
	// m_actualDamageDealt, which can be larger if the hit would have overkilled) -- refund as much of
	// that as the remaining pool can cover.
	Real refund = damageInfo->out.m_actualDamageClipped;
	if( refund > m_shieldPool )
		refund = m_shieldPool;

	if( refund > 0.0f )
	{
		BodyModuleInterface *body = getObject()->getBodyModule();
		if( body )
			body->internalChangeHealth( refund );

		m_shieldPool -= refund;
	}

	if( m_shieldPool <= 0.0f && data->m_revertEarlyWhenDepleted )
		revertShield( TRUE );	// pool used up -- this IS the depleted case
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void ShieldGeneratorUpdateV2::crc( Xfer *xfer )
{
	// extend base class
	SpecialPowerUpdateModule::crc( xfer );
}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version
	* 2: added m_preShieldCurrentHealth (fix: revertShield() re-capping current health needed it)
	* 3: GeneralsMod redesign -- this module no longer mutates real max health at all (see the file
	*    header for why); m_preShieldMaxHealth / m_preShieldCurrentHealth are gone, replaced by a
	*    single absorption-pool value, m_shieldPool. This intentionally does NOT preserve compatibility
	*    with saves made under versions 1-2 -- acceptable here since this is actively-iterating,
	*    mod-local content with no released saves to support. */
// ------------------------------------------------------------------------------------------------
void ShieldGeneratorUpdateV2::xfer( Xfer *xfer )
{
	// version
	XferVersion currentVersion = 3;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	SpecialPowerUpdateModule::xfer( xfer );

	xfer->xferBool( &m_isActive );
	xfer->xferUnsignedInt( &m_revertFrame );
	xfer->xferReal( &m_shieldPool );

	xfer->xferSnapshot( m_weaponIn );
	xfer->xferSnapshot( m_weaponOut );
}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void ShieldGeneratorUpdateV2::loadPostProcess()
{
	// extend base class
	SpecialPowerUpdateModule::loadPostProcess();
}
