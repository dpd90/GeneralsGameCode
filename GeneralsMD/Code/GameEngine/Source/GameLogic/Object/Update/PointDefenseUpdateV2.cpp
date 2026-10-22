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

// FILE: PointDefenseUpdateV2.cpp //////////////////////////////////////////////////////////////////
// Desc:   See PointDefenseUpdateV2.h for the list of changes vs PointDefenseLaserUpdate.
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/BitFlagsIO.h"
#include "Common/ModelState.h"
#include "Common/RandomValue.h"
#include "Common/ThingTemplate.h"
#include "Common/Xfer.h"

#include "GameClient/Drawable.h"
#include "GameClient/FXList.h"

#include "GameLogic/GameLogic.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/Object.h"
#include "GameLogic/ObjectCreationList.h"
#include "GameLogic/ObjectIter.h"
#include "GameLogic/Module/PointDefenseUpdateV2.h"
#include "GameLogic/Module/PhysicsUpdate.h"
#include "GameLogic/Module/SpecialPowerUpdateModule.h"
#include "GameLogic/Weapon.h"



//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PointDefenseUpdateV2ModuleData::PointDefenseUpdateV2ModuleData()
{
	m_weaponTemplate			= nullptr;
	m_scanFrames					= 0;
	m_idleScanFrames			= 0;			// never idle-sleep unless a modder opts in
	m_scanRange						= 0.0f;
	m_velocityFactor			= 0.0f;
	m_maxTrackedTargets		= 1;			// same single-target behavior as PointDefenseLaserUpdate unless raised
	m_minimumInterceptRange	= 0.0f;		// disabled -- intercept everything in range, same as before this field existed

	// GeneralsMod @feature Dimitar 02/10/2026: block limit / cooldown -- all off by default
	m_blocksBeforeCooldown				= 0;			// block forever
	m_cooldownFrames							= 0;
	m_blockCounterResetFrames			= 0;			// partial count never decays
	m_shieldHealth								= 0.0f;		// health pool off
	m_drainMultiplier							= 1.0f;
	m_defaultDrain								= 0.0f;
	m_shieldRegenRate							= 0.0f;		// no regen
	m_shieldRegenDelayFrames			= 0;
	m_activeConditionState				= MODELCONDITION_INVALID;
	m_cooldownConditionState			= MODELCONDITION_INVALID;
	m_switchStateTemplate					= nullptr;
	m_alteredStateWhenOnCooldown	= FALSE;	// Altered while active/charged

	// GeneralsMod @feature Dimitar 02/10/2026: interference -- off unless InterferenceGroup is given
	m_interferenceGroup						= NAMEKEY_INVALID;
	m_interferenceRange						= 0.0f;
	m_interferenceScanFrames			= LOGICFRAMES_PER_SECOND / 2;	// 500 ms
	m_interferenceCooldownFrames	= 0;			// use CooldownDuration
	m_hasInterference							= TRUE;		// can be knocked down by a clash
	m_interferenceFX							= nullptr;
	m_interferenceOCL							= nullptr;
	m_switchStateEffectsOnInterference	= TRUE;
}

//-------------------------------------------------------------------------------------------------
/** InterferenceGroup: a free name, hashed once at INI load so runtime comparisons are just an int compare. */
//-------------------------------------------------------------------------------------------------
/*static*/ void PointDefenseUpdateV2ModuleData::parseInterferenceGroup( INI* ini, void * /*instance*/, void *store, const void* /*userData*/ )
{
	*(NameKeyType*)store = TheNameKeyGenerator->nameToKey( ini->getNextToken() );
}

