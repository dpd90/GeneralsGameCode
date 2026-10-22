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

// FILE: ScaledUpdateV2.cpp ///////////////////////////////////////////////////////////////////////
// Desc:   GeneralsMod @feature Dimitar 02/10/2026 -- see ScaledUpdateV2.h.
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/INI.h"
#include "Common/ModelState.h"
#include "Common/ThingTemplate.h"
#include "Common/Xfer.h"

#include "GameClient/Drawable.h"
#include "GameClient/FXList.h"

#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"
#include "GameLogic/ObjectCreationList.h"
#include "GameLogic/Module/ScaledUpdateV2.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ScaledUpdateV2ModuleData::ScaledUpdateV2ModuleData()
{
	m_duration				= 0;											// forever
	m_onRemovalOCL		= nullptr;
	m_onRemovalFX			= nullptr;
	m_showCondition		= MODELCONDITION_INVALID;	// always shown
	m_startScale			= 0.0f;
	m_endScale				= 1.0f;
	m_scaleInFrames		= 0;
	m_scaleOutFrames	= 0;
	m_scaleCurve			= SACURVE_LINEAR;
	m_fadeInFrames		= 0;
	m_fadeOutFrames		= 0;
	m_maxOpacity			= 1.0f;
	m_fadeCurve				= SACURVE_LINEAR;
}

