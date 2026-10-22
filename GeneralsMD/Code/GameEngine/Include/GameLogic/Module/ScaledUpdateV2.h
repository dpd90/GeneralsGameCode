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

// FILE: ScaledUpdateV2.h /////////////////////////////////////////////////////////////////////////
// Desc:   GeneralsMod @feature Dimitar 02/10/2026
//         Scales and/or fades THIS object's own model in and out -- the self-contained counterpart of
//         ScaledAttachmentUpdateV2 (which spawns and drives a separate attached object), in the same
//         spirit as DecalUpdateV2 (which drives its own object's decal over a lifetime). Same scale/
//         fade channels, times and curves as ScaledAttachmentUpdateV2, plus two optional drivers:
//
//           - Duration (like DecalUpdateV2): > 0 = the object scales/fades in at creation, holds, runs
//             each channel's Out so it finishes exactly at Duration, then destroys itself -- this IS
//             the object's lifetime, no separate LifetimeUpdate needed (don't add one: it would race
//             this timer). At removal it fires OnRemovalOCL / OnRemovalFX (both optional) at its own
//             position, exactly like DecalUpdateV2. 0 (default) = lives forever.
//           - ShowCondition: optional model condition flag on THIS object -- shown while set,
//             scaled/faded out while clear. Omit = always shown (until Duration says otherwise).
//             Both can be combined.
//
//         Channel rules (same as ScaledAttachmentUpdateV2): a time of 0 = no effect in that direction
//         (in = snap to shown, out = hold until the other channel / Duration finishes). Mid-transition
//         flips reverse smoothly. Fully out = drawable hidden (the object itself still exists).
//         Curves (LINEAR, EASE_IN, EASE_OUT, EASE_IN_OUT) run symmetrically on the way out.
//
//         Notes:
//           - Visual only (Drawable instance scale/opacity, the EMPUpdate mechanism): geometry, not
//             collision or weapon ranges. It overrides the template's InstanceScaleFuzziness.
//           - The initial state is applied in onCreate() (Drawable is bound by then), so a scaling-in
//             object never flashes at full size for a frame.
//           - Sleeps while nothing is changing (like DecalUpdateV2): fully settled + no ShowCondition =
//             no per-frame cost; with Duration it wakes exactly when an Out has to start. With
//             ShowCondition it polls every frame (the flag can change at any time).
//           - Don't put this on an object that a ScaledAttachmentUpdateV2 is also driving -- both would
//             write the same Drawable scale/opacity.
//           - Below full opacity the model renders in the translucent pass and the opacity overrides
//             the material's own alpha -- test with the real material.
//
//         Behavior = ScaledUpdateV2 ModuleTag_Grow
//           Duration      = 5000            ; ms; removes the object at the end; 0 = forever (default)
//           OnRemovalOCL  = OCL_Something   ; optional, fired when Duration removes the object
//           OnRemovalFX   = FX_Something    ; optional, same
//           ShowCondition = USER_1          ; optional
//           StartScale    = 0.0
//           EndScale      = 1.0
//           ScaleInTime   = 600
//           ScaleOutTime  = 400
//           ScaleCurve    = EASE_OUT
//           FadeInTime    = 0
//           FadeOutTime   = 1000
//           MaxOpacity    = 1.0
//           FadeCurve     = LINEAR
//         End
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "GameLogic/Module/UpdateModule.h"
#include "GameLogic/Module/CreateModule.h"
#include "GameLogic/Module/ScaledAttachmentUpdateV2.h"	// ScaledAttachmentCurve + shared channel/curve helpers

class ObjectCreationList;
class FXList;

//-------------------------------------------------------------------------------------------------
class ScaledUpdateV2ModuleData : public UpdateModuleData
{
public:
	UnsignedInt							m_duration;				///< Duration: frames until self-destruct; 0 = forever
	const ObjectCreationList*	m_onRemovalOCL;		///< OnRemovalOCL: fired when Duration removes the object (nullptr = none)
	const FXList*						m_onRemovalFX;		///< OnRemovalFX: same, an FXList (nullptr = none)
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

	ScaledUpdateV2ModuleData();
	static void buildFieldParse(MultiIniFieldParse& p);
};

//-------------------------------------------------------------------------------------------------
class ScaledUpdateV2 : public UpdateModule, public CreateModuleInterface
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( ScaledUpdateV2, "ScaledUpdateV2" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( ScaledUpdateV2, ScaledUpdateV2ModuleData )

public:

	ScaledUpdateV2( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	static Int getInterfaceMask() { return UpdateModule::getInterfaceMask() | MODULEINTERFACE_CREATE; }

	// BehaviorModule
	virtual CreateModuleInterface* getCreate() override { return this; }

	// UpdateModule
	virtual UpdateSleepTime update() override;
	virtual DisabledMaskType getDisabledTypesToProcess() const override { return DISABLEDMASK_ALL; }	///< Duration and transitions keep running while disabled

	// CreateModuleInterface -- initial visuals, once the Drawable is bound (it isn't in the constructor)
	virtual void onCreate() override;
	virtual void onBuildComplete() override {}
	virtual Bool shouldDoOnBuildComplete() const override { return false; }

private:

	Bool conditionShown() const;
	void applyVisuals();

	// saved
	UnsignedInt	m_startFrame;
	Real				m_scaleProgress;
	Real				m_fadeProgress;
	Bool				m_fullyOut;

	// not saved
	Int					m_appliedHidden;	///< last value pushed to setDrawableHidden; -1 = unknown
};
