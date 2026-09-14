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

// FILE: W3DPersistentAnimModelDraw.cpp ///////////////////////////////////////////////////////////
// Desc:   See W3DPersistentAnimModelDraw.h.
///////////////////////////////////////////////////////////////////////////////////////////////////

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
// GeneralsMod @feature Dimitar 13/09/2026: unlike GameEngine/.cpp files, GameEngineDevice/.cpp files
// do NOT start with "PreRTS.h" -- this subsystem's own precompiled header (cmake_pch.hxx, built from
// z_gameenginedevice's target_precompile_headers list) covers the same ground, and PreRTS.h itself
// (Core/GameEngine/Include/Precompiled/PreRTS.h) is not even on this target's include path.
#include "Common/INI.h"
#include "Common/Xfer.h"

#include "GameClient/Drawable.h"

#include "GameLogic/Object.h"

#include "W3DDevice/GameClient/Module/W3DPersistentAnimModelDraw.h"

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
W3DPersistentAnimModelDrawModuleData::W3DPersistentAnimModelDrawModuleData()
{
	m_animateThroughDisabledTypes = DISABLEDMASK_ALL;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
/*static*/ void W3DPersistentAnimModelDrawModuleData::buildFieldParse(MultiIniFieldParse& p)
{
	W3DModelDrawModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "AnimateThroughDisabledTypes",	INI::parseDisabledMaskType,	nullptr, offsetof( W3DPersistentAnimModelDrawModuleData, m_animateThroughDisabledTypes ) },
		{ nullptr, nullptr, nullptr, 0 }
	};
	p.add(dataFieldParse);
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
W3DPersistentAnimModelDraw::W3DPersistentAnimModelDraw( Thing *thing, const ModuleData *moduleData ) : W3DModelDraw( thing, moduleData )
{
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
W3DPersistentAnimModelDraw::~W3DPersistentAnimModelDraw()
{
}

// ------------------------------------------------------------------------------------------------
/** W3DModelDraw::doDrawModule() unconditionally calls this (virtually) every frame with whatever
	* Drawable::getShouldAnimate() decided. Rather than letting a pause happen and undoing it
	* afterward (the first approach tried here -- and actively wrong: each undo resumes from the
	* exact frame it was just frozen at, since no time passes between the freeze and the undo, so the
	* animation's position gets reset to itself every single frame instead of ever advancing -- which
	* looks identical to being frozen), this swallows the incoming pauseAnim==TRUE request entirely
	* when every currently-active blocking DisabledType (from the same fixed five-type list
	* Drawable::getShouldAnimate() uses -- duplicated here since there's no way to ask the engine
	* "what type caused this?", only to re-derive it) is one this module has been told to animate
	* through. Swallowing means the base implementation -- and therefore the underlying render
	* object's Set_Animation() call -- is never invoked for that request, so playback is never
	* touched and simply continues. Any other reason to pause (a fixed-list type NOT listed in
	* AnimateThroughDisabledTypes, or a real DISABLED_UNDERPOWERED pause via AnimationsRequirePower)
	* is passed straight through to the base class, unchanged. */
// ------------------------------------------------------------------------------------------------
void W3DPersistentAnimModelDraw::setPauseAnimation(Bool pauseAnim)
{
	if( pauseAnim )
	{
		const W3DPersistentAnimModelDrawModuleData *data = getW3DPersistentAnimModelDrawModuleData();
		if( DISABLEDMASK_ANY_SET( data->m_animateThroughDisabledTypes ) )
		{
			const Object *obj = getDrawable()->getObject();
			if( obj && obj->isDisabled() )
			{
				// The exact fixed set Drawable::getShouldAnimate() treats as unconditionally
				// animation-stopping, independent of AnimationsRequirePower. KEEP THIS IN SYNC with
				// that function if its own list ever changes.
				static const DisabledType kAlwaysBlocksAnimation[] =
				{
					DISABLED_HACKED, DISABLED_PARALYZED, DISABLED_EMP, DISABLED_SUBDUED, DISABLED_UNMANNED
				};
				static const Int kNumAlwaysBlocksAnimation = sizeof(kAlwaysBlocksAnimation) / sizeof(kAlwaysBlocksAnimation[0]);

				Bool blockedByFixedListAtAll = FALSE;
				Bool stillGenuinelyBlocked = FALSE;
				for( Int i = 0; i < kNumAlwaysBlocksAnimation; ++i )
				{
					DisabledType t = kAlwaysBlocksAnimation[i];
					if( obj->isDisabledByType( t ) )
					{
						blockedByFixedListAtAll = TRUE;
						if( !TEST_DISABLEDMASK( data->m_animateThroughDisabledTypes, t ) )
							stillGenuinelyBlocked = TRUE;
					}
				}

				if( blockedByFixedListAtAll && !stillGenuinelyBlocked )
				{
					// every fixed-list block currently active is one we've been told to ignore --
					// swallow the request. do NOT forward to the base class; that's the whole point.
					return;
				}
			}
		}
	}

	W3DModelDraw::setPauseAnimation(pauseAnim);
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void W3DPersistentAnimModelDraw::crc( Xfer *xfer )
{
	// extend base class
	W3DModelDraw::crc( xfer );
}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void W3DPersistentAnimModelDraw::xfer( Xfer *xfer )
{
	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	W3DModelDraw::xfer( xfer );
}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void W3DPersistentAnimModelDraw::loadPostProcess()
{
	// extend base class
	W3DModelDraw::loadPostProcess();
}
