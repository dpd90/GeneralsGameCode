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

// FILE: DamageOverTimeUpdateV2.cpp /////////////////////////////////////////////////////////////////
// Desc:   See DamageOverTimeUpdateV2.h.
///////////////////////////////////////////////////////////////////////////////////////////////////

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/INI.h"
#include "Common/Xfer.h"

#include "GameClient/FXList.h"

#include "GameLogic/Damage.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"
#include "GameLogic/ObjectCreationList.h"
#include "GameLogic/Weapon.h"
#include "GameLogic/WeaponStatus.h"

#include "Common/ThingTemplate.h"
#include "GameLogic/Module/DamageOverTimeUpdateV2.h"

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
DamageOverTimeUpdateV2ModuleData::DamageOverTimeUpdateV2ModuleData()
{
	m_lifetimeFrames = 0;
	m_rateFrames = 0;
	m_amount = 0.0f;
	m_amountPercent = 0.0f;
	m_fireInitially = FALSE;
	m_weaponTemplate = nullptr;
	m_ocl = nullptr;
	m_fxList = nullptr;
	// default trigger AND default dealt type are both channel 1 of 8 (OVERTIME1..OVERTIME8) -- see
	// GameLogic/Damage.h. Give each separately-configured instance on an object a different single
	// TriggerDamageTypes channel (and ModuleTag) so they coexist without refreshing/clobbering each
	// other; the two fields are independent, so a modder can widen either one on its own (e.g. trigger
	// from one channel but deal a mix of real types, or vice versa).
	m_triggerDamageTypes = setDamageTypeFlag( DAMAGE_TYPE_FLAGS_NONE, DAMAGE_OVERTIME1 );
	m_dealDamageTypes = setDamageTypeFlag( DAMAGE_TYPE_FLAGS_NONE, DAMAGE_OVERTIME1 );
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
/*static*/ void DamageOverTimeUpdateV2ModuleData::buildFieldParse(MultiIniFieldParse& p)
{
	UpdateModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "Lifetime",						INI::parseDurationUnsignedInt,	nullptr, offsetof( DamageOverTimeUpdateV2ModuleData, m_lifetimeFrames ) },
		{ "Rate",								INI::parseDurationUnsignedInt,	nullptr, offsetof( DamageOverTimeUpdateV2ModuleData, m_rateFrames ) },
		{ "Amount",							INI::parseReal,									nullptr, offsetof( DamageOverTimeUpdateV2ModuleData, m_amount ) },
		{ "Amount%",						INI::parsePercentToReal,				nullptr, offsetof( DamageOverTimeUpdateV2ModuleData, m_amountPercent ) },
		{ "FireInitially",			INI::parseBool,									nullptr, offsetof( DamageOverTimeUpdateV2ModuleData, m_fireInitially ) },
		{ "Weapon",							INI::parseWeaponTemplate,			nullptr, offsetof( DamageOverTimeUpdateV2ModuleData, m_weaponTemplate ) },
		{ "OCL",								INI::parseObjectCreationList,	nullptr, offsetof( DamageOverTimeUpdateV2ModuleData, m_ocl ) },
		{ "FXList",							INI::parseFXList,								nullptr, offsetof( DamageOverTimeUpdateV2ModuleData, m_fxList ) },
		{ "TriggerDamageTypes",	INI::parseDamageTypeFlags,			nullptr, offsetof( DamageOverTimeUpdateV2ModuleData, m_triggerDamageTypes ) },
		{ "DealDamageTypes",		INI::parseDamageTypeFlags,			nullptr, offsetof( DamageOverTimeUpdateV2ModuleData, m_dealDamageTypes ) },
		{ "RequiredKindOf",			KindOfMaskType::parseFromINI,	nullptr, offsetof( DamageOverTimeUpdateV2ModuleData, m_requiredKindOf ) },
		{ "ForbiddenKindOf",		KindOfMaskType::parseFromINI,	nullptr, offsetof( DamageOverTimeUpdateV2ModuleData, m_forbiddenKindOf ) },
		{ nullptr, nullptr, nullptr, 0 }
	};
	p.add(dataFieldParse);
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
DamageOverTimeUpdateV2::DamageOverTimeUpdateV2( Thing *thing, const ModuleData *moduleData ) : UpdateModule( thing, moduleData )
{
	m_tickFrame = 0;
	m_stopFrame = 0;
	m_tickAmount = 0.0f;
	m_sourceID = INVALID_ID;
	m_deathType = DEATH_NORMAL;
	m_dealingTick = FALSE;

	const DamageOverTimeUpdateV2ModuleData *data = getDamageOverTimeUpdateV2ModuleData();

	// Skip the Weapon allocation entirely if this instance can never trigger on THIS object -- safe to check
	// here because our KindOf is a static property of our template, already resolved by Thing(tt) at the top
	// of Object's own constructor, well before any behavior module (including us) is constructed. OCL/FXList
	// need no equivalent guard -- both are just raw const pointers into globally-owned template registries
	// (assigned once at INI parse time), never allocated or copied per module instance, so there is nothing
	// to skip for them. Meant for setups using up to 24 KindOf-gated instances per object (3 KindOf variants
	// x 8 OVERTIME channels) on DefaultThingTemplate, where any single object can match at most a handful of
	// them -- this avoids allocating/loading ammo for a Weapon on the ~2/3 of instances that are permanently
	// dead on that particular object.
	m_weapon = nullptr;
	if( data->m_weaponTemplate && getObject()->isKindOfMulti( data->m_requiredKindOf, data->m_forbiddenKindOf ) )
	{
		m_weapon = TheWeaponStore->allocateNewWeapon( data->m_weaponTemplate, PRIMARY_WEAPON );
		m_weapon->loadAmmoNow( getObject() );
	}

	// inert until triggered -- do NOT tick every frame just for sitting on an object that's never
	// been hit by a qualifying DamageType (this is what makes it cheap to put on DefaultThingTemplate)
	setWakeFrame( getObject(), UPDATE_SLEEP_FOREVER );
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
DamageOverTimeUpdateV2::~DamageOverTimeUpdateV2()
{
	deleteInstance( m_weapon );
}

// ------------------------------------------------------------------------------------------------
/** Damage has been dealt -- if it's a DamageType we react to, start (or refresh) the effect. */
// ------------------------------------------------------------------------------------------------
void DamageOverTimeUpdateV2::onDamage( DamageInfo *damageInfo )
{

	if( !damageInfo || m_dealingTick )
	{
		return;	// m_dealingTick: this is our own tick's damage arriving back through the normal damage
							// pipeline (ticks are dealt using type(s) drawn straight from our own DealDamageTypes
							// field, so they can pass through ArmorSet -- see the header comment); ignore it, in
							// case a dealt type is also one we watch for in TriggerDamageTypes, or every tick
							// (including a FireInitially immediate one) would refresh us
	}

	const DamageOverTimeUpdateV2ModuleData *d = getDamageOverTimeUpdateV2ModuleData();

	// KindOf gate -- lets DefaultThingTemplate carry this module (via InheritableModule) while still
	// excluding whole categories of unit (e.g. ForbiddenKindOf = INFANTRY) without a RemoveModule on
	// every excluded object. isKindOfMulti(mustBeSet, mustBeClear): an empty mask on either side is a
	// no-op requirement (testForAll/testForNone against zero bits is vacuously true).
	if( !getObject()->isKindOfMulti( d->m_requiredKindOf, d->m_forbiddenKindOf ) )
	{
		return;
	}

	Bool triggerMatch = getDamageTypeFlag( d->m_triggerDamageTypes, damageInfo->in.m_damageType );

	if( triggerMatch )
		startOrRefresh( damageInfo );
}

// ------------------------------------------------------------------------------------------------
/** Being healed cancels the effect outright (same call as PoisonedBehavior::onHealing()). */
// ------------------------------------------------------------------------------------------------
void DamageOverTimeUpdateV2::onHealing( DamageInfo *damageInfo )
{
	stopEffect();
	setWakeFrame( getObject(), UPDATE_SLEEP_FOREVER );
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
UpdateSleepTime DamageOverTimeUpdateV2::update()
{
	if( m_stopFrame == 0 )
	{
		// not currently active -- shouldn't normally be woken in this state, but tolerate it quietly
		return UPDATE_SLEEP_FOREVER;
	}

	const DamageOverTimeUpdateV2ModuleData *d = getDamageOverTimeUpdateV2ModuleData();
	UnsignedInt now = TheGameLogic->getFrame();

	if( m_tickFrame != 0 && now >= m_tickFrame )
	{
		// deal the tick damage once per DamageType flagged in DealDamageTypes -- each one goes through
		// the target's own ArmorSet coefficient for that type independently, same as a genuine hit of
		// that type would (this is what lets DealDamageTypes = NONE +ARMOR_PIERCING +FLAME deal a mixed
		// effect: full Amount as BOTH types, every tick, not split between them). m_dealingTick guards
		// the whole loop against re-triggering/refreshing ourselves, in case a dealt type is also
		// flagged in TriggerDamageTypes.
		dealTickDamage();
		fireTickEffects();

		m_tickFrame = now + d->m_rateFrames;
	}

	if( m_stopFrame != 0 && now >= m_stopFrame )
	{
		stopEffect();
		return UPDATE_SLEEP_FOREVER;
	}

	return calcSleepTime();
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
UpdateSleepTime DamageOverTimeUpdateV2::calcSleepTime()
{
	// UPDATE_SLEEP requires a count-of-frames, not an absolute-frame -- frameToSleepTime handles that
	UnsignedInt now = TheGameLogic->getFrame();
	if( m_stopFrame == 0 || m_stopFrame == now )
		return UPDATE_SLEEP_FOREVER;
	UpdateSleepTime sleep = frameToSleepTime( m_tickFrame, m_stopFrame );
	return sleep;
}

// ------------------------------------------------------------------------------------------------
/** Starts the effect fresh, or refreshes an already-active one from a new qualifying hit. Amount/
	* Amount% are recaptured from THIS hit (not additive with any previous one -- a refresh, not a
	* stack), and Lifetime restarts from now. If we're already ticking, the next scheduled tick is
	* preserved rather than pushed out (same as PoisonedBehavior::startPoisonedEffects()) -- getting
	* re-hit doesn't delay the damage you were already about to take.
	*
	* If FireInitially is set, this ALSO deals one tick's worth of damage/Weapon/OCL/FXList right now,
	* on every qualifying hit (fresh start or refresh alike) -- on top of, not instead of, the normal
	* Rate-cadence tick already scheduled above. Worth knowing: we're called from onDamage(), which is
	* itself called synchronously from inside the TRIGGERING hit's own ActiveBody::attemptDamage() call
	* (from its module-notify loop) -- so dealTickDamage()'s own getObject()->attemptDamage() call here
	* is a nested, reentrant call into attemptDamage() while that outer call is still on the stack, not
	* a fresh top-level one the way a normal scheduled tick gets from update(). This is safe (nothing in
	* ActiveBody::attemptDamage() is static/non-reentrant, and the object's own behavior-module list is
	* fixed for the duration of combat, so the outer call's own in-progress module-notify loop resumes
	* correctly once our nested call returns) -- it just means any sibling module later in the object's
	* module list is notified of OUR immediate tick's hit before it is notified of the original
	* triggering hit that caused it, since our nested call's full notify pass completes first. */
// ------------------------------------------------------------------------------------------------
void DamageOverTimeUpdateV2::startOrRefresh( const DamageInfo *damageInfo )
{
	const DamageOverTimeUpdateV2ModuleData *d = getDamageOverTimeUpdateV2ModuleData();
	UnsignedInt now = TheGameLogic->getFrame();

	m_tickAmount = d->m_amount + d->m_amountPercent * damageInfo->out.m_actualDamageDealt;
	m_sourceID = damageInfo->in.m_sourceID;
	m_deathType = damageInfo->in.m_deathType;
	m_stopFrame = now + d->m_lifetimeFrames;

	if( m_tickFrame != 0 )
		m_tickFrame = min( m_tickFrame, now + d->m_rateFrames );
	else
		m_tickFrame = now + d->m_rateFrames;

	if( d->m_fireInitially )
	{
		dealTickDamage();
		fireTickEffects();
	}

	setWakeFrame( getObject(), calcSleepTime() );
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void DamageOverTimeUpdateV2::stopEffect()
{
	m_tickFrame = 0;
	m_stopFrame = 0;
	m_tickAmount = 0.0f;
	m_sourceID = INVALID_ID;
}

// ------------------------------------------------------------------------------------------------
/** Deals one tick's worth of damage -- once per DamageType flagged in DealDamageTypes, each getting
	* the FULL m_tickAmount independently (not split across them), so each passes through the target's
	* own ArmorSet coefficient for that type on its own -- same as a genuine hit of that type would (see
	* the header comment for why this is what makes DealDamageTypes = NONE +ARMOR_PIERCING +FLAME deal
	* a genuinely mixed effect). Shared by update()'s normal Rate-cadence tick and startOrRefresh()'s
	* FireInitially immediate tick. m_dealingTick guards the whole loop against re-triggering/
	* refreshing ourselves, in case a dealt type is also flagged in TriggerDamageTypes. */
// ------------------------------------------------------------------------------------------------
void DamageOverTimeUpdateV2::dealTickDamage()
{
	const DamageOverTimeUpdateV2ModuleData *d = getDamageOverTimeUpdateV2ModuleData();

	m_dealingTick = TRUE;
	for( Int i = 0; i < DAMAGE_NUM_TYPES; ++i )
	{
		DamageType dt = (DamageType)i;
		if( !getDamageTypeFlag( d->m_dealDamageTypes, dt ) )
			continue;

		DamageInfo damage;
		damage.in.m_amount = m_tickAmount;
		damage.in.m_sourceID = m_sourceID;
		damage.in.m_damageType = dt;
		damage.in.m_deathType = m_deathType;
		getObject()->attemptDamage( &damage );
	}
	m_dealingTick = FALSE;
}

// ------------------------------------------------------------------------------------------------
/** Fires Weapon/OCL/FXList at our own position -- called once per tick from update() (and, when
	* FireInitially is set, once more from startOrRefresh()), same "fire at self" idiom
	* ShieldGeneratorUpdateV2's WeaponIn/OCLIn/FXListIn use, just repeated every tick. */
// ------------------------------------------------------------------------------------------------
void DamageOverTimeUpdateV2::fireTickEffects()
{
	const DamageOverTimeUpdateV2ModuleData *d = getDamageOverTimeUpdateV2ModuleData();
	Object *obj = getObject();

	if( m_weapon && m_weapon->getStatus() == READY_TO_FIRE )
		m_weapon->forceFireWeapon( obj, obj->getPosition() );

	if( d->m_ocl )
		ObjectCreationList::create( d->m_ocl, obj, nullptr );

	if( d->m_fxList )
		FXList::doFXObj( d->m_fxList, obj );
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void DamageOverTimeUpdateV2::crc( Xfer *xfer )
{
	// extend base class
	UpdateModule::crc( xfer );
}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version
	*/
// ------------------------------------------------------------------------------------------------
void DamageOverTimeUpdateV2::xfer( Xfer *xfer )
{
	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpdateModule::xfer( xfer );

	xfer->xferUnsignedInt( &m_tickFrame );
	xfer->xferUnsignedInt( &m_stopFrame );
	xfer->xferReal( &m_tickAmount );
	xfer->xferObjectID( &m_sourceID );
	xfer->xferUser( &m_deathType, sizeof(m_deathType) );

	// m_dealingTick is transient (only ever TRUE for the duration of the tick-dealing loop inside
	// dealTickDamage()) -- never persisted, always FALSE at rest

	xfer->xferSnapshot( m_weapon );
}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void DamageOverTimeUpdateV2::loadPostProcess()
{
	// extend base class
	UpdateModule::loadPostProcess();
}
