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

//-------------------------------------------------------------------------------------------------
/** BreakApartSubObject / OverkillBreakApartSubObject / ExtremeOverkillBreakApartSubObject -- a
	* plain, un-phased, space-separated list of ROOT bone/subobject names. Each root recursively
	* breaks apart its whole subtree in BreakApartModel's own hierarchy (see
	* BreakApartModelHelper.h's GetBreakApartSubtreeBones()) -- naming "TURRET" here also breaks
	* apart "BARREL01"/"BARREL02" if they hang off TURRET in the model, with no need to list them
	* separately. Multiple statements accumulate (same convention as FX/OCL/Weapon above), so a
	* modder can either list every root on one line or split them across several
	* `BreakApartSubObject = ...` lines. */
//-------------------------------------------------------------------------------------------------
static void parseBoneNameList( INI* ini, void * /*instance*/, void *store, const void* /*userData*/ )
{
	BADB_BoneNameVec* v = (BADB_BoneNameVec*)store;
	for (const char* token = ini->getNextTokenOrNull(); token; token = ini->getNextTokenOrNull())
	{
		v->push_back(AsciiString(token));
	}
}

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
		{ "BreakApartSubObject",							parseBoneNameList,							nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_breakApartSubObject ) },
		{ "RemainSubObject",									parseBoneNameList,							nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_remainSubObject ) },
		{ "OverkillPercentageFromMaxHealth",	INI::parsePercentToReal,				nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_overkillPercentageFromMaxHealth ) },
		{ "OverkillDamageGreaterOrEqThan",		INI::parseReal,									nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_overkillDamageGreaterOrEqThan ) },
		{ "OverkillBreakApartSubObject",			parseBoneNameList,							nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_overkillBreakApartSubObject ) },
		{ "ExtremeOverkillPercentageFromMaxHealth", INI::parsePercentToReal,		nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_extremeOverkillPercentageFromMaxHealth ) },
		{ "ExtremeOverkillDamageGreaterOrEqThan", INI::parseReal,					nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_extremeOverkillDamageGreaterOrEqThan ) },
		{ "ExtremeOverkillBreakApartSubObject", parseBoneNameList,							nullptr, offsetof( BreakApartDeathBehaviorV2ModuleData, m_extremeOverkillBreakApartSubObject ) },
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
/** Outward force for a debris piece, with a fixed mostly-upward pitch band -- unlike
	* SlowDeathBehavior's FlingPitch/FlingPitchVariance, the user's own Force/OverkillForce/
	* ExtremeOverkillForce fields carry no pitch control of their own, so this module hardcodes a
	* reasonable "scatter outward and up" pitch range rather than exposing one. Revisit if per-tier
	* pitch control is ever wanted.
	*
	* GeneralsMod @feature Dimitar 21/09/2026: biasDirXY, when non-null, is the (not necessarily
	* normalized) horizontal direction the piece should fly roughly TOWARD -- typically the vector
	* from the dying object's own center to that piece's actual world position (see
	* spawnBreakApartDebris() below), so a part mounted on the tank's left flies off left-ish instead
	* of in a fully random direction that could just as easily send it clean across to the right.
	* The actual yaw is that bias direction's own angle plus a random +/- spreadAngle jitter, so it
	* still scatters naturally rather than every piece flying in a perfectly straight radial line.
	* biasDirXY == nullptr (or a degenerate near-zero-length vector) falls back to the old, fully
	* random -PI..PI behavior -- used for the RemainPieceOCL remainder piece, which has no off-center
	* position of its own to derive a direction from. */
//-------------------------------------------------------------------------------------------------
static void calcRandomOutwardForce(Real magnitude, const Coord3D* biasDirXY, Real spreadAngle, Coord3D& force)
{
	Real angle;
	if (biasDirXY != nullptr && (biasDirXY->x * biasDirXY->x + biasDirXY->y * biasDirXY->y) > 0.0001f)
		angle = (Real)atan2(biasDirXY->y, biasDirXY->x) + GameLogicRandomValueReal(-spreadAngle, spreadAngle);
	else
		angle = GameLogicRandomValueReal(-PI, PI);

	Real pitch = GameLogicRandomValueReal(0.15f * PI, 0.5f * PI);

	Matrix3D mtx(1);
	mtx.Scale(magnitude);
	mtx.Rotate_Z(angle);
	mtx.Rotate_Y(-pitch);

	Vector3 v = mtx.Get_X_Vector();

	force.x = v.X;
	force.y = v.Y;
	force.z = v.Z;
}

