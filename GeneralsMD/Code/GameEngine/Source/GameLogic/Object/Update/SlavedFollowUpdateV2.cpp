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

// FILE: SlavedFollowUpdateV2.cpp /////////////////////////////////////////////////////////////////
// Desc:   GeneralsMod @feature Dimitar 02/11/2026 -- see SlavedFollowUpdateV2.h for the INI fields.
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/Team.h"
#include "Common/Xfer.h"
#include "GameLogic/Damage.h"
#include "GameLogic/AI.h"
#include "GameLogic/ExperienceTracker.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/LogicRandomValue.h"
#include "GameLogic/Object.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/TerrainLogic.h"
#include "GameLogic/Weapon.h"
#include "GameLogic/WeaponSet.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/SlavedFollowUpdateV2.h"
#include "GameLogic/Module/SpawnBehavior.h"
#include "GameLogic/Module/StealthUpdate.h"

namespace
{
	// Only targets within range of a point (our formation slot), so attacking them never makes us chase.
	class PartitionFilterWithinRangeOfPoint : public PartitionFilter
	{
	public:
		PartitionFilterWithinRangeOfPoint( const Coord3D *pos, Real range ) : m_pos( *pos ), m_rangeSqr( range * range ) { }
		virtual Bool allow( Object *objOther ) override
		{
			return ThePartitionManager->getDistanceSquared( objOther, &m_pos, FROM_BOUNDINGSPHERE_2D ) <= m_rangeSqr;
		}
#if defined(RTS_DEBUG)
		virtual const char* debugGetName() override { return "PartitionFilterWithinRangeOfPoint"; }
#endif
	private:
		Coord3D m_pos;
		Real m_rangeSqr;
	};
}

//-------------------------------------------------------------------------------------------------
SlavedFollowUpdateV2::SlavedFollowUpdateV2( Thing *thing, const ModuleData* moduleData ) : UpdateModule( thing, moduleData )
{
	m_slaver = INVALID_ID;
	m_assistTargetID = INVALID_ID;
	m_assistBlockedTargetID = INVALID_ID;
	m_lastOrderedAttackID = INVALID_ID;
	m_assistBlockedUntil = 0;
	m_lastFollowPoint.zero();
	m_hasLastFollowPoint = FALSE;
	m_framesToWait = 0;
	m_nextScanFrame = 0;
	m_lastCheckPos.zero();
	m_lastCheckFrame = 0;
	m_stuckFrames = 0;
	m_attackCheckPos.zero();
	m_attackCheckFrame = 0;
	m_attackStallFrames = 0;
}

//-------------------------------------------------------------------------------------------------
SlavedFollowUpdateV2::~SlavedFollowUpdateV2()
{
}

//-------------------------------------------------------------------------------------------------
void SlavedFollowUpdateV2::onEnslave( const Object *slaver )
{
	startSlavedEffects( slaver );
}

//-------------------------------------------------------------------------------------------------
void SlavedFollowUpdateV2::onSlaverDie( const DamageInfo *info )
{
	stopSlavedEffects();
}

//-------------------------------------------------------------------------------------------------
void SlavedFollowUpdateV2::onSlaverDamage( const DamageInfo *info )
{
	// Only slaves with a ProneUpdate will even care.
	AIUpdateInterface *ai = getObject()->getAIUpdateInterface();
	if( ai )
		ai->aiGoProne( info, CMD_FROM_AI );
}

