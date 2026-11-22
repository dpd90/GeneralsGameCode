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

// FILE: SlavedFollowUpdateV2.h ///////////////////////////////////////////////////////////////////
// Desc:   GeneralsMod @feature Dimitar 02/11/2026 -- strict-follow variant of SlavedUpdate.
//
//         Slaves keep a fixed formation slot around their master instead of wandering. No repair,
//         no drone spotting, no scouting ahead, no "fly over the master's target".
//         Kept from SlavedUpdate: unselectable while slaved, stealth grant from a stealthed master,
//         defect when the master changes sides, crash (DISABLED_UNMANNED) when the master vanishes,
//         go prone on master damage, StayOnSameLayerAsMaster.
//
//         Priorities, evaluated every UpdateRate frames:
//           1. Leash: farther than MaxFollowDistance from the master -> move back to the slot
//              (also cancels an attack when LeashBreaksAttack = Yes).
//           "Engage range" = the slave's largest weapon range + EngageRangeTolerance, measured from
//           the slave's own formation slot. Slaves never chase anything outside it.
//           2. Assist: master has an ENEMY victim within engage range of our slot -> attack it.
//              If the leash had to pull the slave off that victim, or the attack we ordered didn't
//              take (the slave is no longer attacking it on the next check), assist on it pauses for
//              AssistRetryDelay so the slave doesn't ping-pong or freeze.
//           3. Attack in progress: an assist attack is dropped as soon as the master stops attacking
//              that target (Stop, retarget, moving away). Any attack whose target leaves engage
//              range of our slot is dropped. Other attacks need AllowIndependentAttack = Yes.
//           3b. Independent scan (AllowIndependentAttack = Yes): every IndependentScanRate, pick the
//              closest attackable enemy within engage range of our slot and attack it.
//              Set AutoAcquireEnemiesWhenIdle = No on the slave's AIUpdateInterface so the stock AI
//              doesn't chase targets out to VisionRange; this module does the acquiring instead.
//           4. Follow: re-issue the move when the slot drifted RepathThreshold away from the last
//              issued goal, or when idle and not at the slot.
//           Attack stall: attacking something outside weapon range without moving for
//           StuckRepathTime -> drop it, pause that target for AssistRetryDelay, go back to following.
//           Stuck watchdog (steps 1 and 4): a slave that should be heading to its slot but hasn't
//           moved for StuckRepathTime (or is flagged blocked-and-stuck) gets a fresh move to a
//           reachable spot near the slot (findPositionAround, must be pathable by the slave).
//
//         Formation slots are computed live from the slave's position in the master's SpawnBehavior
//         list (SpawnBehaviorInterface::getSlaveIndex), so they rebalance as drones die/respawn.
//
//         Example:
//           Behavior = SlavedFollowUpdateV2 ModuleTag_SlavedFollow
//             FollowDistance          = 30
//             FollowAngleSpread       = 360    ; degrees
//             FollowAngleOffset       = 180    ; degrees, 0 = in front of the master
//             RotateWithMaster        = Yes
//             FollowJitter            = 5
//             MaxFollowDistance       = 80
//             RepathThreshold         = 15
//             UpdateRate              = 2      ; logic frames
//             AllowIndependentAttack  = Yes
//             AssistMasterTarget      = Yes
//             LeashBreaksAttack       = Yes
//             AssistRetryDelay        = 2000   ; ms
//             IndependentScanRate     = 500    ; ms
//             EngageRangeTolerance    = 10
//             TransferExperienceToMaster = Yes ; kills by the slave give XP to the master
//             StuckRepathTime         = 1000   ; ms without progress before a move is re-issued
//             StayOnSameLayerAsMaster = No
//           End
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Common/INI.h"
#include "GameLogic/Module/UpdateModule.h"

class DamageInfo;

