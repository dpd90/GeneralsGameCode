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

// FILE: BreakApartHelicopterSlowDeathBehaviorV2.cpp //////////////////////////////////////////////
// GeneralsMod @feature Dimitar 10/11/2026 -- see the header for the design.
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
#include "GameLogic/Module/BreakApartHelicopterSlowDeathBehaviorV2.h"
#include "GameLogic/Module/PhysicsUpdate.h"
#include <vector>
#include "GameLogic/Object.h"
#include "GameLogic/ObjectCreationList.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BreakApartHelicopterSlowDeathBehaviorV2ModuleData::BreakApartHelicopterSlowDeathBehaviorV2ModuleData()
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
	m_forceSpreadAngle = PI / 3.0f;	// 60 degrees, same default as the jet/vehicle modules
	m_inheritVelocity = 1.0f;
	m_breakApartPieceOCL = nullptr;
	m_breakApartPieceFX = nullptr;
}

//-------------------------------------------------------------------------------------------------
/*static*/ void BreakApartHelicopterSlowDeathBehaviorV2ModuleData::buildFieldParse( MultiIniFieldParse &p )
{
	HelicopterSlowDeathBehaviorModuleData::buildFieldParse( p );

	static const FieldParse dataFieldParse[] =
	{
		{ "BreakApartModel",												INI::parseAsciiString,					nullptr, offsetof( BreakApartHelicopterSlowDeathBehaviorV2ModuleData, m_breakApartModel ) },
		{ "AlwaysBreakApartSubObject",							BreakApartParseBoneNameList,		nullptr, offsetof( BreakApartHelicopterSlowDeathBehaviorV2ModuleData, m_alwaysBreakApartSubObject ) },
		{ "BreakApartSubObject",										BreakApartParseBoneNameList,		nullptr, offsetof( BreakApartHelicopterSlowDeathBehaviorV2ModuleData, m_breakApartSubObject ) },
		{ "OverkillBreakApartSubObject",						BreakApartParseBoneNameList,		nullptr, offsetof( BreakApartHelicopterSlowDeathBehaviorV2ModuleData, m_overkillBreakApartSubObject ) },
		{ "ExtremeOverkillBreakApartSubObject",			BreakApartParseBoneNameList,		nullptr, offsetof( BreakApartHelicopterSlowDeathBehaviorV2ModuleData, m_extremeOverkillBreakApartSubObject ) },
		{ "RemainSubObject",												BreakApartParseBoneNameList,		nullptr, offsetof( BreakApartHelicopterSlowDeathBehaviorV2ModuleData, m_remainSubObject ) },
		{ "BreakApartChance",												INI::parsePercentToReal,				nullptr, offsetof( BreakApartHelicopterSlowDeathBehaviorV2ModuleData, m_breakApartChance ) },
		{ "OverkillBreakApartChance",								INI::parsePercentToReal,				nullptr, offsetof( BreakApartHelicopterSlowDeathBehaviorV2ModuleData, m_overkillBreakApartChance ) },
		{ "ExtremeOverkillBreakApartChance",				INI::parsePercentToReal,				nullptr, offsetof( BreakApartHelicopterSlowDeathBehaviorV2ModuleData, m_extremeOverkillBreakApartChance ) },
		{ "OverkillPercentageFromMaxHealth",				INI::parsePercentToReal,				nullptr, offsetof( BreakApartHelicopterSlowDeathBehaviorV2ModuleData, m_overkillPercentageFromMaxHealth ) },
		{ "OverkillDamageGreaterOrEqThan",					INI::parseReal,									nullptr, offsetof( BreakApartHelicopterSlowDeathBehaviorV2ModuleData, m_overkillDamageGreaterOrEqThan ) },
		{ "ExtremeOverkillPercentageFromMaxHealth",	INI::parsePercentToReal,				nullptr, offsetof( BreakApartHelicopterSlowDeathBehaviorV2ModuleData, m_extremeOverkillPercentageFromMaxHealth ) },
		{ "ExtremeOverkillDamageGreaterOrEqThan",		INI::parseReal,									nullptr, offsetof( BreakApartHelicopterSlowDeathBehaviorV2ModuleData, m_extremeOverkillDamageGreaterOrEqThan ) },
		{ "Force",																	INI::parseReal,									nullptr, offsetof( BreakApartHelicopterSlowDeathBehaviorV2ModuleData, m_force ) },
		{ "OverkillForce",													INI::parseReal,									nullptr, offsetof( BreakApartHelicopterSlowDeathBehaviorV2ModuleData, m_overkillForce ) },
		{ "ExtremeOverkillForce",										INI::parseReal,									nullptr, offsetof( BreakApartHelicopterSlowDeathBehaviorV2ModuleData, m_extremeOverkillForce ) },
		{ "ForceSpreadAngle",												INI::parseAngleReal,						nullptr, offsetof( BreakApartHelicopterSlowDeathBehaviorV2ModuleData, m_forceSpreadAngle ) },
		{ "InheritVelocity",												INI::parsePercentToReal,				nullptr, offsetof( BreakApartHelicopterSlowDeathBehaviorV2ModuleData, m_inheritVelocity ) },
		{ "BreakApartPieceOCL",											INI::parseObjectCreationList,		nullptr, offsetof( BreakApartHelicopterSlowDeathBehaviorV2ModuleData, m_breakApartPieceOCL ) },
		{ "BreakApartPieceFX",											INI::parseFXList,								nullptr, offsetof( BreakApartHelicopterSlowDeathBehaviorV2ModuleData, m_breakApartPieceFX ) },
		{ nullptr, nullptr, nullptr, 0 }
	};

	p.add( dataFieldParse );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BreakApartHelicopterSlowDeathBehaviorV2::BreakApartHelicopterSlowDeathBehaviorV2( Thing *thing, const ModuleData *moduleData )
	: HelicopterSlowDeathBehavior( thing, moduleData )
{
	m_deathTier = 0;
	m_alwaysPartsDone = FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BreakApartHelicopterSlowDeathBehaviorV2::~BreakApartHelicopterSlowDeathBehaviorV2()
{
}

//-------------------------------------------------------------------------------------------------
/** Called by SlowDeathBehavior::onDie() only when THIS module wins the slow-death lottery. The
	* random parts tear off right now; the guaranteed parts wait for the vanilla blade fly-off moment
	* (HelicopterSlowDeathBehavior::beginSlowDeath() has just picked m_bladeFlyOffFrame), unless both
	* blade delays are 0, in which case they go right now too. */
//-------------------------------------------------------------------------------------------------
void BreakApartHelicopterSlowDeathBehaviorV2::beginSlowDeath( const DamageInfo *damageInfo )
{
	// SlowDeathBehavior::beginSlowDeath() guards itself against a second call; mirror that so a
	// repeat call can't roll and tear off more parts.
	const Bool alreadyActivated = isSlowDeathActivated();

	HelicopterSlowDeathBehavior::beginSlowDeath( damageInfo );

	if( alreadyActivated )
		return;

	const BreakApartHelicopterSlowDeathBehaviorV2ModuleData *d = getBreakApartHelicopterSlowDeathBehaviorV2ModuleData();
	m_deathTier = BreakApartCalcOverkillTier( getObject(), damageInfo,
		d->m_overkillPercentageFromMaxHealth, d->m_overkillDamageGreaterOrEqThan,
		d->m_extremeOverkillPercentageFromMaxHealth, d->m_extremeOverkillDamageGreaterOrEqThan );

	breakApartRandomParts();

	// no blade delay configured -> vanilla never runs its blade event; break the guaranteed parts now
	if( m_bladeFlyOffFrame == 0 )
		breakApartAlwaysParts();
}

//-------------------------------------------------------------------------------------------------
/** Runs the vanilla update first (spiral, blade countdown + FXBlade/OCLBlade/pilot eject, hit
	* ground, final blow-up), then breaks the guaranteed parts on the frame the blade countdown
	* reaches 0, or on ground impact if the countdown hadn't finished yet (vanilla skips its blade
	* event once on the ground, but the guaranteed parts must still come off). */
//-------------------------------------------------------------------------------------------------
UpdateSleepTime BreakApartHelicopterSlowDeathBehaviorV2::update()
{
	UpdateSleepTime result = HelicopterSlowDeathBehavior::update();

	if( !m_alwaysPartsDone && isSlowDeathActivated() && !getObject()->isDestroyed() &&
			( m_bladeFlyOffFrame == 0 || m_hitGroundFrame != 0 ) )
	{
		breakApartAlwaysParts();
	}

	return result;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Real BreakApartHelicopterSlowDeathBehaviorV2::getTierForce()
{
	const BreakApartHelicopterSlowDeathBehaviorV2ModuleData *d = getBreakApartHelicopterSlowDeathBehaviorV2ModuleData();
	return (m_deathTier == 2) ? d->m_extremeOverkillForce :
				 (m_deathTier == 1) ? d->m_overkillForce :
				 d->m_force;
}

//-------------------------------------------------------------------------------------------------
/** Event 1 -- same per-part rolling as BreakApartJetSlowDeathBehaviorV2::breakApartParts(), except
	* that everything inside an AlwaysBreakApartSubObject subtree is reserved for event 2 and never
	* rolls here (it is still carried along if a random part above it breaks). */
//-------------------------------------------------------------------------------------------------
void BreakApartHelicopterSlowDeathBehaviorV2::breakApartRandomParts()
{
	const BreakApartHelicopterSlowDeathBehaviorV2ModuleData *d = getBreakApartHelicopterSlowDeathBehaviorV2ModuleData();

	if( d->m_breakApartModel.isEmpty() )
		return;

	// This tier's root list; an empty overkill/extreme list falls back to BreakApartSubObject.
	const BreakApartBoneNameVec *rootBones = &d->m_breakApartSubObject;
	if( m_deathTier == 2 && !d->m_extremeOverkillBreakApartSubObject.empty() )
		rootBones = &d->m_extremeOverkillBreakApartSubObject;
	else if( m_deathTier == 1 && !d->m_overkillBreakApartSubObject.empty() )
		rootBones = &d->m_overkillBreakApartSubObject;

	if( rootBones->empty() )
		return;

	const Real chance = (m_deathTier == 2) ? d->m_extremeOverkillBreakApartChance :
											(m_deathTier == 1) ? d->m_overkillBreakApartChance :
											d->m_breakApartChance;

	if( chance <= 0.0f )
		return;

	// Bones that never roll on their own: RemainSubObject subtrees, plus the guaranteed parts'
	// subtrees (reserved for event 2).
	BreakApartBoneNameVec noRollBones;
	for( BreakApartBoneNameVec::const_iterator it = d->m_remainSubObject.begin(); it != d->m_remainSubObject.end(); ++it )
		BreakApartAppendSubtree( d->m_breakApartModel, *it, BreakApartBoneNameVec(), &noRollBones );
	for( BreakApartBoneNameVec::const_iterator it = d->m_alwaysBreakApartSubObject.begin(); it != d->m_alwaysBreakApartSubObject.end(); ++it )
		BreakApartAppendSubtree( d->m_breakApartModel, *it, BreakApartBoneNameVec(), &noRollBones );

	// Every mesh bone in each listed root's subtree rolls once (deepest first, synced RNG).
	BreakApartBoneNameVec rolledBones;
	BreakApartBoneNameVec brokenBones;		// deepest first
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

			if( BreakApartBoneNameListContains( noRollBones, *boneIt ) )
				continue;

			const Bool breaks = (chance >= 1.0f) || (GameLogicRandomValueReal( 0.0f, 1.0f ) < chance);
			if( breaks )
				brokenBones.push_back( *boneIt );
		}
	}

	// Same as the jet (user confirmed 11/11/2026): a broken part takes every part below it that didn't
	// break on its own roll, so a broken parent flies off together with its unbroken children.
	detachPieces( brokenBones, TRUE );
}

//-------------------------------------------------------------------------------------------------
/** Event 2 -- AlwaysBreakApartSubObject. Runs exactly once. A listed bone with a mesh breaks off as
	* one piece carrying its subtree; a listed bone without a mesh sends each of its top-most mesh
	* children off as separate pieces. Anything already removed (carried off by a random part in
	* event 1) is skipped. */
//-------------------------------------------------------------------------------------------------
void BreakApartHelicopterSlowDeathBehaviorV2::breakApartAlwaysParts()
{
	if( m_alwaysPartsDone )
		return;
	m_alwaysPartsDone = TRUE;

	const BreakApartHelicopterSlowDeathBehaviorV2ModuleData *d = getBreakApartHelicopterSlowDeathBehaviorV2ModuleData();
	if( d->m_breakApartModel.isEmpty() || d->m_alwaysBreakApartSubObject.empty() )
		return;

	BreakApartBoneNameVec pieceBones;
	for( BreakApartBoneNameVec::const_iterator rootIt = d->m_alwaysBreakApartSubObject.begin(); rootIt != d->m_alwaysBreakApartSubObject.end(); ++rootIt )
	{
		// mesh bones only, children before parent; the root itself is last if it has a mesh
		BreakApartBoneNameVec subtree;
		if( !GetBreakApartSubtreeBones( d->m_breakApartModel, *rootIt, &subtree ) || subtree.empty() )
			continue;

		if( BreakApartBoneNameListContains( subtree, *rootIt ) )
		{
			// root has a mesh: one piece, everything below rides along
			if( !BreakApartBoneNameListContains( m_removedBones, *rootIt ) && !BreakApartBoneNameListContains( pieceBones, *rootIt ) )
				pieceBones.push_back( *rootIt );
			continue;
		}

		// meshless root (e.g. a bare hub bone): every top-most mesh bone below it is its own piece.
		// A bone is top-most if it isn't inside another subtree bone's own subtree.
		BreakApartBoneNameVec nonTopBones;
		for( BreakApartBoneNameVec::const_iterator it = subtree.begin(); it != subtree.end(); ++it )
		{
			BreakApartBoneNameVec childSubtree;
			if( !GetBreakApartSubtreeBones( d->m_breakApartModel, *it, &childSubtree ) )
				continue;
			for( BreakApartBoneNameVec::const_iterator cIt = childSubtree.begin(); cIt != childSubtree.end(); ++cIt )
			{
				if( cIt->compareNoCase( *it ) != 0 && !BreakApartBoneNameListContains( nonTopBones, *cIt ) )
					nonTopBones.push_back( *cIt );
			}
		}

		for( BreakApartBoneNameVec::const_iterator it = subtree.begin(); it != subtree.end(); ++it )
		{
			if( BreakApartBoneNameListContains( nonTopBones, *it ) )
				continue;
			if( BreakApartBoneNameListContains( m_removedBones, *it ) || BreakApartBoneNameListContains( pieceBones, *it ) )
				continue;
			pieceBones.push_back( *it );
		}
	}


	// a guaranteed part with a mesh (e.g. a rotor) takes everything below it (its blades) along
	detachPieces( pieceBones, TRUE );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void BreakApartHelicopterSlowDeathBehaviorV2::detachPieces( const BreakApartBoneNameVec& pieceBones, Bool carryAttached )
{
	if( pieceBones.empty() )
		return;

	const BreakApartHelicopterSlowDeathBehaviorV2ModuleData *d = getBreakApartHelicopterSlowDeathBehaviorV2ModuleData();
	Object *obj = getObject();

	// carryAttached: for each piece (deepest first), collect the parts still attached under it --
	// every mesh bone in its subtree that isn't a piece itself, wasn't already claimed by a deeper
	// piece, and isn't already gone from the helicopter. Without it, every piece is just its own mesh.
	std::vector<BreakApartBoneNameVec> attachedPerPiece( pieceBones.size() );
	BreakApartBoneNameVec claimedBones;
	for( size_t i = 0; carryAttached && i < pieceBones.size(); ++i )
	{
		BreakApartBoneNameVec subtree;
		GetBreakApartSubtreeBones( d->m_breakApartModel, pieceBones[i], &subtree );
		for( BreakApartBoneNameVec::const_iterator it = subtree.begin(); it != subtree.end(); ++it )
		{
			if( it->compareNoCase( pieceBones[i] ) == 0 )
				continue;
			if( BreakApartBoneNameListContains( pieceBones, *it ) ||
					BreakApartBoneNameListContains( claimedBones, *it ) ||
					BreakApartBoneNameListContains( m_removedBones, *it ) )
				continue;
			attachedPerPiece[i].push_back( *it );
			claimedBones.push_back( *it );
		}
	}

	// Pieces keep InheritVelocity of the helicopter's current velocity on top of the outward Force.
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

	// Spawn first (reads the live bone transforms off the helicopter's Drawable), then hide.
	if( d->m_breakApartPieceOCL != nullptr )
	{
		const Real force = getTierForce();
		for( size_t i = 0; i < pieceBones.size(); ++i )
		{
			BreakApartSpawnPiece( obj, d->m_breakApartModel, pieceBones[i], d->m_breakApartPieceOCL, d->m_breakApartPieceFX,
				force, d->m_forceSpreadAngle, inheritedVelocityPtr, &attachedPerPiece[i] );
		}
	}

	// remember what's gone (pieces + what they carried), so event 2 never spawns a part that already
	// left in event 1, then bring the live model in line with it
	m_removedBones.insert( m_removedBones.end(), pieceBones.begin(), pieceBones.end() );
	m_removedBones.insert( m_removedBones.end(), claimedBones.begin(), claimedBones.end() );

	syncLiveSubObjects();
}

//-------------------------------------------------------------------------------------------------
/** Makes the live model show exactly what is still attached: every removed part hidden, and every
	* part BELOW a removed part that is still attached explicitly re-shown. The re-show is needed
	* because W3DModelDraw hides every mesh on child bones of a hidden mesh's bone
	* (doHideShowBoneSubObjs) -- e.g. with FRONTL as the parent of the whole SWHawk hierarchy, hiding
	* FRONTL alone would otherwise make the rotors, pods and tail vanish with it.
	*
	* W3DModelDraw applies m_subObjectVec in vector order and each entry also cascades to the meshes
	* below it, so entries are written parent-first (a child's entry must come after its parent's).
	* Existing entries are updated in place by showSubObject(); with only two break events and the
	* guaranteed parts' subtrees reserved in event 1, a later new entry is never an ancestor of an
	* earlier one with the opposite visibility. The vector re-applies after every model condition swap
	* (RUBBLE, SPECIAL_DAMAGED) and is xfered by W3DModelDraw itself. */
//-------------------------------------------------------------------------------------------------
void BreakApartHelicopterSlowDeathBehaviorV2::syncLiveSubObjects()
{
	const BreakApartHelicopterSlowDeathBehaviorV2ModuleData *d = getBreakApartHelicopterSlowDeathBehaviorV2ModuleData();
	Drawable *draw = getObject()->getDrawable();
	if( draw == nullptr || m_removedBones.empty() )
		return;

	// affected = removed parts + still-attached parts below a removed part
	BreakApartBoneNameVec affected = m_removedBones;
	for( BreakApartBoneNameVec::const_iterator rIt = m_removedBones.begin(); rIt != m_removedBones.end(); ++rIt )
	{
		BreakApartBoneNameVec subtree;
		GetBreakApartSubtreeBones( d->m_breakApartModel, *rIt, &subtree );
		for( BreakApartBoneNameVec::const_iterator it = subtree.begin(); it != subtree.end(); ++it )
		{
			if( !BreakApartBoneNameListContains( affected, *it ) )
				affected.push_back( *it );
		}
	}

	// depth inside the affected set = how many other affected bones have it in their subtree
	std::vector<BreakApartBoneNameVec> subtrees( affected.size() );
	for( size_t i = 0; i < affected.size(); ++i )
		GetBreakApartSubtreeBones( d->m_breakApartModel, affected[i], &subtrees[i] );

	std::vector<Int> depth( affected.size(), 0 );
	Int maxDepth = 0;
	for( size_t i = 0; i < affected.size(); ++i )
	{
		for( size_t j = 0; j < affected.size(); ++j )
		{
			if( i != j && BreakApartBoneNameListContains( subtrees[j], affected[i] ) )
				++depth[i];
		}
		if( depth[i] > maxDepth )
			maxDepth = depth[i];
	}

	// write parent-first
	Bool anyChange = FALSE;
	for( Int level = 0; level <= maxDepth; ++level )
	{
		for( size_t i = 0; i < affected.size(); ++i )
		{
			if( depth[i] != level )
				continue;

			const Bool show = !BreakApartBoneNameListContains( m_removedBones, affected[i] );
			BreakApartBoneNameVec subObjectNames;
			if( !GetBreakApartBoneSubObjectNames( d->m_breakApartModel, affected[i], &subObjectNames ) )
				continue;

			for( BreakApartBoneNameVec::const_iterator nameIt = subObjectNames.begin(); nameIt != subObjectNames.end(); ++nameIt )
			{
				draw->showSubObject( *nameIt, show );
				anyChange = TRUE;
			}
		}
	}

	if( anyChange )
		draw->updateSubObjects();
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void BreakApartHelicopterSlowDeathBehaviorV2::crc( Xfer *xfer )
{
	// extend base class
	HelicopterSlowDeathBehavior::crc( xfer );
}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version -- death tier, whether the guaranteed parts already broke, and the bones
	*    already gone from the live model (a save can land between death and the blade fly-off). The
	*    hidden subobjects themselves are persisted by W3DModelDraw's own xfer of m_subObjectVec. */
// ------------------------------------------------------------------------------------------------
void BreakApartHelicopterSlowDeathBehaviorV2::xfer( Xfer *xfer )
{
	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	HelicopterSlowDeathBehavior::xfer( xfer );

	xfer->xferInt( &m_deathTier );
	xfer->xferBool( &m_alwaysPartsDone );

	UnsignedShort removedCount = (UnsignedShort)m_removedBones.size();
	xfer->xferUnsignedShort( &removedCount );
	if( xfer->getXferMode() == XFER_LOAD )
	{
		m_removedBones.clear();
		for( UnsignedShort i = 0; i < removedCount; ++i )
		{
			AsciiString boneName;
			xfer->xferAsciiString( &boneName );
			m_removedBones.push_back( boneName );
		}
	}
	else
	{
		for( BreakApartBoneNameVec::iterator it = m_removedBones.begin(); it != m_removedBones.end(); ++it )
			xfer->xferAsciiString( &(*it) );
	}
}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void BreakApartHelicopterSlowDeathBehaviorV2::loadPostProcess()
{
	// extend base class
	HelicopterSlowDeathBehavior::loadPostProcess();
}
