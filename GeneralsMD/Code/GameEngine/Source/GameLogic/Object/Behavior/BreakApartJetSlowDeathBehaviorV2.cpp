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

// FILE: BreakApartJetSlowDeathBehaviorV2.cpp /////////////////////////////////////////////////////
// GeneralsMod @feature Dimitar 26/10/2026 -- see the header for the design.
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/INI.h"
#include "Common/RandomValue.h"
#include "Common/ThingTemplate.h"
#include "Common/Xfer.h"
#include "GameClient/BreakApartModelHelper.h"
#include "GameClient/Drawable.h"
#include "GameClient/FXList.h"
#include "GameLogic/BreakApartDebrisHelper.h"
#include "GameLogic/Damage.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Module/BodyModule.h"
#include "GameLogic/Module/BreakApartJetSlowDeathBehaviorV2.h"
#include "GameLogic/Module/PhysicsUpdate.h"
#include <vector>
#include "GameLogic/Object.h"
#include "GameLogic/ObjectCreationList.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BreakApartJetSlowDeathBehaviorV2ModuleData::BreakApartJetSlowDeathBehaviorV2ModuleData()
{
	m_breakApartChance = 0.0f;
	m_overkillBreakApartChance = 0.0f;
	m_extremeOverkillBreakApartChance = 0.0f;
	m_overkillPercentageFromMaxHealth = HUGE_DAMAGE_AMOUNT;
	m_overkillDamageGreaterOrEqThan = HUGE_DAMAGE_AMOUNT;
	m_extremeOverkillPercentageFromMaxHealth = HUGE_DAMAGE_AMOUNT;
	m_extremeOverkillDamageGreaterOrEqThan = HUGE_DAMAGE_AMOUNT;
	m_force = 0.0f;
	m_overkillForce = 0.0f;
	m_extremeOverkillForce = 0.0f;
	m_forceSpreadAngle = PI / 3.0f;	// 60 degrees, same default as BreakApartDeathBehaviorV2
	m_inheritVelocity = 1.0f;
	m_breakApartPieceOCL = nullptr;
	m_breakApartPieceFX = nullptr;
	m_oclSpecificHitGround = nullptr;
	m_oclSpecificFinalBlowUp = nullptr;
	m_oclSpecificOnGroundDeath = nullptr;
	m_airPitchRate = 0.0f;
	m_airPitchRateDelta = 1.0f;
	m_maxAirPitch = 0.0f;
}