//-------------------------------------------------------------------------------------------------
/** Fling + tumble a freshly-spawned debris piece exactly like every other one -- shared by both the
	* per-bone pieces below and the single remainder piece, so the two spawn paths behave identically
	* aside from which subobject(s) end up visible on the clone. */
//-------------------------------------------------------------------------------------------------
static void applyBreakApartDebrisForce(Object* debris, Real force, const Coord3D* biasDirXY, Real spreadAngle)
{
	if (force <= 0.0f)
		return;

	PhysicsBehavior* physics = debris->getPhysics();
	if (physics == nullptr)
		return;

	Coord3D forceVec;
	calcRandomOutwardForce(force, biasDirXY, spreadAngle, forceVec);
	physics->setAllowToFall(true);
	physics->applyForce(&forceVec);
	// GeneralsMod @feature Dimitar 19/09/2026: give the piece some tumble too -- applyForce() alone
	// only ever produces straight-line motion, never rotation. applyRandomRotation() is the same
	// helper the engine's own disposition-based debris (SEND_IT_FLYING/SEND_IT_UP in
	// ObjectCreationList.cpp) effectively hand-rolls itself; it reads ShockMaxYaw/ShockMaxPitch/
	// ShockMaxRoll off the piece's OWN PhysicsBehavior ModuleData (nonzero engine defaults, but
	// overridable per BreakApartDebrisPiece template) and also flips on AllowBouncing.
	physics->applyRandomRotation();
}

