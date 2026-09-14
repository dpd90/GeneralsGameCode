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

// FILE: DecalUpdateV2.h /////////////////////////////////////////////////////////////////////////
//
//	GeneralsMod @feature Dimitar 12/09/2026
//	Self-contained ground decal effect: paints a RadiusDecal (the same terrain-conforming projected
//	texture used by the ShadowTexture-as-decal trick, but without its owner-only visibility limit
//	and without being tied to a unit's own death) at this object's position, fades and resizes it
//	over a fixed lifetime, then destroys its own object -- a single authoritative timer, no separate
//	LifetimeUpdate racing against the decal's own fade. Modeled directly on FireOCLBehaviorV2's own
//	self-destruct-on-completion pattern (see FireOCLBehaviorV2::update()'s thisIsFinalScan branch).
//
//	Intended use: attach to a short-lived, usually invisible/inert hazard or superweapon-effect
//	object -- typically alongside a FireWeaponUpdate on the same object for the actual gameplay
//	damage -- so the ground paint and the gameplay effect share exactly one lifetime.
//
//	Visibility to enemies is controlled entirely by the nested DecalTemplate block's own
//	OnlyVisibleToOwningPlayer field (defined on RadiusDecalTemplate, see GameClient/RadiusDecal.h):
//	set it to No to make the decal visible to everyone, not just the owner/allies -- this is the
//	restriction that made the KindOf=FS_FAKE ShadowTexture-as-decal trick unsuitable in the first
//	place.
//
//	Concurrency is capped globally (not per-category) by GameData.ini's MaxDecalCount
//	(TheGlobalData->m_maxDecalCount): once that many DecalUpdateV2 instances currently hold a live
//	decal, additional ones simply paint no decal at all -- the object itself, and any other module
//	on it (e.g. FireWeaponUpdate), is completely unaffected -- rather than forcing an existing decal
//	to disappear early. This mirrors how TerrainTracksRenderObjClassSystem::bindTrack() already
//	handles MaxTerrainTracks in this engine (refuse when the pool is full, not evict-the-oldest --
//	see Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DTerrainTracks.cpp).
//
//	Update() sleeps through a fully settled hold window (e.g. a short ResizeInTime, then a long
//	stretch of nothing changing, then a short ResizeOutTime) instead of ticking every logic frame
//	for no visible reason -- see computeNextSleepTime() in the .cpp. Disabled automatically whenever
//	an OpacityMin/OpacityMax throb range is configured, since the throb changes every frame anyway.
//
//	Camera-frustum and fog-of-war culling for the underlying decal itself (added 14/09/2026, fixing
//	the limitation this comment used to describe -- every live RadiusDecal used to be re-projected
//	onto the terrain every frame regardless of on-screen visibility or shroud) now lives one level
//	down, in W3DProjectedShadowManager::renderShadows()'s m_decalList loop
//	(W3DProjectedShadow.cpp) -- it's a manager-level fix, not specific to this module, since it
//	applies to every decal created via the no-owning-robj addDecal(ShadowTypeInfo*) overload, not
//	just DecalUpdateV2's own. Object-bound decals (PersistentDecalUpdateV2's setPersistentDecal(),
//	via the other addDecal(robj, info) overload) already had an equivalent Is_Really_Visible() skip
//	from the original engine code, unchanged by this.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "Common/GameCommon.h"
#include "GameLogic/Module/UpdateModule.h"
#include "GameClient/RadiusDecal.h"

class ObjectCreationList;
class FXList;

//-------------------------------------------------------------------------------------------------
class DecalUpdateV2ModuleData : public UpdateModuleData
{
public:
	RadiusDecalTemplate	m_decalTemplate;
	Real								m_radiusStart;		///< decal radius at spawn
	Real								m_radiusEnd;			///< decal radius once Duration elapses (== m_radiusStart for no resize at all)
	UnsignedInt					m_duration;				///< total lifetime, in frames -- the object destroys itself once this elapses
	UnsignedInt					m_fadeInTime;			///< frames to ease opacity 0 -> 1 at the start (0 = appear instantly)
	UnsignedInt					m_fadeOutTime;		///< frames to ease opacity 1 -> 0 at the end (0 = disappear instantly)
	UnsignedInt					m_resizeInTime;		///< frames to ease radius RadiusStart -> RadiusEnd at the start (0 = jump instantly)
	UnsignedInt					m_resizeOutTime;	///< frames to ease radius RadiusEnd -> RadiusStart at the end (0 = no shrink-back)
	Bool								m_countToMaxDecalCount;	///< if No, this instance is exempt from GameData.ini's MaxDecalCount pool entirely
	const ObjectCreationList*	m_onRemovalOCL;	///< GeneralsMod @feature Dimitar 12/09/2026: fired when this object is removed at end of Duration (nullptr = none)
	const FXList*				m_onRemovalFX;		///< GeneralsMod @feature Dimitar 12/09/2026: same, but an FXList.ini entry instead of an OCL (nullptr = none)