//-------------------------------------------------------------------------------------------------
/*static*/ void BreakApartJetSlowDeathBehaviorV2ModuleData::buildFieldParse( MultiIniFieldParse &p )
{
	JetSlowDeathBehaviorModuleData::buildFieldParse( p );

	static const FieldParse dataFieldParse[] =
	{
		{ "BreakApartModel",												INI::parseAsciiString,					nullptr, offsetof( BreakApartJetSlowDeathBehaviorV2ModuleData, m_breakApartModel ) },
		{ "BreakApartSubObject",										BreakApartParseBoneNameList,		nullptr, offsetof( BreakApartJetSlowDeathBehaviorV2ModuleData, m_breakApartSubObject ) },
		{ "OverkillBreakApartSubObject",						BreakApartParseBoneNameList,		nullptr, offsetof( BreakApartJetSlowDeathBehaviorV2ModuleData, m_overkillBreakApartSubObject ) },
		{ "ExtremeOverkillBreakApartSubObject",			BreakApartParseBoneNameList,		nullptr, offsetof( BreakApartJetSlowDeathBehaviorV2ModuleData, m_extremeOverkillBreakApartSubObject ) },
		{ "RemainSubObject",												BreakApartParseBoneNameList,		nullptr, offsetof( BreakApartJetSlowDeathBehaviorV2ModuleData, m_remainSubObject ) },
		{ "BreakApartChance",												INI::parsePercentToReal,				nullptr, offsetof( BreakApartJetSlowDeathBehaviorV2ModuleData, m_breakApartChance ) },
		{ "OverkillBreakApartChance",								INI::parsePercentToReal,				nullptr, offsetof( BreakApartJetSlowDeathBehaviorV2ModuleData, m_overkillBreakApartChance ) },
		{ "ExtremeOverkillBreakApartChance",				INI::parsePercentToReal,				nullptr, offsetof( BreakApartJetSlowDeathBehaviorV2ModuleData, m_extremeOverkillBreakApartChance ) },
		{ "OverkillPercentageFromMaxHealth",				INI::parsePercentToReal,				nullptr, offsetof( BreakApartJetSlowDeathBehaviorV2ModuleData, m_overkillPercentageFromMaxHealth ) },
		{ "OverkillDamageGreaterOrEqThan",					INI::parseReal,									nullptr, offsetof( BreakApartJetSlowDeathBehaviorV2ModuleData, m_overkillDamageGreaterOrEqThan ) },
		{ "ExtremeOverkillPercentageFromMaxHealth",	INI::parsePercentToReal,				nullptr, offsetof( BreakApartJetSlowDeathBehaviorV2ModuleData, m_extremeOverkillPercentageFromMaxHealth ) },
		{ "ExtremeOverkillDamageGreaterOrEqThan",		INI::parseReal,									nullptr, offsetof( BreakApartJetSlowDeathBehaviorV2ModuleData, m_extremeOverkillDamageGreaterOrEqThan ) },
		{ "Force",																	INI::parseReal,									nullptr, offsetof( BreakApartJetSlowDeathBehaviorV2ModuleData, m_force ) },
		{ "OverkillForce",													INI::parseReal,									nullptr, offsetof( BreakApartJetSlowDeathBehaviorV2ModuleData, m_overkillForce ) },
		{ "ExtremeOverkillForce",										INI::parseReal,									nullptr, offsetof( BreakApartJetSlowDeathBehaviorV2ModuleData, m_extremeOverkillForce ) },
		{ "ForceSpreadAngle",												INI::parseAngleReal,						nullptr, offsetof( BreakApartJetSlowDeathBehaviorV2ModuleData, m_forceSpreadAngle ) },
		{ "InheritVelocity",												INI::parsePercentToReal,				nullptr, offsetof( BreakApartJetSlowDeathBehaviorV2ModuleData, m_inheritVelocity ) },
		{ "BreakApartPieceOCL",											INI::parseObjectCreationList,		nullptr, offsetof( BreakApartJetSlowDeathBehaviorV2ModuleData, m_breakApartPieceOCL ) },
		{ "BreakApartPieceFX",											INI::parseFXList,								nullptr, offsetof( BreakApartJetSlowDeathBehaviorV2ModuleData, m_breakApartPieceFX ) },
		{ "OCLSpecificHitGround",										INI::parseObjectCreationList,		nullptr, offsetof( BreakApartJetSlowDeathBehaviorV2ModuleData, m_oclSpecificHitGround ) },
		{ "OCLSpecificFinalBlowUp",									INI::parseObjectCreationList,		nullptr, offsetof( BreakApartJetSlowDeathBehaviorV2ModuleData, m_oclSpecificFinalBlowUp ) },
		{ "OCLSpecificOnGroundDeath",								INI::parseObjectCreationList,		nullptr, offsetof( BreakApartJetSlowDeathBehaviorV2ModuleData, m_oclSpecificOnGroundDeath ) },
		{ "AirPitchRate",														INI::parseReal,									nullptr, offsetof( BreakApartJetSlowDeathBehaviorV2ModuleData, m_airPitchRate ) },
		{ "AirPitchRateDelta",											INI::parsePercentToReal,				nullptr, offsetof( BreakApartJetSlowDeathBehaviorV2ModuleData, m_airPitchRateDelta ) },
		{ "MaxAirPitch",														INI::parseAngleReal,						nullptr, offsetof( BreakApartJetSlowDeathBehaviorV2ModuleData, m_maxAirPitch ) },
		{ nullptr, nullptr, nullptr, 0 }
	};

	p.add( dataFieldParse );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BreakApartJetSlowDeathBehaviorV2::BreakApartJetSlowDeathBehaviorV2( Thing *thing, const ModuleData *moduleData )
	: JetSlowDeathBehavior( thing, moduleData )
{
	m_airPitchRate = 0.0f;
	m_airPitchAngle = 0.0f;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BreakApartJetSlowDeathBehaviorV2::~BreakApartJetSlowDeathBehaviorV2()
{
}

//-------------------------------------------------------------------------------------------------
/** GeneralsMod @feature Dimitar 11/11/2026: OCLSpecificOnGroundDeath.
	* JetSlowDeathBehavior::onDie() handles a jet that dies on the ground / on a carrier deck itself
	* (FXOnGroundDeath + OCLOnGroundDeath, then destroys it instantly, no slow death). We use the exact
	* same test BEFORE calling the base, because the base clears OBJECT_STATUS_DECK_HEIGHT_OFFSET and
	* destroys the object -- so our OCL runs on the still-alive jet, same frame, just before the
	* generic one. */
//-------------------------------------------------------------------------------------------------
void BreakApartJetSlowDeathBehaviorV2::onDie( const DamageInfo *damageInfo )
{
	Object *us = getObject();

	if( us->isSignificantlyAboveTerrain() == FALSE || us->getStatusBits().test( OBJECT_STATUS_DECK_HEIGHT_OFFSET ) )
	{
		const BreakApartJetSlowDeathBehaviorV2ModuleData *d = getBreakApartJetSlowDeathBehaviorV2ModuleData();
		ObjectCreationList::create( d->m_oclSpecificOnGroundDeath, us, nullptr );
	}

	JetSlowDeathBehavior::onDie( damageInfo );
}

//-------------------------------------------------------------------------------------------------
/** Called by SlowDeathBehavior::onDie() only when THIS module wins the slow-death lottery, and only
	* for an airborne death (JetSlowDeathBehavior::onDie() handles the on-ground/deck case itself and
	* never gets here). The jet is still flying at full speed at this point, which is exactly when the
	* parts should tear off -- before the roll/fall has even started. */
//-------------------------------------------------------------------------------------------------
void BreakApartJetSlowDeathBehaviorV2::beginSlowDeath( const DamageInfo *damageInfo )
{
	// SlowDeathBehavior::beginSlowDeath() guards itself against a second call; mirror that for our
	// own part so a repeat call can't roll and tear off more parts.
	const Bool alreadyActivated = isSlowDeathActivated();

	JetSlowDeathBehavior::beginSlowDeath( damageInfo );

	if( alreadyActivated )
		return;

	const BreakApartJetSlowDeathBehaviorV2ModuleData *d = getBreakApartJetSlowDeathBehaviorV2ModuleData();

	// in-air nose-down tip starts at its INI rate (applied per frame in update())
	m_airPitchRate = d->m_airPitchRate;
	m_airPitchAngle = 0.0f;
	Int tier = BreakApartCalcOverkillTier( getObject(), damageInfo,
		d->m_overkillPercentageFromMaxHealth, d->m_overkillDamageGreaterOrEqThan,
		d->m_extremeOverkillPercentageFromMaxHealth, d->m_extremeOverkillDamageGreaterOrEqThan );

	breakApartParts( tier );
}

//-------------------------------------------------------------------------------------------------
/** GeneralsMod @feature Dimitar 11/11/2026: OCLSpecificHitGround / OCLSpecificFinalBlowUp.
	* JetSlowDeathBehavior::update() fires OCLHitGround / OCLFinalBlowUp inline, so instead of touching
	* the stock class we mirror its own conditions around the base call:
	*   - final blow-up: same test the base uses (on the ground AND DelayFinalBlowUpFromHitGround
	*     elapsed), checked BEFORE the base call, because the base destroys the object right after
	*     its own OCL -- this way ours runs while the jet is still fully alive.
	*   - hit ground: m_timerOnGroundFrame goes 0 -> non-zero inside the base call exactly on the
	*     frame OCLHitGround fires, so we fire ours right after it.
	* No extra state: both triggers are derived from the base's own (already xfer'd) timers. */
//-------------------------------------------------------------------------------------------------
UpdateSleepTime BreakApartJetSlowDeathBehaviorV2::update()
{
	const BreakApartJetSlowDeathBehaviorV2ModuleData *d = getBreakApartJetSlowDeathBehaviorV2ModuleData();
	Object *us = getObject();

	const Bool wasInAir = (m_timerOnGroundFrame == 0);

	if( isSlowDeathActivated() && !wasInAir &&
			TheGameLogic->getFrame() - m_timerOnGroundFrame >= d->m_delayFinalBlowUpFromHitGround )
	{
		ObjectCreationList::create( d->m_oclSpecificFinalBlowUp, us, nullptr );
	}

	UpdateSleepTime ret = JetSlowDeathBehavior::update();

	if( wasInAir && m_timerOnGroundFrame != 0 )
	{
		ObjectCreationList::create( d->m_oclSpecificHitGround, us, nullptr );
	}

	// GeneralsMod @feature Dimitar 13/11/2026: in-air nose-down tip (AirPitchRate).
	// @fix 13/11/2026: accumulate our OWN angle and write it onto the transform, instead of handing
	// PhysicsBehavior a pitch rate -- the dying jet's locomotor re-levels it every frame via
	// Thing::setOrientation() (see the header), so a physics rate never builds up. This module updates
	// in PHASE_NORMAL, after AIUpdate (PHASE_INITIAL) and PhysicsBehavior (PHASE_PHYSICS), so what we
	// write here is what gets drawn. We rotate about the jet's local Y axis by the DIFFERENCE between
	// the target angle and the nose-down angle the matrix currently has, so whatever else is on the
	// matrix (yaw, and roll if the locomotor didn't wipe it) is kept. Positive Rotate_Y = nose down
	// (verified in matrix3d.h). On the ground-hit frame the base has set m_timerOnGroundFrame and its
	// own PitchRate; we stop touching the jet from then on.
	if( isSlowDeathActivated() && m_timerOnGroundFrame == 0 && d->m_airPitchRate != 0.0f && !us->isDestroyed() )
	{
		m_airPitchAngle += m_airPitchRate;
		m_airPitchRate *= d->m_airPitchRateDelta;
		if( d->m_maxAirPitch > 0.0f && m_airPitchAngle > d->m_maxAirPitch )
			m_airPitchAngle = d->m_maxAirPitch;

		Matrix3D mtx = *us->getTransformMatrix();
		const Vector3 nose = mtx.Get_X_Vector();
		const Real xy = WWMath::Sqrtf( nose.X * nose.X + nose.Y * nose.Y );
		const Real currentNoseDown = WWMath::Atan2( -nose.Z, xy );
		const Real delta = m_airPitchAngle - currentNoseDown;
		if( WWMath::Fabs( delta ) > 0.0001f )
		{
			mtx.Rotate_Y( delta );
			us->setTransformMatrix( &mtx );
		}
	}

	return ret;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void BreakApartJetSlowDeathBehaviorV2::breakApartParts( Int tier )
{
	const BreakApartJetSlowDeathBehaviorV2ModuleData *d = getBreakApartJetSlowDeathBehaviorV2ModuleData();
	Object *obj = getObject();

	if( d->m_breakApartModel.isEmpty() )
		return;

	// This tier's root list; an empty overkill/extreme list falls back to BreakApartSubObject.
	const BreakApartBoneNameVec *rootBones = &d->m_breakApartSubObject;
	if( tier == 2 && !d->m_extremeOverkillBreakApartSubObject.empty() )
		rootBones = &d->m_extremeOverkillBreakApartSubObject;
	else if( tier == 1 && !d->m_overkillBreakApartSubObject.empty() )
		rootBones = &d->m_overkillBreakApartSubObject;

	if( rootBones->empty() )
		return;

	const Real chance = (tier == 2) ? d->m_extremeOverkillBreakApartChance :
											(tier == 1) ? d->m_overkillBreakApartChance :
											d->m_breakApartChance;
	const Real force = (tier == 2) ? d->m_extremeOverkillForce :
										 (tier == 1) ? d->m_overkillForce :
										 d->m_force;

	if( chance <= 0.0f )
		return;

	// RemainSubObject protection, resolved the same way BreakApartDeathBehaviorV2 does
	BreakApartBoneNameVec protectedBones;
	for( BreakApartBoneNameVec::const_iterator it = d->m_remainSubObject.begin(); it != d->m_remainSubObject.end(); ++it )
		BreakApartAppendSubtree( d->m_breakApartModel, *it, BreakApartBoneNameVec(), &protectedBones );

	// GeneralsMod @redesign Dimitar 26/10/2026 (user's spec): EVERY part with a mesh rolls on its own,
	// not just the listed roots. For each listed root, its subtree is walked deepest-first
	// (GetBreakApartSubtreeBones() order: children before parent, pure bones without a mesh are
	// skipped and never roll, but their children still do). Each mesh bone rolls against this tier's
	// chance; one that passes breaks off as ONE rigid piece together with every part still attached
	// below it that didn't already break off on its own roll. RemainSubObject bones never roll on
	// their own, but are carried along if a part above them breaks. Synced RNG: the result decides
	// which Objects get spawned. Overlapping roots are handled by rolling each bone only once.
	BreakApartBoneNameVec rolledBones;		// every bone that has had its roll (or was skipped as protected)
	BreakApartBoneNameVec brokenBones;		// bones whose own roll passed, deepest first
	for( BreakApartBoneNameVec::const_iterator rootIt = rootBones->begin(); rootIt != rootBones->end(); ++rootIt )
	{
		BreakApartBoneNameVec subtree;
		if( !GetBreakApartSubtreeBones( d->m_breakApartModel, *rootIt, &subtree ) )
			continue;

		for( BreakApartBoneNameVec::const_iterator boneIt = subtree.begin(); boneIt != subtree.end(); ++boneIt )
		{
			if( BreakApartBoneNameListContains( rolledBones, *boneIt ) )
				continue;
			rolledBones.push_back( *boneIt );

			if( BreakApartBoneNameListContains( protectedBones, *boneIt ) )
			{
				continue;
			}

			const Bool breaks = (chance >= 1.0f) || (GameLogicRandomValueReal( 0.0f, 1.0f ) < chance);
			if( breaks )
				brokenBones.push_back( *boneIt );
		}
	}

	// For each broken part (deepest first), collect the parts still attached under it: every mesh
	// bone in its subtree that neither broke off on its own nor was already carried by a deeper
	// broken part. Deepest-first order guarantees a broken child claims its own attachments before
	// its broken ancestor looks at the same bones.
	std::vector<BreakApartBoneNameVec> attachedPerPiece( brokenBones.size() );
	BreakApartBoneNameVec claimedBones;
	for( size_t i = 0; i < brokenBones.size(); ++i )
	{
		BreakApartBoneNameVec subtree;
		GetBreakApartSubtreeBones( d->m_breakApartModel, brokenBones[i], &subtree );
		for( BreakApartBoneNameVec::const_iterator it = subtree.begin(); it != subtree.end(); ++it )
		{
			if( it->compareNoCase( brokenBones[i] ) == 0 )
				continue;
			if( BreakApartBoneNameListContains( brokenBones, *it ) || BreakApartBoneNameListContains( claimedBones, *it ) )
				continue;
			attachedPerPiece[i].push_back( *it );
			claimedBones.push_back( *it );
		}
	}

	if( brokenBones.empty() )
		return;

	// Pieces keep InheritVelocity of the jet's own current velocity on top of the outward Force.
	Coord3D inheritedVelocity;
	const Coord3D *inheritedVelocityPtr = nullptr;
	PhysicsBehavior *physics = obj->getPhysics();
	if( physics != nullptr && d->m_inheritVelocity != 0.0f )
	{
		inheritedVelocity = *physics->getVelocity();
		inheritedVelocity.x *= d->m_inheritVelocity;
		inheritedVelocity.y *= d->m_inheritVelocity;
		inheritedVelocity.z *= d->m_inheritVelocity;
		inheritedVelocityPtr = &inheritedVelocity;
	}

	// Spawn first (reads the live bone transforms off the jet's Drawable -- hiding doesn't move bones,
	// but keep the natural order anyway), then hide the same parts on the jet itself.
	if( d->m_breakApartPieceOCL != nullptr )
	{
		for( BreakApartBoneNameVec::const_iterator boneIt = brokenBones.begin(); boneIt != brokenBones.end(); ++boneIt )
		{
			const size_t pieceIndex = boneIt - brokenBones.begin();
			BreakApartSpawnPiece( obj, d->m_breakApartModel, *boneIt, d->m_breakApartPieceOCL, d->m_breakApartPieceFX,
				force, d->m_forceSpreadAngle, inheritedVelocityPtr, &attachedPerPiece[pieceIndex] );
		}
	}

	// Hide the broken parts on the jet's own live model. Drawable::showSubObject() records the
	// request in W3DModelDraw::m_subObjectVec, which W3DModelDraw re-applies after every model
	// condition swap (DAMAGED/REALLYDAMAGED etc.) and xfers itself, so nothing extra needs saving
	// here. Names come from the subobjects actually attached to each bone on BreakApartModel (short
	// name after the "MODEL." prefix), so the live model has to use the same mesh names -- see the
	// ASSET RULE in the header.
	// Broken parts AND the parts they carry (the engine would hide bone-children anyway, but naming
	// them explicitly doesn't rely on the live model's hierarchy matching).
	BreakApartBoneNameVec hideBones = brokenBones;
	hideBones.insert( hideBones.end(), claimedBones.begin(), claimedBones.end() );
	Drawable *draw = obj->getDrawable();
	if( draw != nullptr )
	{
		Bool anyHidden = FALSE;
		for( BreakApartBoneNameVec::const_iterator boneIt = hideBones.begin(); boneIt != hideBones.end(); ++boneIt )
		{
			BreakApartBoneNameVec subObjectNames;
			if( !GetBreakApartBoneSubObjectNames( d->m_breakApartModel, *boneIt, &subObjectNames ) )
				continue;

			for( BreakApartBoneNameVec::const_iterator nameIt = subObjectNames.begin(); nameIt != subObjectNames.end(); ++nameIt )
			{
				draw->showSubObject( *nameIt, FALSE );
				anyHidden = TRUE;
			}
		}

		if( anyHidden )
			draw->updateSubObjects();
	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void BreakApartJetSlowDeathBehaviorV2::crc( Xfer *xfer )
{
	// extend base class
	JetSlowDeathBehavior::crc( xfer );
}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version -- no state of our own: the break-apart happens once, instantly, inside
	*    beginSlowDeath(); spawned pieces are their own Objects, and the hidden subobjects on the jet
	*    are persisted by W3DModelDraw's own xfer of m_subObjectVec.
	* 2: m_airPitchRate (AirPitchRate decays per frame, so the current value must be saved).
	* 3: m_airPitchAngle (the accumulated nose-down angle we re-apply every airborne frame). */
// ------------------------------------------------------------------------------------------------
void BreakApartJetSlowDeathBehaviorV2::xfer( Xfer *xfer )
{
	// version
	XferVersion currentVersion = 3;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	JetSlowDeathBehavior::xfer( xfer );

	if( version >= 2 )
		xfer->xferReal( &m_airPitchRate );
	if( version >= 3 )
		xfer->xferReal( &m_airPitchAngle );
}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void BreakApartJetSlowDeathBehaviorV2::loadPostProcess()
{
	// extend base class
	JetSlowDeathBehavior::loadPostProcess();
}
