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

// FILE: BreakApartDeathBehaviorV2.cpp /////////////////////////////////////////////////////////
// GeneralsMod @feature Dimitar 19/09/2026
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine
#define DEFINE_BREAKAPARTPHASE_NAMES
#include "Common/DrawModule.h"
#include "Common/INI.h"
#include "Common/ThingTemplate.h"
#include "Common/RandomValue.h"
#include "Common/Thing.h"
#include "Common/Xfer.h"
#include "GameClient/BreakApartModelHelper.h"
#include "GameLogic/BreakApartDebrisHelper.h"
#include "GameClient/Drawable.h"
#include "GameClient/FXList.h"
#include "GameLogic/Damage.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/BodyModule.h"
#include "GameLogic/Module/BreakApartDeathBehaviorV2.h"
#include "GameLogic/Module/PhysicsUpdate.h"
#include "GameLogic/Object.h"
#include "GameLogic/ObjectCreationList.h"
#include "GameLogic/Weapon.h"
#include "WWMath/matrix3d.h"

const Real BADB_BEGIN_MIDPOINT_RATIO = 0.35f;
const Real BADB_END_MIDPOINT_RATIO = 0.65f;

// GeneralsMod @feature Dimitar 21/09/2026: sentinel for m_overkillDestructionDelay/
// m_extremeOverkillDestructionDelay (and their *Variance counterparts) meaning "not configured in
// INI -- inherit the base DestructionDelay/DestructionDelayVariance instead". 0xFFFFFFFF is not
// reachable through INI::parseDurationUnsignedInt's own msec-to-frame conversion (ceilf of a
// Real-converted frame count), so it's safe as a real sentinel, not just an implausibly large one --
// same idiom as this engine's own WAIT_INDEFINITELY/UPDATE_SLEEP_FOREVER-style UnsignedInt sentinels
// elsewhere, just scoped locally here since nothing outside this module needs it.
const UnsignedInt BADB_INHERIT_BASE_DESTRUCTION_DELAY = 0xFFFFFFFF;

//-------------------------------------------------------------------------------------------------
BreakApartDeathBehaviorV2ModuleData::BreakApartDeathBehaviorV2ModuleData()
{
	m_destructionDelay = 0;
	m_destructionDelayVariance = 0;
	m_overkillDestructionDelay = BADB_INHERIT_BASE_DESTRUCTION_DELAY;
	m_overkillDestructionDelayVariance = BADB_INHERIT_BASE_DESTRUCTION_DELAY;
	m_extremeOverkillDestructionDelay = BADB_INHERIT_BASE_DESTRUCTION_DELAY;
	m_extremeOverkillDestructionDelayVariance = BADB_INHERIT_BASE_DESTRUCTION_DELAY;
	// HUGE_DAMAGE_AMOUNT (Damage.h) as the default for all four -- an unconfigured tier's AND check
	// (see the field comments in the .h) can never actually trigger on EITHER half alone, so a
	// modder must explicitly opt in to both halves of a tier for it to ever fire.
	m_overkillPercentageFromMaxHealth = HUGE_DAMAGE_AMOUNT;
	m_overkillDamageGreaterOrEqThan = HUGE_DAMAGE_AMOUNT;
	m_extremeOverkillPercentageFromMaxHealth = HUGE_DAMAGE_AMOUNT;
	m_extremeOverkillDamageGreaterOrEqThan = HUGE_DAMAGE_AMOUNT;
	m_force = 0;
	m_overkillForce = 0;
	m_extremeOverkillForce = 0;
	m_forceSpreadAngle = PI / 3.0f;	// 60 degrees
	m_breakApartPieceOCL = nullptr;
	m_remainPieceOCL = nullptr;
	m_breakApartPieceFX = nullptr;
	m_remainPieceFX = nullptr;
}

//-------------------------------------------------------------------------------------------------
static void parseFX( INI* ini, void *instance, void * /*store*/, const void* /*userData*/ )
{
	BreakApartDeathBehaviorV2ModuleData* self = (BreakApartDeathBehaviorV2ModuleData*)instance;
	BreakApartPhaseType phase = (BreakApartPhaseType)INI::scanIndexList(ini->getNextToken(), TheBreakApartPhaseNames);
	for (const char* token = ini->getNextToken(); token; token = ini->getNextTokenOrNull())
	{
		const FXList *fxl = TheFXListStore->findFXList((token));	// could be null! this is OK!
		self->m_fx[phase].push_back(fxl);
	}
}

