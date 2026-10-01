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

#pragma once

#include "WWLib/always.h"
#include "WW3D2/rendobj.h"
#include "WW3D2/w3d_file.h"
#include "WW3D2/dx8vertexbuffer.h"
#include "WW3D2/dx8indexbuffer.h"
#include "WW3D2/shader.h"
#include "WW3D2/vertmaterial.h"

#define MAX_TRACK_EDGE_COUNT	100	//maximum number of edges or divisions in track mark
#define MAX_TRACK_OPAQUE_EDGE	25	//linear fade of edges will begin at this edge
#define FADE_TIME_FRAMES 300000	// 300 seconds at 30 fps - time to fade out an edge and remove it from the system.

// GeneralsMod @feature Dimitar 29/09/2026: every track edge is 4 vertices - two independent strips (left tread, right tread) - instead of
// the original single 2-vertex quad spanning the whole vehicle width.  Vertex order across an edge, in the
// direction of travel: [0] left outer, [1] left inner, [2] right inner, [3] right outer.
#define TRACK_VERTS_PER_EDGE	4

/// How the track texture's U range is split between the left and right tread strips (split mode only).
enum TerrainTrackTextureLayout
{
	TRACK_TEXTURE_WIDE = 0,		///< Classic "whole vehicle" texture: each strip samples only its own edge band of the image (stock textures keep working).
	TRACK_TEXTURE_SHARED,			///< Both strips sample the full image, same orientation (one tread image, used twice).
	TRACK_TEXTURE_MIRRORED,		///< Both strips sample the full image, right strip mirrored so U=0 is the outer edge on both sides.
	TRACK_TEXTURE_ATLAS,			///< Left strip samples U 0..0.5, right strip samples U 0.5..1 (different left/right treads in one texture).

	TRACK_TEXTURE_LAYOUT_COUNT
};

/// Per-unit track mark settings, filled from W3DModelDrawModuleData's TrackMarks* INI fields.
struct TerrainTrackSettings
{
	Real	treadWidth;				///< width of ONE tread strip in world units. 0 = legacy single-quad look (vanilla behaviour).
	Real	treadSpacing;			///< centre-to-centre distance between treads. 0 = measure from the tread bones.
	Real	segmentLength;		///< world units travelled between edges. 0 = default (MAP_XY_FACTOR).
	Real	tileLength;				///< world units per texture repeat along the track. 0 = legacy 0/1 ping-pong V.
	Int		textureLayout;		///< TerrainTrackTextureLayout, split mode only.
	Bool	followTerrain;		///< split mode only: sample terrain height at every vertex instead of only the track centre.
	const Char *leftBone;		///< nullptr/empty = "TREADFX01"
	const Char *rightBone;	///< nullptr/empty = "TREADFX02"

	TerrainTrackSettings() :
		treadWidth(0.0f), treadSpacing(0.0f), segmentLength(0.0f), tileLength(0.0f),
		textureLayout(TRACK_TEXTURE_WIDE), followTerrain(true), leftBone(nullptr), rightBone(nullptr) {}
};

class TerrainTracksRenderObjClassSystem;
class Drawable;

/// Custom render object that draws tracks on the terrain.
/**
This render object handles drawing tracks left by objects moving on the terrain.
*/
class TerrainTracksRenderObjClass : public RenderObjClass
{
	W3DMPO_CODE(TerrainTracksRenderObjClass)

	friend class TerrainTracksRenderObjClassSystem;

public:

	TerrainTracksRenderObjClass();
	virtual ~TerrainTracksRenderObjClass() override;

	/////////////////////////////////////////////////////////////////////////////
	// Render Object Interface (W3D methods)
	/////////////////////////////////////////////////////////////////////////////
	virtual RenderObjClass *	Clone() const override;
	virtual int						Class_ID() const override;
	virtual void					Render(RenderInfoClass & rinfo) override;
	virtual void					Get_Obj_Space_Bounding_Sphere(SphereClass & sphere) const override;
    virtual void					Get_Obj_Space_Bounding_Box(AABoxClass & aabox) const override;

	Int freeTerrainTracksResources();	///<free W3D assets used for this track
	void init( const TerrainTrackSettings &settings, Real boneSpacing, Bool haveBones, const Char *texturename);	///<allocate W3D resources and set size
	void addEdgeToTrack(Real x, Real y);	///< add a new segment to the track
	void addCapEdgeToTrack(Real x, Real y);	///< cap the existing segment so we can resume at an unconnected position.
	void setAirborne() {m_airborne = true; }	///< Starts a new section of track, generally after going airborne.
	void setOwnerDrawable(const Drawable *owner) {m_ownerDrawable = owner;}

protected:
	TextureClass *m_stageZeroTexture;	///<primary texture
	SphereClass	m_boundingSphere;		///<bounding sphere of TerrainTracks
	AABoxClass	m_boundingBox;			///<bounding box of TerrainTracks
	Int			m_activeEdgeCount;			///<number of active edges in segment list
	Int			m_totalEdgesAdded;		///<number of edges ever added to this track
	const Drawable	*m_ownerDrawable;	///<logical object that's laying down tread marks.