//-------------------------------------------------------------------------------------------------
/*static*/ void ScaledUpdateV2ModuleData::buildFieldParse(MultiIniFieldParse& p)
{
	UpdateModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "Duration",				INI::parseDurationUnsignedInt,	nullptr, offsetof( ScaledUpdateV2ModuleData, m_duration ) },
		{ "OnRemovalOCL",		INI::parseObjectCreationList,		nullptr, offsetof( ScaledUpdateV2ModuleData, m_onRemovalOCL ) },
		{ "OnRemovalFX",		INI::parseFXList,								nullptr, offsetof( ScaledUpdateV2ModuleData, m_onRemovalFX ) },
		{ "ShowCondition",	INI::parseIndexList,	ModelConditionFlags::getBitNames(), offsetof( ScaledUpdateV2ModuleData, m_showCondition ) },
		{ "StartScale",			INI::parseReal,									nullptr, offsetof( ScaledUpdateV2ModuleData, m_startScale ) },
		{ "EndScale",				INI::parseReal,									nullptr, offsetof( ScaledUpdateV2ModuleData, m_endScale ) },
		{ "ScaleInTime",		INI::parseDurationUnsignedInt,	nullptr, offsetof( ScaledUpdateV2ModuleData, m_scaleInFrames ) },
		{ "ScaleOutTime",		INI::parseDurationUnsignedInt,	nullptr, offsetof( ScaledUpdateV2ModuleData, m_scaleOutFrames ) },
		{ "ScaleCurve",			INI::parseIndexList,	TheScaledAttachmentCurveNames, offsetof( ScaledUpdateV2ModuleData, m_scaleCurve ) },
		{ "FadeInTime",			INI::parseDurationUnsignedInt,	nullptr, offsetof( ScaledUpdateV2ModuleData, m_fadeInFrames ) },
		{ "FadeOutTime",		INI::parseDurationUnsignedInt,	nullptr, offsetof( ScaledUpdateV2ModuleData, m_fadeOutFrames ) },
		{ "MaxOpacity",			INI::parseReal,									nullptr, offsetof( ScaledUpdateV2ModuleData, m_maxOpacity ) },
		{ "FadeCurve",			INI::parseIndexList,	TheScaledAttachmentCurveNames, offsetof( ScaledUpdateV2ModuleData, m_fadeCurve ) },
		{ nullptr, nullptr, nullptr, 0 }
	};
	p.add(dataFieldParse);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ScaledUpdateV2::ScaledUpdateV2( Thing *thing, const ModuleData* moduleData ) : UpdateModule( thing, moduleData )
{
	m_startFrame		= TheGameLogic->getFrame();
	m_scaleProgress	= 0.0f;
	m_fadeProgress	= 0.0f;
	m_fullyOut			= TRUE;
	m_appliedHidden	= -1;

	setWakeFrame( getObject(), UPDATE_SLEEP_NONE );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ScaledUpdateV2::~ScaledUpdateV2()
{
}

//-------------------------------------------------------------------------------------------------
Bool ScaledUpdateV2::conditionShown() const
{
	const ScaledUpdateV2ModuleData *data = getScaledUpdateV2ModuleData();
	if( data->m_showCondition == MODELCONDITION_INVALID )
		return TRUE;

	const Drawable *draw = getObject()->getDrawable();
	return draw && draw->getModelConditionFlags().test( data->m_showCondition );
}

//-------------------------------------------------------------------------------------------------
/** Called after Object::init() (Drawable bound). Puts the model into its starting state right away,
	* so an object that scales in from 0 never renders one frame at full size first. */
//-------------------------------------------------------------------------------------------------
void ScaledUpdateV2::onCreate()
{
	const ScaledUpdateV2ModuleData *data = getScaledUpdateV2ModuleData();

	if( conditionShown() )
	{
		// Channels with no in-time snap to shown immediately; the rest start from 0 on the next update.
		if( data->m_scaleInFrames == 0 )
			m_scaleProgress = 1.0f;
		if( data->m_fadeInFrames == 0 )
			m_fadeProgress = 1.0f;
		m_fullyOut = FALSE;
	}

	applyVisuals();
}

//-------------------------------------------------------------------------------------------------
void ScaledUpdateV2::applyVisuals()
{
	Drawable *draw = getObject()->getDrawable();
	if( !draw )
		return;

	const ScaledUpdateV2ModuleData *data = getScaledUpdateV2ModuleData();

	Int hiddenInt = m_fullyOut ? 1 : 0;
	if( hiddenInt != m_appliedHidden )
	{
		draw->setDrawableHidden( m_fullyOut );
		m_appliedHidden = hiddenInt;
	}
	if( m_fullyOut )
		return;

	Real scale = data->m_startScale + ( data->m_endScale - data->m_startScale ) * ScaledAttachmentUpdateV2::applyCurve( data->m_scaleCurve, m_scaleProgress );
	if( scale < 0.001f )
		scale = 0.001f;	// never hand W3D a degenerate (zero) scale matrix
	draw->setInstanceScale( scale );

	// Explicit opacity only -- stealth translucency is still multiplied in by the Drawable itself.
	Real opacity = data->m_maxOpacity * ScaledAttachmentUpdateV2::applyCurve( data->m_fadeCurve, m_fadeProgress );
	if( opacity < 0.0f )
		opacity = 0.0f;
	if( opacity > 1.0f )
		opacity = 1.0f;
	draw->setDrawableOpacity( opacity );
}

//-------------------------------------------------------------------------------------------------
/** The update callback. */
//-------------------------------------------------------------------------------------------------
UpdateSleepTime ScaledUpdateV2::update()
{
	const ScaledUpdateV2ModuleData *data = getScaledUpdateV2ModuleData();
	UnsignedInt now = TheGameLogic->getFrame();
	UnsignedInt elapsed = now - m_startFrame;

	// Duration over: remove ourselves (deterministic -- depends only on creation frame + INI). This is
	// the object's one authoritative lifetime, same as DecalUpdateV2 -- no separate LifetimeUpdate.
	if( data->m_duration > 0 && elapsed >= data->m_duration )
	{
		Object *self = getObject();

		// Fired here, not from a death module: this is a removal, not a death. Self is still fully
		// valid, so OCL/FX read its position/team exactly as they would from onDie().
		if( data->m_onRemovalOCL )
			ObjectCreationList::create( data->m_onRemovalOCL, self, nullptr );
		if( data->m_onRemovalFX )
			FXList::doFXObj( data->m_onRemovalFX, self );

		TheGameLogic->destroyObject( self );
		return UPDATE_SLEEP_FOREVER;
	}

	// Each channel starts its Out so that it finishes exactly at Duration (DecalUpdateV2 semantics).
	Bool cond = conditionShown();
	Bool scaleShown = cond && ( data->m_duration == 0 || elapsed + data->m_scaleOutFrames < data->m_duration );
	Bool fadeShown	= cond && ( data->m_duration == 0 || elapsed + data->m_fadeOutFrames < data->m_duration );

	Bool scaleDone	= ScaledAttachmentUpdateV2::advanceChannel( m_scaleProgress, scaleShown, data->m_scaleInFrames, data->m_scaleOutFrames );
	Bool fadeDone		= ScaledAttachmentUpdateV2::advanceChannel( m_fadeProgress, fadeShown, data->m_fadeInFrames, data->m_fadeOutFrames );

	if( scaleShown || fadeShown )
		m_fullyOut = FALSE;
	else if( scaleDone && fadeDone )
	{
		m_fullyOut = TRUE;
		m_scaleProgress = 0.0f;	// so 0-out-time channels (which held) start from 0 if shown again
		m_fadeProgress = 0.0f;
	}

	applyVisuals();

	// --- Sleep (like DecalUpdateV2: don't tick through a settled hold window) ---
	if( !scaleDone || !fadeDone )
		return UPDATE_SLEEP_NONE;	// still animating

	if( data->m_showCondition != MODELCONDITION_INVALID )
		return UPDATE_SLEEP_NONE;	// the flag can change any frame -- poll

	if( data->m_duration > 0 )
	{
		// Next thing that can happen: a channel starting its Out, or the self-destruct.
		UnsignedInt next = data->m_duration;
		if( scaleShown && data->m_scaleOutFrames > 0 && data->m_duration - data->m_scaleOutFrames < next )
			next = data->m_duration - data->m_scaleOutFrames;
		if( fadeShown && data->m_fadeOutFrames > 0 && data->m_duration - data->m_fadeOutFrames < next )
			next = data->m_duration - data->m_fadeOutFrames;

		return ( next > elapsed ) ? UPDATE_SLEEP( next - elapsed ) : UPDATE_SLEEP_NONE;
	}

	return UPDATE_SLEEP_FOREVER;	// settled, no Duration, no condition: nothing will ever change again
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void ScaledUpdateV2::crc( Xfer *xfer )
{
	// extend base class
	UpdateModule::crc( xfer );
}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void ScaledUpdateV2::xfer( Xfer *xfer )
{
	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpdateModule::xfer( xfer );

	xfer->xferUnsignedInt( &m_startFrame );
	xfer->xferReal( &m_scaleProgress );
	xfer->xferReal( &m_fadeProgress );
	xfer->xferBool( &m_fullyOut );
}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void ScaledUpdateV2::loadPostProcess()
{
	// extend base class
	UpdateModule::loadPostProcess();

	// We may be asleep for a while after load -- push the restored state to the Drawable now.
	m_appliedHidden = -1;
	applyVisuals();
}
