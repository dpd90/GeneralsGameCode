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

// FILE: HealAIUpdateV2.cpp ///////////////////////////////////////////////////////////////////////
//
//	GeneralsMod @feature Dimitar 11/09/2026
//	See HealAIUpdateV2.h.
//
//	findHealTarget() mirrors DozerAIUpdate.cpp's findObjectToRepair(): same PartitionFilter-based
//	range scan, but the KindOf(DOZER)/KindOf(STRUCTURE)-locked ActionManager::canRepairObject() check
//	(which this module can't use) is replaced with a KindOf/ForbiddenKindOf filter plus a plain
//	not-at-full-health check. It uses PartitionFilterPossibleToAttack directly -- exactly like
//	DozerAIUpdate.cpp's findMine() does for enemy mines -- rather than going through
//	AIUpdateInterface::getNextMoodTarget(), so it is never subject to that function's
//	human-player-only AI::WITHIN_ATTACK_RANGE restriction. A target found beyond weapon range still
//	comes back as a valid ATTACKRESULT_POSSIBLE_AFTER_MOVING result.
//
//	That alone isn't enough to actually get the unit moving, though:
//	AIAttackApproachTargetState::onEnter() (AIStates.cpp) deliberately refuses to chase beyond weapon
//	range for a HUMAN-controlled unit whenever the attack's command source is CMD_FROM_AI, unless
//	isAllowedToChase() is set -- this is almost certainly what broke the original (reverted)
//	AIUpdateInterfaceV2's ally-chasing, and it's also why DozerAIUpdate's mine-scan uses
//	CMD_FROM_DOZER instead of CMD_FROM_AI (sidestepping the check rather than opting into it). update()
//	below calls setAllowedToChase(TRUE) before aiAttackObject() -- the same opt-in
//	AssaultTransportAIUpdate.cpp already uses for its own chasing passengers -- so the target is both
//	found AND actually pursued beyond weapon range, just like an auto-acquired enemy.
//
//	update() folds DozerPrimaryIdleState::update()'s "bored" idle-timeout pattern directly into this
//	module's own update() (no second state machine -- this module only has the one behavior), and
//	adds the full-health self-stop check that the stock AIAttackState has no equivalent for.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine
#include "Common/GameCommon.h"
#include "Common/Xfer.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Module/BodyModule.h"
#include "GameClient/Drawable.h"
#include "GameLogic/Module/HealAIUpdateV2.h"
#include "GameLogic/Object.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/Weapon.h"	// for NO_MAX_SHOTS_LIMIT

//-------------------------------------------------------------------------------------------------
HealAIUpdateV2ModuleData::HealAIUpdateV2ModuleData()
{
	m_boredTime = 0.0f;
	m_boredRange = 0.0f;
	// m_kindof / m_forbiddenKindof default-construct to KINDOFMASK_NONE
}

//-------------------------------------------------------------------------------------------------
/*static*/ void HealAIUpdateV2ModuleData::buildFieldParse( MultiIniFieldParse &p )
{
	AIUpdateModuleData::buildFieldParse( p );

	static const FieldParse dataFieldParse[] =
	{
		{ "BoredTime",				INI::parseDurationReal,				nullptr, offsetof( HealAIUpdateV2ModuleData, m_boredTime ) },
		{ "BoredRange",				INI::parseReal,								nullptr, offsetof( HealAIUpdateV2ModuleData, m_boredRange ) },
		{ "KindOf",						KindOfMaskType::parseFromINI,	nullptr, offsetof( HealAIUpdateV2ModuleData, m_kindof ) },
		{ "ForbiddenKindOf",	KindOfMaskType::parseFromINI,	nullptr, offsetof( HealAIUpdateV2ModuleData, m_forbiddenKindof ) },
		{ nullptr, nullptr, nullptr, 0 }
	};

	p.add( dataFieldParse );

}

