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

// FILE: W3DBreakApartModelHelper.cpp ///////////////////////////////////////////////////////////
// GeneralsMod @feature Dimitar 19/09/2026, redesigned 19/09/2026
// Desc: Concrete W3D implementation of GetBreakApartSubtreeBones() (see BreakApartModelHelper.h,
//       GameEngine/Include/GameClient, for the full design rationale). Note this file lives under
//       GameEngineDevice, so per this mod's own convention it does NOT include "PreRTS.h".
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "GameClient/BreakApartModelHelper.h"

#include "Lib/BaseType.h"
#include "Common/AsciiString.h"

#include <WW3D2/assetmgr.h>
#include <WW3D2/rendobj.h>
#include <WW3D2/htree.h>
#include <WWMath/aabox.h>
#include <WWMath/matrix3d.h>
#include <WWMath/vector3.h>

#include <vector>

namespace
{
	//---------------------------------------------------------------------------------------------
	struct CachedBreakApartModel
	{
		AsciiString name;
		RenderObjClass* renderObj;	///< may be nullptr if the model failed to load -- still cached
																///< so a bad model name doesn't retry Create_Render_Obj every call
	};

	// GeneralsMod @feature Dimitar 19/09/2026: a small, process-lifetime cache of reference models.
	// A plain linear vector is deliberately used instead of a map -- a mod is only ever going to
	// name a handful of distinct BreakApartModel files, so a linear scan here costs nothing
	// measurable and avoids needing an AsciiString hash/comparator.
	std::vector<CachedBreakApartModel> TheBreakApartModelCache;

	//---------------------------------------------------------------------------------------------
	RenderObjClass* findOrLoadBreakApartModel( const AsciiString& modelName )
	{
		for( std::vector<CachedBreakApartModel>::iterator it = TheBreakApartModelCache.begin(); it != TheBreakApartModelCache.end(); ++it )
		{
			if( it->name == modelName )
				return it->renderObj;
		}

		// Not cached yet -- load it now. This RenderObjClass is deliberately never added to any
		// Scene and never Render()'d or given an animation (no Set_Animation call is ever made on
		// it) -- it exists purely as a standalone geometry/skeleton source, kept alive for the rest
		// of the process. Same standalone-RenderObjClass lifetime convention W3DGhostObject and
		// W3DBridgeBuffer already use for their own non-Drawable render objects.
		RenderObjClass* renderObj = WW3DAssetManager::Get_Instance()->Create_Render_Obj( modelName.str() );

		CachedBreakApartModel entry;
		entry.name = modelName;
		entry.renderObj = renderObj;
		TheBreakApartModelCache.push_back( entry );

		return renderObj;
	}

	//---------------------------------------------------------------------------------------------
	// Post-order DFS: recurse into every child of boneIndex FIRST, then (only if boneIndex itself
	// has at least one subobject attached) append boneIndex's own name -- this is what guarantees
	// every bone in the output precedes its own ancestors, matching "children fall apart before
	// their parent" from this module's own design discussion. childrenOf is indexed by bone index
	// and pre-built by the caller (see GetBreakApartSubtreeBones below) so this never has to
	// re-scan the whole skeleton at each recursion level.
	void visitBreakApartBone( RenderObjClass* renderObj, const HTreeClass* htree, const std::vector< std::vector<int> >& childrenOf, int boneIndex, std::vector<AsciiString>* outBoneNames )
	{
		const std::vector<int>& kids = childrenOf[boneIndex];
		for( std::vector<int>::const_iterator it = kids.begin(); it != kids.end(); ++it )
		{
			visitBreakApartBone( renderObj, htree, childrenOf, *it, outBoneNames );
		}

		if( renderObj->Get_Num_Sub_Objects_On_Bone( boneIndex ) > 0 )
		{
			outBoneNames->push_back( AsciiString( htree->Get_Bone_Name( boneIndex ) ) );
		}
	}
}

