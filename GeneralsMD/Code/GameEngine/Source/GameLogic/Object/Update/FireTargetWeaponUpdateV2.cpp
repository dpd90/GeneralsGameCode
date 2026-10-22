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

// FILE: FireTargetWeaponUpdateV2.cpp /////////////////////////////////////////////////////////////
// Desc:   GeneralsMod @feature Dimitar 07/10/2026 -- see FireTargetWeaponUpdateV2.h for the INI fields.
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#define DEFINE_WEAPONSLOTTYPE_NAMES		// for TheWeaponSlotTypeNamesLookupList (WeaponSlot field)

#include "Common/BitFlagsIO.h"
#include "Common/GlobalData.h"
#include "Common/ModelState.h"
#include "Common/RandomValue.h"
#include "Common/ThingTemplate.h"
#include "Common/Xfer.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/Object.h"
#include "GameLogic/ObjectIter.h"
#include "GameLogic/Module/BodyModule.h"
#include "GameLogic/Module/ContainModule.h"
#include "GameLogic/Module/FireTargetWeaponUpdateV2.h"
#include "GameLogic/Module/PhysicsUpdate.h"
#include "GameLogic/Module/SpecialPowerUpdateModule.h"
#include "GameLogic/Weapon.h"
#include "GameLogic/WeaponSet.h"

//-------------------------------------------------------------------------------------------------
// TargetPriority names -- order must match FireTargetPriorityType.
//-------------------------------------------------------------------------------------------------
static const LookupListRec TheFireTargetPriorityLookupList[] =
{
	{ "NEAREST",								FTW_PRIORITY_NEAREST },
	{ "FARTHEST",								FTW_PRIORITY_FARTHEST },
	{ "LOWEST_HEALTH",					FTW_PRIORITY_LOWEST_HEALTH },
	{ "HIGHEST_HEALTH",					FTW_PRIORITY_HIGHEST_HEALTH },
	{ "LOWEST_HEALTH_PERCENT",	FTW_PRIORITY_LOWEST_HEALTH_PERCENT },
	{ "HIGHEST_HEALTH_PERCENT",	FTW_PRIORITY_HIGHEST_HEALTH_PERCENT },
	{ "MOST_EXPENSIVE",					FTW_PRIORITY_MOST_EXPENSIVE },
	{ "RANDOM",									FTW_PRIORITY_RANDOM },

	{ nullptr, 0 }
};
static_assert(ARRAY_SIZE(TheFireTargetPriorityLookupList) == FTW_PRIORITY_COUNT + 1, "Incorrect array size");

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
FireTargetWeaponUpdateV2ModuleData::FireTargetWeaponUpdateV2ModuleData()
{
	m_weaponTemplate						= nullptr;
	m_triggerWeaponSlot					= FTWU2_NO_SLOT;	// self-targeting mode
	m_weaponSlot								= FTWU2_NO_SLOT;	// trigger slot in companion mode, else PRIMARY
	m_scanFrames								= 0;
	m_idleScanFrames						= 0;			// never idle-sleep unless a modder opts in
	m_scanRange									= 0.0f;
	m_velocityFactor						= 0.0f;
	m_maxTrackedTargets					= 1;
	m_minimumTargetRange				= 0.0f;
	m_targetPriority						= FTW_PRIORITY_NEAREST;
	m_shotsPerVolley						= 1;
	m_minTargetSeparationAngle	= 0.0f;		// no spread
	m_volleyShotDelayFrames			= 0;			// whole volley on one frame
	m_volleysBeforeCooldown			= 0;			// never cool down
	m_cooldownFrames						= 0;
	m_volleyCounterResetFrames	= 0;
	m_activeConditionState			= MODELCONDITION_INVALID;
	m_cooldownConditionState		= MODELCONDITION_INVALID;
	m_switchStateTemplate				= nullptr;
	m_alteredStateWhenOnCooldown	= FALSE;
}

//-------------------------------------------------------------------------------------------------
/*static*/ void FireTargetWeaponUpdateV2ModuleData::buildFieldParse(MultiIniFieldParse& p)
{
	ModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "WeaponTemplate",						INI::parseWeaponTemplate,				nullptr, offsetof( FireTargetWeaponUpdateV2ModuleData, m_weaponTemplate ) },
		{ "TriggerWeaponSlot",				INI::parseLookupList,						TheWeaponSlotTypeNamesLookupList, offsetof( FireTargetWeaponUpdateV2ModuleData, m_triggerWeaponSlot ) },
		{ "WeaponSlot",								INI::parseLookupList,						TheWeaponSlotTypeNamesLookupList, offsetof( FireTargetWeaponUpdateV2ModuleData, m_weaponSlot ) },
		{ "PrimaryTargetTypes",				KindOfMaskType::parseFromINI,		nullptr, offsetof( FireTargetWeaponUpdateV2ModuleData, m_primaryTargetKindOf ) },
		{ "SecondaryTargetTypes",			KindOfMaskType::parseFromINI,		nullptr, offsetof( FireTargetWeaponUpdateV2ModuleData, m_secondaryTargetKindOf ) },
		{ "TertiaryTargetTypes",			KindOfMaskType::parseFromINI,		nullptr, offsetof( FireTargetWeaponUpdateV2ModuleData, m_tertiaryTargetKindOf ) },
		{ "ScanRate",									INI::parseDurationUnsignedInt,	nullptr, offsetof( FireTargetWeaponUpdateV2ModuleData, m_scanFrames ) },
		{ "IdleScanRate",							INI::parseDurationUnsignedInt,	nullptr, offsetof( FireTargetWeaponUpdateV2ModuleData, m_idleScanFrames ) },
		{ "ScanRange",								INI::parseReal,									nullptr, offsetof( FireTargetWeaponUpdateV2ModuleData, m_scanRange ) },
		{ "PredictTargetVelocityFactor",	INI::parseReal,							nullptr, offsetof( FireTargetWeaponUpdateV2ModuleData, m_velocityFactor ) },
		{ "MaxTrackedTargets",				INI::parseUnsignedInt,					nullptr, offsetof( FireTargetWeaponUpdateV2ModuleData, m_maxTrackedTargets ) },
		{ "MinimumTargetRange",				INI::parseReal,									nullptr, offsetof( FireTargetWeaponUpdateV2ModuleData, m_minimumTargetRange ) },
		{ "TargetPriority",						INI::parseLookupList,						TheFireTargetPriorityLookupList, offsetof( FireTargetWeaponUpdateV2ModuleData, m_targetPriority ) },
		{ "ShotsPerVolley",						INI::parseUnsignedInt,					nullptr, offsetof( FireTargetWeaponUpdateV2ModuleData, m_shotsPerVolley ) },
		{ "MinTargetSeparationAngle",	INI::parseAngleReal,						nullptr, offsetof( FireTargetWeaponUpdateV2ModuleData, m_minTargetSeparationAngle ) },
		{ "VolleyShotDelay",					INI::parseDurationUnsignedInt,	nullptr, offsetof( FireTargetWeaponUpdateV2ModuleData, m_volleyShotDelayFrames ) },
		{ "VolleysBeforeCooldown",		INI::parseUnsignedInt,					nullptr, offsetof( FireTargetWeaponUpdateV2ModuleData, m_volleysBeforeCooldown ) },
		{ "CooldownDuration",					INI::parseDurationUnsignedInt,	nullptr, offsetof( FireTargetWeaponUpdateV2ModuleData, m_cooldownFrames ) },
		{ "VolleyCounterResetTime",		INI::parseDurationUnsignedInt,	nullptr, offsetof( FireTargetWeaponUpdateV2ModuleData, m_volleyCounterResetFrames ) },
		{ "ActiveConditionState",			INI::parseIndexList,	ModelConditionFlags::getBitNames(), offsetof( FireTargetWeaponUpdateV2ModuleData, m_activeConditionState ) },
		{ "CooldownConditionState",		INI::parseIndexList,	ModelConditionFlags::getBitNames(), offsetof( FireTargetWeaponUpdateV2ModuleData, m_cooldownConditionState ) },
		{ "SwitchStateTemplate",			INI::parseSpecialPowerTemplate,	nullptr, offsetof( FireTargetWeaponUpdateV2ModuleData, m_switchStateTemplate ) },
		{ "AlteredStateWhenOnCooldown",	INI::parseBool,								nullptr, offsetof( FireTargetWeaponUpdateV2ModuleData, m_alteredStateWhenOnCooldown ) },
		{ nullptr, nullptr, nullptr, 0 }
	};
	p.add(dataFieldParse);
}