//-------------------------------------------------------------------------------------------------
static Bool boneNameListContains(const BADB_BoneNameVec& list, const AsciiString& name)
{
	for (BADB_BoneNameVec::const_iterator it = list.begin(); it != list.end(); ++it)
	{
		if (it->compareNoCase(name) == 0)
			return true;
	}
	return false;
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

	// GeneralsMod @feature Dimitar 19/09/2026: the dying object's OWN Drawable is still alive right
	// now (TheGameLogic->destroyObject() doesn't happen until DestructionDelay elapses, well after
	// this function returns) -- so its Draw module still has the LIVE, per-instance bone state
	// (e.g. turret/barrel aim rotation, applied via Capture_Bone/Control_Bone on the render object
	// every frame) that the standalone/never-rendered BreakApartModel reference clone has no way to
	// know about. Drawable::getCurrentWorldspaceClientBonePositions() is the existing,
	// GameLogic-safe accessor for a bone's CURRENT world transform -- LaserUpdate::updateStartPos()
	// is the existing precedent for calling it from a z_gameengine Update module; it is NOT the
	// same as the clientOnly_-prefixed W3DModelDraw internals underneath it, which really are
	// logic-unsafe. This is purely a per-client VISUAL read: it never feeds into the spawned
	// debris Object's own SYNCED position/orientation (still set from obj->getPosition()/
	// getOrientation() below, unchanged), only into that Object's own Draw module's local
	// rendering offset -- the same category of already-per-client, already-synced-state-derived
	// visual as the live unit's own turret rendering already is.
	const Drawable* drawable = obj->getDrawable();

	// GeneralsMod @feature Dimitar 19/09/2026: resolve RemainSubObject's protection ONCE, up front,
	// reusing GetBreakApartSubtreeBones() itself -- a "protected" root pulls in its own whole
	// subtree exactly the same way a BreakApartSubObject root pulls ITS subtree in for breaking
	// apart, just walked for the opposite purpose. Independent of which tier's rootBones we're
	// processing below, so it only needs to happen once per death.
	std::vector<AsciiString> protectedBones;
	for (BADB_BoneNameVec::const_iterator protIt = d->m_remainSubObject.begin(); protIt != d->m_remainSubObject.end(); ++protIt)
	{
		std::vector<AsciiString> protectedSubtree;
		if (GetBreakApartSubtreeBones(d->m_breakApartModel, *protIt, &protectedSubtree))
			protectedBones.insert(protectedBones.end(), protectedSubtree.begin(), protectedSubtree.end());
	}

	std::vector<AsciiString> allBrokenBones;

	if (d->m_breakApartPieceOCL != nullptr)
	{
		for (BADB_BoneNameVec::const_iterator rootIt = rootBones.begin(); rootIt != rootBones.end(); ++rootIt)
		{
			std::vector<AsciiString> subtreeBones;
			if (!GetBreakApartSubtreeBones(d->m_breakApartModel, *rootIt, &subtreeBones))
				continue;	// root bone missing from the reference model (or model failed to load) -- skip it

			// GeneralsMod @feature Dimitar 19/09/2026: drop any bone explicitly protected via
			// RemainSubObject before it's treated as broken apart at all -- it neither spawns its
			// own piece below nor gets added to allBrokenBones, so it stays visible in the
			// RemainPieceOCL/RemainPieceFX remainder piece instead (see boneNameListContains()
			// above and m_remainSubObject's own comment in the header).
			std::vector<AsciiString> brokenBones;
			for (std::vector<AsciiString>::const_iterator it = subtreeBones.begin(); it != subtreeBones.end(); ++it)
			{
				if (!boneNameListContains(protectedBones, *it))
					brokenBones.push_back(*it);
			}

			allBrokenBones.insert(allBrokenBones.end(), brokenBones.begin(), brokenBones.end());

			for (std::vector<AsciiString>::const_iterator boneIt = brokenBones.begin(); boneIt != brokenBones.end(); ++boneIt)
			{
				Object* debris = ObjectCreationList::create( d->m_breakApartPieceOCL, obj, obj->getPosition(), nullptr, obj->getOrientation() );
				if (debris == nullptr)
					continue;

				// Look up this bone's current world transform on the LIVE dying Drawable. May fail
				// (bone doesn't exist on the live model's current condition-state variant, or the
				// object has no Drawable at all for some reason) -- in that case, fall back below to
				// an APPROXIMATE world transform derived from the reference model's own bind pose.
				Matrix3D boneWorldXform;
				Bool haveBoneXform = (drawable != nullptr) &&
					drawable->getCurrentWorldspaceClientBonePositions( boneIt->str(), boneWorldXform );
				// GeneralsMod @fix Dimitar 21/09/2026: root-caused -- a bone can legitimately resolve on
				// the standalone BreakApartModel reference skeleton (used above to enumerate the subtree)
				// while having no same-named bone on the LIVE dying unit's own model at all (e.g. a bone
				// that exists purely to give one subtree several independent debris chunks). That's the
				// common case, not an error: the live lookup above legitimately fails for most bones on
				// a finely-subdivided BreakApartModel.

				if (!haveBoneXform)
				{
					// GeneralsMod @fix Dimitar 21/09/2026: root-caused the "pieces float on landing"
					// bug down to THIS gap. Previously, when no live transform was found, boneWorldXform
					// was simply never set at all, and everything below degraded to nullptr/false --
					// which left this debris Object's own SIM/physics-tracked transform sitting at
					// wherever ObjectCreationList::create() spawned it above: the DYING OBJECT's own
					// root position (obj->getPosition()/getOrientation()), not the bone's position.
					// PhysicsBehavior then ground-clamps THAT (wrong) point -- the hull's own root
					// height, not the height the bone/mesh actually sits at (e.g. mounted up on a
					// turret) -- while the Draw-side ground-offset correction in
					// W3DBreakApartPieceDraw.cpp only ever compensated for the small mesh-to-its-own-
					// pivot offset, not this much larger root-to-bone-mount-height gap. Net effect: the
					// visible mesh renders however many units above the ground that the bone sits above
					// the hull root -- exactly the reported "floats when landing" symptom, and exactly
					// why TURRET/BARREL01 (the two bones that DID have a live transform) always worked
					// while every fallback bone (TURRET02, BARREL02/03, TURRET01/03/04) never did.
					//
					// Fix: approximate the bone's world transform as
					// dyingObjectTransform * boneBindPoseOnReferenceModel -- the ANCHOR (the dying
					// Object's own current position/orientation) is exact; only the ROTATION comes from
					// the reference model's REST pose rather than the unit's actual live pose (there's
					// no live bone to read, so this is the best available stand-in -- see
					// GetBreakApartBoneBindTransform()'s own comment in BreakApartModelHelper.h). This
					// still anchors the debris Object's physics point to roughly the right spot on the
					// dying unit instead of leaving it down at the root.
					Matrix3D boneBindXform;
					if (GetBreakApartBoneBindTransform(d->m_breakApartModel, *boneIt, &boneBindXform))
					{
						Matrix3D::Multiply(*obj->getTransformMatrix(), boneBindXform, &boneWorldXform);
						haveBoneXform = true;
					}
				}

				// GeneralsMod @fix Dimitar 19/09/2026: re-anchor the debris Object's own SIM
				// position/orientation onto the bone's world transform (live if available, else the
				// bind-pose approximation just computed above), BEFORE telling the Draw module which
				// subobject to show. Without this, the Object stayed spawned at the DYING OBJECT's own
				// root transform (obj->getPosition()/getOrientation(), passed to
				// ObjectCreationList::create() above) -- fine for a piece that happens to BE the model
				// root, but wrong for anything mounted away from it (e.g. a barrel mounted up on a
				// turret): PhysicsBehavior moves/rotates the Object around THAT root point, not around
				// the bone's own position, while setBreakApartPiece()'s m_pieceOffset only visually
				// re-aligns the RENDER once, at spawn -- so as physics then evolves the (wrong) root-
				// anchored transform frame by frame, the visible piece swings around a pivot that isn't
				// where it geometrically is (looks like it falls from/rotates around the wrong height/
				// point entirely, or floats above the ground once landed). setTransformMatrix() here
				// (Thing::setTransformMatrix(), public, the same call the engine's own object-
				// placement code uses) updates the Object's real synced transform AND propagates to
				// its Drawable via Object::reactToTransformChange()'s existing
				// `m_drawable->setTransformMatrix(...)` call -- so by the time setBreakApartPiece()
				// below reads getDrawable()->getTransformMatrix(), it already reflects the bone's own
				// position, which makes its m_pieceOffset math collapse to exactly the fixed
				// root-to-bone geometric offset (no per-frame drift), and PhysicsBehavior now moves/
				// rotates the Object around the bone's own point instead of the model's root -- the
				// visible piece and the physics body become the same rigid point. Left at the old
				// root-anchored spawn position/orientation only in the (now rare) case where neither a
				// live transform NOR a bind-pose fallback could be resolved at all (model failed to
				// load, or the bone name itself is bad) -- setBreakApartPiece() still degrades
				// gracefully via its own targetBoneIndex-only ground-offset correction in that case.
				//
				// GeneralsMod @fix Dimitar 23/09/2026: root-caused "rotation looks weird, like the
				// pivot isn't on the part itself" -- anchoring the SIM transform at the bone's own raw
				// pivot (as above) means PhysicsBehavior tumbles/rotates the piece around THAT point,
				// but a bone's pivot is very often NOT anywhere near its subobject's own visual center
				// (a barrel's bone sits at its mount point, with the tube extending well past it in one
				// direction) -- once the piece is free-flying debris with no bone attachment left to
				// justify pivoting on the original mount point, rotating around it looks exactly like
				// what was reported: the mesh visibly swinging around an point off in space rather than
				// tumbling about roughly its own middle. Fix: shift the SIM anchor from the bone's raw
				// pivot to (approximately) the subobject's own local bounding-box center
				// (GetBreakApartSubObjectLocalCenter(), BreakApartModelHelper.h) BEFORE calling
				// setTransformMatrix() -- Translate() (not Adjust_*_Translation -- see the 21/09/2026
				// lesson elsewhere in this file) rotates that LOCAL offset by boneWorldXform's own
				// current orientation before adding it, so the recentered point is correct regardless
				// of this bone's own orientation. Deliberately does NOT change what's passed to
				// setBreakApartPiece() below (still the TRUE, non-recentered boneWorldXform) -- that
				// function reads getDrawable()->getTransformMatrix() (now the RECENTERED transform,
				// via reactToTransformChange()) purely as ITS OWN pre-overwrite baseline before
				// computing pieceRootXform from the true bone transform passed in, so its EXISTING
				// m_pieceOffset math (Inverse(drawable transform at spawn) * pieceRootXform) ends up
				// automatically capturing the fixed local offset from the new centroid back to the
				// true mesh position -- no changes needed inside setBreakApartPiece() itself, and that
				// offset already rotates correctly with the piece as it tumbles, for the exact same
				// reason m_pieceOffset was introduced in the first place (see its own 19/09/2026
				// comment in W3DBreakApartPieceDraw.cpp).
				if (haveBoneXform)
				{
					Matrix3D pivotWorldXform = boneWorldXform;
					Vector3 localCenter(0.0f, 0.0f, 0.0f);
					if (GetBreakApartSubObjectLocalCenter(d->m_breakApartModel, *boneIt, &localCenter))
						pivotWorldXform.Translate(localCenter);
					debris->setTransformMatrix(&pivotWorldXform);
				}

				// Tell whichever Draw module on this fresh Object is ours which single subobject of
				// BreakApartModel to actually show -- a no-op on every other DrawModule (see
				// DrawModule::setBreakApartPiece()'s own comment in Common/DrawModule.h).
				for (DrawModule** dm = debris->getDrawable()->getDrawModules(); *dm; ++dm)
				{
					(*dm)->setBreakApartPiece(d->m_breakApartModel, *boneIt, haveBoneXform ? &boneWorldXform : nullptr);
				}

				// GeneralsMod @feature Dimitar 21/09/2026: bias the outward force toward this piece's
				// own actual offset from the dying object's center (flat, XY only -- see
				// calcRandomOutwardForce()'s own comment) rather than a fully random direction, so a
				// piece mounted on the tank's left reliably flies off left-ish. Only possible when we
				// have SOME world position for the bone (haveBoneXform, live or bind-pose-approximate)
				// -- otherwise there's nothing to derive a direction from, and calcRandomOutwardForce()
				// itself already degrades to the old fully random behavior for a degenerate/absent
				// direction.
				Coord3D outwardDirXY = { 0.0f, 0.0f, 0.0f };
				if (haveBoneXform)
				{
					outwardDirXY.x = boneWorldXform.Get_X_Translation() - obj->getPosition()->x;
					outwardDirXY.y = boneWorldXform.Get_Y_Translation() - obj->getPosition()->y;
				}
				applyBreakApartDebrisForce(debris, force, haveBoneXform ? &outwardDirXY : nullptr, d->m_forceSpreadAngle);
				FXList::doFXObj(d->m_breakApartPieceFX, debris);
			}
		}
	}
	else
	{
		// GeneralsMod @feature Dimitar 19/09/2026: still need allBrokenBones even with no
		// BreakApartPieceOCL configured (a modder using ONLY RemainPieceOCL, e.g.) -- the
		// per-bone SPAWN loop above is skipped, but the bone-name RESOLUTION it would have fed into
		// allBrokenBones still has to happen so the remainder piece knows which bones to hide.
		for (BADB_BoneNameVec::const_iterator rootIt = rootBones.begin(); rootIt != rootBones.end(); ++rootIt)
		{
			std::vector<AsciiString> subtreeBones;
			if (!GetBreakApartSubtreeBones(d->m_breakApartModel, *rootIt, &subtreeBones))
				continue;

			for (std::vector<AsciiString>::const_iterator it = subtreeBones.begin(); it != subtreeBones.end(); ++it)
			{
				if (!boneNameListContains(protectedBones, *it))
					allBrokenBones.push_back(*it);
			}
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
			applyBreakApartDebrisForce(remainder, force, nullptr, d->m_forceSpreadAngle);
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
	Int tier = 0;
	Real force = d->m_force;
	Real maxHealth = obj->getBodyModule() ? obj->getBodyModule()->getMaxHealth() : 0.0f;
	if (damageInfo != nullptr && maxHealth > 0.0f)
	{
		Real dealt = damageInfo->out.m_actualDamageDealt;
		Real percentOfMaxHealth = dealt / maxHealth;
		Real wastedDamage = dealt - damageInfo->out.m_healthBeforeDamage;
		if (percentOfMaxHealth >= d->m_extremeOverkillPercentageFromMaxHealth &&
			wastedDamage >= d->m_extremeOverkillDamageGreaterOrEqThan)
		{
			tier = 2;
			force = d->m_extremeOverkillForce;
		}
		else if (percentOfMaxHealth >= d->m_overkillPercentageFromMaxHealth &&
			wastedDamage >= d->m_overkillDamageGreaterOrEqThan)
		{
			tier = 1;
			force = d->m_overkillForce;
		}
	}

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