//-------------------------------------------------------------------------------------------------
Bool GetBreakApartSubtreeBones( const AsciiString& modelName, const AsciiString& rootBoneName, std::vector<AsciiString>* outBoneNames )
{
	RenderObjClass* renderObj = findOrLoadBreakApartModel( modelName );
	if( renderObj == nullptr )
		return false;

	const HTreeClass* htree = renderObj->Get_HTree();
	if( htree == nullptr )
		return false;

	// WW3D's Get_Bone_Index() (htree.cpp) is a linear scan that returns 0 BOTH when nothing
	// matches AND when the skeleton's own true root bone (pivot 0) is the match -- it's genuinely
	// ambiguous from the return value alone. A vehicle's root/master bone is very commonly named
	// after the hull itself (e.g. "CHASSIS"), so treating every 0 as "not found" silently drops
	// exactly that case: naming the model's real root bone as a BreakApartSubObject root would
	// always resolve to "not found" and skip the whole subtree with no error at all. Disambiguate
	// by checking whether pivot 0's OWN name actually matches what was asked for.
	int rootIndex = htree->Get_Bone_Index( rootBoneName.str() );
	if( rootIndex == 0 && rootBoneName.compareNoCase( htree->Get_Bone_Name( 0 ) ) != 0 )
		return false;

	// Build a bone-index -> list-of-child-bone-indices map by scanning the whole skeleton once.
	// Bone 0 (the true root) reports itself as its own parent (HTreeClass::Get_Parent_Index(0)
	// returns 0), so it's deliberately skipped here rather than being recorded as its own child.
	int numPivots = htree->Num_Pivots();
	std::vector< std::vector<int> > childrenOf( numPivots );
	for( int i = 1; i < numPivots; ++i )
	{
		int parent = htree->Get_Parent_Index( i );
		childrenOf[parent].push_back( i );
	}

	visitBreakApartBone( renderObj, htree, childrenOf, rootIndex, outBoneNames );
	return true;
}

//-------------------------------------------------------------------------------------------------
Bool GetBreakApartBoneBindTransform( const AsciiString& modelName, const AsciiString& boneName, Matrix3D* outBindTransform )
{
	RenderObjClass* renderObj = findOrLoadBreakApartModel( modelName );
	if( renderObj == nullptr )
		return false;

	const HTreeClass* htree = renderObj->Get_HTree();
	if( htree == nullptr )
		return false;

	// Same bone-index-0 ambiguity as GetBreakApartSubtreeBones above -- see its comment.
	int boneIndex = htree->Get_Bone_Index( boneName.str() );
	if( boneIndex == 0 && boneName.compareNoCase( htree->Get_Bone_Name( 0 ) ) != 0 )
		return false;

	// GeneralsMod @feature Dimitar 21/09/2026: this renderObj is the SAME process-lifetime-cached,
	// never-rendered, never-added-to-Scene reference instance findOrLoadBreakApartModel() hands out
	// to every caller (including GetBreakApartSubtreeBones above, which never touches its
	// transform) -- so forcing it to identity here before reading Get_Bone_Transform() is safe:
	// nothing else depends on this shared instance's transform surviving between calls, and this is
	// the same single-threaded game-logic/client call pattern the rest of this engine already
	// relies on elsewhere. This is unrelated to (and shares no RenderObjClass instance with) the
	// PER-PIECE clone W3DBreakApartPieceDraw::setBreakApartPiece() creates for actually displaying a
	// piece -- that is its own separate Create_Render_Obj() call, added to the Scene.
	Matrix3D identityXform( true );
	renderObj->Set_Transform( identityXform );
	*outBindTransform = renderObj->Get_Bone_Transform( boneIndex );
	return true;
}

//-------------------------------------------------------------------------------------------------
Bool GetBreakApartSubObjectLocalCenter( const AsciiString& modelName, const AsciiString& boneName, Vector3* outLocalCenter )
{
	RenderObjClass* renderObj = findOrLoadBreakApartModel( modelName );
	if( renderObj == nullptr )
		return false;

	const HTreeClass* htree = renderObj->Get_HTree();
	if( htree == nullptr )
		return false;

	// Same bone-index-0 ambiguity as GetBreakApartSubtreeBones above -- see its comment.
	int boneIndex = htree->Get_Bone_Index( boneName.str() );
	if( boneIndex == 0 && boneName.compareNoCase( htree->Get_Bone_Name( 0 ) ) != 0 )
		return false;

	// Unlike GetBreakApartBoneBindTransform above, no Set_Transform(identity) needed here --
	// Get_Obj_Space_Bounding_Box() is already expressed in the subobject's own local space,
	// independent of renderObj's current world transform.
	RenderObjClass* targetSub = renderObj->Get_Sub_Object_On_Bone( 0, boneIndex );
	if( targetSub == nullptr )
		return false;

	AABoxClass subBox;
	targetSub->Get_Obj_Space_Bounding_Box( subBox );
	*outLocalCenter = subBox.Center;
	REF_PTR_RELEASE( targetSub );
	return true;
}