//-------------------------------------------------------------------------------------------------
class SlavedFollowUpdateV2ModuleData : public UpdateModuleData
{
public:
	Real m_followDistance;					///< radius of the formation slot around the master
	Real m_followAngleSpread;				///< radians; arc the slots are spread over (2*PI = full ring)
	Real m_followAngleOffset;				///< radians; centre of the arc, 0 = master's facing
	Bool m_rotateWithMaster;				///< slots turn with the master's facing
	Real m_followJitter;						///< random wobble added to each issued move goal
	Real m_maxFollowDistance;				///< leash; 0 = no leash
	Real m_repathThreshold;					///< slot must drift this far before a new move is issued
	UnsignedInt m_updateRate;				///< logic frames between checks (1 = every frame)
	Bool m_allowIndependentAttack;	///< may keep attacking targets it acquired itself
	Bool m_assistMasterTarget;			///< attack the master's current victim when reachable
	Bool m_leashBreaksAttack;				///< leaving the leash cancels any attack
	UnsignedInt m_assistRetryDelay;	///< frames assist on a target pauses after the leash pulled us off it
	UnsignedInt m_independentScanRate;	///< frames between independent target scans
	Real m_engageRangeTolerance;		///< added to weapon range when deciding what's in reach of our slot
	Bool m_transferExperienceToMaster;	///< route the slave's XP to the master (ExperienceTracker sink)
	UnsignedInt m_stuckRepathTime;		///< frames without progress before a stuck move is re-issued
	Bool m_stayOnSameLayerAsMaster;

	SlavedFollowUpdateV2ModuleData()
	{
		m_followDistance = 30.0f;
		m_followAngleSpread = 2.0f * PI;
		m_followAngleOffset = PI;
		m_rotateWithMaster = TRUE;
		m_followJitter = 0.0f;
		m_maxFollowDistance = 80.0f;
		m_repathThreshold = 15.0f;
		m_updateRate = 2;
		m_allowIndependentAttack = TRUE;
		m_assistMasterTarget = TRUE;
		m_leashBreaksAttack = TRUE;
		m_assistRetryDelay = LOGICFRAMES_PER_SECOND * 2;
		m_independentScanRate = LOGICFRAMES_PER_SECOND / 2;
		m_engageRangeTolerance = 10.0f;
		m_transferExperienceToMaster = TRUE;
		m_stuckRepathTime = LOGICFRAMES_PER_SECOND;
		m_stayOnSameLayerAsMaster = FALSE;
	}

	static void buildFieldParse(MultiIniFieldParse& p)
	{
		UpdateModuleData::buildFieldParse(p);
		static const FieldParse dataFieldParse[] =
		{
			{ "FollowDistance",						INI::parseReal,					nullptr, offsetof( SlavedFollowUpdateV2ModuleData, m_followDistance ) },
			{ "FollowAngleSpread",				INI::parseAngleReal,		nullptr, offsetof( SlavedFollowUpdateV2ModuleData, m_followAngleSpread ) },
			{ "FollowAngleOffset",				INI::parseAngleReal,		nullptr, offsetof( SlavedFollowUpdateV2ModuleData, m_followAngleOffset ) },
			{ "RotateWithMaster",					INI::parseBool,					nullptr, offsetof( SlavedFollowUpdateV2ModuleData, m_rotateWithMaster ) },
			{ "FollowJitter",							INI::parseReal,					nullptr, offsetof( SlavedFollowUpdateV2ModuleData, m_followJitter ) },
			{ "MaxFollowDistance",				INI::parseReal,					nullptr, offsetof( SlavedFollowUpdateV2ModuleData, m_maxFollowDistance ) },
			{ "RepathThreshold",					INI::parseReal,					nullptr, offsetof( SlavedFollowUpdateV2ModuleData, m_repathThreshold ) },
			{ "UpdateRate",								INI::parseUnsignedInt,	nullptr, offsetof( SlavedFollowUpdateV2ModuleData, m_updateRate ) },
			{ "AllowIndependentAttack",		INI::parseBool,					nullptr, offsetof( SlavedFollowUpdateV2ModuleData, m_allowIndependentAttack ) },
			{ "AssistMasterTarget",				INI::parseBool,					nullptr, offsetof( SlavedFollowUpdateV2ModuleData, m_assistMasterTarget ) },
			{ "LeashBreaksAttack",				INI::parseBool,					nullptr, offsetof( SlavedFollowUpdateV2ModuleData, m_leashBreaksAttack ) },
			{ "AssistRetryDelay",					INI::parseDurationUnsignedInt,	nullptr, offsetof( SlavedFollowUpdateV2ModuleData, m_assistRetryDelay ) },
			{ "IndependentScanRate",			INI::parseDurationUnsignedInt,	nullptr, offsetof( SlavedFollowUpdateV2ModuleData, m_independentScanRate ) },
			{ "EngageRangeTolerance",			INI::parseReal,					nullptr, offsetof( SlavedFollowUpdateV2ModuleData, m_engageRangeTolerance ) },
			{ "TransferExperienceToMaster",	INI::parseBool,					nullptr, offsetof( SlavedFollowUpdateV2ModuleData, m_transferExperienceToMaster ) },
			{ "StuckRepathTime",					INI::parseDurationUnsignedInt,	nullptr, offsetof( SlavedFollowUpdateV2ModuleData, m_stuckRepathTime ) },
			{ "StayOnSameLayerAsMaster",	INI::parseBool,					nullptr, offsetof( SlavedFollowUpdateV2ModuleData, m_stayOnSameLayerAsMaster ) },
			{ nullptr, nullptr, nullptr, 0 }
		};
		p.add(dataFieldParse);
	}
};

