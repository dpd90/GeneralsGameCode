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

// FILE: ScaledAttachmentUpdateV2.h ///////////////////////////////////////////////////////////////
// Desc:   GeneralsMod @feature Dimitar 02/10/2026
//         Spawns one attachment object (e.g. a shield bubble) when its owner is created, keeps it
//         glued to the owner (position + facing, optionally at a bone + offset), and scales and/or
//         fades it in and out whenever the owner gains/loses a model condition flag. W3D animation
//         has no scale channel, so this is the way to get a real "grow in" effect: the attachment is
//         its own object and its Drawable's instance scale is driven here (same mechanism EMPUpdate
//         uses). Completely generic -- the only link to anything else is the condition flag, which
//         any module can set (PointDefenseUpdateV2's ActiveConditionState, SwitchStateV2, upgrades...).
//
//         Behavior:
//           - The attachment is spawned on the owner's first update (not in the constructor -- the
//             owner isn't fully created yet), on the owner's team, with the owner as producer.
//           - ShowCondition: while the owner has it, the attachment scales/fades IN; when it clears,
//             it scales/fades OUT. Omitted = always shown (plays the "in" once at creation).
//             Flipping mid-transition reverses smoothly from the current value.
//           - Scale and fade are independent channels, each with its own in/out time and curve.
//             A time of 0 means "no effect in that direction": an in-time of 0 snaps to the shown
//             value, an out-time of 0 keeps the shown value until the other channel has finished.
//             When the "out" side has fully finished, the attachment is hidden completely.
//           - Hidden while the owner is contained (transport/building) or its own Drawable is hidden
//             (e.g. stealthed to the viewing player). The owner's opacity (own-stealth translucency)
//             is multiplied in as well. These are purely per-client visuals -- they never touch the
//             logic state, so they can't desync.
//           - Owner dies: the attachment does its "out" transition, then is destroyed. The destroy
//             time is max(ScaleOutTime, FadeOutTime) after death, independent of the condition flags,
//             so it's deterministic. Owner removed instantly (sold/deleted): attachment is destroyed
//             at the same moment (onDelete).
//           - Owner captured: the attachment changes team with it.
//           - AttachBone uses the bone's PRISTINE (default-pose) position -- it won't follow turret
//             rotation or animation. Offset is in the owner's local space (rotates with the owner).
//           - FollowOwnerRotation (default Yes): No = the attachment keeps the facing it was spawned
//             with (the owner's facing at that moment) and never turns with the owner -- e.g. a shield
//             bubble that shouldn't visibly spin when the tank turns. Its POSITION still follows the
//             owner exactly (an off-center bone/Offset still stays on the same spot of the owner).
//           - The attachment object itself should be inert: KindOf NO_COLLIDE UNATTACKABLE INERT, not
//             selectable, plain W3DModelDraw, no damageable body.
//           - Below full opacity the model is drawn in the translucent pass (capped by
//             MaxVisibleTranslucentObjects, 512 by default), and the opacity overrides the material's
//             own alpha -- test with the real material.
//
//         Behavior = ScaledAttachmentUpdateV2 ModuleTag_ShieldBubble
//           ObjectName    = PDShieldBubble
//           AttachBone    = SHIELD          ; optional
//           Offset        = X:0 Y:0 Z:0     ; optional
//           FollowOwnerRotation = No        ; optional; default Yes
//           ShowCondition = USER_1          ; optional; omit = always shown
//           StartScale    = 0.0
//           EndScale      = 1.0
//           ScaleInTime   = 600             ; ms; 0 = snap
//           ScaleOutTime  = 400
//           ScaleCurve    = EASE_OUT        ; LINEAR (default) | EASE_IN | EASE_OUT | EASE_IN_OUT
//           FadeInTime    = 300             ; ms; 0 = no fade (default)
//           FadeOutTime   = 400
//           MaxOpacity    = 0.8             ; default 1.0
//           FadeCurve     = LINEAR
//         End
//
//         Curves are applied symmetrically on the way out (the same curve run backwards), so
//         EASE_OUT grows fast-then-gentle and shrinks gentle-then-fast -- a "collapse".
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "GameLogic/Module/UpdateModule.h"

