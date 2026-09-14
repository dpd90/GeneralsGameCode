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

// FILE: RadiusDecal.h ///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Common/GameCommon.h"
#include "Common/GameType.h"
#include "GameClient/Color.h"

enum ShadowType CPP_11(: Int);
class Player;
class Shadow;
class RadiusDecalTemplate;

// ------------------------------------------------------------------------------------------------
class RadiusDecal
{
	friend class RadiusDecalTemplate;
private:
	const RadiusDecalTemplate*	m_template;
	Shadow*											m_decal;
	Bool												m_empty;
public:
	RadiusDecal();
	RadiusDecal(const RadiusDecal& that);
	RadiusDecal& operator=(const RadiusDecal& that);
	~RadiusDecal();

	void xferRadiusDecal( Xfer *xfer );

	// please note: it is very important, for game/net sync reasons, to ensure that
	// isEmpty() returns the same value, regardless of whether this decal will
	// be visible to the local player or not.
	Bool isEmpty() const { return m_empty; }
	void clear();
	void update();
	void setPosition(const Coord3D& pos);
	void setOpacity( const Real o );
	void setRadius( const Real r );	///< GeneralsMod @feature Dimitar 12/09/2026: grow/shrink after creation (Shadow already supports runtime resize, see Shadow::setSize()).

	// GeneralsMod @feature Dimitar 12/09/2026: exposes the same OpacityMin/OpacityMax/OpacityThrobTime
	// sine-throb math already used internally by update() (see RadiusDecal.cpp), but as a pure query
	// against an arbitrary frame offset instead of always reading TheGameLogic->getFrame() and always
	// applying the result -- so a caller (e.g. DecalUpdateV2) can gate WHEN the throb runs and combine
	// it with its own opacity logic (a fade), rather than being forced to call update() every frame.
	// Returns 1.0 (a harmless no-op) if this decal holds no template, or if OpacityMin/OpacityMax were
	// never set away from their default of 1.0/1.0.
	Real computeThrobOpacity( UnsignedInt frameOffset ) const;
};

// ------------------------------------------------------------------------------------------------
class RadiusDecalTemplate
{
	friend class RadiusDecal;
private:
	AsciiString		m_name;
	ShadowType		m_shadowType;
	Real					m_minOpacity;
	Real					m_maxOpacity;
	UnsignedInt		m_opacityThrobTime;
	Color					m_color;
	Bool					m_onlyVisibleToOwningPlayer;

	// GeneralsMod @feature Dimitar 12/09/2026: track whether OpacityMin/OpacityMax were actually
	// written in the INI (vs merely holding their structural default of 1.0/1.0) -- callers that
	// want Color's own alpha channel to define the decal's opacity (see getBaseOpacity()) need to
	// know whether to honor that, or defer to an explicitly-authored Opacity throb range instead.
	Bool					m_minOpacitySet;
	Bool					m_maxOpacitySet;

public:
	RadiusDecalTemplate();

	Bool valid() const { return m_name.isNotEmpty(); }
	void xferRadiusDecalTemplate( Xfer *xfer );

	// please note: it is very important, for game/net sync reasons, to ensure that
	// a valid radiusdecal is created, even if will not be visible to the local player,
	// since some logic makes decisions based on this.
	// GeneralsMod @feature Dimitar 14/09/2026: allowOffscreenCulling defaults FALSE, preserving
	// every existing caller's behavior unchanged (RadiusDecalUpdate, SpectreGunshipUpdate,
	// NeutronMissileUpdate, DeliverPayloadAIUpdate, DynamicShroudClearingRangeUpdate, InGameUI's
	// radius cursors, ObjectCreationList's delivery decal, etc. -- all targeting/area indicators
	// that must stay visible regardless of the local player's own vision/shroud). Only
	// DecalUpdateV2 passes TRUE. See Shadow::ShadowTypeInfo::m_allowOffscreenCulling for the full
	// rationale -- this parameter just forwards into that field.
	void createRadiusDecal(const Coord3D& pos, Real radius, const Player* owningPlayer, RadiusDecal& result, Bool allowOffscreenCulling = FALSE) const;

	static void parseRadiusDecalTemplate(INI* ini, void *instance, void * store, const void* /*userData*/);

	// GeneralsMod @feature Dimitar 12/09/2026: Color's own A: component (0-255, packed into bits
	// 24-31 of m_color by INI::parseColorInt) is what now defines the decal's full-strength opacity
	// when no explicit OpacityMin/OpacityMax throb range overrides it -- see DecalUpdateV2's
	// computeAndApplyDecalState(). Returns 1.0 if Color was never specified at all (m_color's
	// pre-existing "unset" sentinel -- createRadiusDecal() substitutes the owning player's color
	// in that case, so alpha would be meaningless anyway).
	Real getBaseOpacity() const;

	// GeneralsMod @feature Dimitar 12/09/2026: true if OpacityMin and/or OpacityMax was explicitly
	// authored in this DecalTemplate block -- when false, the throb system (and OpacityThrobTime)
	// is skipped entirely in favor of a flat opacity derived from Color's alpha (getBaseOpacity()).
	Bool hasOpacityRange() const { return m_minOpacitySet || m_maxOpacitySet; }

private:
	// GeneralsMod @feature Dimitar 12/09/2026: thin wrappers around INI::parsePercentToReal that
	// also flag m_minOpacitySet/m_maxOpacitySet -- see hasOpacityRange().
	static void parseOpacityMin(INI* ini, void *instance, void *store, const void* userData);
	static void parseOpacityMax(INI* ini, void *instance, void *store, const void* userData);
};