	DecalUpdateV2ModuleData()
	{
		m_radiusStart = 50.0f;
		m_radiusEnd = 50.0f;
		m_duration = 5 * LOGICFRAMES_PER_SECOND;
		m_fadeInTime = 0;
		m_fadeOutTime = LOGICFRAMES_PER_SECOND;
		m_resizeInTime = 0;
		m_resizeOutTime = 0;
		m_countToMaxDecalCount = TRUE;
		m_onRemovalOCL = nullptr;
		m_onRemovalFX = nullptr;
	}

	static void buildFieldParse(MultiIniFieldParse& p)
	{
		UpdateModuleData::buildFieldParse(p);
		static const FieldParse dataFieldParse[] =
		{
			{ "DecalTemplate",	RadiusDecalTemplate::parseRadiusDecalTemplate,	nullptr, offsetof( DecalUpdateV2ModuleData, m_decalTemplate ) },
			{ "RadiusStart",		INI::parseReal,									nullptr, offsetof( DecalUpdateV2ModuleData, m_radiusStart ) },
			{ "RadiusEnd",			INI::parseReal,									nullptr, offsetof( DecalUpdateV2ModuleData, m_radiusEnd ) },
			{ "Duration",				INI::parseDurationUnsignedInt,	nullptr, offsetof( DecalUpdateV2ModuleData, m_duration ) },
			{ "FadeInTime",			INI::parseDurationUnsignedInt,	nullptr, offsetof( DecalUpdateV2ModuleData, m_fadeInTime ) },
			{ "FadeOutTime",		INI::parseDurationUnsignedInt,	nullptr, offsetof( DecalUpdateV2ModuleData, m_fadeOutTime ) },
			{ "ResizeInTime",		INI::parseDurationUnsignedInt,	nullptr, offsetof( DecalUpdateV2ModuleData, m_resizeInTime ) },
			{ "ResizeOutTime",	INI::parseDurationUnsignedInt,	nullptr, offsetof( DecalUpdateV2ModuleData, m_resizeOutTime ) },
			{ "CountToMaxDecalCount",	INI::parseBool,						nullptr, offsetof( DecalUpdateV2ModuleData, m_countToMaxDecalCount ) },
			{ "OnRemovalOCL",		INI::parseObjectCreationList,		nullptr, offsetof( DecalUpdateV2ModuleData, m_onRemovalOCL ) },
			{ "OnRemovalFX",		INI::parseFXList,								nullptr, offsetof( DecalUpdateV2ModuleData, m_onRemovalFX ) },
			{ 0, 0, 0, 0 }
		};
		p.add(dataFieldParse);
	}
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
class DecalUpdateV2 : public UpdateModule
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( DecalUpdateV2, "DecalUpdateV2" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( DecalUpdateV2, DecalUpdateV2ModuleData )

public:

	DecalUpdateV2( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration -- defined out-of-line in
	// the .cpp (custom cleanup needed: decrementing s_liveDecalCount), so no EMPTY_DTOR here.

	virtual UpdateSleepTime update() override;

private:

	void tryCreateDecal();
	void computeAndApplyDecalState( UnsignedInt elapsed );

	// GeneralsMod @feature Dimitar 14/09/2026: lets update() sleep through a long, fully settled
	// hold window (e.g. a 500ms ResizeInTime, then 30 real seconds of nothing changing, then a
	// 500ms ResizeOutTime) instead of ticking every logic frame for no visible reason -- see the
	// .cpp for the full rationale, including why an OpacityMin/OpacityMax throb range disables
	// this entirely rather than trying to sleep through it.
	UpdateSleepTime computeNextSleepTime( UnsignedInt elapsed ) const;

	RadiusDecal		m_decal;
	UnsignedInt		m_startFrame;
	Bool					m_countedTowardCap;		///< whether this instance is currently included in s_liveDecalCount

	// GeneralsMod @feature Dimitar 12/09/2026: neither the constructor nor onObjectCreated() run
	// late enough to see this object's real spawn position (both run from inside Object::Object()
	// itself, before the caller that spawned us calls setPosition()) -- confirmed via debug logging
	// (position read (0,0,0) in both). update() is the first point position is guaranteed valid
	// (see FireOCLBehaviorV2::createEmitters(), which defers for the identical reason). This flag
	// makes the deferred attempt happen exactly once.
	Bool					m_decalAttempted;

	// GeneralsMod @feature Dimitar 12/09/2026: global (not per-category) cap on concurrently live
	// DecalUpdateV2 decals, enforced against TheGlobalData->m_maxDecalCount. Safe as a bare static:
	// GameLogic::reset() calls destroyAllObjectsImmediate() between matches, which runs every live
	// instance's destructor (and therefore its decrement) before a new game can create any more.
	static Int s_liveDecalCount;
};