//-------------------------------------------------------------------------------------------------
static void parseOCL( INI* ini, void *instance, void * /*store*/, const void* /*userData*/ )
{
	BreakApartDeathBehaviorV2ModuleData* self = (BreakApartDeathBehaviorV2ModuleData*)instance;
	BreakApartPhaseType phase = (BreakApartPhaseType)INI::scanIndexList(ini->getNextToken(), TheBreakApartPhaseNames);
	for (const char* token = ini->getNextToken(); token; token = ini->getNextTokenOrNull())
	{
		const ObjectCreationList *ocl = TheObjectCreationListStore->findObjectCreationList(token);	// could be null! this is OK!
		self->m_ocls[phase].push_back(ocl);
	}
}

//-------------------------------------------------------------------------------------------------
static void parseWeapon( INI* ini, void *instance, void * /*store*/, const void* /*userData*/ )
{
	BreakApartDeathBehaviorV2ModuleData* self = (BreakApartDeathBehaviorV2ModuleData*)instance;
	BreakApartPhaseType phase = (BreakApartPhaseType)INI::scanIndexList(ini->getNextToken(), TheBreakApartPhaseNames);
	for (const char* token = ini->getNextToken(); token; token = ini->getNextTokenOrNull())
	{
		const WeaponTemplate *wt = TheWeaponStore->findWeaponTemplate(token);	// could be null! this is OK!
		self->m_weapons[phase].push_back(wt);
	}
}

// GeneralsMod @refactor Dimitar 26/10/2026: parseBoneNameList(), calcRandomOutwardForce(),
// applyBreakApartDebrisForce(), boneNameListContains(), the per-bone spawn body and the overkill-tier
// formula moved to GameLogic/BreakApartDebrisHelper.h/.cpp (shared with BreakApartJetSlowDeathBehaviorV2).