//-------------------------------------------------------------------------------------------------
UpdateSleepTime SlavedFollowUpdateV2::update()
{
	const SlavedFollowUpdateV2ModuleData* data = getSlavedFollowUpdateV2ModuleData();

	if( m_framesToWait > 0 )
	{
		--m_framesToWait;
		return UPDATE_SLEEP_NONE;
	}
	m_framesToWait = data->m_updateRate > 1 ? data->m_updateRate - 1 : 0;

	if( m_slaver == INVALID_ID )
		return UPDATE_SLEEP_NONE;

	Object *me = getObject();
	AIUpdateInterface *myAI = me->getAIUpdateInterface();
	if( !myAI || !myAI->getCurLocomotor() )
		return UPDATE_SLEEP_NONE;

	Object *master = TheGameLogic->findObjectByID( m_slaver );
	if( !master || master->isEffectivelyDead() || master->isDisabledByType( DISABLED_UNMANNED ) )
	{
		// Same as SlavedUpdate: disable the drone so it crashes.
		stopSlavedEffects();
		me->setDisabled( DISABLED_UNMANNED );
		myAI->aiIdle( CMD_FROM_AI );
		return UPDATE_SLEEP_NONE;
	}

	// Master was hijacked/captured -- follow him to his new team.
	if( master->getTeam()->getRelationship( me->getTeam() ) != ALLIES )
		me->defect( master->getTeam(), 0 );

	if( data->m_stayOnSameLayerAsMaster )
		me->setLayer( master->getLayer() );

	Coord3D followPoint;
	computeFollowPoint( master, &followPoint );

	const Bool attacking = myAI->isAttacking();

	const UnsignedInt now = TheGameLogic->getFrame();
	Object *goal = attacking ? myAI->getGoalObject() : nullptr;
	const Bool assistAttack = attacking && goal && m_assistTargetID != INVALID_ID && goal->getID() == m_assistTargetID;

	// An attack we ordered (assist or scan) is no longer running: it didn't take, or it ended.
	// Pause that target for AssistRetryDelay instead of re-ordering it every check -- re-ordering a
	// failing attack each check froze the slave in place while the master walked away.
	if( m_lastOrderedAttackID != INVALID_ID && !attacking )
	{
		m_assistBlockedTargetID = m_lastOrderedAttackID;
		m_assistBlockedUntil = now + data->m_assistRetryDelay;
		m_lastOrderedAttackID = INVALID_ID;
		m_assistTargetID = INVALID_ID;
	}

	// 1. Leash.
	if( data->m_maxFollowDistance > 0.0f )
	{
		Real distSqr = ThePartitionManager->getDistanceSquared( me, master, FROM_CENTER_2D );
		if( distSqr > sqr( data->m_maxFollowDistance ) && ( data->m_leashBreaksAttack || !attacking ) )
		{
			if( assistAttack )
			{
				// Pulled off the master's target -- don't send us straight back at it.
				m_assistBlockedTargetID = m_assistTargetID;
				m_assistBlockedUntil = now + data->m_assistRetryDelay;
			}
			m_assistTargetID = INVALID_ID;

			// Only re-issue the return move when needed, not every check.
			Bool slotDrifted = !m_hasLastFollowPoint;
			if( m_hasLastFollowPoint )
			{
				Real dx = followPoint.x - m_lastFollowPoint.x;
				Real dy = followPoint.y - m_lastFollowPoint.y;
				slotDrifted = ( dx*dx + dy*dy ) > sqr( data->m_repathThreshold );
			}
			if( attacking || slotDrifted || !myAI->isMoving() )
				issueFollowMove( &followPoint );
			else if( isStuck( &followPoint, now ) )
				issueFollowMove( &followPoint, TRUE );
			return UPDATE_SLEEP_NONE;
		}
	}

	// 2. Assist the master's current victim.
	if( data->m_assistMasterTarget )
	{
		AIUpdateInterface *masterAI = master->getAIUpdateInterface();
		Object *masterTarget = masterAI ? masterAI->getCurrentVictim() : nullptr;

		Bool blocked = masterTarget && masterTarget->getID() == m_assistBlockedTargetID && now < m_assistBlockedUntil;
		if( masterTarget && !blocked && canAssistAgainst( masterTarget, &followPoint ) )
		{
			// Only (re)issue on a target change, so we don't restart it every check.
			if( m_assistTargetID != masterTarget->getID() )
			{
				// A human player's unit refuses to approach an out-of-range target for a CMD_FROM_AI attack
				// (treated as auto-acquire) unless allowed to chase -- AIAttackApproachTargetState::onEnter.
				// Same opt-in as AssaultTransportAIUpdate / HealAIUpdateV2. Not saved by AIUpdate, so set
				// it on every order.
				myAI->setAllowedToChase( TRUE );
				if( me->getRelationship( masterTarget ) == ENEMIES )
					myAI->aiAttackObject( masterTarget, NO_MAX_SHOTS_LIMIT, CMD_FROM_AI );
				else
					myAI->aiForceAttackObject( masterTarget, NO_MAX_SHOTS_LIMIT, CMD_FROM_AI );
				m_assistTargetID = masterTarget->getID();
				m_lastOrderedAttackID = masterTarget->getID();
				m_hasLastFollowPoint = FALSE;
			}
			else if( isAttackStalled( masterTarget, now ) )
				pauseTargetAndReturn( masterTarget->getID(), &followPoint, now );
			return UPDATE_SLEEP_NONE;
		}
	}

	// 3. An attack already in progress.
	//    Reaching here with an assist attack means the master no longer wants that target
	//    (Stop, retarget, moved out of reach) -- drop it. Other attacks (our scan, or the stock AI's
	//    own auto-acquire) are kept only while allowed and still within engage range of our slot.
	if( attacking )
	{
		if( !assistAttack && data->m_allowIndependentAttack && goal && !goal->isEffectivelyDead() && isInEngageRange( goal, &followPoint ) )
		{
			if( isAttackStalled( goal, now ) )
				pauseTargetAndReturn( goal->getID(), &followPoint, now );
			return UPDATE_SLEEP_NONE;
		}

		m_assistTargetID = INVALID_ID;
		issueFollowMove( &followPoint );
		return UPDATE_SLEEP_NONE;
	}

	// 3b. Look for something to shoot that doesn't take us off the leash.
	if( data->m_allowIndependentAttack && now >= m_nextScanFrame )
	{
		m_nextScanFrame = now + ( data->m_independentScanRate > 0 ? data->m_independentScanRate : 1 );
		Object *victim = findIndependentTarget( &followPoint );
		if( victim && victim->getID() == m_assistBlockedTargetID && now < m_assistBlockedUntil )
			victim = nullptr;	// paused: its last attack didn't take
		if( victim )
		{
			m_assistTargetID = INVALID_ID;
			myAI->setAllowedToChase( TRUE );	// see the assist order above
			myAI->aiAttackObject( victim, NO_MAX_SHOTS_LIMIT, CMD_FROM_AI );
			m_lastOrderedAttackID = victim->getID();
			m_hasLastFollowPoint = FALSE;
			return UPDATE_SLEEP_NONE;
		}
	}
	m_assistTargetID = INVALID_ID;

	// 4. Follow.
	const Real repathSqr = sqr( data->m_repathThreshold );
	Bool slotDrifted = TRUE;
	if( m_hasLastFollowPoint )
	{
		Real dx = followPoint.x - m_lastFollowPoint.x;
		Real dy = followPoint.y - m_lastFollowPoint.y;
		slotDrifted = ( dx*dx + dy*dy ) > repathSqr;
	}

	if( slotDrifted )
		issueFollowMove( &followPoint );
	else if( myAI->isIdle() && ThePartitionManager->getDistanceSquared( me, &followPoint, FROM_CENTER_2D ) > repathSqr )
	{
		// Arrived somewhere else (bumped, finished an attack, ...) -- go back to the slot.
		issueFollowMove( &followPoint );
	}
	else if( isStuck( &followPoint, now ) )
	{
		// "Moving" but going nowhere (path failed, slot on an impassable spot, blocked...).
		issueFollowMove( &followPoint, TRUE );
	}

	return UPDATE_SLEEP_NONE;
}

