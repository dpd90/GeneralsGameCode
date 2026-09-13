/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2026 TheSuperHackers
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

#include "W3DDevice/GameClient/W3DScorch.h"

#include "Common/GameMemory.h"
#include "Common/GameType.h"
#include "Common/GlobalData.h"
#include "Common/MapObject.h"
#include "W3DDevice/GameClient/TerrainTex.h"
#include "W3DDevice/GameClient/WorldHeightMap.h"
#include "WW3D2/dx8wrapper.h"

namespace
{
	// GeneralsMod @feature Dimitar 13/09/2026: replaces the old hardcoded "1.5" pitch constant that
	// used to live inline in writeScorchToBuffer(). That constant forced a fixed 50%-of-cell-width
	// gap between cells (wasteful -- far more than mipmap-bleed protection actually needs), and it
	// only produced a valid non-overflowing layout for the specific case of SCORCH_PER_ROW == 3.
	//
	// This version uses a small FIXED-PIXEL gap instead (standard texture-atlas practice: just
	// enough to guard against mip-level box-filter bleed between neighboring cells), applied
	// uniformly between cells AND around the outer border. The outer border matters because this
	// texture uses TEXTURE_ADDRESS_REPEAT (TextureFilterClass's default, never overridden here) --
	// without it, the left/top edge cells could bleed into the right/bottom edge cells at low mip
	// levels, exactly like any two adjacent interior cells would without a gap between them.
	//
	// SCORCH_TEXTURE_SIZE_PIXELS must match whatever pixel size the actual ScorchTexture (see
	// GlobalData::m_scorchTexture, default "EXScorch01.tga") is authored at, and SCORCH_GRID_DIM
	// must always equal W3DScorch::SCORCH_PER_ROW. Update both together if either changes.
	const Real SCORCH_TEXTURE_SIZE_PIXELS = 512.0f;
	const Real SCORCH_TEXTURE_GAP_PIXELS = 4.0f;
	const Int SCORCH_GRID_DIM = 4;

	const Real SCORCH_GAP_NORM = SCORCH_TEXTURE_GAP_PIXELS / SCORCH_TEXTURE_SIZE_PIXELS;
	// N cells + (N+1) gaps (one border gap on each outer edge, plus N-1 gaps between cells) == 1.0
	const Real SCORCH_CELL_NORM = (1.0f - (SCORCH_GRID_DIM + 1) * SCORCH_GAP_NORM) / SCORCH_GRID_DIM;
	const Real SCORCH_PITCH_NORM = SCORCH_CELL_NORM + SCORCH_GAP_NORM;
}

W3DScorch::W3DScorch(bool deduplicateScorches)
  : m_vertexScorch(nullptr)
  , m_indexScorch(nullptr)
  , m_scorchTexture(nullptr)
  , m_curNumScorchVertices(0)
  , m_curNumScorchIndices(0)
  , m_needBufferRecompute(true)
  , m_deduplicateScorches(deduplicateScorches)
{}

W3DScorch::~W3DScorch() { freeBuffers(); }

void W3DScorch::allocateBuffers()
{
	freeBuffers();
	m_vertexScorch = NEW_REF(DX8VertexBufferClass, (DX8_FVF_XYZDUV1, MAX_SCORCH_VERTEX, DX8VertexBufferClass::USAGE_DEFAULT));
	m_indexScorch = NEW_REF(DX8IndexBufferClass, (MAX_SCORCH_INDEX));
	m_scorchTexture = NEW ScorchTextureClass;
	invalidateBuffers();
}

void W3DScorch::freeBuffers()
{
	REF_PTR_RELEASE(m_vertexScorch);
	REF_PTR_RELEASE(m_indexScorch);
	REF_PTR_RELEASE(m_scorchTexture);
}

void W3DScorch::clearAllScorches()
{
	m_scorches.clear();
	invalidateBuffers();
}

void W3DScorch::invalidateBuffers()
{
	m_needBufferRecompute = true;
	m_curNumScorchVertices = 0;
	m_curNumScorchIndices = 0;
}

void W3DScorch::invalidateTexture()
{
	if (m_scorchTexture)
	{
		m_scorchTexture->Invalidate();
	}
}

void W3DScorch::addScorch(Vector3 location, Real radius, Scorches type)
{
	TScorch scorch;
	scorch.location = location;
	scorch.radius = radius;
	if (type >= 0 && (Int)type < SCORCH_MARKS_IN_TEXTURE)
		scorch.scorchType = type;
	else
		scorch.scorchType = SCORCH_1;

	if (m_deduplicateScorches && isDuplicate(scorch))
	{
		return;
	}

	if ((Int)m_scorches.size() >= MAX_SCORCH_MARKS)
	{
		m_scorches.pop_front();
	}
	m_scorches.push_back(scorch);

	invalidateBuffers();
}