//-------------------------------------------------------------------------------------------------
namespace
{
	//-------------------------------------------------------------------------------------------
	// Sort order for candidates: tier first (Primary < Secondary < Tertiary), then priority key,
	// then distance as the tie-breaker (so e.g. "lowest health" among equally healthy targets
	// picks the nearest one). Returns TRUE if A should come before B.
	//-------------------------------------------------------------------------------------------
	Bool isBetterCandidate( Int tierA, Real keyA, Real distA, Int tierB, Real keyB, Real distB )
	{
		if( tierA != tierB )
			return tierA < tierB;
		if( keyA != keyB )
			return keyA < keyB;
		return distA < distB;
	}

	//-------------------------------------------------------------------------------------------
	// Insert (id, key, dist) into a fixed-capacity list kept sorted best-first (single tier).
	// Same idea as PointDefenseUpdateV2's insertSortedCandidate, with a secondary sort key.
	//-------------------------------------------------------------------------------------------
	void insertSortedCandidate( ObjectID *ids, Real *keys, Real *dists, Int &count, Int cap, ObjectID id, Real key, Real dist )
	{
		if( cap <= 0 )
			return;

		Int i;
		if( count < cap )
		{
			i = count;
			++count;
		}
		else if( isBetterCandidate( 0, key, dist, 0, keys[cap - 1], dists[cap - 1] ) )
		{
			i = cap - 1;	// drop the current worst
		}
		else
			return;

		while( i > 0 && isBetterCandidate( 0, key, dist, 0, keys[i - 1], dists[i - 1] ) )
		{
			ids[i] = ids[i - 1];
			keys[i] = keys[i - 1];
			dists[i] = dists[i - 1];
			--i;
		}
		ids[i] = id;
		keys[i] = key;
		dists[i] = dist;
	}

	//-------------------------------------------------------------------------------------------
	// Same classification as WeaponSet::getVictimAntiMask() (file-local there) -- copied from
	// PointDefenseUpdateV2.cpp. We fire by hand instead of through WeaponSet, so the engine's Anti*
	// enforcement never runs for us; without this every Anti* field on the weapon would be ignored.
	//-------------------------------------------------------------------------------------------
	Int classifyForAntiMask( const Object *victim )
	{
		if( victim->isKindOf( KINDOF_SMALL_MISSILE ) )
			return WEAPON_ANTI_SMALL_MISSILE; //All missiles are also projectiles!
		else if( victim->isKindOf( KINDOF_BALLISTIC_MISSILE ) )
			return WEAPON_ANTI_BALLISTIC_MISSILE;
		else if( victim->isKindOf( KINDOF_PROJECTILE ) )
			return WEAPON_ANTI_PROJECTILE;
		else if( victim->isKindOf( KINDOF_MINE ) || victim->isKindOf( KINDOF_DEMOTRAP ) )
			return WEAPON_ANTI_MINE | WEAPON_ANTI_GROUND;
		else if( victim->isAirborneTarget() )
		{
			if( victim->isKindOf( KINDOF_VEHICLE ) )
				return WEAPON_ANTI_AIRBORNE_VEHICLE;
			else if( victim->isKindOf( KINDOF_INFANTRY ) )
				return WEAPON_ANTI_AIRBORNE_INFANTRY;
			else if( victim->isKindOf( KINDOF_PARACHUTE ) )
				return WEAPON_ANTI_PARACHUTE;
			return 0; //airborne, but not a category this weapon can ever be configured to hit
		}
		else
			return WEAPON_ANTI_GROUND;
	}
}

//-------------------------------------------------------------------------------------------------
FireTargetWeaponUpdateV2::FireTargetWeaponUpdateV2( Thing *thing, const ModuleData* moduleData ) : UpdateModule( thing, moduleData )
{
	for( Int i = 0; i < FTWU2_MAX_TRACKED_TARGETS; ++i )
	{
		m_trackedTargets[i] = INVALID_ID;
		m_trackedTier[i] = 0;
	}
	m_numTracked = 0;

	for( Int i = 0; i < FTWU2_MAX_SHOTS_PER_VOLLEY; ++i )
		m_volleyTargets[i] = INVALID_ID;
	m_volleyShotCount			= 0;
	m_volleyNextShot			= 0;
	m_volleyNextShotFrame	= 0;
	m_volleyCounted				= FALSE;
	m_nextVolleyFrame			= 0;
	m_volleyWeaponTemplate	= nullptr;

	m_volleysThisCycle		= 0;
	m_lastVolleyFrame			= 0;
	m_cooldownEndFrame		= 0;		// start active
	m_phaseVisualsApplied	= FALSE;

	const FireTargetWeaponUpdateV2ModuleData *data = getFireTargetWeaponUpdateV2ModuleData();
	if( data->isCompanionMode() )
	{
		// Companion mode: we're woken by the owner's own shots (BehaviorModule::onOwnerShotFired). Not
		// saved -- modules are re-constructed on load, so this re-registers then too.
		getObject()->registerShotFiredListener();
		m_nextScanFrames = 0;
	}
	else
	{
		// Stagger the first scan so units placed on the same frame don't scan in lock-step forever.
		// (Only rolled in self-targeting mode, so companion units don't consume logic RNG here.)
		m_nextScanFrames = ( data->m_scanFrames > 1 ) ? GameLogicRandomValue( 0, (Int)data->m_scanFrames - 1 ) : 0;
	}

	// First update applies the initial visuals; companion mode then sleeps until a shot wakes it.
	setWakeFrame( getObject(), UPDATE_SLEEP_NONE );
}