//-------------------------------------------------------------------------------------------------
HealAIUpdateV2::HealAIUpdateV2( Thing *thing, const ModuleData* moduleData ) : AIUpdateInterface( thing, moduleData )
{
	m_idleTooLongTimestamp = 0;
}

//-------------------------------------------------------------------------------------------------
// GeneralsMod @feature Dimitar 11/09/2026: MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE only *declares*
// the destructor (protected virtual), same as it does for crc/xfer/loadPostProcess -- it must be
// defined somewhere too, or it's an LNK2019 unresolved external for the scalar deleting destructor.
// No custom cleanup needed here, so EMPTY_DTOR (see DozerPrimaryIdleState in DozerAIUpdate.cpp for
// the same pattern) is enough.
//-------------------------------------------------------------------------------------------------
EMPTY_DTOR( HealAIUpdateV2 )

//-------------------------------------------------------------------------------------------------
/** Find the closest damaged, KindOf-eligible ally within BoredRange that our weapon can actually
	* target. See file header comment for why this bypasses getNextMoodTarget(). */
//-------------------------------------------------------------------------------------------------
/*static*/ Object* HealAIUpdateV2::findHealTarget( Object *self, const HealAIUpdateV2ModuleData *data )
{

	if( self == nullptr || data == nullptr )
		return nullptr;

	PartitionFilterRelationship filterRelationship( self, PartitionFilterRelationship::ALLOW_ALLIES );
	PartitionFilterAcceptByKindOf filterKindof( data->m_kindof, data->m_forbiddenKindof );
	PartitionFilterSameMapStatus filterMapStatus( self );
	PartitionFilterAlive filterAlive;
	PartitionFilterPossibleToAttack filterAttack( ATTACK_NEW_TARGET, self, CMD_FROM_AI );
	PartitionFilter *filters[] = { &filterRelationship, &filterKindof, &filterMapStatus, &filterAlive, &filterAttack, nullptr };

	ObjectIterator *iter = ThePartitionManager->iterateObjectsInRange( self->getPosition(), data->m_boredRange, FROM_CENTER_2D, filters );
	MemoryPoolObjectHolder hold( iter );

	Object *closest = nullptr;
	Real closestDistSqr = 0.0f;
	for( Object *obj = iter->first(); obj; obj = iter->next() )
	{

		if( obj == self )
			continue;

		BodyModuleInterface *body = obj->getBodyModule();
		if( body == nullptr || body->getHealth() >= body->getMaxHealth() )
			continue;	// already at full health -- nothing to heal

		Real distSqr = ThePartitionManager->getDistanceSquared( self, obj, FROM_CENTER_2D );
		if( closest == nullptr || distSqr < closestDistSqr )
		{

			closest = obj;
			closestDistSqr = distSqr;

		}

	}

	return closest;

}

//-------------------------------------------------------------------------------------------------
// GeneralsMod @feature Dimitar 11/09/2026: model condition flags live on the Drawable, not the
// Object -- Object only exposes setters/clearers, no getter/test -- so route through getDrawable()
// whenever we need to check our own current animation state.
//-------------------------------------------------------------------------------------------------
static Bool testModelConditionFlag( const Object *self, ModelConditionFlagType f )
{

	const Drawable *draw = self->getDrawable();
	if( draw == nullptr )
		return FALSE;

	return draw->getModelConditionFlags().test( f );

}

//-------------------------------------------------------------------------------------------------
/** True if any of the fire-loop animation flags (preattack/firing/between-shots/reloading/turret-
	* rotate) are currently set. */
//-------------------------------------------------------------------------------------------------
static Bool anyFireLoopFlagSet( const Object *self )
{

	return testModelConditionFlag( self, MODELCONDITION_FIRING_A )
			|| testModelConditionFlag( self, MODELCONDITION_BETWEEN_FIRING_SHOTS_A )
			|| testModelConditionFlag( self, MODELCONDITION_PREATTACK_A )
			|| testModelConditionFlag( self, MODELCONDITION_RELOADING_A )
			|| testModelConditionFlag( self, MODELCONDITION_TURRET_ROTATE );

}