Bool W3DScorch::isDuplicate(const TScorch& scorch) const
{
	const Real limit = scorch.radius / 4;
	for (std::deque<TScorch>::const_iterator it = m_scorches.begin(); it != m_scorches.end(); ++it)
	{
		if (it->scorchType == scorch.scorchType &&
		    fabsf(scorch.location.X - it->location.X) < limit &&
		    fabsf(scorch.location.Y - it->location.Y) < limit &&
		    fabsf(scorch.radius - it->radius) < limit)
		{
			return true;
		}
	}
	return false;
}

void W3DScorch::drawScorches(WorldHeightMap& map)
{
	updateScorches(map);
	if (m_curNumScorchIndices == 0)
	{
		return;
	}
	DX8Wrapper::Set_Index_Buffer(m_indexScorch, 0);
	DX8Wrapper::Set_Vertex_Buffer(m_vertexScorch);
	DX8Wrapper::Set_Shader(ShaderClass::_PresetAlphaShader);

	DX8Wrapper::Set_Texture(0, m_scorchTexture);
	DX8Wrapper::Draw_Triangles(0, m_curNumScorchIndices / 3, 0, m_curNumScorchVertices);
}

static Real getMapHeight(WorldHeightMap& map, Int x, Int y)
{
	x += map.getBorderSizeInline();
	y += map.getBorderSizeInline();
	return map.getDataPtr()[x + y * map.getXExtent()] * MAP_HEIGHT_SCALE;
}

void W3DScorch::updateScorches(WorldHeightMap& map)
{
	if (!m_needBufferRecompute || m_scorches.empty() || !m_indexScorch || !m_vertexScorch)
	{
		return;
	}

	m_needBufferRecompute = false;
	m_curNumScorchVertices = 0;
	m_curNumScorchIndices = 0;

	DX8IndexBufferClass::WriteLockClass lockIdxBuffer(m_indexScorch);
	UnsignedShort* ib = lockIdxBuffer.Get_Index_Array();

	DX8VertexBufferClass::WriteLockClass lockVtxBuffer(m_vertexScorch);
	VertexFormatXYZDUV1* vb = (VertexFormatXYZDUV1*)lockVtxBuffer.Get_Vertex_Array();

	Real shadeR = (TheGlobalData->m_terrainAmbient[0].red + TheGlobalData->m_terrainDiffuse[0].red) / 2.0f;
	Real shadeG = (TheGlobalData->m_terrainAmbient[0].green + TheGlobalData->m_terrainDiffuse[0].green) / 2.0f;
	Real shadeB = (TheGlobalData->m_terrainAmbient[0].blue + TheGlobalData->m_terrainDiffuse[0].blue) / 2.0f;
	UnsignedInt diffuse = DX8Wrapper::Convert_Color_Clamp(Vector4(shadeR, shadeG, shadeB, 1.0f));

	// TheSuperHackers @info Scorches are written in reverse order to ensure that the last added scorches fit in the buffers.
	for (std::deque<TScorch>::reverse_iterator it = m_scorches.rbegin(); it != m_scorches.rend(); ++it)
	{
		if (writeScorchToBuffer(*it, map, diffuse,
		                        vb + m_curNumScorchVertices, ib + m_curNumScorchIndices) == SCORCH_BUFFER_FULL)
		{
			return;
		}
	}
}