//-------------------------------------------------------------------------------------------------
FireTargetWeaponUpdateV2::~FireTargetWeaponUpdateV2()
{

}

//-------------------------------------------------------------------------------------------------
void FireTargetWeaponUpdateV2::onObjectCreated()
{
	const FireTargetWeaponUpdateV2ModuleData *data = getFireTargetWeaponUpdateV2ModuleData();
	const char *name = getObject()->getTemplate()->getName().str();

	if( data->isCompanionMode() )
	{
		if( data->m_shotsPerVolley < 2 )
		{
			DEBUG_CRASH( ("FireTargetWeaponUpdateV2 for %s is in companion mode (TriggerWeaponSlot) with ShotsPerVolley=%d -- it will never add extra shots (ShotsPerVolley includes the unit's own shot)",
				name, data->m_shotsPerVolley) );
		}
	}
	else
	{
		if( !data->m_weaponTemplate )
		{
			DEBUG_CRASH( ("FireTargetWeaponUpdateV2 for %s doesn't have a valid weapon template (required without TriggerWeaponSlot)", name) );
			return;
		}

		WeaponBonus bonus;
		bonus.clear();
		Real attackRange = data->m_weaponTemplate->getAttackRange( bonus );
		if( data->m_scanRange <= attackRange )
		{
			DEBUG_CRASH( ("FireTargetWeaponUpdateV2 for %s requires the scan range (%.1f) being larger than the firing range (%.1f)",
				name, data->m_scanRange, attackRange) );
		}
	}

	if( data->m_maxTrackedTargets > (UnsignedInt)FTWU2_MAX_TRACKED_TARGETS )
	{
		DEBUG_CRASH( ("FireTargetWeaponUpdateV2 for %s has MaxTrackedTargets=%d, clamping to %d",
			name, data->m_maxTrackedTargets, FTWU2_MAX_TRACKED_TARGETS) );
	}

	if( data->m_shotsPerVolley > (UnsignedInt)FTWU2_MAX_SHOTS_PER_VOLLEY )
	{
		DEBUG_CRASH( ("FireTargetWeaponUpdateV2 for %s has ShotsPerVolley=%d, clamping to %d",
			name, data->m_shotsPerVolley, FTWU2_MAX_SHOTS_PER_VOLLEY) );
	}

	if( data->m_volleysBeforeCooldown > 0 && data->m_cooldownFrames == 0 )
	{
		DEBUG_CRASH( ("FireTargetWeaponUpdateV2 for %s has VolleysBeforeCooldown=%d but no CooldownDuration -- it will never cool down",
			name, data->m_volleysBeforeCooldown) );
	}

	// NOTE: the initial visual / SwitchStateV2 sync is done on the first update(), not here -- the
	// Drawable isn't bound yet (same reason as PointDefenseUpdateV2).
}

//-------------------------------------------------------------------------------------------------
/** The update callback. */
//-------------------------------------------------------------------------------------------------
UpdateSleepTime FireTargetWeaponUpdateV2::update()
{
	Object *me = getObject();
	if( me->isEffectivelyDead() )
		return UPDATE_SLEEP_FOREVER;

	const FireTargetWeaponUpdateV2ModuleData *data = getFireTargetWeaponUpdateV2ModuleData();

	if( !m_phaseVisualsApplied )
	{
		applyPhaseVisuals( m_cooldownEndFrame != 0, FALSE );
		m_phaseVisualsApplied = TRUE;
	}

	// Cooldown: normally we sleep through it and only get here once it's over.
	if( m_cooldownEndFrame != 0 )
	{
		if( TheGameLogic->getFrame() < m_cooldownEndFrame )
			return frameToSleepTime( m_cooldownEndFrame );

		exitCooldown();
	}

	if( data->isCompanionMode() )
		return updateCompanion();

	return updateSelfTargeting();
}

//-------------------------------------------------------------------------------------------------
/** Companion mode: volleys are started by onOwnerShotFired(); update() only fires the delayed extras
	* of a running volley, then starts the cooldown if it's due, then sleeps until the next real shot. */
//-------------------------------------------------------------------------------------------------
UpdateSleepTime FireTargetWeaponUpdateV2::updateCompanion()
{
	if( isVolleyInProgress() )
	{
		if( isShooterBusy() || !m_volleyWeaponTemplate )
		{
			endVolley();
		}
		else
		{
			RangeInfo ri;
			computeRanges( ri, m_volleyWeaponTemplate );
			continueVolley( ri );
		}
	}

	if( isVolleyInProgress() )
		return frameToSleepTime( m_volleyNextShotFrame );

	if( isVolleyLimitReached() )
		return enterCooldown();

	return UPDATE_SLEEP_FOREVER;	// until the owner fires again
}

//-------------------------------------------------------------------------------------------------
/** Companion mode trigger: the owner just fired one of its own WeaponSet weapons. If it's
	* TriggerWeaponSlot's and the shot really went out, its shot is shot 1 of a volley -- arm the extras. */