//-------------------------------------------------------------------------------------------------
// Slot = master position + FollowDistance along an angle picked from our index in the master's
// slave list. The list is live, so the formation rebalances when slaves die or respawn.
//-------------------------------------------------------------------------------------------------
void SlavedFollowUpdateV2::computeFollowPoint( const Object *master, Coord3D *outPos )
{
	const SlavedFollowUpdateV2ModuleData* data = getSlavedFollowUpdateV2ModuleData();
	Object *me = getObject();

	Int index = -1;
	Int count = 0;
	SpawnBehaviorInterface *spawnInterface = master->getSpawnBehaviorInterface();
	if( spawnInterface )
		index = spawnInterface->getSlaveIndex( me->getID(), &count );

	if( index < 0 || count <= 0 )
	{
		// Enslaved by something other than SpawnBehavior (e.g. an OCL): spread by ID.
		count = 8;
		index = (Int)( (UnsignedInt)me->getID() % (UnsignedInt)count );
	}

	const Real spread = data->m_followAngleSpread;
	Real angle = data->m_followAngleOffset;
	if( spread >= 2.0f * PI - 0.001f )
	{
		// Full ring: no slot on the seam twice.
		angle += spread * (Real)index / (Real)count;
	}
	else if( count > 1 )
	{
		angle += -0.5f * spread + spread * (Real)index / (Real)( count - 1 );
	}

	if( data->m_rotateWithMaster )
		angle += master->getOrientation();

	*outPos = *master->getPosition();
	outPos->x += data->m_followDistance * Cos( angle );
	outPos->y += data->m_followDistance * Sin( angle );
	outPos->z = TheTerrainLogic->getGroundHeight( outPos->x, outPos->y );
}