// FORWARD REFERENCES /////////////////////////////////////////////////////////////////////////////
class Player;
enum ModelConditionFlagType CPP_11(: Int);

//-------------------------------------------------------------------------------------------------
enum ScaledAttachmentCurve CPP_11(: Int)
{
	SACURVE_LINEAR = 0,
	SACURVE_EASE_IN,
	SACURVE_EASE_OUT,
	SACURVE_EASE_IN_OUT,

	SACURVE_COUNT
};
extern const char *const TheScaledAttachmentCurveNames[];	///< INI names for ScaledAttachmentCurve (shared with ScaledUpdateV2)

//-------------------------------------------------------------------------------------------------
class ScaledAttachmentUpdateV2ModuleData : public UpdateModuleData
{
public:
	AsciiString							m_objectName;			///< ObjectName: thing template of the attachment
	AsciiString							m_attachBone;			///< AttachBone: optional owner bone (pristine position)
	Coord3D									m_offset;					///< Offset: owner-local offset added on top
	Bool										m_followOwnerRotation;	///< FollowOwnerRotation: Yes (default) = turn with the owner; No = keep the spawn facing
	ModelConditionFlagType	m_showCondition;	///< ShowCondition: MODELCONDITION_INVALID = always shown
	Real										m_startScale;			///< StartScale
	Real										m_endScale;				///< EndScale
	UnsignedInt							m_scaleInFrames;	///< ScaleInTime
	UnsignedInt							m_scaleOutFrames;	///< ScaleOutTime
	Int											m_scaleCurve;			///< ScaleCurve (ScaledAttachmentCurve)
	UnsignedInt							m_fadeInFrames;		///< FadeInTime
	UnsignedInt							m_fadeOutFrames;	///< FadeOutTime
	Real										m_maxOpacity;			///< MaxOpacity
	Int											m_fadeCurve;			///< FadeCurve (ScaledAttachmentCurve)

	ScaledAttachmentUpdateV2ModuleData();
	static void buildFieldParse(MultiIniFieldParse& p);
};

//-------------------------------------------------------------------------------------------------
class ScaledAttachmentUpdateV2 : public UpdateModule
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( ScaledAttachmentUpdateV2, "ScaledAttachmentUpdateV2" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( ScaledAttachmentUpdateV2, ScaledAttachmentUpdateV2ModuleData )

public:

	ScaledAttachmentUpdateV2( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	virtual void onObjectCreated() override;
	virtual UpdateSleepTime update() override;
	virtual DisabledMaskType getDisabledTypesToProcess() const override { return DISABLEDMASK_ALL; }	///< keep following/animating while the owner is EMP'd, unpowered, etc.
	virtual void onDelete() override;
	virtual void onCapture( Player *oldOwner, Player *newOwner ) override;

	// Shared with ScaledUpdateV2.
	static Bool advanceChannel( Real &progress, Bool shown, UnsignedInt inFrames, UnsignedInt outFrames );	///< returns TRUE if the channel has nothing left to do in the current direction
	static Real applyCurve( Int curve, Real t );

private:

	Object* spawnAttachment();
	void positionAttachment( Object *att );
	void applyVisuals( Object *att );
	Bool ownerWantsShown() const;

	// saved
	ObjectID		m_attachmentID;
	Bool				m_spawned;
	Real				m_scaleProgress;			///< 0..1 along the scale channel
	Real				m_fadeProgress;				///< 0..1 along the fade channel
	Bool				m_fullyOut;						///< out-transition finished -> attachment hidden
	UnsignedInt	m_destroyFrame;				///< owner died: destroy the attachment on this frame (0 = not dying)

	// not saved (re-derived)
	Coord3D			m_localAttachPos;			///< bone + offset in owner-local space, computed once lazily
	Bool				m_localAttachPosValid;
	Int					m_appliedHidden;			///< last value pushed to setDrawableHidden; -1 = unknown
};
