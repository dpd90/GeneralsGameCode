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

// FILE:		MissileAIUpdateV2.h
// Author:	Michael S. Booth, December 2001
// Desc:		Missile behavior
//
// GeneralsMod @feature Dimitar 25/09/2026: MissileAIUpdateV2 -- a copy of vanilla MissileAIUpdate
// (behaves identically) plus what happens when a TRACKED target (TryToFollowTarget = Yes) dies:
//
//   DetonateAtLastTargetPosition = Yes   ; default Yes. When the target object is destroyed, fly to its
//                                        ; last tracked position and detonate there (mid-air too). No =
//                                        ; vanilla behavior (missile silently vanishes, no detonation).
//   ShouldRetarget = No                  ; default No. Yes = the moment the target becomes effectively
//                                        ; dead, scan once for a new target. Found -> chase it (and repeat
//                                        ; when that one dies). Not found -> keep following the wreck /
//                                        ; falling hull, then DetonateAtLastTargetPosition when it's gone.
//   RetargetRange = 150                  ; scan radius, measured from the dead target's position
//   MaxRetargets = 0                     ; 0 = unlimited
//   RetargetRequiredKindOf = VEHICLE AIRCRAFT ; optional; candidate must match at least ONE listed KindOf
//   RetargetForbiddenKindOf = INFANTRY   ; optional; candidate must match NONE of these
//   RetargetCanBeDecoyed = Yes           ; default Yes. A new target with countermeasures (flares) gets a
//                                        ; fresh chance to decoy the missile, same rules as at launch.
//
// Candidates must also be enemies, alive, not stealthed-and-undetected, not contained, and legal for the
// firing weapon's Anti* mask (AntiGround/AntiAirborneVehicle/...). Decoyed missiles never retarget; they
// detonate (without damage) at the flare's last position. Position shots (TryToFollowTarget = No, or a
// weapon ScatterRadius) are unaffected by all of the above.

#pragma once

#include "Common/GameType.h"
#include "Common/GlobalData.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/WeaponBonusConditionFlags.h"
#include "Common/INI.h"
#include "WWMath/matrix3d.h"

enum ParticleSystemID CPP_11(: Int);
class FXList;


//-------------------------------------------------------------------------------------------------
class MissileAIUpdateV2ModuleData : public AIUpdateModuleData
{
public:
	Bool						m_tryToFollowTarget;	///< if true, attack object, not pos
	UnsignedInt			m_fuelLifetime;				///< num frames till missile runs out of motive power (0 == inf)
	UnsignedInt			m_ignitionDelay;			///< delay in frames from when missile is 'fired', to when it starts moving		15
	Real						m_initialVel;
	Real						m_initialDist;
	Real						m_diveDistance;				///< If I get this close to my target, start ignoring my preferred height
	const FXList*		m_ignitionFX;					///< FXList to do when missile 'ignites'
	Bool						m_useWeaponSpeed;			///< if true, limit speed of projectile to the Weapon's info
	Bool						m_detonateOnNoFuel;		///< If true, don't just stop thrusting, blow up when out of gas
	Int							m_garrisonHitKillCount;
	KindOfMaskType	m_garrisonHitKillKindof;			///< the kind(s) of units that can be collided with
	KindOfMaskType	m_garrisonHitKillKindofNot;		///< the kind(s) of units that CANNOT be collided with
	const FXList*		m_garrisonHitKillFX;
	Real						m_distanceScatterWhenJammed;	///< How far I scatter when Jammed

	Real						m_lockDistance;				///< If I get this close to my target, guaranteed hit.
	Bool						m_detonateCallsKill;			///< if true, kill() will be called, instead of KILL_SELF state, which calls destroy.
  Int             m_killSelfDelay;      ///< If I have detonated and entered the KILL-SELF state, how ling do I wait before I Kill/destroy self?
	// GeneralsMod @feature Dimitar 25/09/2026
	Bool            m_detonateAtLastTargetPosition; ///< target destroyed -> detonate at its last position instead of vanishing
	Bool            m_shouldRetarget;               ///< target effectively dead -> look for a new one
	Real            m_retargetRange;                ///< scan radius around the dead target's position
	Int             m_maxRetargets;                 ///< 0 == unlimited
	KindOfMaskType  m_retargetRequiredKindOf;       ///< if not empty, candidate must match at least one
	KindOfMaskType  m_retargetForbiddenKindOf;      ///< candidate must match none
	Bool            m_retargetCanBeDecoyed;         ///< new target's countermeasures may decoy us

	MissileAIUpdateV2ModuleData();

	static void buildFieldParse(MultiIniFieldParse& p);

};