	struct edgeInfo{
		Vector3	endPointPos[TRACK_VERTS_PER_EDGE];			///<the endpoints on the edge (see TRACK_VERTS_PER_EDGE for order)
		Vector2	endPointUV[TRACK_VERTS_PER_EDGE];			///< uv coordinates at each end point
		Int		timeAdded;				///< time when edge was created.
		Real	alpha;					///< current alpha value for rendering
	};

	edgeInfo	m_edges[MAX_TRACK_EDGE_COUNT];	///<edges at each segment break
	Vector3		m_lastAnchor;						///<location of last edge center
	Int			m_bottomIndex;						///<points at oldest edge on track
	Int			m_topIndex;						///<points to newest edge on track
	Bool		m_haveAnchor;					///<set to false until first edge is added
	Bool		m_bound;						///<object is bound to owner and accepts new edges
	Real		m_width;						///<total track width (outer edge to outer edge)
	Real		m_length;						///<length of each track segment
	Real		m_laneOffset[TRACK_VERTS_PER_EDGE];	///<lateral offset of each edge vertex from the track centre (+ = right of travel direction)
	Real		m_laneU[TRACK_VERTS_PER_EDGE];			///<U coordinate of each edge vertex while driving forward
	Real		m_tileLength;				///<world units per texture repeat along the track, 0 = legacy ping-pong
	Real		m_vAccum;						///<running V coordinate for continuous tiling
	Bool		m_split;						///<TRUE = two separate tread strips, FALSE = legacy full-width look
	Bool		m_followTerrain;		///<sample terrain height per vertex
	Bool		m_airborne;					///< Did the vehicle bounce up into the air?
	Bool		m_haveCap;					///< is the segment capped so we can stop and resume at new location.

	void fillEdge( edgeInfo &edge, const Vector3 &vPos, const Vector3 &vDir, const Vector3 &vX, Bool onGround, Real v );	///<compute all vertices/UVs of one edge
	TerrainTracksRenderObjClass	*m_nextSystem;			///<next track system
	TerrainTracksRenderObjClass *m_prevSystem;			///<previous track system
};

/// System for drawing, updating, and re-using tread mark render objects.
/**
This system keeps track of all the active track mark objects and reuses them
when they expire.  It also renders all the track marks that were submitted in
this frame.
*/

class TerrainTracksRenderObjClassSystem
{
	friend class TerrainTracksRenderObjClass;

public:

	TerrainTracksRenderObjClassSystem();
	~TerrainTracksRenderObjClassSystem();

	void ReleaseResources();	///< Release all dx8 resources so the device can be reset.
	void ReAcquireResources();  ///< Reacquire all resources after device reset.

	void setDetail();

	void flush ();	///<draw all tracks that were requested for rendering.
	void update();	///<update the state of all edges (fade alpha, remove old, etc.)

	void init( SceneClass *TerrainTracksScene);	///< pre-allocate track objects
	void shutdown();		///< release all pre-allocated track objects, called by destructor
	void Reset();	///<empties the system, ready for a new scene.

	TerrainTracksRenderObjClass *bindTrack(RenderObjClass *renderObject, const TerrainTrackSettings &settings, const Char *texturename);	///<track object to be controlled by owner
	void unbindTrack( TerrainTracksRenderObjClass *mod );	///<releases control of track object

protected:
	DX8VertexBufferClass		*m_vertexBuffer;	///<vertex buffer used to draw all tracks
	DX8IndexBufferClass			*m_indexBuffer;	///<indices defining triangles in maximum length track
	VertexMaterialClass	  	  *m_vertexMaterialClass;	///< vertex lighting material
	ShaderClass m_shaderClass; ///<shader or rendering state for heightmap

	TerrainTracksRenderObjClass *m_usedModules;	///<active objects being rendered in the scene
	TerrainTracksRenderObjClass *m_freeModules;	//<unused modules that are free to use again
	SceneClass	*m_TerrainTracksScene;		///<scene that will contain all the TerrainTracks

	Int	m_edgesToFlush;			///< number of edges to flush on next render.

	void releaseTrack( TerrainTracksRenderObjClass *mod );	///<returns track object to free store.
	void clearTracks();	///<reset the amount of visible track marks of each object.

	Int m_maxTankTrackEdges;	///<maximum length of tank track
	Int m_maxTankTrackOpaqueEdges;	///<maximum length of tank track before it starts fading.
	Int m_maxTankTrackFadeDelay;	///<maximum amount of time a tank track segment remains visible.

};

extern TerrainTracksRenderObjClassSystem *TheTerrainTracksRenderObjClassSystem; ///< singleton for track drawing system.
