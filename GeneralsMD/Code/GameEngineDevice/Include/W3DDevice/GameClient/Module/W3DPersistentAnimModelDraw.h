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

// FILE: W3DPersistentAnimModelDraw.h /////////////////////////////////////////////////////////////
//
//	GeneralsMod @feature Dimitar 13/09/2026
//	Just like W3DModelDraw (every existing field still works -- Model, Animation, ConditionState,
//	AnimationsRequirePower, all of it), except a configurable set of DisabledType values can be
//	told to NOT pause this draw module's animation, even though they normally would.
//
//	Why this exists: Drawable::getShouldAnimate() (GameClient/Drawable.cpp) unconditionally stops
//	animation whenever an object is disabled by any of a fixed, hardcoded set of types --
//	DISABLED_HACKED, DISABLED_PARALYZED, DISABLED_EMP, DISABLED_SUBDUED, DISABLED_UNMANNED --
//	regardless of AnimationsRequirePower (that field only ever gates the separate
//	DISABLED_UNDERPOWERED case). There's no INI-level way to opt a specific Draw module out of that
//	for a specific disable type -- e.g. a unit with a subdual-based "stun" mechanic that should keep
//	playing an idle/glow animation while stunned, instead of freezing solid. This module adds that
//	opt-out, per Draw module instance, without touching Drawable.cpp at all.
//
//	How: W3DModelDraw::doDrawModule() unconditionally calls the VIRTUAL setPauseAnimation() every
//	frame with whatever Drawable::getShouldAnimate() decided (which knows nothing of this module's
//	AnimateThroughDisabledTypes field). Rather than letting that pause happen and trying to undo it
//	afterward -- which was the first approach tried here, and turned out to be actively wrong: every
//	single frame the base freezes the model at its current frame (Set_Animation(..., ANIM_MODE_MANUAL))
//	and an after-the-fact un-pause resumes it from that SAME frame number, since zero time passes
//	between the two calls; the net effect is the animation's position gets reset to itself dozens of
//	times a second, which is indistinguishable from being frozen -- this module instead OVERRIDES
//	setPauseAnimation() itself and silently swallows the incoming pauseAnim==TRUE request whenever
//	every currently-active fixed-list DisabledType (the same five re-derived below -- there's no way
//	to ask the engine "what disable type caused this?", only to re-derive it, so if that list in
//	Drawable.cpp ever changes, this needs to be revisited to match) is one we've been told to animate
//	through. Swallowing means the base W3DModelDraw::setPauseAnimation() is never called at all for
//	that request, so the render object's animation state is never touched and simply keeps playing
//	uninterrupted, exactly as if the object had never been disabled. Any other reason to pause (a
//	fixed-list type NOT in AnimateThroughDisabledTypes, or a real DISABLED_UNDERPOWERED pause via
//	AnimationsRequirePower) is passed through to the base class unchanged -- this module only ever
//	suppresses a pause it would otherwise have made, never forces an unpause that wasn't requested,
//	and never suppresses anything except the exact fixed-list types it's configured for.
//
//	Usage -- give the target object this instead of a plain W3DModelDraw:
//
//	Draw = W3DPersistentAnimModelDraw ModuleTag_01
//	  ... every existing W3DModelDraw field still works here, unchanged ...
//	  AnimateThroughDisabledTypes = ALL -HACKED         ; NONE/ALL/+X/-X tokens, same convention as
//	                                                     ; any other DamageTypeFlags-style field
//	                                                     ; (e.g. DamageTypes elsewhere in this mod).
//	                                                     ; Only HACKED/PARALYZED/EMP/SUBDUED/UNMANNED
//	                                                     ; actually do anything here -- every other
//	                                                     ; DisabledType (UNDERPOWERED, FREEFALL,
//	                                                     ; AWESTRUCK, BRAINWASHED, HELD, the two
//	                                                     ; SCRIPT_* ones, DEFAULT) doesn't pause
//	                                                     ; animation in the base engine today, so
//	                                                     ; listing them here is accepted but inert.
//	                                                     ; Default: ALL -- the whole point of this
//	                                                     ; module (as opposed to plain W3DModelDraw)
//	                                                     ; is to keep animating through everything by
//	                                                     ; default; list the field only to carve out
//	                                                     ; exceptions you DO still want to freeze for
//	                                                     ; (e.g. "ALL -HACKED" above).
//	End
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "Common/DisabledTypes.h"
#include "W3DDevice/GameClient/Module/W3DModelDraw.h"

//-------------------------------------------------------------------------------------------------
class W3DPersistentAnimModelDrawModuleData : public W3DModelDrawModuleData
{
public:

	DisabledMaskType	m_animateThroughDisabledTypes;	///< which DisabledTypes should NOT pause this draw module's animation. Default ALL (that's the whole point of using this module over plain W3DModelDraw -- list the field only to carve out exceptions). Of the full DisabledType set, only HACKED/PARALYZED/EMP/SUBDUED/UNMANNED currently do anything -- see the file header.

	W3DPersistentAnimModelDrawModuleData();

	static void buildFieldParse(MultiIniFieldParse& p);

};

//-------------------------------------------------------------------------------------------------
class W3DPersistentAnimModelDraw : public W3DModelDraw
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( W3DPersistentAnimModelDraw, "W3DPersistentAnimModelDraw" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( W3DPersistentAnimModelDraw, W3DPersistentAnimModelDrawModuleData )

public:

	W3DPersistentAnimModelDraw( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	// W3DModelDraw -- intercept the base class's per-frame pause decision instead of correcting it
	// after the fact; see the file header for why doDrawModule() itself is deliberately NOT overridden.
	virtual void setPauseAnimation(Bool pauseAnim) override;

};