//-------------------------------------------------------------------------------------------------
class MissileAIUpdateV2 : public AIUpdateInterface, public ProjectileUpdateInterface
{
	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( MissileAIUpdateV2, "MissileAIUpdateV2" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( MissileAIUpdateV2, MissileAIUpdateV2ModuleData );

public:
	MissileAIUpdateV2( Thing *thing, const ModuleData* moduleData );

	enum MissileStateType	 // Stored in save file, don't renumber.
	{
		PRELAUNCH			= 0,
		LAUNCH				= 1, ///< released from launcher, falling
		IGNITION			= 2, ///< engines ignite
		ATTACK_NOTURN	= 3, ///< fly toward victim
		ATTACK				= 4, ///< fly toward victim
		DEAD					= 5,
		KILL					= 6, ///< Hit victim (cheat).
		KILL_SELF			= 7, ///< Destroy self.
	};

	virtual ProjectileUpdateInterface* getProjectileUpdateInterface() override { return this; }
	virtual void projectileFireAtObjectOrPosition( const Object *victim, const Coord3D *victimPos, const WeaponTemplate *detWeap, const ParticleSystemTemplate* exhaustSysOverride ) override;
	virtual void projectileLaunchAtObjectOrPosition(const Object *victim, const Coord3D* victimPos, const Object *launcher, WeaponSlotType wslot, Int specificBarrelToUse, const WeaponTemplate* detWeap, const ParticleSystemTemplate* exhaustSysOverride) override;
	virtual Bool projectileHandleCollision( Object *other ) override;
	virtual Bool projectileIsArmed() const override { return m_isArmed; }
	virtual ObjectID projectileGetLauncherID() const override { return m_launcherID; }
	virtual void setFramesTillCountermeasureDiversionOccurs( UnsignedInt frames ) override; ///< Number of frames till missile diverts to countermeasures.
	virtual void projectileNowJammed() override;///< We lose our Object target and scatter to the ground

	virtual Bool processCollision(PhysicsBehavior *physics, Object *other) override; ///< Returns true if the physics collide should apply the force.  Normally not.  jba.

	virtual UpdateSleepTime update() override;
	virtual void onDelete() override;


protected:

	void detonate();

private:

	MissileStateType			m_state;									///< the behavior state of the missile
	UnsignedInt						m_stateTimestamp;					///< time of state change
	UnsignedInt						m_nextTargetTrackTime;		///< if nonzero, how often we update our target pos
	ObjectID							m_launcherID;							///< ID of object that launched us (INVALID_ID if not yet launched)
	ObjectID							m_victimID;								///< ID of object that I am rocketing towards (INVALID_ID if not yet launched)
	UnsignedInt						m_fuelExpirationDate;			///< how long 'til we run out of fuel
	Real									m_noTurnDistLeft;					///< when zero, ok to start turning
	Real									m_maxAccel;
	Coord3D								m_originalTargetPos;			///< When firing uphill, we aim high to clear the brow of the hill.  jba.
	Coord3D								m_prevPos;
	WeaponBonusConditionFlags		m_extraBonusFlags;
	const WeaponTemplate*	m_detonationWeaponTmpl;		///< weapon to fire at end (or null)
	const ParticleSystemTemplate* m_exhaustSysTmpl;
	ParticleSystemID			m_exhaustID;								///< our exhaust particle system (if any)
	UnsignedInt						m_framesTillDecoyed;			///< Number of frames before missile will get distracted by decoy countermeasures.
	Bool									m_isTrackingTarget;				///< Was I originally shot at a moving object?
	Bool									m_isArmed;								///< if true, missile will explode on contact
	Bool									m_noDamage;								///< if true, missile will not cause damage when it detonates. (Used for flares).
	Bool									m_isJammed;								///< No target, just shooting at a scattered position

	// GeneralsMod @feature Dimitar 25/09/2026
	Coord3D               m_lastTargetPos;          ///< last known position of the tracked target (updated every frame)
	Int                   m_retargetCount;          ///< how many times we've switched targets
	Bool                  m_retargetScanDone;       ///< already scanned for a replacement for the CURRENT target
	Bool                  m_goingToLastPos;         ///< target lost, flying to m_lastTargetPos to detonate

	void doPrelaunchState();
	void doLaunchState();
	void doIgnitionState();
	void doAttackState(Bool turnOK);
	void doKillState();
	void doKillSelfState();
	void doDeadState();

	void airborneTargetGone();											///< My airborne target has died, so I have to do something cool to make up for that

	// GeneralsMod @feature Dimitar 25/09/2026
	void trackTarget();                             ///< per-frame: record target pos, handle death / destruction
	Bool tryRetarget();                             ///< scan around m_lastTargetPos, switch to the nearest legal target
	void retargetTo( Object *victim );
	void targetLost();                              ///< no target any more: last-position detonation (or vanilla vanish)
	Bool checkArrivedAtLastTargetPos();             ///< detonate when we reach m_lastTargetPos

	void tossExhaust();
	void switchToState(MissileStateType s);


};
