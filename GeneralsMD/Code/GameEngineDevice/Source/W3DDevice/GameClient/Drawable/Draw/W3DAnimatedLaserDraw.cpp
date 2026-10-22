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

// FILE: W3DAnimatedLaserDraw.cpp /////////////////////////////////////////////////////////////////
// Desc:   See W3DAnimatedLaserDraw.h.
///////////////////////////////////////////////////////////////////////////////////////////////////

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
// GeneralsMod @feature Dimitar 06/10/2026: unlike GameEngine/.cpp files, GameEngineDevice/.cpp files
// do NOT start with "PreRTS.h" -- see W3DPersistentAnimModelDraw.cpp for the same note.
#include "Common/INI.h"
#include "Common/Xfer.h"
#include "GameClient/ClientRandomValue.h"
#include "GameLogic/GameLogic.h"
#include <math.h>

#include "W3DDevice/GameClient/Module/W3DAnimatedLaserDraw.h"
#include "WW3D2/assetmgr.h"
#include "WW3D2/segline.h"
#include "WW3D2/texture.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
W3DAnimatedLaserDrawModuleData::W3DAnimatedLaserDrawModuleData()
{
	m_randomTexture = FALSE;
	m_textureLifetimeFrames = 1.0f;	// one logic frame (~33ms) if TextureLifetime is omitted
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
W3DAnimatedLaserDrawModuleData::~W3DAnimatedLaserDrawModuleData()
{
}

//-------------------------------------------------------------------------------------------------
/** Texture = a.tga b.tga c.tga ...
	* Fills the flip-book list AND mirrors the first entry into W3DLaserDraw's own single
	* m_textureName, so the inherited W3DLaserDraw constructor sets up the beams exactly as it
	* always has (texture-mapping mode, aspect ratio) using frame 0. */
//-------------------------------------------------------------------------------------------------
void W3DAnimatedLaserDrawModuleData::parseTextureList( INI* ini, void *instance, void *store, const void* /*userData*/ )
{
	W3DAnimatedLaserDrawModuleData *self = (W3DAnimatedLaserDrawModuleData*)instance;
	std::vector<AsciiString> *names = (std::vector<AsciiString>*)store;

	names->clear();
	for( const char *token = ini->getNextTokenOrNull(); token; token = ini->getNextTokenOrNull() )
		names->push_back( token );

	self->m_textureName = names->empty() ? AsciiString::TheEmptyString : (*names)[ 0 ];
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void W3DAnimatedLaserDrawModuleData::buildFieldParse(MultiIniFieldParse& p)
{
	// Our table is added FIRST on purpose: INI::initFromINIMulti uses the first table that knows a
	// field name, so our list-aware "Texture" wins over W3DLaserDrawModuleData's single-string one.
	static const FieldParse dataFieldParse[] =
	{
		{ "Texture",						W3DAnimatedLaserDrawModuleData::parseTextureList,	nullptr, offsetof( W3DAnimatedLaserDrawModuleData, m_textureNames ) },
		{ "RandomTexture",			INI::parseBool,																		nullptr, offsetof( W3DAnimatedLaserDrawModuleData, m_randomTexture ) },
		{ "TextureLifetime",	INI::parseDurationReal,														nullptr, offsetof( W3DAnimatedLaserDrawModuleData, m_textureLifetimeFrames ) },
		{ nullptr, nullptr, nullptr, 0 }
	};
	p.add(dataFieldParse);

	W3DLaserDrawModuleData::buildFieldParse(p);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
W3DAnimatedLaserDraw::W3DAnimatedLaserDraw( Thing *thing, const ModuleData* moduleData ) :
	W3DLaserDraw( thing, moduleData ),
	m_currentTextureIndex(0),
	m_animStartFrame(TheGameLogic ? TheGameLogic->getFrame() : 0),
	m_lastStep(-1)
{
	const W3DAnimatedLaserDrawModuleData *data = getW3DAnimatedLaserDrawModuleData();

	// Load every frame up front (Get_Texture hands us a reference we own), so swapping later is
	// just a pointer change on the existing lines -- no asset lookups while drawing.
	m_textures.reserve( data->m_textureNames.size() );
	for( size_t i = 0; i < data->m_textureNames.size(); ++i )
	{
		TextureClass *tex = WW3DAssetManager::Get_Instance()->Get_Texture( data->m_textureNames[ i ].str() );
		if( tex )
		{
			if( !tex->Is_Initialized() )
				tex->Init();
			m_textures.push_back( tex );
		}
		else
		{
			DEBUG_LOG(( "W3DAnimatedLaserDraw: texture '%s' could not be loaded, skipping it.", data->m_textureNames[ i ].str() ));
		}
	}

	// frame 0 is already on the lines (W3DLaserDraw's constructor used m_textureName == entry 0)
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
W3DAnimatedLaserDraw::~W3DAnimatedLaserDraw()
{
	// The lines keep their own reference to whichever texture they currently show (REF_PTR_SET in
	// SegLineRendererClass::Set_Texture), and W3DLaserDraw's destructor -- which runs after this one
	// -- releases the lines and its own m_texture. We only drop the references we took ourselves.
	for( size_t i = 0; i < m_textures.size(); ++i )
		REF_PTR_RELEASE( m_textures[ i ] );
	m_textures.clear();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void W3DAnimatedLaserDraw::doDrawModule(const Matrix3D* transformMtx)
{
	// all beam geometry/positioning is exactly W3DLaserDraw's
	W3DLaserDraw::doDrawModule( transformMtx );

	updateTextureAnimation();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void W3DAnimatedLaserDraw::updateTextureAnimation()
{
	const Int count = (Int)m_textures.size();
	if( count <= 1 )
		return;

	const W3DAnimatedLaserDrawModuleData *data = getW3DAnimatedLaserDrawModuleData();
	const Real rate = data->m_textureLifetimeFrames > 0.0f ? data->m_textureLifetimeFrames : 1.0f;
	const UnsignedInt now = TheGameLogic->getFrame();

	// Which texture slot are we in? Computed from total elapsed time since creation, not by adding
	// up per-change delays, so there's no rounding drift: slot N starts at exactly N * rate frames,
	// i.e. N * TextureLifetime msec -- the same clock LifetimeUpdate's msec values run on.
	const UnsignedInt elapsed = ( now > m_animStartFrame ) ? ( now - m_animStartFrame ) : 0;
	const Int step = (Int)floorf( (Real)elapsed / rate + 0.0001f );	// tiny epsilon: 30ms*3 == 90ms lands on slot 3, not 2.999
	if( step == m_lastStep )
		return;

	Int newIndex;
	if( data->m_randomTexture )
	{
		if( m_lastStep < 0 )
		{
			newIndex = GameClientRandomValue( 0, count - 1 );
		}
		else
		{
			// pick any OTHER texture, so the bolt visibly changes on every step
			newIndex = GameClientRandomValue( 0, count - 2 );
			if( newIndex >= m_currentTextureIndex )
				++newIndex;
		}
	}
	else
	{
		newIndex = step % count;
	}

	m_lastStep = step;
	applyTexture( newIndex );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void W3DAnimatedLaserDraw::applyTexture( Int index )
{
	if( index < 0 || index >= (Int)m_textures.size() || !m_line3D )
		return;

	m_currentTextureIndex = index;

	const W3DAnimatedLaserDrawModuleData *data = getW3DAnimatedLaserDrawModuleData();
	const UnsignedInt numLines = data->m_numBeams * data->m_segments;
	for( UnsignedInt i = 0; i < numLines; ++i )
	{
		if( m_line3D[ i ] )
			m_line3D[ i ]->Set_Texture( m_textures[ index ] );
	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void W3DAnimatedLaserDraw::crc( Xfer *xfer )
{
	// extend base class
	W3DLaserDraw::crc( xfer );
}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void W3DAnimatedLaserDraw::xfer( Xfer *xfer )
{
	// version
	const XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	W3DLaserDraw::xfer( xfer );

	// animation state is purely cosmetic and not saved -- it restarts on load (see loadPostProcess)
}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void W3DAnimatedLaserDraw::loadPostProcess()
{
	// extend base class
	W3DLaserDraw::loadPostProcess();

	// restart the flip-book from now (animation state is cosmetic and not saved)
	m_animStartFrame = TheGameLogic->getFrame();
	m_lastStep = -1;
}