//-------------------------------------------------------------------------------------------------
void SlavedFollowUpdateV2::issueFollowMove( const Coord3D *followPoint, Bool findReachableSpot )
{
	const SlavedFollowUpdateV2ModuleData* data = getSlavedFollowUpdateV2ModuleData();
	AIUpdateInterface *ai = getObject()->getAIUpdateInterface();
	if( !ai )
		return;

	Coord3D goal = *followPoint;
	if( data->m_followJitter > 0.0f )
	{
		Real dir = GameLogicRandomValueReal( 0.0f, 2.0f * PI );
		Real len = GameLogicRandomValueReal( 0.0f, data->m_followJitter );
		goal.x += len * Cos( dir );
		goal.y += len * Sin( dir );
		goal.z = TheTerrainLogic->getGroundHeight( goal.x, goal.y );
	}

	// An AI-sourced move on a non-idle unit is only a *temporary* 20 s move layered on top of the
	// current state (AIUpdateInterface::privateMoveToPosition) -- an attack is NOT cancelled and
	// resumes afterwards. Go idle first so the move really replaces whatever we were doing.
	if( !ai->isIdle() )
		ai->aiIdle( CMD_FROM_AI );
	if( findReachableSpot )
	{
		// Retry after getting stuck: pick a spot near the slot that we can actually path to.
		FindPositionOptions fpOptions;
		fpOptions.maxRadius = data->m_followDistance > 20.0f ? data->m_followDistance : 20.0f;
		fpOptions.sourceToPathToDest = getObject();
		Coord3D reachable;
		if( ThePartitionManager->findPositionAround( followPoint, &fpOptions, &reachable ) )
			goal = reachable;
		else
		{
			// Nothing reachable around the slot -- head for the master instead.
			Object *master = TheGameLogic->findObjectByID( m_slaver );
			if( master )
				goal = *master->getPosition();
		}
	}

	ai->aiMoveToPosition( &goal, CMD_FROM_AI );
	m_lastOrderedAttackID = INVALID_ID;	// we cancelled it ourselves
	m_stuckFrames = 0;
	m_lastCheckFrame = 0;

	// Remember the un-jittered slot so jitter never counts as drift.
	m_lastFollowPoint = *followPoint;
	m_hasLastFollowPoint = TRUE;
}

//-------------------------------------------------------------------------------------------------
// Should be heading to the slot but not getting anywhere? Called only when no new move is about
// to be issued anyway. Progress = moved at least 1 unit since the previous check.
//-------------------------------------------------------------------------------------------------
Bool SlavedFollowUpdateV2::isStuck( const Coord3D *followPoint, UnsignedInt now )
{
	const SlavedFollowUpdateV2ModuleData* data = getSlavedFollowUpdateV2ModuleData();
	Object *me = getObject();
	AIUpdateInterface *ai = me->getAIUpdateInterface();

	const Coord3D *pos = me->getPosition();
	Real dx = pos->x - m_lastCheckPos.x;
	Real dy = pos->y - m_lastCheckPos.y;
	UnsignedInt elapsed = ( m_lastCheckFrame != 0 && now > m_lastCheckFrame ) ? now - m_lastCheckFrame : 0;
	// Gap since the last check (we were attacking, or a move was just issued): start measuring afresh.
	const UnsignedInt checkGap = data->m_updateRate > 1 ? data->m_updateRate : 1;
	if( elapsed > 2 * checkGap )
		elapsed = 0;
	m_lastCheckPos = *pos;
	m_lastCheckFrame = now;

	Bool atSlot = ThePartitionManager->getDistanceSquared( me, followPoint, FROM_CENTER_2D ) <= sqr( data->m_repathThreshold );
	if( atSlot || !ai || ai->isAttacking() )
	{
		m_stuckFrames = 0;
		return FALSE;
	}

	if( ai->isBlockedAndStuck() )
		return TRUE;

	if( dx*dx + dy*dy < 1.0f )
		m_stuckFrames += elapsed;
	else
		m_stuckFrames = 0;

	return m_stuckFrames >= data->m_stuckRepathTime;
}

