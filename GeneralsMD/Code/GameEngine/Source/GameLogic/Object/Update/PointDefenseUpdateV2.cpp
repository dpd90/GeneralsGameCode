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
#include "Common/RandomValue.h"
#include "Common/ThingTemplate.h"
#include "Common/Xfer.h"

#include "GameClient/Drawable.h"

#include "GameLogic/GameLogic.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/Object.h"
#include "GameLogic/ObjectIter.h"
#include "GameLogic/Module/PointDefenseUpdateV2.h"
#include "GameLogic/Module/PhysicsUpdate.h"
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

	// OPTIMIZATION: stagger the very first scan so a batch of these units built or placed on the
	// same frame don't stay locked in step, all doing their expensive full scans on the same tick
	// forever after. Purely cosmetic for load, but costs nothing.
	const PointDefenseUpdateV2ModuleData *data = getPointDefenseUpdateV2ModuleData();
	m_nextScanFrames = ( data->m_scanFrames > 1 ) ? GameLogicRandomValue( 0, (Int)data->m_scanFrames - 1 ) : 0;

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
		return UPDATE_SLEEP_NONE;
	}

	//Periodic scanning (expensive)
	Bool foundAnything = scanForTargets();
	fireWhenReady();

	if( foundAnything )
	{
		m_nextScanFrames = data->m_scanFrames;
		return UPDATE_SLEEP_NONE;
	}

	//No enemies at all within ScanRange.
	m_nextScanFrames = 0;
	if( data->m_idleScanFrames > 0 )
		return UPDATE_SLEEP( data->m_idleScanFrames );

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
	Real minRangeSq = ( data->m_minimumInterceptRange > 0.0f ) ? data->m_minimumInterceptRange * data->m_minimumInterceptRange : 0.0f;

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
		Bool actuallyFired = w->fireWeapon( getObject(), target );
		deleteInstance(w);

		// Only start the reload timer if the shot actually went out. fireWeapon() can still silently
		// refuse internally (WeaponTemplate::fireWeaponTemplate()'s own range checks, e.g. if the
		// weapon still has its own MinimumAttackRange set) -- if we didn't fire, don't burn a reload
		// cycle on nothing, or a single unreachable cached target could jam us against every other
		// target too.
		if( actuallyFired )
			m_nextShotAvailableInFrames = wt->getDelayBetweenShots( fireBonus );
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
	Real minRangeSq = ( data->m_minimumInterceptRange > 0.0f ) ? data->m_minimumInterceptRange * data->m_minimumInterceptRange : 0.0f;

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
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void PointDefenseUpdateV2::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
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

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void PointDefenseUpdateV2::loadPostProcess()
{

	// extend base class
	UpdateModule::loadPostProcess();

}