//-------------------------------------------------------------------------------------------------
/*static*/ void PointDefenseUpdateV2ModuleData::buildFieldParse(MultiIniFieldParse& p)
{
	ModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "WeaponTemplate",				INI::parseWeaponTemplate,				nullptr, offsetof( PointDefenseUpdateV2ModuleData, m_weaponTemplate ) },
		{ "PrimaryTargetTypes",		KindOfMaskType::parseFromINI,								nullptr, offsetof( PointDefenseUpdateV2ModuleData, m_primaryTargetKindOf ) },
		{ "SecondaryTargetTypes",	KindOfMaskType::parseFromINI,								nullptr, offsetof( PointDefenseUpdateV2ModuleData, m_secondaryTargetKindOf ) },
		{ "ScanRate",							INI::parseDurationUnsignedInt,	nullptr, offsetof( PointDefenseUpdateV2ModuleData, m_scanFrames ) },
		{ "IdleScanRate",					INI::parseDurationUnsignedInt,	nullptr, offsetof( PointDefenseUpdateV2ModuleData, m_idleScanFrames ) },
		{ "ScanRange",						INI::parseReal,									nullptr, offsetof( PointDefenseUpdateV2ModuleData, m_scanRange ) },
		{ "PredictTargetVelocityFactor", INI::parseReal,					nullptr, offsetof( PointDefenseUpdateV2ModuleData, m_velocityFactor ) },
		{ "MaxTrackedTargets",		INI::parseUnsignedInt,					nullptr, offsetof( PointDefenseUpdateV2ModuleData, m_maxTrackedTargets ) },
		{ "MinimumInterceptRange", INI::parseReal,					nullptr, offsetof( PointDefenseUpdateV2ModuleData, m_minimumInterceptRange ) },
		{ "BlocksBeforeCooldown",	INI::parseUnsignedInt,					nullptr, offsetof( PointDefenseUpdateV2ModuleData, m_blocksBeforeCooldown ) },
		{ "CooldownDuration",			INI::parseDurationUnsignedInt,	nullptr, offsetof( PointDefenseUpdateV2ModuleData, m_cooldownFrames ) },
		{ "BlockCounterResetTime",	INI::parseDurationUnsignedInt,	nullptr, offsetof( PointDefenseUpdateV2ModuleData, m_blockCounterResetFrames ) },
		{ "ShieldHealth",					INI::parseReal,									nullptr, offsetof( PointDefenseUpdateV2ModuleData, m_shieldHealth ) },
		{ "DrainMultiplier",			INI::parseReal,									nullptr, offsetof( PointDefenseUpdateV2ModuleData, m_drainMultiplier ) },
		{ "DefaultDrain",					INI::parseReal,									nullptr, offsetof( PointDefenseUpdateV2ModuleData, m_defaultDrain ) },
		{ "ShieldRegenRate",			INI::parseReal,									nullptr, offsetof( PointDefenseUpdateV2ModuleData, m_shieldRegenRate ) },
		{ "ShieldRegenDelay",			INI::parseDurationUnsignedInt,	nullptr, offsetof( PointDefenseUpdateV2ModuleData, m_shieldRegenDelayFrames ) },
		{ "ActiveConditionState",	INI::parseIndexList,	ModelConditionFlags::getBitNames(), offsetof( PointDefenseUpdateV2ModuleData, m_activeConditionState ) },
		{ "CooldownConditionState",	INI::parseIndexList,	ModelConditionFlags::getBitNames(), offsetof( PointDefenseUpdateV2ModuleData, m_cooldownConditionState ) },
		{ "SwitchStateTemplate",	INI::parseSpecialPowerTemplate,	nullptr, offsetof( PointDefenseUpdateV2ModuleData, m_switchStateTemplate ) },
		{ "AlteredStateWhenOnCooldown", INI::parseBool,				nullptr, offsetof( PointDefenseUpdateV2ModuleData, m_alteredStateWhenOnCooldown ) },
		{ "InterferenceGroup",		parseInterferenceGroup,					nullptr, offsetof( PointDefenseUpdateV2ModuleData, m_interferenceGroup ) },
		{ "InterferenceRange",		INI::parseReal,									nullptr, offsetof( PointDefenseUpdateV2ModuleData, m_interferenceRange ) },
		{ "InterferenceScanRate",	INI::parseDurationUnsignedInt,	nullptr, offsetof( PointDefenseUpdateV2ModuleData, m_interferenceScanFrames ) },
		{ "InterferenceCooldownDuration", INI::parseDurationUnsignedInt, nullptr, offsetof( PointDefenseUpdateV2ModuleData, m_interferenceCooldownFrames ) },
		{ "HasInterference",			INI::parseBool,									nullptr, offsetof( PointDefenseUpdateV2ModuleData, m_hasInterference ) },
		{ "InterferenceFX",				INI::parseFXList,								nullptr, offsetof( PointDefenseUpdateV2ModuleData, m_interferenceFX ) },
		{ "InterferenceOCL",			INI::parseObjectCreationList,		nullptr, offsetof( PointDefenseUpdateV2ModuleData, m_interferenceOCL ) },
		{ "SwitchStateEffectsOnInterference", INI::parseBool,			nullptr, offsetof( PointDefenseUpdateV2ModuleData, m_switchStateEffectsOnInterference ) },
		{ nullptr, nullptr, nullptr, 0 }
	};
	p.add(dataFieldParse);
}

//-------------------------------------------------------------------------------------------------
namespace
{
	// Small helper: insert (id, distSq) into a fixed-capacity list kept sorted nearest-first.
	// Only used inside scanForTargets(), which is already the "expensive, infrequent" path.
	void insertSortedCandidate( ObjectID *ids, Real *distsSq, Int &count, Int cap, ObjectID id, Real distSq )
	{
		if( count < cap )
		{
			Int i = count;
			while( i > 0 && distsSq[i - 1] > distSq )
			{
				ids[i] = ids[i - 1];
				distsSq[i] = distsSq[i - 1];
				--i;
			}
			ids[i] = id;
			distsSq[i] = distSq;
			++count;
		}
		else if( cap > 0 && distSq < distsSq[cap - 1] )
		{
			Int i = cap - 1;
			while( i > 0 && distsSq[i - 1] > distSq )
			{
				ids[i] = ids[i - 1];
				distsSq[i] = distsSq[i - 1];
				--i;
			}
			ids[i] = id;
			distsSq[i] = distSq;
		}
	}