//-------------------------------------------------------------------------------------------------
// Attacking something outside our weapon range, but not moving toward it (failed path, can't turn,
// blocked...) for StuckRepathTime? Within weapon range we're aiming/firing, so never "stalled".
//-------------------------------------------------------------------------------------------------
Bool SlavedFollowUpdateV2::isAttackStalled( const Object *target, UnsignedInt now )
{
	const SlavedFollowUpdateV2ModuleData* data = getSlavedFollowUpdateV2ModuleData();
	Object *me = getObject();

	const Coord3D *pos = me->getPosition();
	Real dx = pos->x - m_attackCheckPos.x;
	Real dy = pos->y - m_attackCheckPos.y;
	UnsignedInt elapsed = ( m_attackCheckFrame != 0 && now > m_attackCheckFrame ) ? now - m_attackCheckFrame : 0;
	const UnsignedInt checkGap = data->m_updateRate > 1 ? data->m_updateRate : 1;
	if( elapsed > 2 * checkGap )
		elapsed = 0;
	m_attackCheckPos = *pos;
	m_attackCheckFrame = now;

	Real inRange = me->getLargestWeaponRange() + 5.0f;
	if( !target || ThePartitionManager->getDistanceSquared( me, target, FROM_BOUNDINGSPHERE_2D ) <= sqr( inRange ) )
	{
		m_attackStallFrames = 0;
		return FALSE;
	}

	if( dx*dx + dy*dy < 1.0f )
		m_attackStallFrames += elapsed;
	else
		m_attackStallFrames = 0;

	return m_attackStallFrames >= data->m_stuckRepathTime;
}

//-------------------------------------------------------------------------------------------------
void SlavedFollowUpdateV2::pauseTargetAndReturn( ObjectID targetID, const Coord3D *followPoint, UnsignedInt now )
{
	const SlavedFollowUpdateV2ModuleData* data = getSlavedFollowUpdateV2ModuleData();
	m_assistBlockedTargetID = targetID;
	m_assistBlockedUntil = now + data->m_assistRetryDelay;
	m_assistTargetID = INVALID_ID;
	m_attackStallFrames = 0;
	m_attackCheckFrame = 0;
	issueFollowMove( followPoint, TRUE );
}

//-------------------------------------------------------------------------------------------------
// Can we hit the master's victim from our slot?
//-------------------------------------------------------------------------------------------------
Bool SlavedFollowUpdateV2::canAssistAgainst( Object *target, const Coord3D *followPoint )
{
	Object *me = getObject();

	if( target->isEffectivelyDead() || !me->isAbleToAttack() )
		return FALSE;

	// Non-enemies (master force-attacking an ally/neutral/own object) need a forced attack.
	AbleToAttackType attackType = ( me->getRelationship( target ) == ENEMIES ) ? ATTACK_NEW_TARGET : ATTACK_NEW_TARGET_FORCED;
	if( me->getAbleToAttackSpecificObject( attackType, target, CMD_FROM_AI ) == ATTACKRESULT_NOT_POSSIBLE )
		return FALSE;

	return isInEngageRange( target, followPoint );
}

//-------------------------------------------------------------------------------------------------
// Within our weapon range (+ tolerance) of our formation slot? Measured to the target's bounding
// sphere, like weapon range, so big structures count from their edge.
//-------------------------------------------------------------------------------------------------
Bool SlavedFollowUpdateV2::isInEngageRange( const Object *target, const Coord3D *followPoint )
{
	const SlavedFollowUpdateV2ModuleData* data = getSlavedFollowUpdateV2ModuleData();
	Real range = getObject()->getLargestWeaponRange() + data->m_engageRangeTolerance;
	return ThePartitionManager->getDistanceSquared( target, followPoint, FROM_BOUNDINGSPHERE_2D ) <= sqr( range );
}