//-------------------------------------------------------------------------------------------------
/** Force the fire-loop animation flags off directly, bypassing whatever normally drives them. */
//-------------------------------------------------------------------------------------------------
static void clearFireLoopFlags( Object *self )
{

	self->clearModelConditionFlags( MAKE_MODELCONDITION_MASK5( MODELCONDITION_PREATTACK_A,
																															 MODELCONDITION_FIRING_A,
																															 MODELCONDITION_BETWEEN_FIRING_SHOTS_A,
																															 MODELCONDITION_RELOADING_A,
																															 MODELCONDITION_TURRET_ROTATE ) );

}

//-------------------------------------------------------------------------------------------------
/** End the current heal attack right now: return to idle at the AI level, and clear the fire-loop
	* flags immediately rather than waiting for the general not-attacking cleanup in update() (below)
	* to catch them a frame later -- see that cleanup's comment for why it exists at all. */
//-------------------------------------------------------------------------------------------------
static void endHealAttack( Object *self, AIUpdateInterface *ai )
{

	ai->aiIdle( CMD_FROM_AI );
	clearFireLoopFlags( self );

}

//-------------------------------------------------------------------------------------------------
UpdateSleepTime HealAIUpdateV2::update()
{

	// extend the normal AI system
	UpdateSleepTime result = AIUpdateInterface::update();

	Object *self = getObject();
	if( self->isEffectivelyDead() )
		return result;

	const HealAIUpdateV2ModuleData *data = getHealAIUpdateV2ModuleData();

	// GeneralsMod @feature Dimitar 11/09/2026: the stock AIAttackState only ever stops attacking when
	// the victim dies -- it has no "target no longer needs attention" concept, since that was never
	// needed for killing enemies. Self-terminate once our current heal victim is back to full health.
	if( isAttacking() )
	{

		// GeneralsMod @feature Dimitar 11/09/2026: keep the healing weapon equipped for as long as
		// we're actually engaged. See WEAPONSET_HEALING_DETAIL below for why this flag exists at all.
		// setWeaponSetFlag()/clearWeaponSetFlag() unconditionally recompute the active weapon set on
		// every call, so guard with testWeaponSetFlag() -- this branch runs every single frame of a
		// (potentially long) heal, not just once like Dozer's own idle-check callers do.
		if( !self->testWeaponSetFlag( WEAPONSET_HEALING_DETAIL ) )
			self->setWeaponSetFlag( WEAPONSET_HEALING_DETAIL );

		// GeneralsMod @feature Dimitar 11/09/2026: the stock AIAttackState has no "target no longer
		// needs attention" concept -- it only ever stops attacking when the victim dies, and it checks
		// that unconditionally every frame in AIAttackState::update() itself, regardless of which
		// inner attack sub-state (approach/aim/fire/etc) is currently active, so we don't need our
		// own victim-null/dead check here to catch that case -- the engine already has, before we
		// ever get control back this frame. Heal targets are different: they can become "done"
		// without dying, so a target back at full health is the one stop condition we do have to
		// detect ourselves.
		Object *victim = getCurrentVictim();
		if( victim != nullptr )
		{

			BodyModuleInterface *victimBody = victim->getBodyModule();
			if( victimBody != nullptr && victimBody->getHealth() >= victimBody->getMaxHealth() )
				endHealAttack( self, this );

		}

	}
	else
	{

		// GeneralsMod @feature Dimitar 11/09/2026: not actively healing right now -- drop the flag so
		// the healing weapon (kept off the normally-orderable slot in INI via
		// WeaponSet{ Conditions = None }, only swapped in via WeaponSet{ Conditions = HEALING_DETAIL }
		// while this flag is set) isn't available for the player to manually order against ANY unit,
		// ally or enemy. Same WeaponSet-condition-gating trick DozerAIUpdate uses to keep
		// DozerMineDisarmingWeapon from being player-orderable outside its own bored mine-scan.
		if( self->testWeaponSetFlag( WEAPONSET_HEALING_DETAIL ) )
			self->clearWeaponSetFlag( WEAPONSET_HEALING_DETAIL );

		// GeneralsMod @feature Dimitar 11/09/2026: this is the actual fix for the stuck-arm-animation
		// bug. Whenever the attack ends via the engine's own native victim-died exit rather than
		// through endHealAttack() above (confirmed via diagnostic logging: isAttacking() can already
		// read FALSE the instant we get here, with endHealAttack() never having run this stretch at
		// all), the fire-loop animation flags from the last active frame are simply left set with
		// nothing left to clear them -- the unit's arm keeps animating in place forever. Whenever we
		// are NOT attacking, for ANY reason, make sure these flags are off, regardless of who ended
		// the attack or whether endHealAttack() above ever ran.
		if( anyFireLoopFlagSet( self ) )
			clearFireLoopFlags( self );

	}

	// "bored" auto-scan, folded in directly from DozerPrimaryIdleState::update() (see
	// DozerAIUpdate.cpp) -- this module only has the one behavior, so it doesn't need a whole second
	// state machine the way DozerAIUpdate does.
	if( !isIdle() )
	{

		m_idleTooLongTimestamp = TheGameLogic->getFrame();

	}
	else if( TheGameLogic->getFrame() - m_idleTooLongTimestamp > data->m_boredTime )
	{

		// only try this potentially-expensive scan every so often, not every frame
		m_idleTooLongTimestamp = TheGameLogic->getFrame();

		// GeneralsMod @feature Dimitar 11/09/2026: must equip the healing weapon *before* scanning,
		// not just before attacking -- with the default WeaponSet{ Conditions = None / PRIMARY None }
		// this unit has literally no weapon in any slot until the flag is set, and
		// findHealTarget()'s PartitionFilterPossibleToAttack asks "can THIS unit's CURRENT weapon hit
		// this candidate" -- with no weapon equipped that's always "no", so every candidate was
		// silently rejected and the scan always came back empty. Equip first, then scan.
		self->setWeaponSetFlag( WEAPONSET_HEALING_DETAIL );

		Object *healTarget = findHealTarget( self, data );
		if( healTarget != nullptr )
		{

			// GeneralsMod @feature Dimitar 11/09/2026: AIAttackApproachTargetState::onEnter() (in
			// AIStates.cpp) deliberately refuses to chase beyond weapon range for a HUMAN-controlled
			// unit whenever ai->getLastCommandSource() == CMD_FROM_AI, unless isAllowedToChase() is
			// set -- this is almost certainly what broke the original (reverted) AIUpdateInterfaceV2's
			// ally-chasing, and it's also why DozerAIUpdate's own mine-scan uses CMD_FROM_DOZER instead
			// of CMD_FROM_AI (sidestepping this check entirely rather than opting into it). We use the
			// proper opt-in instead of borrowing Dozer's command source: setAllowedToChase(TRUE) is the
			// same mechanism AssaultTransportAIUpdate.cpp already uses for its own chasing passengers.
			setAllowedToChase( TRUE );
			aiAttackObject( healTarget, NO_MAX_SHOTS_LIMIT, CMD_FROM_AI );

		}
		else
		{

			// nothing to heal this time -- don't leave the healing weapon equipped and orderable
			// until the next bored-scan comes around.
			self->clearWeaponSetFlag( WEAPONSET_HEALING_DETAIL );

		}

	}

	return result;

}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void HealAIUpdateV2::crc( Xfer *xfer )
{

	AIUpdateInterface::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void HealAIUpdateV2::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	AIUpdateInterface::xfer( xfer );

	xfer->xferUnsignedInt( &m_idleTooLongTimestamp );

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void HealAIUpdateV2::loadPostProcess()
{

	AIUpdateInterface::loadPostProcess();

}
