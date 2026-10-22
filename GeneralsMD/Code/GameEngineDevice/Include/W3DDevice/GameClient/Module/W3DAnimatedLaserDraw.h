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

// FILE: W3DAnimatedLaserDraw.h ///////////////////////////////////////////////////////////////////
//
//	GeneralsMod @feature Dimitar 06/10/2026
//	Exactly W3DLaserDraw (beams, colors, widths, segments, arc, tiling, scroll, fade -- all of it,
//	unchanged, inherited), plus a texture flip-book: Texture accepts a space-separated LIST of
//	textures, and the beam swaps between them every TextureLifetime milliseconds, looping for as
//	long as the laser object lives (its own LaserUpdate/LifetimeUpdate decide when it dies). Meant
//	for lightning-strike style effects where the bolt shape should flicker between several frames.
//
//	Usage -- give the laser object this instead of W3DLaserDraw:
//
//	Draw = W3DAnimatedLaserDraw ModuleTag_01
//	  ... every existing W3DLaserDraw field still works here, unchanged (NumBeams, InnerBeamWidth,
//	  OuterBeamWidth, InnerColor, OuterColor, MaxIntensityLifetime, FadeLifetime, ScrollRate, Tile,
//	  Segments, ArcHeight, SegmentOverlapRatio, TilingScalar) ...
//	  Texture           = exswionlght1.tga exswionlght2.tga exswionlght3.tga exswionlght4.tga
//	  RandomTexture     = No    ; No  = play the list in order (1,2,3,4,1,2,...)
//	                            ; Yes = pick a random texture from the list on every change
//	                            ;       (never the same one twice in a row, when 2+ are listed)
//	  TextureLifetime = 30      ; msec each texture is held before switching to the next
//	End
//
//	Timing is measured from the moment the laser object is created, on the same game clock as
//	LifetimeUpdate, and is NOT rounded per change: texture N is shown from N*TextureLifetime ms on.
//	So TextureLifetime = 30 with a 90 ms lifetime shows textures 1, 2 and 3 (at 0, 30 and 60 ms)
//	and the object dies before texture 4 is due -- 90 / 30 = 3 textures. (The game only redraws
//	logic state every ~33 ms, so a rate shorter than that can skip a texture between two frames.)
//
//	Notes:
//	- With a single texture listed this behaves identically to W3DLaserDraw.
//	- All listed textures should share the same width/height ratio: the Tile = Yes tiling factor is
//	  computed from the FIRST texture's aspect ratio only (same as W3DLaserDraw's single texture).
//	- Purely client-side/visual. Random picks use the game-CLIENT random stream, so they never
//	  affect the synced simulation (no desync risk) and are not saved.
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include <vector>

#include "W3DDevice/GameClient/Module/W3DLaserDraw.h"

class TextureClass;

//-------------------------------------------------------------------------------------------------
class W3DAnimatedLaserDrawModuleData : public W3DLaserDrawModuleData
{
public:
	std::vector<AsciiString> m_textureNames;	///< full flip-book list; m_textureName (base) mirrors entry 0
	Bool m_randomTexture;
	Real m_textureLifetimeFrames;				///< TextureLifetime, converted from msec to (fractional) logic frames

	W3DAnimatedLaserDrawModuleData();
	virtual ~W3DAnimatedLaserDrawModuleData() override;
	static void buildFieldParse(MultiIniFieldParse& p);

	static void parseTextureList( INI* ini, void *instance, void *store, const void* userData );
};

//-------------------------------------------------------------------------------------------------
/** W3D laser draw with an animated (flip-book) texture */
//-------------------------------------------------------------------------------------------------
class W3DAnimatedLaserDraw : public W3DLaserDraw
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( W3DAnimatedLaserDraw, "W3DAnimatedLaserDraw" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( W3DAnimatedLaserDraw, W3DAnimatedLaserDrawModuleData )

public:

	W3DAnimatedLaserDraw( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	virtual void doDrawModule(const Matrix3D* transformMtx) override;

protected:

	void updateTextureAnimation();
	void applyTexture( Int index );

	std::vector<TextureClass*> m_textures;	///< one ref held per entry, released in destructor
	Int m_currentTextureIndex;
	UnsignedInt m_animStartFrame;						///< logic frame the laser was created on (re-anchored on load)
	Int m_lastStep;													///< last change-step applied, -1 = none yet
};
