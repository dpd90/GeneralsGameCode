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

// FILE: ScaledAttachmentUpdateV2.cpp /////////////////////////////////////////////////////////////
// Desc:   GeneralsMod @feature Dimitar 02/10/2026 -- see ScaledAttachmentUpdateV2.h.
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/INI.h"
#include "Common/ModelState.h"
#include "Common/Player.h"
#include "Common/ThingFactory.h"
#include "Common/ThingTemplate.h"
#include "Common/Xfer.h"

#include "GameClient/Drawable.h"

#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"
#include "GameLogic/Module/ScaledAttachmentUpdateV2.h"

//-------------------------------------------------------------------------------------------------
// Order must match ScaledAttachmentCurve. Not static: ScaledUpdateV2 parses its curves with it too.
const char *const TheScaledAttachmentCurveNames[] =
{
	"LINEAR",
	"EASE_IN",
	"EASE_OUT",
	"EASE_IN_OUT",

	nullptr
};
static_assert(ARRAY_SIZE(TheScaledAttachmentCurveNames) == SACURVE_COUNT + 1, "Incorrect array size");

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ScaledAttachmentUpdateV2ModuleData::ScaledAttachmentUpdateV2ModuleData()
{
	m_offset.zero();
	m_followOwnerRotation	= TRUE;
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
/*static*/ void ScaledAttachmentUpdateV2ModuleData::buildFieldParse(MultiIniFieldParse& p)
{
	UpdateModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "ObjectName",			INI::parseAsciiString,					nullptr, offsetof( ScaledAttachmentUpdateV2ModuleData, m_objectName ) },
		{ "AttachBone",			INI::parseAsciiString,					nullptr, offsetof( ScaledAttachmentUpdateV2ModuleData, m_attachBone ) },
		{ "Offset",					INI::parseCoord3D,							nullptr, offsetof( ScaledAttachmentUpdateV2ModuleData, m_offset ) },
		{ "FollowOwnerRotation",	INI::parseBool,						nullptr, offsetof( ScaledAttachmentUpdateV2ModuleData, m_followOwnerRotation ) },
		{ "ShowCondition",	INI::parseIndexList,	ModelConditionFlags::getBitNames(), offsetof( ScaledAttachmentUpdateV2ModuleData, m_showCondition ) },
		{ "StartScale",			INI::parseReal,									nullptr, offsetof( ScaledAttachmentUpdateV2ModuleData, m_startScale ) },
		{ "EndScale",				INI::parseReal,									nullptr, offsetof( ScaledAttachmentUpdateV2ModuleData, m_endScale ) },
		{ "ScaleInTime",		INI::parseDurationUnsignedInt,	nullptr, offsetof( ScaledAttachmentUpdateV2ModuleData, m_scaleInFrames ) },
		{ "ScaleOutTime",		INI::parseDurationUnsignedInt,	nullptr, offsetof( ScaledAttachmentUpdateV2ModuleData, m_scaleOutFrames ) },
		{ "ScaleCurve",			INI::parseIndexList,	TheScaledAttachmentCurveNames, offsetof( ScaledAttachmentUpdateV2ModuleData, m_scaleCurve ) },
		{ "FadeInTime",			INI::parseDurationUnsignedInt,	nullptr, offsetof( ScaledAttachmentUpdateV2ModuleData, m_fadeInFrames ) },
		{ "FadeOutTime",		INI::parseDurationUnsignedInt,	nullptr, offsetof( ScaledAttachmentUpdateV2ModuleData, m_fadeOutFrames ) },
		{ "MaxOpacity",			INI::parseReal,									nullptr, offsetof( ScaledAttachmentUpdateV2ModuleData, m_maxOpacity ) },
		{ "FadeCurve",			INI::parseIndexList,	TheScaledAttachmentCurveNames, offsetof( ScaledAttachmentUpdateV2ModuleData, m_fadeCurve ) },
		{ nullptr, nullptr, nullptr, 0 }
	};
	p.add(dataFieldParse);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ScaledAttachmentUpdateV2::ScaledAttachmentUpdateV2( Thing *thing, const ModuleData* moduleData ) : UpdateModule( thing, moduleData )
{
	m_attachmentID				= INVALID_ID;
	m_spawned							= FALSE;
	m_scaleProgress				= 0.0f;
	m_fadeProgress				= 0.0f;
	m_fullyOut						= TRUE;		// starts hidden; the first update decides whether to scale/fade in
	m_destroyFrame				= 0;

	m_localAttachPos.zero();
	m_localAttachPosValid	= FALSE;
	m_appliedHidden				= -1;

	// The attachment is spawned on the first update -- the owner isn't fully created yet here.
	setWakeFrame( getObject(), UPDATE_SLEEP_NONE );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ScaledAttachmentUpdateV2::~ScaledAttachmentUpdateV2()
{
	// Nothing to free: the attachment is a normal game object owned by GameLogic, and is destroyed
	// in onDelete() (or by the death countdown in update()). Don't touch GameLogic from here -- this
	// also runs during map teardown.
}

//-------------------------------------------------------------------------------------------------
void ScaledAttachmentUpdateV2::onObjectCreated()
{
	const ScaledAttachmentUpdateV2ModuleData *data = getScaledAttachmentUpdateV2ModuleData();
	if( data->m_objectName.isEmpty() )
	{
		DEBUG_CRASH( ("ScaledAttachmentUpdateV2 on %s has no ObjectName -- nothing will be attached",
			getObject()->getTemplate()->getName().str() ) );
	}
}

//-------------------------------------------------------------------------------------------------
/** Shapes a 0..1 progress value. Applied symmetrically in both directions (see header). */
//-------------------------------------------------------------------------------------------------
/*static*/ Real ScaledAttachmentUpdateV2::applyCurve( Int curve, Real t )
{
	if( t <= 0.0f )
		return 0.0f;
	if( t >= 1.0f )
		return 1.0f;

	switch( curve )
	{
		case SACURVE_EASE_IN:			return t * t;
		case SACURVE_EASE_OUT:		return t * ( 2.0f - t );
		case SACURVE_EASE_IN_OUT:	return t * t * ( 3.0f - 2.0f * t );
		case SACURVE_LINEAR:
		default:									return t;
	}
}

//-------------------------------------------------------------------------------------------------
/** Moves one channel one frame toward its target. A 0 time in the current direction means "no effect":
	* in = snap to shown, out = hold the current value (the other channel decides when we're done).
	* Returns TRUE once the channel has nothing left to do in the current direction. */
//-------------------------------------------------------------------------------------------------
/*static*/ Bool ScaledAttachmentUpdateV2::advanceChannel( Real &progress, Bool shown, UnsignedInt inFrames, UnsignedInt outFrames )
{
	if( shown )
	{
		if( inFrames == 0 )
			progress = 1.0f;
		else
		{
			progress += 1.0f / (Real)inFrames;
			if( progress > 1.0f )
				progress = 1.0f;
		}
		return progress >= 1.0f;
	}

	if( outFrames == 0 )
		return TRUE;

	progress -= 1.0f / (Real)outFrames;
	if( progress < 0.0f )
		progress = 0.0f;
	return progress <= 0.0f;
}

//-------------------------------------------------------------------------------------------------
Bool ScaledAttachmentUpdateV2::ownerWantsShown() const
{
	const ScaledAttachmentUpdateV2ModuleData *data = getScaledAttachmentUpdateV2ModuleData();
	if( data->m_showCondition == MODELCONDITION_INVALID )
		return TRUE;

	// Model condition flags are set by game logic (Object::setModelConditionState), so this is
	// deterministic. Nothing that changes logic state (object creation/destruction) depends on it
	// anyway -- see the death countdown in update().
	const Drawable *ownerDraw = getObject()->getDrawable();
	return ownerDraw && ownerDraw->getModelConditionFlags().test( data->m_showCondition );
}

//-------------------------------------------------------------------------------------------------
Object* ScaledAttachmentUpdateV2::spawnAttachment()
{
	const ScaledAttachmentUpdateV2ModuleData *data = getScaledAttachmentUpdateV2ModuleData();
	if( data->m_objectName.isEmpty() )
		return nullptr;

	const ThingTemplate *tt = TheThingFactory->findTemplate( data->m_objectName );
	if( !tt )
	{
		DEBUG_CRASH( ("ScaledAttachmentUpdateV2 on %s: ObjectName '%s' not found",
			getObject()->getTemplate()->getName().str(), data->m_objectName.str() ) );
		return nullptr;
	}

	Object *owner = getObject();
	Object *att = TheThingFactory->newObject( tt, owner->getTeam() );
	if( !att )
		return nullptr;

	att->setProducer( owner );
	m_appliedHidden = -1;

	// Start with the owner's facing. With FollowOwnerRotation = No this is the facing it keeps forever.
	att->setOrientation( owner->getOrientation() );
	positionAttachment( att );
	return att;
}

//-------------------------------------------------------------------------------------------------
void ScaledAttachmentUpdateV2::positionAttachment( Object *att )
{
	const ScaledAttachmentUpdateV2ModuleData *data = getScaledAttachmentUpdateV2ModuleData();
	const Object *owner = getObject();

	// Bone + offset in owner-local space, looked up once. Pristine bone position -- the same thing
	// Object::getSingleLogicalBonePosition() uses for logic purposes (weapon fire bones etc.).
	if( !m_localAttachPosValid )
	{
		m_localAttachPos.zero();
		const Drawable *ownerDraw = owner->getDrawable();
		if( data->m_attachBone.isNotEmpty() && ownerDraw )
		{
			Coord3D bonePos;
			if( ownerDraw->getPristineBonePositions( data->m_attachBone.str(), 0, &bonePos, nullptr, 1 ) == 1 )
				m_localAttachPos = bonePos;
			else
				DEBUG_LOG( ("ScaledAttachmentUpdateV2 on %s: AttachBone '%s' not found, using the object's center",
					owner->getTemplate()->getName().str(), data->m_attachBone.str() ) );
		}
		m_localAttachPos.add( data->m_offset );
		m_localAttachPosValid = TRUE;
	}

	Vector3 local( m_localAttachPos.x, m_localAttachPos.y, m_localAttachPos.z );
	Vector3 world;
	Matrix3D::Transform_Vector( *owner->getTransformMatrix(), local, &world );

	Coord3D pos;
	pos.set( world.X, world.Y, world.Z );
	Real angle = owner->getOrientation();

	// Skip the partition update when nothing moved (parked units, structures).
	const Coord3D *cur = att->getPosition();
	if( cur->x != pos.x || cur->y != pos.y || cur->z != pos.z )
		att->setPosition( &pos );
	// FollowOwnerRotation = No: keep the spawn facing (set in spawnAttachment()), never turn with the owner.
	if( data->m_followOwnerRotation && att->getOrientation() != angle )
		att->setOrientation( angle );
}

//-------------------------------------------------------------------------------------------------
/** Pushes scale/opacity/hidden to the attachment's Drawable. Purely visual, per client: the
	* contained/hidden/opacity inputs from the owner's Drawable can differ between clients (stealth),
	* which is fine because nothing here feeds back into logic state. */
//-------------------------------------------------------------------------------------------------
void ScaledAttachmentUpdateV2::applyVisuals( Object *att )
{
	Drawable *draw = att->getDrawable();
	if( !draw )
		return;

	const ScaledAttachmentUpdateV2ModuleData *data = getScaledAttachmentUpdateV2ModuleData();
	const Object *owner = getObject();
	const Drawable *ownerDraw = owner->getDrawable();

	Bool hidden = m_fullyOut
		|| owner->getContainedBy() != nullptr
		|| ( ownerDraw && ownerDraw->isDrawableEffectivelyHidden() );

	Int hiddenInt = hidden ? 1 : 0;
	if( hiddenInt != m_appliedHidden )
	{
		draw->setDrawableHidden( hidden );
		m_appliedHidden = hiddenInt;
	}
	if( hidden )
		return;

	Real scale = data->m_startScale + ( data->m_endScale - data->m_startScale ) * applyCurve( data->m_scaleCurve, m_scaleProgress );
	if( scale < 0.001f )
		scale = 0.001f;	// never hand W3D a degenerate (zero) scale matrix
	draw->setInstanceScale( scale );

	Real opacity = data->m_maxOpacity * applyCurve( data->m_fadeCurve, m_fadeProgress );
	if( ownerDraw )
		opacity *= ownerDraw->getEffectiveOpacity();	// follow the owner's own stealth translucency
	if( opacity < 0.0f )
		opacity = 0.0f;
	if( opacity > 1.0f )
		opacity = 1.0f;
	draw->setDrawableOpacity( opacity );
}

//-------------------------------------------------------------------------------------------------
/** The update callback. */
//-------------------------------------------------------------------------------------------------
UpdateSleepTime ScaledAttachmentUpdateV2::update()
{
	const ScaledAttachmentUpdateV2ModuleData *data = getScaledAttachmentUpdateV2ModuleData();
	Object *owner = getObject();
	UnsignedInt now = TheGameLogic->getFrame();

	if( !m_spawned )
	{
		m_spawned = TRUE;
		Object *spawned = spawnAttachment();
		if( !spawned )
			return UPDATE_SLEEP_FOREVER;
		m_attachmentID = spawned->getID();
	}

	Object *att = TheGameLogic->findObjectByID( m_attachmentID );
	if( !att )
	{
		// Destroyed by something else -- don't respawn, just stop.
		m_attachmentID = INVALID_ID;
		return UPDATE_SLEEP_FOREVER;
	}

	// Owner died: run the "out" transition, then destroy. The destroy frame depends only on the
	// death frame and INI times (never on condition flags or progress), so it's deterministic.
	if( owner->isEffectivelyDead() && m_destroyFrame == 0 )
	{
		UnsignedInt outFrames = ( data->m_scaleOutFrames > data->m_fadeOutFrames ) ? data->m_scaleOutFrames : data->m_fadeOutFrames;
		m_destroyFrame = now + outFrames;
	}
	if( m_destroyFrame != 0 && now >= m_destroyFrame )
	{
		TheGameLogic->destroyObject( att );
		m_attachmentID = INVALID_ID;
		return UPDATE_SLEEP_FOREVER;
	}

	Bool shown = ( m_destroyFrame == 0 ) && ownerWantsShown();
	Bool scaleDone	= advanceChannel( m_scaleProgress, shown, data->m_scaleInFrames, data->m_scaleOutFrames );
	Bool fadeDone		= advanceChannel( m_fadeProgress, shown, data->m_fadeInFrames, data->m_fadeOutFrames );

	if( shown )
		m_fullyOut = FALSE;
	else if( scaleDone && fadeDone )
	{
		// Out side finished. Reset both channels so channels with a 0 out-time (which "held" their
		// value) start from 0 again next time we're shown.
		m_fullyOut = TRUE;
		m_scaleProgress = 0.0f;
		m_fadeProgress = 0.0f;
	}

	positionAttachment( att );
	applyVisuals( att );

	return UPDATE_SLEEP_NONE;
}

//-------------------------------------------------------------------------------------------------
/** Owner is being destroyed right now (sold, deleted, end of slow death...): take the attachment
	* with it. GameLogic::destroyObject() explicitly supports destroying sub-objects from here. */
//-------------------------------------------------------------------------------------------------
void ScaledAttachmentUpdateV2::onDelete()
{
	if( m_attachmentID == INVALID_ID || !TheGameLogic )
		return;

	Object *att = TheGameLogic->findObjectByID( m_attachmentID );
	if( att )
		TheGameLogic->destroyObject( att );
	m_attachmentID = INVALID_ID;
}

//-------------------------------------------------------------------------------------------------
/** Owner changed hands (hijack, capture...): the attachment follows. The owner's team is already
	* updated by the time this is called. */
//-------------------------------------------------------------------------------------------------
void ScaledAttachmentUpdateV2::onCapture( Player * /*oldOwner*/, Player * /*newOwner*/ )
{
	if( m_attachmentID == INVALID_ID )
		return;

	Object *att = TheGameLogic->findObjectByID( m_attachmentID );
	if( att && getObject()->getTeam() )
		att->setTeam( getObject()->getTeam() );
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void ScaledAttachmentUpdateV2::crc( Xfer *xfer )
{
	// extend base class
	UpdateModule::crc( xfer );
}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void ScaledAttachmentUpdateV2::xfer( Xfer *xfer )
{
	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpdateModule::xfer( xfer );

	xfer->xferObjectID( &m_attachmentID );
	xfer->xferBool( &m_spawned );
	xfer->xferReal( &m_scaleProgress );
	xfer->xferReal( &m_fadeProgress );
	xfer->xferBool( &m_fullyOut );
	xfer->xferUnsignedInt( &m_destroyFrame );
}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void ScaledAttachmentUpdateV2::loadPostProcess()
{
	// extend base class
	UpdateModule::loadPostProcess();

	// Re-derive the unsaved bits; the next update re-applies scale/opacity/hidden.
	m_localAttachPosValid = FALSE;
	m_appliedHidden = -1;
}