//-------------------------------------------------------------------------------------------------
// Closest attackable, visible, unfogged enemy within engage range of our slot (stock AI target rules).
//-------------------------------------------------------------------------------------------------
Object *SlavedFollowUpdateV2::findIndependentTarget( const Coord3D *followPoint )
{
	const SlavedFollowUpdateV2ModuleData* data = getSlavedFollowUpdateV2ModuleData();
	Object *me = getObject();
	if( !me->isAbleToAttack() )
		return nullptr;

	Real range = me->getLargestWeaponRange() + data->m_engageRangeTolerance;

	// Search around ourselves far enough to cover the whole circle around the slot.
	Real dx = me->getPosition()->x - followPoint->x;
	Real dy = me->getPosition()->y - followPoint->y;
	Real searchRange = range + (Real)sqrtf( dx*dx + dy*dy );

	PartitionFilterWithinRangeOfPoint filterRange( followPoint, range );
	return TheAI->findClosestEnemy( me, searchRange, AI::CAN_SEE | AI::CAN_ATTACK | AI::UNFOGGED, nullptr, &filterRange );
}

//-------------------------------------------------------------------------------------------------
void SlavedFollowUpdateV2::startSlavedEffects( const Object *slaver )
{
	if( slaver == nullptr )
		return;

	m_slaver = slaver->getID();
	m_assistTargetID = INVALID_ID;
	m_hasLastFollowPoint = FALSE;

	// mark selves as not selectable
	getObject()->setStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_UNSELECTABLE ) );

	// Kills by the slave count for the master (same mechanism Overlord/Helix riders use).
	// The sink is saved by ExperienceTracker itself. If the master dies, the sink lookup fails and
	// XP falls back to the slave (which normally isn't trainable, so it's simply lost).
	if( getSlavedFollowUpdateV2ModuleData()->m_transferExperienceToMaster && getObject()->getExperienceTracker() )
		getObject()->getExperienceTracker()->setExperienceSink( slaver->getID() );

	if( slaver->testStatus( OBJECT_STATUS_STEALTHED ) )
	{
		StealthUpdate *myStealth = getObject()->getStealth();
		if( myStealth )
			myStealth->receiveGrant( true );
	}
}

//-------------------------------------------------------------------------------------------------
void SlavedFollowUpdateV2::stopSlavedEffects()
{
	// Stop routing XP to a master we no longer serve (only if it's the sink we set).
	ExperienceTracker *xp = getObject()->getExperienceTracker();
	if( xp && m_slaver != INVALID_ID && xp->getExperienceSink() == m_slaver )
		xp->setExperienceSink( INVALID_ID );

	m_slaver = INVALID_ID;
	m_assistTargetID = INVALID_ID;
	m_hasLastFollowPoint = FALSE;

	getObject()->clearStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_UNSELECTABLE ) );
	getObject()->clearDisabled( DISABLED_HELD );
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void SlavedFollowUpdateV2::crc( Xfer *xfer )
{
	UpdateModule::crc( xfer );
}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version
	* 2: Assist retry block
	* 3: Independent scan frame
	* 4: Stuck watchdog
	* 5: Last ordered attack, attack-stall watchdog */
// ------------------------------------------------------------------------------------------------
void SlavedFollowUpdateV2::xfer( Xfer *xfer )
{
	XferVersion currentVersion = 5;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	UpdateModule::xfer( xfer );

	xfer->xferObjectID( &m_slaver );
	xfer->xferObjectID( &m_assistTargetID );
	xfer->xferCoord3D( &m_lastFollowPoint );
	xfer->xferBool( &m_hasLastFollowPoint );
	xfer->xferUnsignedInt( &m_framesToWait );
	if( version >= 2 )
	{
		xfer->xferObjectID( &m_assistBlockedTargetID );
		xfer->xferUnsignedInt( &m_assistBlockedUntil );
	}
	if( version >= 3 )
		xfer->xferUnsignedInt( &m_nextScanFrame );
	if( version >= 4 )
	{
		xfer->xferCoord3D( &m_lastCheckPos );
		xfer->xferUnsignedInt( &m_lastCheckFrame );
		xfer->xferUnsignedInt( &m_stuckFrames );
	}
	if( version >= 5 )
	{
		xfer->xferObjectID( &m_lastOrderedAttackID );
		xfer->xferCoord3D( &m_attackCheckPos );
		xfer->xferUnsignedInt( &m_attackCheckFrame );
		xfer->xferUnsignedInt( &m_attackStallFrames );
	}
}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void SlavedFollowUpdateV2::loadPostProcess()
{
	UpdateModule::loadPostProcess();
}