	//-------------------------------------------------------------------------------------------
	// Classifies a potential victim the same way WeaponSet::getVictimAntiMask() does (that function
	// is file-local/static to WeaponSet.cpp, so it isn't directly callable from here) -- returns
	// which WeaponAntiMaskType bit(s) a weapon needs in order to be allowed to hit it. Needed because
	// we fire this weapon directly (fireWeapon() on a manually-picked target) instead of going
	// through the normal WeaponSet target-acquisition path, so none of the engine's usual Anti* mask
	// enforcement (WeaponSet.cpp's getAbleToUseWeaponAgainstTarget(), ~line 881) ever runs for us --
	// without reproducing it, every Anti* field on the weapon (AntiGround/AntiProjectile/
	// AntiSmallMissile/AntiBallisticMissile/etc.) would be silently ignored and this module would
	// just shoot at anything matching PrimaryTargetTypes/SecondaryTargetTypes.
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
PointDefenseUpdateV2::PointDefenseUpdateV2( Thing *thing, const ModuleData* moduleData ) : UpdateModule( thing, moduleData )
{
	for( Int i = 0; i < PDU2_MAX_TRACKED_TARGETS; ++i )
		m_trackedTargets[i] = INVALID_ID;
	m_numTracked = 0;
	m_nextShotAvailableInFrames = 0;
	m_inRange  					= false;

	m_blocksThisCycle			= 0;
	m_lastBlockFrame			= 0;
	m_cooldownEndFrame		= 0;		// start active (fully charged)
	m_phaseVisualsApplied	= FALSE;
	m_currentShieldHealth	= getPointDefenseUpdateV2ModuleData()->m_shieldHealth;	// start full
	m_lastDrainFrame			= 0;
	m_lastRegenFrame			= TheGameLogic->getFrame();

	// OPTIMIZATION: stagger the very first scan so a batch of these units built or placed on the
	// same frame don't stay locked in step, all doing their expensive full scans on the same tick
	// forever after. Purely cosmetic for load, but costs nothing.
	const PointDefenseUpdateV2ModuleData *data = getPointDefenseUpdateV2ModuleData();
	m_nextScanFrames = ( data->m_scanFrames > 1 ) ? GameLogicRandomValue( 0, (Int)data->m_scanFrames - 1 ) : 0;

	// Same staggering for the interference check. Only roll the random number when interference is
	// actually on, so units without it consume exactly the same logic RNG as before this feature.
	m_nextInterferenceCheckFrame = TheGameLogic->getFrame();
	if( data->m_interferenceGroup != NAMEKEY_INVALID && data->m_interferenceScanFrames > 1 )
		m_nextInterferenceCheckFrame += GameLogicRandomValue( 0, (Int)data->m_interferenceScanFrames - 1 );

	setWakeFrame(getObject(), UPDATE_SLEEP_NONE);// No starting sleep, but we want to sleep later.
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PointDefenseUpdateV2::~PointDefenseUpdateV2()
{

}

//-------------------------------------------------------------------------------------------------
void PointDefenseUpdateV2::onObjectCreated()
{
	const PointDefenseUpdateV2ModuleData *data = getPointDefenseUpdateV2ModuleData();

	//Make sure we have a weapon template
	if( !data->m_weaponTemplate )
	{
		DEBUG_CRASH( ("PointDefenseUpdateV2 for %s doesn't have a valid weapon template",
			getObject()->getTemplate()->getName().str() ) );
		return;
	}

	//Make sure our firing range is smaller than the scan range.
	WeaponBonus bonus;
	bonus.clear();
	Real attackRange = data->m_weaponTemplate->getAttackRange( bonus );
	if( data->m_scanRange <= attackRange )
	{
		DEBUG_CRASH( ("PointDefenseUpdateV2 for %s requires the scan range (%.1f) being larger than the firing range (%.1f)",
			getObject()->getTemplate()->getName().str(), data->m_scanRange, attackRange ) );
	}

	if( data->m_maxTrackedTargets > (UnsignedInt)PDU2_MAX_TRACKED_TARGETS )
	{
		DEBUG_CRASH( ("PointDefenseUpdateV2 for %s has MaxTrackedTargets=%d, clamping to %d",
			getObject()->getTemplate()->getName().str(), data->m_maxTrackedTargets, PDU2_MAX_TRACKED_TARGETS ) );
	}

	if( data->m_shieldHealth > 0.0f && data->m_blocksBeforeCooldown > 0 )
	{
		DEBUG_CRASH( ("PointDefenseUpdateV2 for %s has both ShieldHealth and BlocksBeforeCooldown -- they're alternatives; using ShieldHealth",
			getObject()->getTemplate()->getName().str() ) );
	}
	if( data->m_shieldHealth > 0.0f && data->m_cooldownFrames == 0 )
	{
		DEBUG_CRASH( ("PointDefenseUpdateV2 for %s has ShieldHealth but no CooldownDuration -- the shield will never break",
			getObject()->getTemplate()->getName().str() ) );
	}

	if( data->m_shieldHealth <= 0.0f && data->m_blocksBeforeCooldown > 0 && data->m_cooldownFrames == 0 )
	{
		DEBUG_CRASH( ("PointDefenseUpdateV2 for %s has BlocksBeforeCooldown=%d but no CooldownDuration -- it will block forever",
			getObject()->getTemplate()->getName().str(), data->m_blocksBeforeCooldown ) );
	}

	if( data->m_interferenceGroup != NAMEKEY_INVALID )
	{
		if( data->m_interferenceRange <= 0.0f )
		{
			DEBUG_CRASH( ("PointDefenseUpdateV2 for %s has InterferenceGroup but no InterferenceRange -- it will never clash",
				getObject()->getTemplate()->getName().str() ) );
		}
		if( data->m_hasInterference && getInterferenceCooldownFrames() == 0 )
		{
			DEBUG_CRASH( ("PointDefenseUpdateV2 for %s has InterferenceGroup but neither InterferenceCooldownDuration nor CooldownDuration -- it will never clash",
				getObject()->getTemplate()->getName().str() ) );
		}
	}

	// NOTE: the initial visual / SwitchStateV2 sync is NOT done here. onObjectCreated() runs inside
	// Object::init() before the Drawable is bound (sendObjectCreated() comes later), so model
	// condition flags set now would be silently dropped. It's done on the first update() instead.
}

//-------------------------------------------------------------------------------------------------
/** The update callback. */
//-------------------------------------------------------------------------------------------------
UpdateSleepTime PointDefenseUpdateV2::update()
{
	//*** UPDATE PHILOSOPHY (vs PointDefenseLaserUpdate) ***
	//Same idea: don't do the expensive full-radius scan every frame, do it every ScanRate frames
	//and coast on cached targets in between. Two differences:
	// 1) We coast on a small LIST of cached targets, not just one, so losing the current target
	//    doesn't force an emergency re-scan before the next shot can go out.
	// 2) If a scan finds literally nothing in ScanRange, we can tell the engine to not even call
	//    update() again for a while (true sleepy-update), instead of being invoked every frame just
	//    to immediately bail out. This is opt-in via IdleScanRate (0 = disabled, matches V1 exactly).

	Object *me = getObject();
	if( me->isEffectivelyDead() )
		return UPDATE_SLEEP_FOREVER;//No more laser fo you.

	const PointDefenseUpdateV2ModuleData *data = getPointDefenseUpdateV2ModuleData();

	// First update after creation: silently put the visuals / linked SwitchStateV2 into the phase we
	// start in (no In/Out effects -- the unit didn't "power up", it was built that way).
	if( !m_phaseVisualsApplied )
	{
		applyPhaseVisuals( m_cooldownEndFrame != 0, FALSE );
		m_phaseVisualsApplied = TRUE;
	}

	// GeneralsMod @feature Dimitar 02/10/2026: cooldown. Normally we're asleep for the whole thing and
	// only get here once it's over; the frame check covers anything that wakes us early, and a
	// cooldown that expired while we were disabled (update() isn't called then).
	if( m_cooldownEndFrame != 0 )
	{
		if( TheGameLogic->getFrame() < m_cooldownEndFrame )
			return frameToSleepTime( m_cooldownEndFrame );

		exitCooldown();
	}

	// ShieldHealth mode: gradual regen while up (by elapsed frames, so idle sleeps don't lose any).
	regenShield();

	// GeneralsMod @feature Dimitar 02/10/2026: shield interference. Runs on its own (slow) timer,
	// independent of whether any enemies are around.
	if( data->m_interferenceGroup != NAMEKEY_INVALID )
	{
		UnsignedInt now = TheGameLogic->getFrame();
		if( now >= m_nextInterferenceCheckFrame )
		{
			m_nextInterferenceCheckFrame = now + ( data->m_interferenceScanFrames > 0 ? data->m_interferenceScanFrames : 1 );
			// checkInterference() knocks down everyone else it finds either way; we only go down ourselves
			// if a clash can affect us (HasInterference = No is the immune "superpower" shield).
			if( checkInterference() && data->m_hasInterference )
			{
				fireInterferenceEffects();
				return enterCooldown( getInterferenceCooldownFrames(), TRUE );
			}
		}
	}

	//Optimized firing at already-cached targets. (Forcing an early re-scan specifically when the
	//cache runs dry mid-cycle is handled inside fireWhenReady() itself, not here -- see the
	//"hadTracked" logic there. Doing it there instead of with a blanket "m_numTracked > 0" check
	//here matters: this area can easily have plenty of irrelevant enemies wandering through
	//(infantry near a projectile-only shield, say) that never populate the cache at all, and we
	//don't want that to force a full rescan every single frame -- only actually losing a target
	//we were tracking should.)
	if( m_nextScanFrames > 0 )
	{
		m_nextScanFrames--;
		fireWhenReady();
		if( isBlockLimitReached() )
			return enterCooldown( data->m_cooldownFrames, FALSE );
		return UPDATE_SLEEP_NONE;
	}

	//Periodic scanning (expensive)
	Bool foundAnything = scanForTargets();
	fireWhenReady();
	if( isBlockLimitReached() )
		return enterCooldown( data->m_cooldownFrames, FALSE );

	if( foundAnything )
	{
		m_nextScanFrames = data->m_scanFrames;
		return UPDATE_SLEEP_NONE;
	}

	//No enemies at all within ScanRange.
	m_nextScanFrames = 0;
	if( data->m_idleScanFrames > 0 )
	{
		// Don't idle-sleep past our next interference check, or two idle shield units parked next to
		// each other would never notice.
		UnsignedInt sleepFrames = data->m_idleScanFrames;
		if( data->m_interferenceGroup != NAMEKEY_INVALID )
		{
			UnsignedInt now = TheGameLogic->getFrame();
			UnsignedInt untilCheck = ( m_nextInterferenceCheckFrame > now ) ? m_nextInterferenceCheckFrame - now : 1;
			if( untilCheck < sleepFrames )
				sleepFrames = untilCheck;
		}
		return UPDATE_SLEEP( sleepFrames );
	}

	return UPDATE_SLEEP_NONE;
}

//-------------------------------------------------------------------------------------------------
void PointDefenseUpdateV2::mergeBucket( const ObjectID *ids, Int count )
{
	const PointDefenseUpdateV2ModuleData *data = getPointDefenseUpdateV2ModuleData();
	Int cap = (Int)data->m_maxTrackedTargets;
	if( cap <= 0 )
		cap = 1;
	if( cap > PDU2_MAX_TRACKED_TARGETS )
		cap = PDU2_MAX_TRACKED_TARGETS;

	for( Int i = 0; i < count && m_numTracked < cap; ++i )
		m_trackedTargets[ m_numTracked++ ] = ids[i];
}

//-------------------------------------------------------------------------------------------------
void PointDefenseUpdateV2::fireWhenReady()
{
	const PointDefenseUpdateV2ModuleData *data = getPointDefenseUpdateV2ModuleData();
	Object *me = getObject();

	WeaponBonus bonus;
	bonus.clear();
	Real fireRange = data->m_weaponTemplate->getAttackRange( bonus );
	Real fireRangeSq = fireRange * fireRange;
	// MinimumInterceptRange, or the weapon's own MinimumAttackRange if that's larger: fireWeaponTemplate()
	// silently refuses targets inside the latter, so never pick one (see fireWhenReady()).
	Real minRange = data->m_minimumInterceptRange;
	if( data->m_weaponTemplate->getMinimumAttackRange() > minRange )
		minRange = data->m_weaponTemplate->getMinimumAttackRange();
	Real minRangeSq = ( minRange > 0.0f ) ? minRange * minRange : 0.0f;

	Int hadTracked = m_numTracked; //remember whether we walked in with anything to lose

	//Walk the cached target list: drop anything dead/gone, keep the rest in order, and remember
	//the first one that's actually within firing range right now.
	Object *target = nullptr;
	Int liveCount = 0;
	for( Int i = 0; i < m_numTracked; ++i )
	{
		Object *candidate = TheGameLogic->findObjectByID( m_trackedTargets[i] );
		if( !candidate || candidate->isEffectivelyDead() )
			continue; //drop it -- don't copy forward, this is how the list shrinks.

		m_trackedTargets[ liveCount++ ] = m_trackedTargets[i];

		if( !target )
		{
			Real distSq = ThePartitionManager->getDistanceSquared( me, candidate, FROM_CENTER_2D );
			//MinimumInterceptRange: if the shooter has already breached this inner radius, leave its
			//shot alone rather than swatting it down right on top of us -- and don't fall through to
			//fireWeapon() for it either, since the weapon's own MinimumAttackRange (if any) would
			//silently refuse to fire and still cost us a reload cycle for nothing.
			if( distSq <= fireRangeSq && distSq >= minRangeSq )
				target = candidate;
		}
	}
	m_numTracked = liveCount;
	m_inRange = ( target != nullptr );

	//OPTIMIZATION vs V1: instead of V1's blind GameLogicRandomValue(0,3)-frame gamble whenever its
	//one cached target dies, force an early re-scan only when the cache just ran out completely --
	//and only when it actually had something in it a moment ago. An already-empty cache (nothing
	//worth tracking found last scan) is left alone to respect ScanRate/IdleScanRate normally.
	if( hadTracked > 0 && m_numTracked == 0 )
		m_nextScanFrames = 0;

	if( m_nextShotAvailableInFrames > 0 )
	{
		//We can't fire this frame.
		m_nextShotAvailableInFrames--;
		return;
	}

	if( target )
	{
		WeaponTemplate *wt = data->m_weaponTemplate;
		WeaponBonus fireBonus;
		fireBonus.clear();

		Weapon* w = TheWeaponStore->allocateNewWeapon( wt, TERTIARY_WEAPON );
		w->loadAmmoNow( getObject() );
		w->fireWeapon( getObject(), target );
		// GeneralsMod fix 02/10/2026: fireWeapon()'s return value means "the clip RELOADED", NOT "a shot
		// went out" (see Weapon::privateFireWeapon()) -- with a normal clip it's FALSE for every real
		// shot, which meant no DelayBetweenShots and no block counting at all (fired every frame,
		// BlocksBeforeCooldown never reached). privateFireWeapon() stamps m_lastFireFrame whenever it
		// goes through the fire path; it only bails out before that if the weapon isn't READY_TO_FIRE,
		// which a freshly loaded weapon always is. The one remaining silent refusal inside
		// fireWeaponTemplate() -- the weapon's own MinimumAttackRange -- is prevented up front by folding
		// it into minRangeSq, so such a target is never picked in the first place.
		Bool actuallyFired = ( w->getLastShotFrame() == TheGameLogic->getFrame() );
		deleteInstance(w);

		// Only start the reload timer if the shot actually went out -- if we didn't fire, don't burn a
		// reload cycle on nothing, or a single unreachable cached target could jam us against every
		// other target too.
		if( actuallyFired )
		{
			m_nextShotAvailableInFrames = wt->getDelayBetweenShots( fireBonus );

			if( usesShieldHealth() )
			{
				// ShieldHealth: charged once, on the shot that destroys the projectile (laser damage is
				// applied immediately, so it's already dead here if this shot killed it). "Last stand": a
				// projectile worth more than what's left is still blocked -- health just bottoms out at 0.
				if( target->isEffectivelyDead() )
				{
					m_currentShieldHealth -= getProjectileDrain( target );
					if( m_currentShieldHealth < 0.0f )
						m_currentShieldHealth = 0.0f;
					m_lastDrainFrame = TheGameLogic->getFrame();
				}
			}
			else
				registerBlock();	// a "block" = a shot that actually went out (not a confirmed kill)
		}
	}
}

//-------------------------------------------------------------------------------------------------
// GeneralsMod @feature Dimitar 02/10/2026: block limit / cooldown
//-------------------------------------------------------------------------------------------------
void PointDefenseUpdateV2::registerBlock()
{
	const PointDefenseUpdateV2ModuleData *data = getPointDefenseUpdateV2ModuleData();
	if( data->m_blocksBeforeCooldown == 0 )
		return;	// unlimited -- nothing to count

	UnsignedInt now = TheGameLogic->getFrame();

	// BlockCounterResetTime: checked lazily here (on the next block) instead of with a timer -- same
	// result, zero cost while nothing is happening.
	if( data->m_blockCounterResetFrames > 0 && m_blocksThisCycle > 0 && now - m_lastBlockFrame >= data->m_blockCounterResetFrames )
		m_blocksThisCycle = 0;

	++m_blocksThisCycle;
	m_lastBlockFrame = now;
}

//-------------------------------------------------------------------------------------------------
Bool PointDefenseUpdateV2::isBlockLimitReached() const
{
	const PointDefenseUpdateV2ModuleData *data = getPointDefenseUpdateV2ModuleData();
	if( data->m_cooldownFrames == 0 )
		return FALSE;

	if( usesShieldHealth() )
		return m_currentShieldHealth <= 0.0f;

	return data->m_blocksBeforeCooldown > 0
		&& m_blocksThisCycle >= (Int)data->m_blocksBeforeCooldown;
}

//-------------------------------------------------------------------------------------------------
// GeneralsMod @feature Dimitar 05/10/2026: ShieldHealth
//-------------------------------------------------------------------------------------------------
Bool PointDefenseUpdateV2::usesShieldHealth() const
{
	return getPointDefenseUpdateV2ModuleData()->m_shieldHealth > 0.0f;
}

//-------------------------------------------------------------------------------------------------
/** The damage this projectile would have dealt: the PrimaryDamage of the weapon it detonates with
	* (DumbProjectileBehavior / MissileAIUpdate / MissileAIUpdateV2 know it), times DrainMultiplier.
	* Base weapon value -- the launcher's veterancy/upgrade bonuses aren't applied. */
//-------------------------------------------------------------------------------------------------
Real PointDefenseUpdateV2::getProjectileDrain( Object *projectile ) const
{
	const PointDefenseUpdateV2ModuleData *data = getPointDefenseUpdateV2ModuleData();

	for( BehaviorModule** m = projectile->getBehaviorModules(); *m; ++m )
	{
		ProjectileUpdateInterface *pui = (*m)->getProjectileUpdateInterface();
		if( !pui )
			continue;

		const WeaponTemplate *detWeapon = pui->projectileGetDetonationWeapon();
		if( detWeapon )
		{
			WeaponBonus bonus;
			bonus.clear();
			return detWeapon->getPrimaryDamage( bonus ) * data->m_drainMultiplier;
		}
	}

	return data->m_defaultDrain * data->m_drainMultiplier;
}

//-------------------------------------------------------------------------------------------------
void PointDefenseUpdateV2::regenShield()
{
	const PointDefenseUpdateV2ModuleData *data = getPointDefenseUpdateV2ModuleData();
	UnsignedInt now = TheGameLogic->getFrame();

	if( usesShieldHealth() && data->m_shieldRegenRate > 0.0f && m_currentShieldHealth < data->m_shieldHealth )
	{
		// Regen counts from whichever is later: the last time we applied it, or the end of the delay
		// after the last drain.
		UnsignedInt from = m_lastRegenFrame;
		UnsignedInt delayEnd = m_lastDrainFrame + data->m_shieldRegenDelayFrames;
		if( delayEnd > from )
			from = delayEnd;

		if( now > from )
		{
			m_currentShieldHealth += data->m_shieldRegenRate * (Real)( now - from ) / (Real)LOGICFRAMES_PER_SECOND;
			if( m_currentShieldHealth > data->m_shieldHealth )
				m_currentShieldHealth = data->m_shieldHealth;
		}
	}

	m_lastRegenFrame = now;
}

//-------------------------------------------------------------------------------------------------
UpdateSleepTime PointDefenseUpdateV2::enterCooldown( UnsignedInt cooldownFrames, Bool fromInterference )
{
	// Forget everything we were tracking -- the world will look different when we wake up.
	for( Int i = 0; i < PDU2_MAX_TRACKED_TARGETS; ++i )
		m_trackedTargets[i] = INVALID_ID;
	m_numTracked = 0;
	m_inRange = false;
	m_blocksThisCycle = 0;
	m_currentShieldHealth = 0.0f;	// down (a clash empties it too); refilled in exitCooldown()

	if( cooldownFrames == 0 )
		cooldownFrames = 1;	// callers guard against this; just never produce m_cooldownEndFrame == now with a 0 sleep

	m_cooldownEndFrame = TheGameLogic->getFrame() + cooldownFrames;

	// SwitchStateEffectsOnInterference = No: on a clash, switch the linked SwitchStateV2 silently (no
	// In/Out effects) -- InterferenceFX/OCL are the clash's effects. Running out of blocks always fires them.
	Bool fireEffects = !( fromInterference && !getPointDefenseUpdateV2ModuleData()->m_switchStateEffectsOnInterference );
	applyPhaseVisuals( TRUE, fireEffects );

	// Real engine sleep for the whole cooldown -- no per-frame cost.
	return UPDATE_SLEEP( cooldownFrames );
}

//-------------------------------------------------------------------------------------------------
void PointDefenseUpdateV2::exitCooldown()
{
	m_cooldownEndFrame = 0;
	m_blocksThisCycle = 0;
	m_currentShieldHealth = getPointDefenseUpdateV2ModuleData()->m_shieldHealth;	// back at full health
	m_lastRegenFrame = TheGameLogic->getFrame();	// no regen accrued during the cooldown
	m_nextScanFrames = 0;							// scan immediately -- we've been blind for the whole cooldown
	m_nextShotAvailableInFrames = 0;	// fully recharged, no leftover reload
	m_nextInterferenceCheckFrame = TheGameLogic->getFrame();	// check for clashes right away -- still-stacked units go straight back down

	applyPhaseVisuals( FALSE, TRUE );
}

//-------------------------------------------------------------------------------------------------
// GeneralsMod @feature Dimitar 02/10/2026: shield interference
//-------------------------------------------------------------------------------------------------
NameKeyType PointDefenseUpdateV2::getInterferenceGroup() const
{
	return getPointDefenseUpdateV2ModuleData()->m_interferenceGroup;
}

//-------------------------------------------------------------------------------------------------
Bool PointDefenseUpdateV2::hasInterference() const
{
	return getPointDefenseUpdateV2ModuleData()->m_hasInterference;
}

//-------------------------------------------------------------------------------------------------
UnsignedInt PointDefenseUpdateV2::getInterferenceCooldownFrames() const
{
	const PointDefenseUpdateV2ModuleData *data = getPointDefenseUpdateV2ModuleData();
	return ( data->m_interferenceCooldownFrames > 0 ) ? data->m_interferenceCooldownFrames : data->m_cooldownFrames;
}

//-------------------------------------------------------------------------------------------------
Bool PointDefenseUpdateV2::isInterferenceShieldUp() const
{
	const PointDefenseUpdateV2ModuleData *data = getPointDefenseUpdateV2ModuleData();
	if( data->m_interferenceGroup == NAMEKEY_INVALID )
		return FALSE;
	if( m_cooldownEndFrame != 0 )
		return FALSE;	// already down -- no shield to clash with, and don't extend its cooldown
	if( data->m_hasInterference && getInterferenceCooldownFrames() == 0 )
		return FALSE;	// misconfigured (warned in onObjectCreated) -- can't go on cooldown. (An immune
									// HasInterference = No shield never goes on cooldown from a clash, so it doesn't need one.)

	const Object *obj = getObject();
	if( obj->isEffectivelyDead() )
		return FALSE;
	if( obj->isDisabled() )
		return FALSE;	// EMP/underpowered/etc. -- our update isn't even running
	if( obj->getContainedBy() != nullptr )
		return FALSE;	// inside a transport/building
	if( obj->testStatus( OBJECT_STATUS_UNDER_CONSTRUCTION ) )
		return FALSE;

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
/** InterferenceFX / InterferenceOCL, at our own position -- whenever a clash knocks THIS unit down. */
//-------------------------------------------------------------------------------------------------
void PointDefenseUpdateV2::fireInterferenceEffects()
{
	const PointDefenseUpdateV2ModuleData *data = getPointDefenseUpdateV2ModuleData();
	Object *obj = getObject();

	if( data->m_interferenceFX )
		FXList::doFXObj( data->m_interferenceFX, obj );
	if( data->m_interferenceOCL )
		ObjectCreationList::create( data->m_interferenceOCL, obj, nullptr );
}

//-------------------------------------------------------------------------------------------------
/** Knocked down by another unit's clash check. We're not the module currently being updated, so we
	* can't return a sleep time -- reschedule our own update to the end of the cooldown instead. */
//-------------------------------------------------------------------------------------------------
void PointDefenseUpdateV2::startInterferenceCooldown()
{
	fireInterferenceEffects();

	UnsignedInt frames = getInterferenceCooldownFrames();
	enterCooldown( frames, TRUE );
	m_phaseVisualsApplied = TRUE;	// enterCooldown just applied them
	setWakeFrame( getObject(), UPDATE_SLEEP( frames > 0 ? frames : 1 ) );
}

//-------------------------------------------------------------------------------------------------
/** Looks for other units with an active PointDefenseUpdateV2 of our InterferenceGroup within
	* InterferenceRange -- any player. Every one found is put on cooldown right here; the caller puts
	* us on cooldown when this returns TRUE. Deterministic game logic, so safe in multiplayer. */
//-------------------------------------------------------------------------------------------------
Bool PointDefenseUpdateV2::checkInterference()
{
	const PointDefenseUpdateV2ModuleData *data = getPointDefenseUpdateV2ModuleData();
	if( data->m_interferenceRange <= 0.0f || !isInterferenceShieldUp() )
		return FALSE;

	Object *me = getObject();
	const NameKeyType myModuleKey = getModuleNameKey();	// "PointDefenseUpdateV2"

	// No relationship filter on purpose: own, allied and enemy shields all clash.
	ObjectIterator *iter = ThePartitionManager->iterateObjectsInRange( me->getPosition(), data->m_interferenceRange, FROM_CENTER_2D, nullptr );
	MemoryPoolObjectHolder hold(iter);

	Bool clashed = FALSE;
	for( Object *other = iter->first(); other; other = iter->next() )
	{
		if( other == me || other->isEffectivelyDead() )
			continue;

		// Walk all modules (not findUpdateModule) so an object carrying more than one
		// PointDefenseUpdateV2 has every matching one knocked down.
		for( BehaviorModule** m = other->getBehaviorModules(); *m; ++m )
		{
			if( (*m)->getModuleNameKey() != myModuleKey )
				continue;

			PointDefenseUpdateV2 *otherPD = static_cast<PointDefenseUpdateV2*>( *m );
			if( otherPD->getInterferenceGroup() != data->m_interferenceGroup )
				continue;
			if( !otherPD->isInterferenceShieldUp() )
				continue;

			// Meeting an active shield is a clash for us either way; it only goes down if it can be
			// knocked down (HasInterference = No: immune "superpower" shield -- it still knocks US down).
			if( otherPD->hasInterference() )
				otherPD->startInterferenceCooldown();
			clashed = TRUE;
		}
	}

	return clashed;
}

//-------------------------------------------------------------------------------------------------
void PointDefenseUpdateV2::applyPhaseVisuals( Bool onCooldown, Bool fireEffects )
{
	const PointDefenseUpdateV2ModuleData *data = getPointDefenseUpdateV2ModuleData();
	Object *obj = getObject();

	// Condition flags: clear the other phase's flag first, then set this phase's.
	ModelConditionFlagType setFlag		= onCooldown ? data->m_cooldownConditionState : data->m_activeConditionState;
	ModelConditionFlagType clearFlag	= onCooldown ? data->m_activeConditionState : data->m_cooldownConditionState;

	if( clearFlag != MODELCONDITION_INVALID && clearFlag != setFlag )
		obj->clearModelConditionFlags( MAKE_MODELCONDITION_MASK( clearFlag ) );
	if( setFlag != MODELCONDITION_INVALID )
		obj->setModelConditionState( setFlag );

	// Linked SwitchStateV2 (same lookup AutomaticSwitchStateV2Activate / SwitchStateWhenDamagedBehaviorV2 use).
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

		// Only complain on the initial sync, not on every transition.
		if( !handled && !fireEffects )
		{
			DEBUG_CRASH( ("PointDefenseUpdateV2 for %s has SwitchStateTemplate set, but no SwitchStateV2 with that SpecialPowerTemplate was found on the object",
				obj->getTemplate()->getName().str() ) );
		}
	}
}

//-------------------------------------------------------------------------------------------------
Bool PointDefenseUpdateV2::scanForTargets()
{
	const PointDefenseUpdateV2ModuleData *data = getPointDefenseUpdateV2ModuleData();
	Object *me = getObject();

	WeaponBonus bonus;
	bonus.clear();
	Real fireRange = data->m_weaponTemplate->getAttackRange( bonus );
	Real fireRangeSq = fireRange * fireRange;
	// MinimumInterceptRange, or the weapon's own MinimumAttackRange if that's larger: fireWeaponTemplate()
	// silently refuses targets inside the latter, so never pick one (see fireWhenReady()).
	Real minRange = data->m_minimumInterceptRange;
	if( data->m_weaponTemplate->getMinimumAttackRange() > minRange )
		minRange = data->m_weaponTemplate->getMinimumAttackRange();
	Real minRangeSq = ( minRange > 0.0f ) ? minRange * minRange : 0.0f;

	//OPTIMIZATION: prune to enemies only during the grid walk itself (inside the partition manager),
	//instead of pulling back every nearby object -- including our own side -- and rejecting most of
	//them one at a time afterward. Kind-of and stealth checks stay manual below: they're cheap, and
	//their exact rules (e.g. disguise handling) are subtle enough that reusing an unrelated built-in
	//filter here risked silently changing behavior.
	PartitionFilterRelationship relFilter( me, PartitionFilterRelationship::ALLOW_ENEMIES );
	PartitionFilter *filters[] = { &relFilter, nullptr };

	ObjectIterator *iter = ThePartitionManager->iterateObjectsInRange( me->getPosition(), data->m_scanRange, FROM_CENTER_2D, filters );
	MemoryPoolObjectHolder hold(iter);

	Int cap = (Int)data->m_maxTrackedTargets;
	if( cap <= 0 )
		cap = 1;
	if( cap > PDU2_MAX_TRACKED_TARGETS )
		cap = PDU2_MAX_TRACKED_TARGETS;

	ObjectID inRangeIds[2][PDU2_MAX_TRACKED_TARGETS];
	Real     inRangeDistSq[2][PDU2_MAX_TRACKED_TARGETS];
	Int      inRangeCount[2] = { 0, 0 };

	ObjectID outRangeIds[2][PDU2_MAX_TRACKED_TARGETS];
	Real     outRangeDistSq[2][PDU2_MAX_TRACKED_TARGETS];
	Int      outRangeCount[2] = { 0, 0 };

	Bool sawAnything = false;

	for( Object *other = iter->first(); other; other = iter->next() )
	{
		//The partition filter above already restricted this iterator to enemies, so reaching this
		//line means at least one enemy is inside ScanRange -- enough to justify staying fully awake
		//(see IdleScanRate handling in update()), even if `other` itself gets rejected below by the
		//kind-of/airborne/stealth checks.
		sawAnything = true;

		Int index;
		if( other->isAnyKindOf( data->m_primaryTargetKindOf ) )
			index = 0;
		else if( other->isAnyKindOf( data->m_secondaryTargetKindOf ) )
			index = 1;
		else
			continue;

		// Make sure this weapon's Anti* mask actually allows it to hit this kind of target -- see
		// classifyForAntiMask() above for why this replaces V1's cruder isAirborneTarget()-vs-AntiGround
		// check (that one silently rejected plain ballistic projectiles, since those move via Physics
		// alone and never run through AIUpdate, which is the only code that ever marks an object as an
		// airborne target at all).
		if( !(data->m_weaponTemplate->getAntiMask() & classifyForAntiMask( other )) )
			continue;

		if( other->testStatus( OBJECT_STATUS_STEALTHED ) && !other->testStatus( OBJECT_STATUS_DETECTED ) && !other->testStatus( OBJECT_STATUS_DISGUISED ) )
			continue; //We can't see it.

		Real distSq = ThePartitionManager->getDistanceSquared( me, other, FROM_CENTER_2D );

		//MinimumInterceptRange: the shooter has already breached this inner radius -- don't even
		//bother tracking its shot, just let it through. (fireWhenReady() re-checks this too, since a
		//target we're already tracking can drift inside this radius between scans.)
		if( distSq < minRangeSq )
			continue;

		if( distSq <= fireRangeSq )
		{
			insertSortedCandidate( inRangeIds[index], inRangeDistSq[index], inRangeCount[index], cap, other->getID(), distSq );
			continue;
		}

		//Outside fire range. FIXED: PredictTargetVelocityFactor now actually changes which
		//out-of-range candidate we consider "closest" -- V1 computed this same projected position
		//and then re-measured distance to the target's real (non-projected) position, so the
		//projection had no effect. Here we measure distance to the projected point instead.
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

		insertSortedCandidate( outRangeIds[index], outRangeDistSq[index], outRangeCount[index], cap, other->getID(), sortDistSq );
	}

	//Merge into m_trackedTargets[], same priority order as V1: in-range primary, in-range secondary,
	//out-of-range primary, out-of-range secondary -- just keeping up to `cap` per bucket instead of
	//exactly one.
	m_numTracked = 0;
	mergeBucket( inRangeIds[0], inRangeCount[0] );
	mergeBucket( inRangeIds[1], inRangeCount[1] );
	mergeBucket( outRangeIds[0], outRangeCount[0] );
	mergeBucket( outRangeIds[1], outRangeCount[1] );

	return sawAnything;
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void PointDefenseUpdateV2::crc( Xfer *xfer )
{

	// extend base class
	UpdateModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version
	* 2: GeneralsMod -- block limit / cooldown state (m_blocksThisCycle, m_lastBlockFrame, m_cooldownEndFrame)
	* 3: GeneralsMod -- shield interference (m_nextInterferenceCheckFrame)
	* 4: GeneralsMod -- ShieldHealth (m_currentShieldHealth, m_lastDrainFrame, m_lastRegenFrame) */
// ------------------------------------------------------------------------------------------------
void PointDefenseUpdateV2::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 4;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpdateModule::xfer( xfer );

	// tracked targets
	xfer->xferInt( &m_numTracked );
	for( Int i = 0; i < PDU2_MAX_TRACKED_TARGETS; ++i )
		xfer->xferObjectID( &m_trackedTargets[i] );

	// in range
	xfer->xferBool( &m_inRange );

	// next scan frames
	xfer->xferInt( &m_nextScanFrames );

	// next shot available in frames
	xfer->xferInt( &m_nextShotAvailableInFrames );

	// block limit / cooldown (version 1 saves just keep the constructor defaults: active, count 0)
	if( version >= 2 )
	{
		xfer->xferInt( &m_blocksThisCycle );
		xfer->xferUnsignedInt( &m_lastBlockFrame );
		xfer->xferUnsignedInt( &m_cooldownEndFrame );
	}

	// interference (older saves keep the constructor value: check on the next update)
	if( version >= 3 )
		xfer->xferUnsignedInt( &m_nextInterferenceCheckFrame );

	// ShieldHealth (older saves keep the constructor values: full health)
	if( version >= 4 )
	{
		xfer->xferReal( &m_currentShieldHealth );
		xfer->xferUnsignedInt( &m_lastDrainFrame );
		xfer->xferUnsignedInt( &m_lastRegenFrame );
	}

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void PointDefenseUpdateV2::loadPostProcess()
{

	// extend base class
	UpdateModule::loadPostProcess();

	// Re-assert the restored phase on the visuals / linked SwitchStateV2, silently. Every other
	// module's xfer() has already run by now, so this is idempotent if they restored correctly.
	applyPhaseVisuals( m_cooldownEndFrame != 0, FALSE );
	m_phaseVisualsApplied = TRUE;

}
