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

// FILE: DecalUpdateV2.cpp ///////////////////////////////////////////////////////////////////////////
//
//	GeneralsMod @feature Dimitar 12/09/2026
//	See DecalUpdateV2.h for the full design rationale.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/GlobalData.h"
#include "Common/INI.h"
#include "Common/Xfer.h"
#include "GameClient/FXList.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Module/DecalUpdateV2.h"
#include "GameLogic/Object.h"
#include "GameLogic/ObjectCreationList.h"

//-------------------------------------------------------------------------------------------------
Int DecalUpdateV2::s_liveDecalCount = 0;

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
DecalUpdateV2::DecalUpdateV2( Thing *thing, const ModuleData* moduleData ) : UpdateModule( thing, moduleData )
{
	m_startFrame = TheGameLogic->getFrame();
	m_countedTowardCap = FALSE;
	m_decalAttempted = FALSE;

	setWakeFrame( getObject(), UPDATE_SLEEP_NONE );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
DecalUpdateV2::~DecalUpdateV2()
{
	if ( m_countedTowardCap )
	{
		--s_liveDecalCount;
		m_countedTowardCap = FALSE;
	}
	m_decal.clear();
}

//-------------------------------------------------------------------------------------------------
/** Attempt to create the underlying RadiusDecal, respecting MaxDecalCount. Called from the
	* constructor (normal spawn) and again from loadPostProcess() (after a save load, since
	* RadiusDecal::xferRadiusDecal() never actually restores the visual -- see xfer() below). */
//-------------------------------------------------------------------------------------------------
void DecalUpdateV2::tryCreateDecal()
{
	const DecalUpdateV2ModuleData *data = getDecalUpdateV2ModuleData();

	// GeneralsMod @feature Dimitar 12/09/2026: CountToMaxDecalCount = No exempts this instance from
	// the global MaxDecalCount pool entirely -- e.g. per-general superweapon ground indicators, which
	// are already self-limited by their own multi-minute cooldowns (at most one per general at a
	// time), so they should always paint even if the general hazard/effect pool is full.
	if ( data->m_countToMaxDecalCount && s_liveDecalCount >= TheGlobalData->m_maxDecalCount )
		return;

	Object *self = getObject();

	// GeneralsMod @feature Dimitar 14/09/2026: allowOffscreenCulling=TRUE -- the only
	// RadiusDecalTemplate consumer that opts into off-screen/fog-of-war culling (see
	// Shadow::ShadowTypeInfo::m_allowOffscreenCulling for why every other consumer must not).
	data->m_decalTemplate.createRadiusDecal( *self->getPosition(), data->m_radiusStart, self->getControllingPlayer(), m_decal, TRUE );

	// GeneralsMod @feature Dimitar 12/09/2026: RadiusDecal::isEmpty() is guaranteed consistent
	// across all clients regardless of local visibility (see the comment on RadiusDecal.h) -- so
	// gating the cap counter on it, rather than on whether an underlying Shadow* actually got
	// created locally, keeps this cap check deterministic in multiplayer. Instances with
	// CountToMaxDecalCount = No never increment s_liveDecalCount, so m_countedTowardCap correctly
	// stays FALSE and the destructor's decrement below is skipped for them too.
	if ( data->m_countToMaxDecalCount && !m_decal.isEmpty() )
	{
		++s_liveDecalCount;
		m_countedTowardCap = TRUE;
	}
}

//-------------------------------------------------------------------------------------------------
/** Ease opacity (fade in / hold / fade out) and radius (RadiusStart -> RadiusEnd) for how far into
	* the effect's life we are, and push both to the live decal. No-op if we hold no decal (either
	* because MaxDecalCount was hit, or because the DecalTemplate block had no Texture set). */
//-------------------------------------------------------------------------------------------------
void DecalUpdateV2::computeAndApplyDecalState( UnsignedInt elapsed )
{
	if ( m_decal.isEmpty() )
		return;

	const DecalUpdateV2ModuleData *data = getDecalUpdateV2ModuleData();

	// GeneralsMod @feature Dimitar 12/09/2026: RGBA support -- Color's own A: component defines the
	// "full strength" opacity (peakOpacity) that FadeInTime eases up to and FadeOutTime eases down
	// from, UNLESS OpacityMin/OpacityMax were explicitly authored in the DecalTemplate block, in
	// which case the throb runs during the hold window instead. Neither Min nor Max set at all means
	// the throb system is skipped completely: no throb call, ThrobTime unused.
	Bool hasThrob = data->m_decalTemplate.hasOpacityRange();
	Real peakOpacity = hasThrob ? 1.0f : data->m_decalTemplate.getBaseOpacity();

	// GeneralsMod @feature Dimitar 12/09/2026: FadeOutTime is fully decoupled from Duration -- it is
	// always treated as its own fixed-length ramp (the denominator used below is always the
	// configured FadeOutTime itself, never however much of Duration is actually left). When
	// FadeOutTime fits inside Duration, fadeOutStart is pushed back so the ramp still finishes
	// exactly as Duration ends (unchanged from before). When FadeOutTime is longer than Duration,
	// there's no room to delay it at all, so it starts immediately at elapsed 0 and is simply still
	// mid-fade -- at whatever opacity it has reached -- the instant Duration cuts it off. Computed
	// up-front (not just inside the fade-out branch) because fadeInTarget/fadeOutStartOpacity below
	// need it too, to make the throb handoff continuous at both ends.
	UnsignedInt fadeOutStart = ( data->m_fadeOutTime > 0 && data->m_fadeOutTime <= data->m_duration )
		? ( data->m_duration - data->m_fadeOutTime )
		: 0;

	// GeneralsMod @feature Dimitar 12/09/2026: when a throb range is active, FadeInTime/FadeOutTime
	// must hand off to/from the throb's OWN value at that exact instant, or there's a visible snap at
	// each boundary -- fade-in would always reach a flat 1.0 the instant before the throb resumes at
	// whatever its sine phase actually says (the throb's phase-0 value is the min/max midpoint, since
	// sin(0) = 0, not either extreme), and fade-out would always start from a flat 1.0 regardless of
	// the throb's real live value the instant before. fadeInTarget is the throb's value at the very
	// start of the hold window (frame offset 0). fadeOutStartOpacity is the throb's value at the
	// exact holdElapsed the fade-out window begins, so fade-out eases DOWN FROM THAT, not from a flat
	// ceiling. Without a throb, both simply equal peakOpacity (Color's alpha, or 1.0) -- already
	// continuous on their own, since the hold value never changes.
	Real fadeInTarget = peakOpacity;
	Real fadeOutStartOpacity = peakOpacity;
	if ( hasThrob )
	{
		fadeInTarget = m_decal.computeThrobOpacity( 0 );

		UnsignedInt holdElapsedAtFadeOutStart = ( fadeOutStart > data->m_fadeInTime ) ? ( fadeOutStart - data->m_fadeInTime ) : 0;
		fadeOutStartOpacity = m_decal.computeThrobOpacity( holdElapsedAtFadeOutStart );
	}

	Real opacity;
	if ( data->m_fadeInTime > 0 && elapsed < data->m_fadeInTime )
	{
		// fading in, 0 -> fadeInTarget -- throb does not run yet, but the ramp now ends exactly where
		// the hold window (throb or flat) will pick up, instead of always aiming for a flat 1.0.
		opacity = fadeInTarget * (Real)elapsed / (Real)data->m_fadeInTime;
	}
	else if ( data->m_fadeOutTime > 0 && elapsed >= fadeOutStart )
	{
		// fading out, fadeOutStartOpacity -> 0 -- throb does not run here either, but the ramp now
		// starts exactly from whatever the hold window (throb or flat) was actually outputting the
		// instant before, instead of always starting from a flat 1.0.
		// E.g. Duration=900, FadeOutTime=1000: fadeOutStart=0, and by elapsed=900 (destruction) the
		// decal has only completed 900/1000 of the ramp, i.e. it disappears still at ~10% of
		// fadeOutStartOpacity, not fully faded.
		UnsignedInt intoFadeOut = elapsed - fadeOutStart;
		Real fraction = (Real)intoFadeOut / (Real)data->m_fadeOutTime;
		if ( fraction > 1.0f )
			fraction = 1.0f;
		opacity = fadeOutStartOpacity * ( 1.0f - fraction );
	}
	else
	{
		// GeneralsMod @feature Dimitar 12/09/2026: the "hold" window between FadeInTime and
		// FadeOutTime. If an Opacity throb range was explicitly authored, it runs here (phase
		// relative to the start of the hold window, so it always starts in sync exactly when
		// FadeInTime ends -- no discontinuity, since fadeInTarget above is this same call with
		// holdElapsed = 0). Otherwise the hold is simply flat at peakOpacity (Color's alpha, or 1.0
		// if Color wasn't set). "everything at 0" (FadeInTime = FadeOutTime = 0) makes the hold
		// window the entire Duration, i.e. throb (or the flat peak) applies immediately as requested.
		if ( hasThrob )
		{
			UnsignedInt holdElapsed = elapsed - data->m_fadeInTime;
			opacity = m_decal.computeThrobOpacity( holdElapsed );
		}
		else
		{
			opacity = peakOpacity;
		}
	}

	// GeneralsMod @feature Dimitar 12/09/2026: mirrors the opacity fade curve above exactly --
	// ease RadiusStart -> RadiusEnd over ResizeInTime, hold at RadiusEnd, then ease back down to
	// RadiusStart over ResizeOutTime at the end. Independent of FadeInTime/FadeOutTime -- a decal
	// can, for example, grow instantly (ResizeInTime = 0) while still fading in gradually.
	Real radius = data->m_radiusEnd;
	if ( data->m_resizeInTime > 0 && elapsed < data->m_resizeInTime )
	{
		Real t = (Real)elapsed / (Real)data->m_resizeInTime;
		radius = data->m_radiusStart + ( data->m_radiusEnd - data->m_radiusStart ) * t;
	}
	else if ( data->m_resizeOutTime > 0 )
	{
		// GeneralsMod @feature Dimitar 12/09/2026: decoupled from Duration exactly like FadeOutTime
		// above -- the denominator is always the full configured ResizeOutTime, so a ResizeOutTime
		// longer than Duration just gets cut off mid-resize when the object is destroyed, rather than
		// being compressed to fit.
		UnsignedInt resizeOutStart = ( data->m_resizeOutTime <= data->m_duration ) ? ( data->m_duration - data->m_resizeOutTime ) : 0;
		if ( elapsed >= resizeOutStart )
		{
			UnsignedInt intoResizeOut = elapsed - resizeOutStart;
			Real t = (Real)intoResizeOut / (Real)data->m_resizeOutTime;
			if ( t > 1.0f )
				t = 1.0f;
			radius = data->m_radiusEnd + ( data->m_radiusStart - data->m_radiusEnd ) * t;
		}
	}

	m_decal.setOpacity( opacity );
	m_decal.setRadius( radius );
}

//-------------------------------------------------------------------------------------------------
/** The update callback. */
//-------------------------------------------------------------------------------------------------
UpdateSleepTime DecalUpdateV2::update()
{
	Object *self = getObject();

	if ( self->isEffectivelyDead() )
		return UPDATE_SLEEP_FOREVER;

	// GeneralsMod @feature Dimitar 12/09/2026: first point at which this object's real spawn
	// position is guaranteed valid -- see the comment on m_decalAttempted in the header.
	if ( !m_decalAttempted )
	{
		m_decalAttempted = TRUE;
		tryCreateDecal();
	}

	const DecalUpdateV2ModuleData *data = getDecalUpdateV2ModuleData();
	UnsignedInt elapsed = TheGameLogic->getFrame() - m_startFrame;

	if ( elapsed >= data->m_duration )
	{
		// GeneralsMod @feature Dimitar 12/09/2026: destroyObject() is a delete, not a kill -- it never
		// runs the death pipeline (Object::kill()/onDie()), so a Die module like CreateObjectDie would
		// never fire here. OnRemovalOCL fires the same ObjectCreationList::create() call CreateObjectDie
		// itself uses, but directly, independent of death -- self is still fully valid at this point,
		// so OCL nuggets can read its position/team/etc exactly as they would from onDie().
		if ( data->m_onRemovalOCL )
			ObjectCreationList::create( data->m_onRemovalOCL, self, nullptr );

		if ( data->m_onRemovalFX )
			FXList::doFXObj( data->m_onRemovalFX, self );

		// GeneralsMod @feature Dimitar 12/09/2026: one authoritative lifetime, mirroring
		// FireOCLBehaviorV2::update()'s own thisIsFinalScan self-destruct pattern -- no separate
		// LifetimeUpdate that could end the object before (or long after) the decal finishes fading.
		TheGameLogic->destroyObject( self );
		return UPDATE_SLEEP_FOREVER;
	}

	computeAndApplyDecalState( elapsed );

	return computeNextSleepTime( elapsed );
}

//-------------------------------------------------------------------------------------------------
/** GeneralsMod @feature Dimitar 14/09/2026: skip ticking every logic frame through a long, fully
	* settled "hold" window -- e.g. a 500ms ResizeInTime, then 30 real seconds of nothing changing,
	* then a 500ms ResizeOutTime -- by sleeping straight to the next frame that would actually
	* change anything: the start of FadeOutTime/ResizeOutTime, or Duration itself (self-destruct,
	* already handled by the elapsed >= m_duration check at the top of update(), which this wakes
	* straight into). Deliberately returns UPDATE_SLEEP_NONE (tick every frame, no attempt to sleep)
	* whenever an OpacityMin/OpacityMax throb range is configured (hasOpacityRange()) -- the sine
	* throb changes every single frame for as long as the hold window lasts, so there is no settled
	* state to sleep through there; ticking every frame in that case is correct, not a missed
	* optimization. Also returns UPDATE_SLEEP_NONE while still easing in (opacity and/or radius) or
	* already easing out -- both are genuine per-frame animation and must tick every frame; only the
	* flat middle, when neither is happening, is worth sleeping through. */
//-------------------------------------------------------------------------------------------------
UpdateSleepTime DecalUpdateV2::computeNextSleepTime( UnsignedInt elapsed ) const
{
	const DecalUpdateV2ModuleData *data = getDecalUpdateV2ModuleData();

	if ( data->m_decalTemplate.hasOpacityRange() )
		return UPDATE_SLEEP_NONE;

	// still easing IN (opacity and/or radius) -- must animate every frame
	UnsignedInt activeInEnd = ( data->m_fadeInTime > data->m_resizeInTime ) ? data->m_fadeInTime : data->m_resizeInTime;
	if ( elapsed < activeInEnd )
		return UPDATE_SLEEP_NONE;

	// earliest frame at which either the fade-out or resize-out ramp begins, or Duration itself
	// (self-destruct) -- whichever comes first is the next frame anything could actually change.
	UnsignedInt nextChange = data->m_duration;

	if ( data->m_fadeOutTime > 0 )
	{
		UnsignedInt fadeOutStart = ( data->m_fadeOutTime <= data->m_duration ) ? ( data->m_duration - data->m_fadeOutTime ) : 0;
		if ( fadeOutStart < nextChange )
			nextChange = fadeOutStart;
	}

	if ( data->m_resizeOutTime > 0 )
	{
		UnsignedInt resizeOutStart = ( data->m_resizeOutTime <= data->m_duration ) ? ( data->m_duration - data->m_resizeOutTime ) : 0;
		if ( resizeOutStart < nextChange )
			nextChange = resizeOutStart;
	}

	// already at/past the start of a fade-out or resize-out ramp -- animate every frame until it
	// (or Duration) ends
	if ( elapsed >= nextChange )
		return UPDATE_SLEEP_NONE;

	return UPDATE_SLEEP( nextChange - elapsed );
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void DecalUpdateV2::crc( Xfer *xfer )
{

	// extend base class
	UpdateModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void DecalUpdateV2::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpdateModule::xfer( xfer );

	xfer->xferUnsignedInt( &m_startFrame );

	// GeneralsMod @feature Dimitar 12/09/2026: RadiusDecal::xferRadiusDecal() is an existing,
	// engine-wide no-op on the actual save stream (see the "@todo implement me" in
	// Core/GameEngine/Source/GameClient/RadiusDecal.cpp) -- on load it only clears the local
	// reference, it never restores the visual. loadPostProcess() below explicitly recreates the
	// decal instead, which also re-checks MaxDecalCount fresh against the just-loaded game state.
	m_decal.xferRadiusDecal( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void DecalUpdateV2::loadPostProcess()
{

	// extend base class
	UpdateModule::loadPostProcess();

	const DecalUpdateV2ModuleData *data = getDecalUpdateV2ModuleData();
	UnsignedInt now = TheGameLogic->getFrame();
	UnsignedInt elapsed = ( now > m_startFrame ) ? ( now - m_startFrame ) : 0;

	if ( elapsed < data->m_duration )
	{
		m_decalAttempted = TRUE;
		tryCreateDecal();
		computeAndApplyDecalState( elapsed );
	}

}