W3DScorch::WriteScorchResult W3DScorch::writeScorchToBuffer(const TScorch& scorch, WorldHeightMap& map,
                                                          UnsignedInt diffuse, VertexFormatXYZDUV1* curVb,
                                                          UnsignedShort* curIb)
{
	Real radius = scorch.radius;
	Vector3 loc = scorch.location;
	Int type = scorch.scorchType;
	Real amtToFloat = MAP_HEIGHT_SCALE / 10;

	// GeneralsMod @feature Dimitar 12/09/2026: rotate the UV sampling per-mark so repeated uses of
	// the same scorch texture do not all look identically oriented. The angle is derived from the
	// mark's own world position (a cheap positional hash) rather than drawn from an RNG, so it needs
	// no new stored state, is stable across save/load, and never affects sim determinism -- this file
	// is render-only. Only the UV lookup rotates; vertex world positions/heights are untouched.
	Real angleSeed = loc.X * 12.9898f + loc.Y * 78.233f;
	Real rotationAngle = fmodf(fabsf(sinf(angleSeed)) * 43758.5453f, 1.0f) * (2.0f * WWMATH_PI);
	Real cosT = cosf(rotationAngle);
	Real sinT = sinf(rotationAngle);

	// GeneralsMod @feature Dimitar 13/09/2026: fixed-pixel-gap cell placement (see the packing
	// constants at the top of this file) replaces the old "(type % SCORCH_PER_ROW) * 1.5f" pitch.
	Int col = type % SCORCH_PER_ROW;
	Int row = type / SCORCH_PER_ROW;
	Real cellStartU = SCORCH_GAP_NORM + col * SCORCH_PITCH_NORM;
	Real cellStartV = SCORCH_GAP_NORM + row * SCORCH_PITCH_NORM;

	Int minX = REAL_TO_INT_FLOOR((loc.X - radius) / MAP_XY_FACTOR);
	Int minY = REAL_TO_INT_FLOOR((loc.Y - radius) / MAP_XY_FACTOR);
	if (minX < -map.getBorderSizeInline())
		minX = -map.getBorderSizeInline();
	if (minY < -map.getBorderSizeInline())
		minY = -map.getBorderSizeInline();
	Int maxX = REAL_TO_INT_CEIL((loc.X + radius) / MAP_XY_FACTOR);
	Int maxY = REAL_TO_INT_CEIL((loc.Y + radius) / MAP_XY_FACTOR);
	maxX++;
	maxY++;
	if (maxX > map.getXExtent() - map.getBorderSizeInline())
	{
		maxX = map.getXExtent() - map.getBorderSizeInline();
	}
	if (maxY > map.getYExtent() - map.getBorderSizeInline())
	{
		maxY = map.getYExtent() - map.getBorderSizeInline();
	}

	const Int vertexCountX = maxX - minX;
	const Int vertexCountY = maxY - minY;
	if (vertexCountX <= 0 || vertexCountY <= 0)
	{
		return SCORCH_SKIPPED;
	}

	const Int requiredVertices = vertexCountX * vertexCountY;
	const Int requiredIndices = 6 * (vertexCountX - 1) * (vertexCountY - 1);
	if (m_curNumScorchVertices + requiredVertices > MAX_SCORCH_VERTEX ||
	    m_curNumScorchIndices + requiredIndices > MAX_SCORCH_INDEX)
	{
		return SCORCH_BUFFER_FULL;
	}

	Int startVertex = m_curNumScorchVertices;
	Int i, j;
	for (j = minY; j < maxY; j++)
	{
		for (i = minX; i < maxX; i++)
		{
			curVb->diffuse = diffuse;
			Real theZ = amtToFloat + getMapHeight(map, i, j);
			Real X = i * MAP_XY_FACTOR;
			Real Y = j * MAP_XY_FACTOR;
			Real dx = X - loc.X;
			Real dy = Y - loc.Y;
			Real rotatedDx = dx * cosT - dy * sinT;
			Real rotatedDy = dx * sinT + dy * cosT;
			// GeneralsMod @bugfix Dimitar 13/09/2026: the bounding box above deliberately overshoots the
			// true radius (ceil() to the next terrain grid line, plus an extra +1) -- pre-existing, and
			// harmless under the old ~50%-of-cell gap, but our much tighter fixed-pixel gap is nowhere
			// near big enough to absorb it (worst case ~2 terrain cells = ~20 world units, which for a
			// small-radius scorch is a large fraction of its own cell). Clamp so no vertex can ever
			// produce a UV outside the cell it was assigned, regardless of how far dx/dy overshoot.
			Real localU = (rotatedDx / (2 * radius) + 0.5f) * SCORCH_CELL_NORM;
			Real localV = (rotatedDy / (2 * radius) + 0.5f) * SCORCH_CELL_NORM;
			if (localU < 0.0f) localU = 0.0f;
			else if (localU > SCORCH_CELL_NORM) localU = SCORCH_CELL_NORM;
			if (localV < 0.0f) localV = 0.0f;
			else if (localV > SCORCH_CELL_NORM) localV = SCORCH_CELL_NORM;
			curVb->u1 = cellStartU + localU;
			curVb->v1 = cellStartV + localV;
			curVb->x = X;
			curVb->y = Y;
			curVb->z = theZ;
			curVb++;
			m_curNumScorchVertices++;
		}
	}
	Int yOffset = maxX - minX;
	for (j = 0; j < maxY - minY - 1; j++)
	{
		for (i = 0; i < maxX - minX - 1; i++)
		{
			Int xNdx = i + minX + map.getBorderSizeInline();
			Int yNdx = j + minY + map.getBorderSizeInline();
			Bool flipForBlend = map.getFlipState(xNdx, yNdx);
#if 0
			UnsignedByte alpha[4];
			float UA[4], VA[4];
			map.getAlphaUVData(xNdx, yNdx, UA, VA, alpha, &flipForBlend);
#endif
			if (flipForBlend)
			{
				*curIb++ = startVertex + j * yOffset + i + 1;
				*curIb++ = startVertex + j * yOffset + i + yOffset;
				*curIb++ = startVertex + j * yOffset + i;
				*curIb++ = startVertex + j * yOffset + i + 1;
				*curIb++ = startVertex + j * yOffset + i + 1 + yOffset;
				*curIb++ = startVertex + j * yOffset + i + yOffset;
			}
			else
			{
				*curIb++ = startVertex + j * yOffset + i;
				*curIb++ = startVertex + j * yOffset + i + 1 + yOffset;
				*curIb++ = startVertex + j * yOffset + i + yOffset;
				*curIb++ = startVertex + j * yOffset + i;
				*curIb++ = startVertex + j * yOffset + i + 1;
				*curIb++ = startVertex + j * yOffset + i + 1 + yOffset;
			}
			m_curNumScorchIndices += 6;
		}
	}

	return SCORCH_WRITTEN;
}