//-------------------------------------------------------------------------------------------------
void FireTargetWeaponUpdateV2::onOwnerShotFired( const Weapon* weaponFired, ObjectID victimID )
{
	const FireTargetWeaponUpdateV2ModuleData *data = getFireTargetWeaponUpdateV2ModuleData();
	if( !data->isCompanionMode() || !weaponFired )
		return;

	if( (Int)weaponFired->getWeaponSlot() != data->m_triggerWeaponSlot )
		return;

	// The engine notifies even when the weapon silently refused to fire (e.g. MinimumAttackRange).
	UnsignedInt now = TheGameLogic->getFrame();
	if( weaponFired->getLastShotFrame() != now )
		return;

	Object *me = getObject();
	if( me->isEffectivelyDead() || isShooterBusy() )
		return;

	// Cooldown holds only the extras; the real shot already happened. A cooldown that has run out but
	// whose wake-up hasn't been processed yet ends right here.
	if( m_cooldownEndFrame != 0 )
	{
		if( now < m_cooldownEndFrame )
			return;
		exitCooldown();
	}

	// The previous volley's extras are still going out: this shot gets no extras and isn't counted.
	// (Documented: make the weapon's reload longer than (ShotsPerVolley - 1) x VolleyShotDelay.)
	if( isVolleyInProgress() )
		return;

	Int extras = getCap( data->m_shotsPerVolley, FTWU2_MAX_SHOTS_PER_VOLLEY ) - 1;
	if( extras <= 0 )
		return;

	// The extras fire the weapon that just fired (weapon-set upgrades follow), unless overridden.
	const WeaponTemplate *tmpl = data->m_weaponTemplate ? data->m_weaponTemplate : weaponFired->getTemplate();
	if( !tmpl )
		return;

	RangeInfo ri;
	computeRanges( ri, tmpl );

	// One scan per real shot -- nothing between shots. Weapon range at least, ScanRange if larger.
	Real scanRange = (Real)sqrtf( ri.fireRangeSq );
	if( data->m_scanRange > scanRange )
		scanRange = data->m_scanRange;
	scanForTargets( ri, tmpl, scanRange );

	// A ground shot (victimID == INVALID_ID) just has no primary target: extras go to enemies near us.
	if( !startVolley( ri, tmpl, victimID, extras, now + data->m_volleyShotDelayFrames ) )
		return;	// nobody to shoot at -- not a volley, not counted

	m_volleyCounted = TRUE;	// the real shot went out, so the volley counts now
	registerVolley();

	continueVolley( ri );	// VolleyShotDelay = 0: all extras right now

	if( isVolleyInProgress() )
		setWakeFrame( me, frameToSleepTime( m_volleyNextShotFrame ) );
	else if( isVolleyLimitReached() )
		setWakeFrame( me, enterCooldown() );
}

//-------------------------------------------------------------------------------------------------
/** Self-targeting mode: scan on our own schedule and fire our own WeaponTemplate. */
//-------------------------------------------------------------------------------------------------
UpdateSleepTime FireTargetWeaponUpdateV2::updateSelfTargeting()
{
	const FireTargetWeaponUpdateV2ModuleData *data = getFireTargetWeaponUpdateV2ModuleData();
	if( !data->m_weaponTemplate )
		return UPDATE_SLEEP_FOREVER;	// already reported in onObjectCreated()

	// Scaffolding / being sold: hold fire (a running volley is abandoned).
	if( isShooterBusy() )
	{
		endVolley();
		return UPDATE_SLEEP_NONE;
	}

	RangeInfo ri;
	computeRanges( ri, data->m_weaponTemplate );

	pruneTrackedTargets();

	// Periodic full scan (expensive); coast on the cached list in between.
	Bool foundAnything = TRUE;
	if( m_nextScanFrames > 0 )
	{
		m_nextScanFrames--;
	}
	else
	{
		foundAnything = scanForTargets( ri, data->m_weaponTemplate, data->m_scanRange );
		m_nextScanFrames = foundAnything ? (Int)data->m_scanFrames : 0;
	}

	// Finish the running volley first; a new one may start on the same frame the old one ends.
	if( isVolleyInProgress() )
		continueVolley( ri );

	if( !isVolleyInProgress() && TheGameLogic->getFrame() >= m_nextVolleyFrame )
	{
		Int shots = getCap( data->m_shotsPerVolley, FTWU2_MAX_SHOTS_PER_VOLLEY );
		if( startVolley( ri, data->m_weaponTemplate, INVALID_ID, shots, TheGameLogic->getFrame() ) )
			continueVolley( ri );
	}

	// Out of charges -- but only once the current volley's delayed shots are done.
	if( !isVolleyInProgress() && isVolleyLimitReached() )
		return enterCooldown();

	// Nothing at all in ScanRange: optionally sleep instead of being called every frame.
	if( !foundAnything && !isVolleyInProgress() && data->m_idleScanFrames > 0 )
		return UPDATE_SLEEP( data->m_idleScanFrames );

	return UPDATE_SLEEP_NONE;
}

//-------------------------------------------------------------------------------------------------
Bool FireTargetWeaponUpdateV2::isShooterBusy() const
{
	const Object *me = getObject();
	return me->testStatus( OBJECT_STATUS_UNDER_CONSTRUCTION ) || me->testStatus( OBJECT_STATUS_SOLD );
}

//-------------------------------------------------------------------------------------------------
Int FireTargetWeaponUpdateV2::getCap( UnsignedInt value, Int maxValue ) const
{
	Int v = (Int)value;
	if( v < 1 )
		v = 1;
	if( v > maxValue )
		v = maxValue;
	return v;
}

//-------------------------------------------------------------------------------------------------
/** Same as Weapon::computeBonus() (protected there): the shooter's weapon bonus conditions, plus
	* whatever its container passes to passengers, through the global and the weapon's own bonus sets. */
//-------------------------------------------------------------------------------------------------
void FireTargetWeaponUpdateV2::computeShooterBonus( WeaponBonus &bonus, const WeaponTemplate *tmpl ) const
{
	const Object *me = getObject();

	bonus.clear();
	WeaponBonusConditionFlags flags = me->getWeaponBonusCondition();

	const Object *container = me->getContainedBy();
	if( container )
	{
		const ContainModuleInterface *theirContain = container->getContain();
		if( theirContain && theirContain->isWeaponBonusPassedToPassengers() )
			flags |= theirContain->getWeaponBonusPassedToPassengers();
	}

	if( TheGlobalData->m_weaponBonusSet )
		TheGlobalData->m_weaponBonusSet->appendBonuses( flags, bonus );

	const WeaponBonusSet *extra = tmpl->getExtraBonus();
	if( extra )
		extra->appendBonuses( flags, bonus );
}

//-------------------------------------------------------------------------------------------------
void FireTargetWeaponUpdateV2::computeRanges( RangeInfo &ri, const WeaponTemplate *tmpl ) const
{
	const FireTargetWeaponUpdateV2ModuleData *data = getFireTargetWeaponUpdateV2ModuleData();

	// Real bonus, so range upgrades apply (PointDefenseUpdateV2 uses an empty bonus here).
	WeaponBonus bonus;
	computeShooterBonus( bonus, tmpl );
	Real fireRange = tmpl->getAttackRange( bonus );
	ri.fireRangeSq = fireRange * fireRange;

	// MinimumTargetRange, or the weapon's own MinimumAttackRange if larger -- fireWeaponTemplate()
	// silently refuses targets inside the latter, so never pick one.
	Real minRange = data->m_minimumTargetRange;
	if( tmpl->getMinimumAttackRange() > minRange )
		minRange = tmpl->getMinimumAttackRange();
	ri.minRangeSq = ( minRange > 0.0f ) ? minRange * minRange : 0.0f;
}