//-------------------------------------------------------------------------------------------------
class SlavedFollowUpdateV2 : public UpdateModule, public SlavedUpdateInterface
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( SlavedFollowUpdateV2, "SlavedFollowUpdateV2" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( SlavedFollowUpdateV2, SlavedFollowUpdateV2ModuleData )

public:

	SlavedFollowUpdateV2( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	virtual SlavedUpdateInterface* getSlavedUpdateInterface() override { return this; }

	virtual ObjectID getSlaverID() const override { return m_slaver; }
	virtual void onEnslave( const Object *slaver ) override;
	virtual void onSlaverDie( const DamageInfo *info ) override;
	virtual void onSlaverDamage( const DamageInfo *info ) override;
	virtual Bool isSelfTasking() const override { return FALSE; }

	virtual UpdateSleepTime update() override;

private:

	void startSlavedEffects( const Object *slaver );
	void stopSlavedEffects();

	void computeFollowPoint( const Object *master, Coord3D *outPos );
	void issueFollowMove( const Coord3D *followPoint, Bool findReachableSpot = FALSE );
	Bool isStuck( const Coord3D *followPoint, UnsignedInt now );
	Bool isAttackStalled( const Object *target, UnsignedInt now );
	void pauseTargetAndReturn( ObjectID targetID, const Coord3D *followPoint, UnsignedInt now );
	Bool canAssistAgainst( Object *target, const Coord3D *followPoint );
	Bool isInEngageRange( const Object *target, const Coord3D *followPoint );
	Object *findIndependentTarget( const Coord3D *followPoint );

	ObjectID m_slaver;							///< to whom we are enslaved
	ObjectID m_assistTargetID;			///< master's victim we were last ordered to attack
	ObjectID m_assistBlockedTargetID;	///< target paused for AssistRetryDelay (leash pulled us off / attack didn't take)
	ObjectID m_lastOrderedAttackID;	///< last target we ordered an attack on (assist or scan)
	UnsignedInt m_assistBlockedUntil;	///< frame until which that victim is not assisted
	Coord3D m_lastFollowPoint;			///< un-jittered slot position of the last issued move
	Bool m_hasLastFollowPoint;
	UnsignedInt m_framesToWait;
	UnsignedInt m_nextScanFrame;		///< next independent scan
	Coord3D m_lastCheckPos;					///< our position at the previous stuck check
	UnsignedInt m_lastCheckFrame;		///< frame of the previous stuck check
	UnsignedInt m_stuckFrames;			///< frames spent not progressing toward the slot
	Coord3D m_attackCheckPos;				///< our position at the previous attack-stall check
	UnsignedInt m_attackCheckFrame;	///< frame of the previous attack-stall check
	UnsignedInt m_attackStallFrames;	///< frames spent out of weapon range without moving while attacking

};