//-------------------------------------------------------------------------------------------------
/*static*/ void BreakApartDeathBehaviorV2ModuleData::buildFieldParse(MultiIniFieldParse& p)
{
  UpdateModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "DestructionDelay",								INI::parseDurationUnsignedInt,	nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_destructionDelay ) },
		{ "DestructionDelayVariance",					INI::parseDurationUnsignedInt,	nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_destructionDelayVariance ) },
		{ "OverkillDestructionDelay",					INI::parseDurationUnsignedInt,	nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_overkillDestructionDelay ) },
		{ "OverkillDestructionDelayVariance",	INI::parseDurationUnsignedInt,	nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_overkillDestructionDelayVariance ) },
		{ "ExtremeOverkillDestructionDelay",	INI::parseDurationUnsignedInt,	nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_extremeOverkillDestructionDelay ) },
		{ "ExtremeOverkillDestructionDelayVariance", INI::parseDurationUnsignedInt, nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_extremeOverkillDestructionDelayVariance ) },
		{ "FX",																parseFX,												nullptr, 0 },
		{ "OCL",															parseOCL,												nullptr, 0 },
		{ "Weapon",														parseWeapon,										nullptr, 0 },
		{ "BreakApartModel",									INI::parseAsciiString,					nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_breakApartModel ) },
		{ "BreakApartSubObject",							BreakApartParseBoneNameList,							nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_breakApartSubObject ) },
		{ "RemainSubObject",									BreakApartParseBoneNameList,							nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_remainSubObject ) },
		{ "OverkillPercentageFromMaxHealth",	INI::parsePercentToReal,				nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_overkillPercentageFromMaxHealth ) },
		{ "OverkillDamageGreaterOrEqThan",		INI::parseReal,									nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_overkillDamageGreaterOrEqThan ) },
		{ "OverkillBreakApartSubObject",			BreakApartParseBoneNameList,							nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_overkillBreakApartSubObject ) },
		{ "ExtremeOverkillPercentageFromMaxHealth", INI::parsePercentToReal,		nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_extremeOverkillPercentageFromMaxHealth ) },
		{ "ExtremeOverkillDamageGreaterOrEqThan", INI::parseReal,					nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_extremeOverkillDamageGreaterOrEqThan ) },
		{ "ExtremeOverkillBreakApartSubObject", BreakApartParseBoneNameList,							nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_extremeOverkillBreakApartSubObject ) },
		{ "Force",														INI::parseReal,									nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_force ) },
		{ "OverkillForce",										INI::parseReal,									nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_overkillForce ) },
		{ "ExtremeOverkillForce",							INI::parseReal,									nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_extremeOverkillForce ) },
		{ "ForceSpreadAngle",									INI::parseAngleReal,						nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_forceSpreadAngle ) },
		{ "BreakApartPieceOCL",								INI::parseObjectCreationList,		nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_breakApartPieceOCL ) },
		{ "RemainPieceOCL",										INI::parseObjectCreationList,		nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_remainPieceOCL ) },
		{ "BreakApartPieceFX",								INI::parseFXList,								nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_breakApartPieceFX ) },
		{ "RemainPieceFX",										INI::parseFXList,								nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_remainPieceFX ) },
		{ nullptr, nullptr, nullptr, 0 }
	};
  p.add(dataFieldParse);
	p.add(DieMuxData::getFieldParse(), offsetof( BreakApartDeathBehaviorV2ModuleData, m_dieMuxData ));
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BreakApartDeathBehaviorV2::BreakApartDeathBehaviorV2( Thing *thing, const ModuleData* moduleData ) : UpdateModule( thing, moduleData )
{
	m_flags = 0;
	m_midpointFrame = 0;
	m_destructionFrame = 0;
	m_dieTier = -1;
	m_dieForce = 0.0f;

	setWakeFrame(getObject(), UPDATE_SLEEP_FOREVER);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BreakApartDeathBehaviorV2::~BreakApartDeathBehaviorV2()
{
}

//-------------------------------------------------------------------------------------------------
/** For each root bone in rootBones, walks its whole descendant subtree on the reference model
	* (children before their own parent -- see GetBreakApartSubtreeBones()) and spawns ONE debris
	* piece per resolved bone from the single, generic BreakApartPieceOCL. The spawned Object is
	* positioned/oriented exactly like the dying object itself (NOT at a computed per-bone offset --
	* the spawned piece is a full clone of BreakApartModel with every OTHER subobject hidden, so
	* placing the clone where the original stood is what puts the one visible piece in the right
	* spot, the same way the original object's own bones lined up when it was still whole).
	*
	* GeneralsMod @feature Dimitar 19/09/2026: afterward, if RemainPieceOCL is set, spawns exactly
	* ONE more piece representing everything that did NOT get its own individual piece above --
	* every bone name resolved across every root in rootBones is collected into allBrokenBones as we
	* go, then handed to DrawModule::setBreakApartRemainder() on the remainder Object's own Draw
	* module, which shows every subobject EXCEPT those bones (the mirror image of setBreakApartPiece
	* above, which shows exactly one). If rootBones is empty (this tier has no BreakApartSubObject
	* configured at all), allBrokenBones stays empty too, so the remainder piece ends up showing the
	* WHOLE model -- i.e. an object with only RemainPieceOCL set and no BreakApartSubObject lists
	* turns entirely into one flung/tumbling debris piece instead of just vanishing, which follows
	* naturally from "everything not broken apart" rather than needing to be special-cased. */
//-------------------------------------------------------------------------------------------------
void BreakApartDeathBehaviorV2::spawnBreakApartDebris( const BADB_BoneNameVec& rootBones, Real force )
{
	const BreakApartDeathBehaviorV2ModuleData* d = getBreakApartDeathBehaviorV2ModuleData();
	Object* obj = getObject();

	if (d->m_breakApartModel.isEmpty())
		return;

	// (Live-bone alignment against the still-alive dying Drawable now happens inside
	// BreakApartSpawnPiece() -- see BreakApartDebrisHelper.cpp.)

	// GeneralsMod @feature Dimitar 19/09/2026: resolve RemainSubObject's protection ONCE, up front --
	// a protected root pulls in its own whole subtree, exactly like a BreakApartSubObject root does.
	BreakApartBoneNameVec protectedBones;
	for (BADB_BoneNameVec::const_iterator protIt = d->m_remainSubObject.begin(); protIt != d->m_remainSubObject.end(); ++protIt)
		BreakApartAppendSubtree(d->m_breakApartModel, *protIt, BreakApartBoneNameVec(), &protectedBones);

	// Every bone that gets broken off (protected bones excluded, duplicates across overlapping roots
	// removed), in root order, children before parent. Also needed with no BreakApartPieceOCL at all
	// (a modder using ONLY RemainPieceOCL) so the remainder piece knows which bones to hide.
	BreakApartBoneNameVec allBrokenBones;
	for (BADB_BoneNameVec::const_iterator rootIt = rootBones.begin(); rootIt != rootBones.end(); ++rootIt)
		BreakApartAppendSubtree(d->m_breakApartModel, *rootIt, protectedBones, &allBrokenBones);

	if (d->m_breakApartPieceOCL != nullptr)
	{
		for (BreakApartBoneNameVec::const_iterator boneIt = allBrokenBones.begin(); boneIt != allBrokenBones.end(); ++boneIt)
		{
			BreakApartSpawnPiece(obj, d->m_breakApartModel, *boneIt, d->m_breakApartPieceOCL, d->m_breakApartPieceFX,
				force, d->m_forceSpreadAngle, nullptr);
		}
	}

	if (d->m_remainPieceOCL != nullptr)
	{
		Object* remainder = ObjectCreationList::create( d->m_remainPieceOCL, obj, obj->getPosition(), nullptr, obj->getOrientation() );
		if (remainder != nullptr)
		{
			for (DrawModule** dm = remainder->getDrawable()->getDrawModules(); *dm; ++dm)
			{
				(*dm)->setBreakApartRemainder(d->m_breakApartModel, allBrokenBones);
			}

			// GeneralsMod @feature Dimitar 21/09/2026: no directional bias for the remainder piece --
			// it's positioned at the dying object's own root, with no off-center offset of its own
			// to derive an outward direction from, so it keeps the old fully random scatter.
			BreakApartApplyDebrisForce(remainder, force, nullptr, d->m_forceSpreadAngle);
			FXList::doFXObj(d->m_remainPieceFX, remainder);
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void BreakApartDeathBehaviorV2::onDie( const DamageInfo *damageInfo )
{
	const BreakApartDeathBehaviorV2ModuleData* d = getBreakApartDeathBehaviorV2ModuleData();

	if (isBreakApartActivated())
		return;	// defensive -- Object::onDie() should only ever call us once anyway

	if (!d->m_dieMuxData.isDieApplicable(getObject(), damageInfo))
		return;

	Object* obj = getObject();

	// GeneralsMod @fix Dimitar 22/09/2026: freeze/relinquish control of the dying unit immediately,
	// exactly like SlowDeathBehavior::onDie() does -- the user pointed out this module diverged from
	// SlowDeathBehavior's own behavior here: a unit dying via SlowDeathBehavior stops moving and
	// drops out of player control the instant it dies, while this module previously left the unit
	// fully mobile/controllable for the entire DestructionDelay window before actual destruction.
	// Same two calls SlowDeathBehavior itself uses (both ordinary public engine APIs, not
	// SlowDeathBehavior-internal machinery):
	//   - AIUpdateInterface::markAsDead() sets the AI's own "I'm dead" flag and calls
	//     Object::setEffectivelyDead(TRUE); the locomotor update checks isAiInDeadState() every tick
	//     and refuses to move the unit unless its locomotor specifically opts in via
	//     getLocomotorWorksWhenDead() (almost none do), which is what actually halts movement.
	//     isAiInDeadState() is also checked first and returned out of early, same as
	//     SlowDeathBehavior -- guards against another die module (ideally another SlowDeathBehavior/
	//     BreakApartDeathBehaviorV2 instance) having already marked this object dead.
	//   - TheGameLogic->deselectObject() forcibly clears the unit from every player's selection that
	//     same frame, which is what actually revokes player control (can't issue orders to a unit
	//     that's no longer selected).
	AIUpdateInterface* ai = obj->getAIUpdateInterface();
	if (ai != nullptr)
	{
		if (ai->isAiInDeadState())
			return;
		ai->markAsDead();
	}
	TheGameLogic->deselectObject(obj, PLAYERMASK_ALL, TRUE);

	// Pick which tier applies from an AND of two independent checks on THIS hit -- see the field
	// comments on m_overkillPercentageFromMaxHealth/m_overkillDamageGreaterOrEqThan (and their
	// extreme-tier counterparts) in the .h for the full rationale; short version:
	//   1) percentOfMaxHealth = this hit's raw damage / the object's own max health -- "how big was
	//      this hit relative to how tough this unit fundamentally is". Rules out a weak weapon ever
	//      qualifying, no matter how little health the target had left when it landed.
	//   2) wastedDamage = this hit's raw damage - the health the object had immediately before it --
	//      how much of this hit's damage was never actually needed to kill the target. Rules out a
	//      merely-large hit that happened to land on a still-healthy target with little real excess
	//      (the user's own example: 100 max health, 50 remaining, 51 damage dealt -- 51% of max
	//      health, but only 1 point actually wasted -- not a real overkill).
	// Both use m_actualDamageDealt (Damage.h), the UNCLIPPED damage this hit tried to deal (after
	// multipliers, before being clamped to the object's remaining health) -- not
	// m_actualDamageClipped, which by definition can never exceed m_healthBeforeDamage and so could
	// never show any "waste" at all.
	//
	// getMaxHealth() (BodyModuleInterface, GameLogic/Module/BodyModule.h) reads the object's CURRENT
	// health cap (reflecting any max-health-increasing upgrades already applied), not its spawn-time
	// baseline (getInitialHealth()) -- the right "how tough is this unit right now" denominator.
	// m_healthBeforeDamage (DamageInfoOutput, set by ActiveBody::attemptDamage) is only meaningful
	// on the DamageInfo passed into onDie() -- not safe to read from any earlier, non-lethal hit.
	Int tier = BreakApartCalcOverkillTier(obj, damageInfo,
		d->m_overkillPercentageFromMaxHealth, d->m_overkillDamageGreaterOrEqThan,
		d->m_extremeOverkillPercentageFromMaxHealth, d->m_extremeOverkillDamageGreaterOrEqThan);
	Real force = (tier == 2) ? d->m_extremeOverkillForce : (tier == 1) ? d->m_overkillForce : d->m_force;

	// GeneralsMod @feature Dimitar 19/09/2026: DON'T spawn debris here -- cache the selected tier
	// instead and spawn it later, in update(), at the same moment the dying object is actually
	// destroyed (see the m_destructionFrame branch below). Spawning instantly at onDie() meant the
	// still-standing dying object and its own freshly-spawned debris pieces would visibly overlap
	// for the entire DestructionDelay window; this makes DestructionDelay govern both. Cached as a
	// tier INDEX rather than a BADB_BoneNameVec* so it survives a save/load mid-death (see xfer()).
	m_dieTier = tier;
	m_dieForce = force;

	// GeneralsMod @feature Dimitar 21/09/2026: pick this death's own DestructionDelay/
	// DestructionDelayVariance from the SAME tier index that just selected the debris root list and
	// Force above -- an overkill/extreme-overkill death can linger for a totally different length of
	// time (and variance spread) than a normal one. BADB_INHERIT_BASE_DESTRUCTION_DELAY (the
	// constructor's default for all four *Overkill*DestructionDelay* fields) means "not overridden in
	// INI for this tier" -- falls back to the base DestructionDelay/DestructionDelayVariance exactly
	// as if no tier-specific override existed, so a modder who only retimes the normal tier doesn't
	// have to also restate it for the other two.
	UnsignedInt destructionDelay = d->m_destructionDelay;
	UnsignedInt destructionDelayVariance = d->m_destructionDelayVariance;
	if (tier == 2)
	{
		if (d->m_extremeOverkillDestructionDelay != BADB_INHERIT_BASE_DESTRUCTION_DELAY)
			destructionDelay = d->m_extremeOverkillDestructionDelay;
		if (d->m_extremeOverkillDestructionDelayVariance != BADB_INHERIT_BASE_DESTRUCTION_DELAY)
			destructionDelayVariance = d->m_extremeOverkillDestructionDelayVariance;
	}
	else if (tier == 1)
	{
		if (d->m_overkillDestructionDelay != BADB_INHERIT_BASE_DESTRUCTION_DELAY)
			destructionDelay = d->m_overkillDestructionDelay;
		if (d->m_overkillDestructionDelayVariance != BADB_INHERIT_BASE_DESTRUCTION_DELAY)
			destructionDelayVariance = d->m_overkillDestructionDelayVariance;
	}

	UnsignedInt now = TheGameLogic->getFrame();
	m_destructionFrame = destructionDelay + GameLogicRandomValue(0, destructionDelayVariance);
	m_midpointFrame = GameLogicRandomValue( BADB_BEGIN_MIDPOINT_RATIO * m_destructionFrame, BADB_END_MIDPOINT_RATIO * m_destructionFrame );
	m_destructionFrame += now;
	m_midpointFrame += now;

	m_flags |= (1<<BREAK_APART_ACTIVATED);

	doPhaseStuff(BAPHASE_INITIAL);

	setWakeFrame(obj, UPDATE_SLEEP_NONE);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void BreakApartDeathBehaviorV2::doPhaseStuff(BreakApartPhaseType phase)
{
	const BreakApartDeathBehaviorV2ModuleData* d = getBreakApartDeathBehaviorV2ModuleData();
	Int idx, listSize;

	listSize = d->m_fx[phase].size();
	if (listSize > 0)
	{
		idx = GameLogicRandomValue(0, listSize-1);
		const FXList* fxl = d->m_fx[phase][idx];
		FXList::doFXObj(fxl, getObject(), nullptr);
	}

	listSize = d->m_ocls[phase].size();
	if (listSize > 0)
	{
		idx = GameLogicRandomValue(0, listSize-1);
		const ObjectCreationList* ocl = d->m_ocls[phase][idx];
		ObjectCreationList::create(ocl, getObject(), nullptr);
	}

	listSize = d->m_weapons[phase].size();
	if (listSize > 0)
	{
		idx = GameLogicRandomValue(0, listSize-1);
		const WeaponTemplate* wt = d->m_weapons[phase][idx];
		if (wt)
		{
			TheWeaponStore->createAndFireTempWeapon(wt, getObject(), getObject()->getPosition());
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
UpdateSleepTime BreakApartDeathBehaviorV2::update()
{
	DEBUG_ASSERTCRASH(isBreakApartActivated(), ("hmm, this should not be possible"));

	Object* obj = getObject();
	UnsignedInt now = TheGameLogic->getFrame();

	if( now >= m_midpointFrame && (m_flags & (1<<MIDPOINT_EXECUTED)) == 0 )
	{
		doPhaseStuff(BAPHASE_MIDPOINT);
		m_flags |= (1<<MIDPOINT_EXECUTED);
	}

	if (now >= m_destructionFrame)
	{
		// GeneralsMod @feature Dimitar 19/09/2026: debris spawns HERE now, not at onDie() -- see the
		// comment in onDie(). m_dieTier is guaranteed >= 0 by this point: update() only ever runs
		// after onDie() set it and called setWakeFrame(UPDATE_SLEEP_NONE).
		if (m_dieTier >= 0)
		{
			const BreakApartDeathBehaviorV2ModuleData* dd = getBreakApartDeathBehaviorV2ModuleData();
			const BADB_BoneNameVec* rootBones =
				(m_dieTier == 2) ? &dd->m_extremeOverkillBreakApartSubObject :
				(m_dieTier == 1) ? &dd->m_overkillBreakApartSubObject :
				&dd->m_breakApartSubObject;
			spawnBreakApartDebris(*rootBones, m_dieForce);
		}

		doPhaseStuff(BAPHASE_FINAL);
		TheGameLogic->destroyObject(obj);
	}

	return UPDATE_SLEEP_NONE;
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void BreakApartDeathBehaviorV2::crc( Xfer *xfer )
{

	// extend base class
	UpdateModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void BreakApartDeathBehaviorV2::xfer( Xfer *xfer )
{

	// version
	// GeneralsMod @feature Dimitar 19/09/2026: version 2 adds m_dieTier/m_dieForce -- needed once
	// debris spawn moved from onDie() to the deferred point in update() (see onDie()'s own
	// comment), since a save/load that lands mid-death (after onDie(), before m_destructionFrame)
	// must not lose which tier was already selected.
	XferVersion currentVersion = 2;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpdateModule::xfer( xfer );

	// midpoint frame
	xfer->xferUnsignedInt( &m_midpointFrame );

	// destruction frame
	xfer->xferUnsignedInt( &m_destructionFrame );

	// flags
	xfer->xferUnsignedInt( &m_flags );

	// GeneralsMod @feature Dimitar 19/09/2026: die tier / force (version 2+)
	if( version >= 2 )
	{
		xfer->xferInt( &m_dieTier );
		xfer->xferReal( &m_dieForce );
	}

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void BreakApartDeathBehaviorV2::loadPostProcess()
{

	// extend base class
	UpdateModule::loadPostProcess();

}