//-------------------------------------------------------------------------------------------------
Bool FireTargetWeaponUpdateV2::isValidTarget( Object *target ) const
{
	if( !target || target->isEffectivelyDead() )
		return FALSE;

	// Still an enemy? (it may have been captured / changed sides since we cached it)
	if( getObject()->getRelationship( target ) != ENEMIES )
		return FALSE;

	if( target->testStatus( OBJECT_STATUS_STEALTHED ) && !target->testStatus( OBJECT_STATUS_DETECTED ) && !target->testStatus( OBJECT_STATUS_DISGUISED ) )
		return FALSE; //We can't see it.

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
Bool FireTargetWeaponUpdateV2::isInFireRange( Object *target, const RangeInfo &ri ) const
{
	Real distSq = ThePartitionManager->getDistanceSquared( getObject(), target, FROM_CENTER_2D );
	return distSq <= ri.fireRangeSq && distSq >= ri.minRangeSq;
}

//-------------------------------------------------------------------------------------------------
/** TargetPriority as a single number, lower = better. */
//-------------------------------------------------------------------------------------------------
Real FireTargetWeaponUpdateV2::getPriorityKey( Object *target, Real distSq ) const
{
	const FireTargetWeaponUpdateV2ModuleData *data = getFireTargetWeaponUpdateV2ModuleData();

	switch( data->m_targetPriority )
	{
		case FTW_PRIORITY_FARTHEST:
			return -distSq;

		case FTW_PRIORITY_LOWEST_HEALTH:
		case FTW_PRIORITY_HIGHEST_HEALTH:
		case FTW_PRIORITY_LOWEST_HEALTH_PERCENT:
		case FTW_PRIORITY_HIGHEST_HEALTH_PERCENT:
		{
			const BodyModuleInterface *body = target->getBodyModule();
			Real health = body ? body->getHealth() : 0.0f;
			Real maxHealth = body ? body->getMaxHealth() : 0.0f;
			Real percent = ( maxHealth > 0.0f ) ? health / maxHealth : 0.0f;

			if( data->m_targetPriority == FTW_PRIORITY_LOWEST_HEALTH )					return health;
			if( data->m_targetPriority == FTW_PRIORITY_HIGHEST_HEALTH )					return -health;
			if( data->m_targetPriority == FTW_PRIORITY_LOWEST_HEALTH_PERCENT )	return percent;
			return -percent;
		}

		case FTW_PRIORITY_MOST_EXPENSIVE:
			// Raw template build cost (no faction/handicap modifiers -- those are the OWNER's, and we
			// only want a stable "how valuable is this kind of unit" ranking).
			return -(Real)target->getTemplate()->friend_getBuildCost();

		case FTW_PRIORITY_RANDOM:
			return GameLogicRandomValueReal( 0.0f, 1.0f );	// synced logic RNG -- safe in multiplayer

		case FTW_PRIORITY_NEAREST:
		default:
			return distSq;
	}
}

//-------------------------------------------------------------------------------------------------
/** Drops dead / no-longer-valid entries from the tracked list (keeping order). Forces an early
	* re-scan only when the cache just ran dry, exactly like PointDefenseUpdateV2. */
//-------------------------------------------------------------------------------------------------
void FireTargetWeaponUpdateV2::pruneTrackedTargets()
{
	Int hadTracked = m_numTracked;
	Int liveCount = 0;
	for( Int i = 0; i < m_numTracked; ++i )
	{
		Object *candidate = TheGameLogic->findObjectByID( m_trackedTargets[i] );
		if( !isValidTarget( candidate ) )
			continue;

		m_trackedTargets[liveCount] = m_trackedTargets[i];
		m_trackedTier[liveCount] = m_trackedTier[i];
		++liveCount;
	}
	for( Int i = liveCount; i < m_numTracked; ++i )
		m_trackedTargets[i] = INVALID_ID;
	m_numTracked = liveCount;

	if( hadTracked > 0 && m_numTracked == 0 )
		m_nextScanFrames = 0;
}

//-------------------------------------------------------------------------------------------------
/** Full scan. Keeps up to MaxTrackedTargets candidates: in-range ones first (tier, then
	* TargetPriority), then out-of-range ones (tier, then predicted distance) so a target about to
	* enter range is already cached. Returns TRUE if any enemy at all is inside ScanRange. */
//-------------------------------------------------------------------------------------------------
Bool FireTargetWeaponUpdateV2::scanForTargets( const RangeInfo &ri, const WeaponTemplate *tmpl, Real scanRange )
{
	const FireTargetWeaponUpdateV2ModuleData *data = getFireTargetWeaponUpdateV2ModuleData();
	Object *me = getObject();

	// Enemies only, pruned inside the partition manager's grid walk.
	PartitionFilterRelationship relFilter( me, PartitionFilterRelationship::ALLOW_ENEMIES );
	PartitionFilter *filters[] = { &relFilter, nullptr };

	ObjectIterator *iter = ThePartitionManager->iterateObjectsInRange( me->getPosition(), scanRange, FROM_CENTER_2D, filters );
	MemoryPoolObjectHolder hold(iter);

	Int cap = getCap( data->m_maxTrackedTargets, FTWU2_MAX_TRACKED_TARGETS );

	const KindOfMaskType *tierKindOf[FTWU2_NUM_TIERS] =
	{
		&data->m_primaryTargetKindOf,
		&data->m_secondaryTargetKindOf,
		&data->m_tertiaryTargetKindOf
	};

	// [0] = in fire range, [1] = outside it -- per tier
	ObjectID ids[2][FTWU2_NUM_TIERS][FTWU2_MAX_TRACKED_TARGETS];
	Real     keys[2][FTWU2_NUM_TIERS][FTWU2_MAX_TRACKED_TARGETS];
	Real     dists[2][FTWU2_NUM_TIERS][FTWU2_MAX_TRACKED_TARGETS];
	Int      counts[2][FTWU2_NUM_TIERS];
	for( Int r = 0; r < 2; ++r )
		for( Int t = 0; t < FTWU2_NUM_TIERS; ++t )
			counts[r][t] = 0;

	Bool sawAnything = FALSE;

	for( Object *other = iter->first(); other; other = iter->next() )
	{
		sawAnything = TRUE;	// an enemy is in ScanRange -- stay awake even if this one gets rejected

		Int tier = -1;
		for( Int t = 0; t < FTWU2_NUM_TIERS; ++t )
		{
			if( other->isAnyKindOf( *tierKindOf[t] ) )
			{
				tier = t;
				break;
			}
		}
		if( tier < 0 )
			continue;

		if( !(tmpl->getAntiMask() & classifyForAntiMask( other )) )
			continue;

		if( !isValidTarget( other ) )
			continue;

		Real distSq = ThePartitionManager->getDistanceSquared( me, other, FROM_CENTER_2D );
		if( distSq < ri.minRangeSq )
			continue;

		if( distSq <= ri.fireRangeSq )
		{
			insertSortedCandidate( ids[0][tier], keys[0][tier], dists[0][tier], counts[0][tier], cap,
				other->getID(), getPriorityKey( other, distSq ), distSq );
			continue;
		}

		// Outside fire range: rank by (optionally velocity-predicted) distance.
		Real sortDistSq = distSq;
		if( data->m_velocityFactor != 0.0f && !other->isKindOf( KINDOF_IMMOBILE ) )
		{
			PhysicsBehavior *physics = other->getPhysics();
			if( physics )
			{
				const Coord3D *vel = physics->getVelocity();
				Coord3D projected = *other->getPosition();
				projected.x += vel->x * data->m_velocityFactor;
				projected.y += vel->y * data->m_velocityFactor;
				projected.z += vel->z * data->m_velocityFactor;
				sortDistSq = ThePartitionManager->getDistanceSquared( me, &projected, FROM_CENTER_2D );
			}
		}
		insertSortedCandidate( ids[1][tier], keys[1][tier], dists[1][tier], counts[1][tier], cap,
			other->getID(), sortDistSq, sortDistSq );
	}

	// Merge: in-range Primary, Secondary, Tertiary, then out-of-range Primary, Secondary, Tertiary.
	for( Int i = 0; i < m_numTracked; ++i )
		m_trackedTargets[i] = INVALID_ID;
	m_numTracked = 0;
	for( Int r = 0; r < 2; ++r )
	{
		for( Int t = 0; t < FTWU2_NUM_TIERS; ++t )
		{
			for( Int i = 0; i < counts[r][t] && m_numTracked < cap; ++i )
			{
				m_trackedTargets[m_numTracked] = ids[r][t][i];
				m_trackedTier[m_numTracked] = t;
				++m_numTracked;
			}
		}
	}

	return sawAnything;
}

//-------------------------------------------------------------------------------------------------
/** Picks the volley's targets and arms it (shots start at firstShotFrame).
	*   1) candidates = valid tracked targets currently in fire range (minus primaryID), sorted by
	*      tier / TargetPriority / distance
	*   2) spread pass: take them in order, skipping any whose direction is within
	*      MinTargetSeparationAngle of an already-picked target -- or of the primary target (companion
	*      mode: the unit's own shot already went there)
	*   3) relax pass: fill leftover shots from unpicked candidates, ignoring the angle
	*   4) fewer targets than shots: cycle over the picked targets, then the primary target
	*      (1 enemy takes the whole volley)
	* Returns FALSE (nothing armed) if there is nobody to shoot at. */
//-------------------------------------------------------------------------------------------------
Bool FireTargetWeaponUpdateV2::startVolley( const RangeInfo &ri, const WeaponTemplate *tmpl, ObjectID primaryID, Int shots, UnsignedInt firstShotFrame )
{
	const FireTargetWeaponUpdateV2ModuleData *data = getFireTargetWeaponUpdateV2ModuleData();
	Object *me = getObject();
	const Coord3D *myPos = me->getPosition();

	if( shots <= 0 || !tmpl )
		return FALSE;
	if( shots > FTWU2_MAX_SHOTS_PER_VOLLEY )
		shots = FTWU2_MAX_SHOTS_PER_VOLLEY;

	// Companion mode's real-shot victim: only used if it's a valid enemy target (a force-attacked
	// neutral or a ground shot has none).
	Object *primary = ( primaryID != INVALID_ID ) ? TheGameLogic->findObjectByID( primaryID ) : nullptr;
	if( primary && !isValidTarget( primary ) )
		primary = nullptr;

	Bool primaryHasDir = FALSE;
	Real primaryDirX = 0.0f;
	Real primaryDirY = 0.0f;
	if( primary )
	{
		Real dx = primary->getPosition()->x - myPos->x;
		Real dy = primary->getPosition()->y - myPos->y;
		Real len = (Real)sqrtf( dx * dx + dy * dy );
		if( len > 0.001f )
		{
			primaryHasDir = TRUE;
			primaryDirX = dx / len;
			primaryDirY = dy / len;
		}
	}

	// --- 1) candidates ---
	ObjectID	candId[FTWU2_MAX_TRACKED_TARGETS];
	Int				candTier[FTWU2_MAX_TRACKED_TARGETS];
	Real			candKey[FTWU2_MAX_TRACKED_TARGETS];
	Real			candDist[FTWU2_MAX_TRACKED_TARGETS];
	Real			candDirX[FTWU2_MAX_TRACKED_TARGETS];
	Real			candDirY[FTWU2_MAX_TRACKED_TARGETS];
	Bool			candHasDir[FTWU2_MAX_TRACKED_TARGETS];
	Int				numCand = 0;

	for( Int i = 0; i < m_numTracked; ++i )
	{
		if( m_trackedTargets[i] == primaryID )
			continue;	// already got the unit's own shot

		Object *obj = TheGameLogic->findObjectByID( m_trackedTargets[i] );
		if( !isValidTarget( obj ) )
			continue;

		Real distSq = ThePartitionManager->getDistanceSquared( me, obj, FROM_CENTER_2D );
		if( distSq > ri.fireRangeSq || distSq < ri.minRangeSq )
			continue;

		Int tier = m_trackedTier[i];
		Real key = getPriorityKey( obj, distSq );

		// direction from us (2D), for the spread check
		Real dx = obj->getPosition()->x - myPos->x;
		Real dy = obj->getPosition()->y - myPos->y;
		Real len = (Real)sqrtf( dx * dx + dy * dy );
		Bool hasDir = ( len > 0.001f );
		Real dirX = hasDir ? dx / len : 0.0f;
		Real dirY = hasDir ? dy / len : 0.0f;

		// insertion sort by (tier, key, distance)
		Int pos = numCand;
		while( pos > 0 && isBetterCandidate( tier, key, distSq, candTier[pos - 1], candKey[pos - 1], candDist[pos - 1] ) )
		{
			candId[pos]			= candId[pos - 1];
			candTier[pos]		= candTier[pos - 1];
			candKey[pos]		= candKey[pos - 1];
			candDist[pos]		= candDist[pos - 1];
			candDirX[pos]		= candDirX[pos - 1];
			candDirY[pos]		= candDirY[pos - 1];
			candHasDir[pos]	= candHasDir[pos - 1];
			--pos;
		}
		candId[pos]			= obj->getID();
		candTier[pos]		= tier;
		candKey[pos]		= key;
		candDist[pos]		= distSq;
		candDirX[pos]		= dirX;
		candDirY[pos]		= dirY;
		candHasDir[pos]	= hasDir;
		++numCand;
	}

	if( numCand == 0 && !primary )
		return FALSE;	// nothing to shoot -- no volley, no reload, no charge used

	Int		picked[FTWU2_MAX_SHOTS_PER_VOLLEY];
	Bool	used[FTWU2_MAX_TRACKED_TARGETS];
	for( Int i = 0; i < numCand; ++i )
		used[i] = FALSE;
	Int numPicked = 0;

	// --- 2) spread pass ---
	Real sepAngle = data->m_minTargetSeparationAngle;
	if( sepAngle > PI )
		sepAngle = PI;
	Bool useSpread = ( sepAngle > 0.0f && ( shots > 1 || primaryHasDir ) );
	Real cosSep = useSpread ? (Real)Cos( sepAngle ) : 1.0f;	// angle between A and B < sep  <=>  dot(A,B) > cos(sep)

	if( useSpread )
	{
		for( Int c = 0; c < numCand && numPicked < shots; ++c )
		{
			Bool ok = TRUE;
			if( candHasDir[c] )
			{
				if( primaryHasDir && candDirX[c] * primaryDirX + candDirY[c] * primaryDirY > cosSep )
					ok = FALSE;

				for( Int p = 0; ok && p < numPicked; ++p )
				{
					Int q = picked[p];
					if( !candHasDir[q] )
						continue;
					Real dot = candDirX[c] * candDirX[q] + candDirY[c] * candDirY[q];
					if( dot > cosSep )
						ok = FALSE;
				}
			}
			if( ok )
			{
				picked[numPicked++] = c;
				used[c] = TRUE;
			}
		}
	}

	// --- 3) relax pass (also the whole selection when there's no spread) ---
	for( Int c = 0; c < numCand && numPicked < shots; ++c )
	{
		if( used[c] )
			continue;
		picked[numPicked++] = c;
		used[c] = TRUE;
	}

	// --- 4) build the volley: picked targets first, then cycle over them + the primary target ---
	ObjectID cycle[FTWU2_MAX_SHOTS_PER_VOLLEY + 1];
	Int cycleCount = 0;
	for( Int p = 0; p < numPicked; ++p )
		cycle[cycleCount++] = candId[ picked[p] ];
	if( primary )
		cycle[cycleCount++] = primaryID;

	for( Int i = 0; i < FTWU2_MAX_SHOTS_PER_VOLLEY; ++i )
		m_volleyTargets[i] = INVALID_ID;
	for( Int s = 0; s < shots; ++s )
		m_volleyTargets[s] = cycle[ s % cycleCount ];

	m_volleyShotCount				= shots;
	m_volleyNextShot				= 0;
	m_volleyNextShotFrame		= firstShotFrame;
	m_volleyCounted					= FALSE;
	m_volleyWeaponTemplate	= tmpl;

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
/** Fires every shot of the running volley that's due this frame (all of them when VolleyShotDelay
	* is 0). The first shot that actually goes out counts the volley and starts DelayBetweenShots. */
//-------------------------------------------------------------------------------------------------
void FireTargetWeaponUpdateV2::continueVolley( const RangeInfo &ri )
{
	const FireTargetWeaponUpdateV2ModuleData *data = getFireTargetWeaponUpdateV2ModuleData();
	UnsignedInt now = TheGameLogic->getFrame();

	while( isVolleyInProgress() && now >= m_volleyNextShotFrame )
	{
		Int shotIndex = m_volleyNextShot++;
		m_volleyNextShotFrame = now + data->m_volleyShotDelayFrames;

		Object *target = resolveShotTarget( shotIndex, ri );
		if( !target )
			continue;	// nothing at all to shoot -- drop this shot, never hold the volley open

		if( fireShotAt( target, m_volleyWeaponTemplate ) && !m_volleyCounted )
		{
			m_volleyCounted = TRUE;
			registerVolley();

			WeaponBonus bonus;
			computeShooterBonus( bonus, m_volleyWeaponTemplate );	// real ROF bonus
			m_nextVolleyFrame = now + m_volleyWeaponTemplate->getDelayBetweenShots( bonus );
		}
	}

	if( !isVolleyInProgress() )
		endVolley();
}

//-------------------------------------------------------------------------------------------------
/** The target for one shot: its own volley target if still valid and in range; else the next valid
	* volley target (cycling forward); else any valid tracked target in range; else nullptr. */
//-------------------------------------------------------------------------------------------------
Object* FireTargetWeaponUpdateV2::resolveShotTarget( Int shotIndex, const RangeInfo &ri )
{
	for( Int k = 0; k < m_volleyShotCount; ++k )
	{
		Int j = ( shotIndex + k ) % m_volleyShotCount;
		Object *obj = TheGameLogic->findObjectByID( m_volleyTargets[j] );
		if( isValidTarget( obj ) && isInFireRange( obj, ri ) )
			return obj;
	}

	for( Int i = 0; i < m_numTracked; ++i )
	{
		Object *obj = TheGameLogic->findObjectByID( m_trackedTargets[i] );
		if( isValidTarget( obj ) && isInFireRange( obj, ri ) )
			return obj;
	}

	return nullptr;
}

//-------------------------------------------------------------------------------------------------
/** One shot with a fresh weapon in WeaponSlot (so that slot's fire bones are used). */
//-------------------------------------------------------------------------------------------------
Bool FireTargetWeaponUpdateV2::fireShotAt( Object *target, const WeaponTemplate *tmpl )
{
	if( !tmpl )
		return FALSE;

	Object *me = getObject();

	Weapon *w = TheWeaponStore->allocateNewWeapon( tmpl, (WeaponSlotType)getShotSlot() );
	w->loadAmmoNow( me );
	w->fireWeapon( me, target );
	// fireWeapon()'s return value means "the clip reloaded", not "a shot went out" -- see
	// PointDefenseUpdateV2.cpp. privateFireWeapon() stamps the last-shot frame when it really fires.
	Bool actuallyFired = ( w->getLastShotFrame() == TheGameLogic->getFrame() );
	deleteInstance( w );

	return actuallyFired;
}

//-------------------------------------------------------------------------------------------------
/** WeaponSlot if set, else TriggerWeaponSlot (companion mode: same bones as the unit's own shot), else PRIMARY. */
//-------------------------------------------------------------------------------------------------
Int FireTargetWeaponUpdateV2::getShotSlot() const
{
	const FireTargetWeaponUpdateV2ModuleData *data = getFireTargetWeaponUpdateV2ModuleData();
	if( data->m_weaponSlot != FTWU2_NO_SLOT )
		return data->m_weaponSlot;
	if( data->m_triggerWeaponSlot != FTWU2_NO_SLOT )
		return data->m_triggerWeaponSlot;
	return PRIMARY_WEAPON;
}

//-------------------------------------------------------------------------------------------------
void FireTargetWeaponUpdateV2::endVolley()
{
	for( Int i = 0; i < FTWU2_MAX_SHOTS_PER_VOLLEY; ++i )
		m_volleyTargets[i] = INVALID_ID;
	m_volleyShotCount			= 0;
	m_volleyNextShot			= 0;
	m_volleyNextShotFrame	= 0;
	m_volleyCounted				= FALSE;
	m_volleyWeaponTemplate	= nullptr;
}

//-------------------------------------------------------------------------------------------------
// charges / cooldown (same as PointDefenseUpdateV2's block limit, counted in volleys)
//-------------------------------------------------------------------------------------------------
void FireTargetWeaponUpdateV2::registerVolley()
{
	const FireTargetWeaponUpdateV2ModuleData *data = getFireTargetWeaponUpdateV2ModuleData();
	if( data->m_volleysBeforeCooldown == 0 )
		return;

	UnsignedInt now = TheGameLogic->getFrame();
	if( data->m_volleyCounterResetFrames > 0 && m_volleysThisCycle > 0 && now - m_lastVolleyFrame >= data->m_volleyCounterResetFrames )
		m_volleysThisCycle = 0;

	++m_volleysThisCycle;
	m_lastVolleyFrame = now;
}

//-------------------------------------------------------------------------------------------------
Bool FireTargetWeaponUpdateV2::isVolleyLimitReached() const
{
	const FireTargetWeaponUpdateV2ModuleData *data = getFireTargetWeaponUpdateV2ModuleData();
	return data->m_cooldownFrames > 0
		&& data->m_volleysBeforeCooldown > 0
		&& m_volleysThisCycle >= (Int)data->m_volleysBeforeCooldown;
}

//-------------------------------------------------------------------------------------------------
UpdateSleepTime FireTargetWeaponUpdateV2::enterCooldown()
{
	const FireTargetWeaponUpdateV2ModuleData *data = getFireTargetWeaponUpdateV2ModuleData();

	for( Int i = 0; i < FTWU2_MAX_TRACKED_TARGETS; ++i )
		m_trackedTargets[i] = INVALID_ID;
	m_numTracked = 0;
	endVolley();
	m_volleysThisCycle = 0;

	UnsignedInt cooldownFrames = data->m_cooldownFrames > 0 ? data->m_cooldownFrames : 1;
	m_cooldownEndFrame = TheGameLogic->getFrame() + cooldownFrames;

	applyPhaseVisuals( TRUE, TRUE );

	return UPDATE_SLEEP( cooldownFrames );
}

//-------------------------------------------------------------------------------------------------
void FireTargetWeaponUpdateV2::exitCooldown()
{
	m_cooldownEndFrame	= 0;
	m_volleysThisCycle	= 0;
	m_nextScanFrames		= 0;		// scan immediately -- we've been blind for the whole cooldown
	m_nextVolleyFrame		= 0;		// fully recharged, no leftover reload

	applyPhaseVisuals( FALSE, TRUE );
}

//-------------------------------------------------------------------------------------------------
void FireTargetWeaponUpdateV2::applyPhaseVisuals( Bool onCooldown, Bool fireEffects )
{
	const FireTargetWeaponUpdateV2ModuleData *data = getFireTargetWeaponUpdateV2ModuleData();
	Object *obj = getObject();

	ModelConditionFlagType setFlag		= onCooldown ? data->m_cooldownConditionState : data->m_activeConditionState;
	ModelConditionFlagType clearFlag	= onCooldown ? data->m_activeConditionState : data->m_cooldownConditionState;

	if( clearFlag != MODELCONDITION_INVALID && clearFlag != setFlag )
		obj->clearModelConditionFlags( MAKE_MODELCONDITION_MASK( clearFlag ) );
	if( setFlag != MODELCONDITION_INVALID )
		obj->setModelConditionState( setFlag );

	if( data->m_switchStateTemplate )
	{
		Bool goToAltered = onCooldown ? data->m_alteredStateWhenOnCooldown : !data->m_alteredStateWhenOnCooldown;

		Bool handled = FALSE;
		for( BehaviorModule** u = obj->getBehaviorModules(); *u; ++u )
		{
			SpecialPowerUpdateInterface *spu = (*u)->getSpecialPowerUpdateInterface();
			if( spu && spu->forceSwitchState( data->m_switchStateTemplate, goToAltered, fireEffects ) )
			{
				handled = TRUE;
				break;
			}
		}

		if( !handled && !fireEffects )
		{
			DEBUG_CRASH( ("FireTargetWeaponUpdateV2 for %s has SwitchStateTemplate set, but no SwitchStateV2 with that SpecialPowerTemplate was found on the object",
				obj->getTemplate()->getName().str() ) );
		}
	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void FireTargetWeaponUpdateV2::crc( Xfer *xfer )
{

	// extend base class
	UpdateModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version
	* 2: GeneralsMod 09/10/2026 -- companion mode: the running volley's weapon template (by name) */
// ------------------------------------------------------------------------------------------------
void FireTargetWeaponUpdateV2::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 2;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpdateModule::xfer( xfer );

	// tracked targets
	xfer->xferInt( &m_numTracked );
	for( Int i = 0; i < FTWU2_MAX_TRACKED_TARGETS; ++i )
	{
		xfer->xferObjectID( &m_trackedTargets[i] );
		xfer->xferInt( &m_trackedTier[i] );
	}
	xfer->xferInt( &m_nextScanFrames );

	// volley in progress
	for( Int i = 0; i < FTWU2_MAX_SHOTS_PER_VOLLEY; ++i )
		xfer->xferObjectID( &m_volleyTargets[i] );
	xfer->xferInt( &m_volleyShotCount );
	xfer->xferInt( &m_volleyNextShot );
	xfer->xferUnsignedInt( &m_volleyNextShotFrame );
	xfer->xferBool( &m_volleyCounted );
	xfer->xferUnsignedInt( &m_nextVolleyFrame );

	// the running volley's weapon (companion mode fires whatever weapon triggered it)
	if( version >= 2 )
	{
		AsciiString weaponName;
		if( xfer->getXferMode() == XFER_SAVE && m_volleyWeaponTemplate )
			weaponName = m_volleyWeaponTemplate->getName();
		xfer->xferAsciiString( &weaponName );
		if( xfer->getXferMode() == XFER_LOAD )
			m_volleyWeaponTemplate = weaponName.isEmpty() ? nullptr : TheWeaponStore->findWeaponTemplate( weaponName );
	}
	else if( xfer->getXferMode() == XFER_LOAD )
	{
		m_volleyWeaponTemplate = getFireTargetWeaponUpdateV2ModuleData()->m_weaponTemplate;
	}

	// charges / cooldown
	xfer->xferInt( &m_volleysThisCycle );
	xfer->xferUnsignedInt( &m_lastVolleyFrame );
	xfer->xferUnsignedInt( &m_cooldownEndFrame );

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void FireTargetWeaponUpdateV2::loadPostProcess()
{

	// extend base class
	UpdateModule::loadPostProcess();

	// Re-assert the restored phase on the visuals / linked SwitchStateV2, silently.
	applyPhaseVisuals( m_cooldownEndFrame != 0, FALSE );
	m_phaseVisualsApplied = TRUE;

}
