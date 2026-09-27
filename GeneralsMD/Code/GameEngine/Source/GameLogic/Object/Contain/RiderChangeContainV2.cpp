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

// FILE: RiderChangeContainV2.cpp /////////////////////////////////////////////////////////////////
// GeneralsMod @feature Dimitar 26/09/2026
// Desc: RiderChangeContain with an unlimited, repeatable "Rider =" list. See header for usage.
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/Player.h"
#include "Common/PlayerList.h"
#include "Common/ThingTemplate.h"
#include "Common/ThingFactory.h"
#include "Common/Xfer.h"

#include "GameClient/ControlBar.h"
#include "GameClient/Drawable.h"
#include "GameClient/InGameUI.h"

#include "GameLogic/AI.h"
#include "GameLogic/ExperienceTracker.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"
#include "GameLogic/Locomotor.h"

#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/StealthUpdate.h"
#include "GameLogic/Module/RiderChangeContainV2.h"

// ------------------------------------------------------------------------------------------------
RiderChangeContainV2ModuleData::RiderChangeContainV2ModuleData()
{
	m_scuttleFrames = 0;
	m_scuttleState = MODELCONDITION_TOPPLED;
	m_scuttleOnDeath = TRUE;
	m_noScuttleTurnNeutral = FALSE;
}

// ------------------------------------------------------------------------------------------------
/** Each "Rider = ..." statement appends one row. Row syntax is identical to vanilla RiderN, so the
	* vanilla row parser is reused as-is. */
// ------------------------------------------------------------------------------------------------
void RiderChangeContainV2ModuleData::parseRider( INI* ini, void *instance, void *store, const void* userData )
{
	std::vector<RiderInfo>* riders = (std::vector<RiderInfo>*)store;

	RiderInfo info;
	RiderChangeContainModuleData::parseRiderInfo( ini, instance, &info, userData );
	riders->push_back( info );
}

// ------------------------------------------------------------------------------------------------
void RiderChangeContainV2ModuleData::buildFieldParse(MultiIniFieldParse& p)
{
	TransportContainModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "Rider",					parseRider,											nullptr, offsetof( RiderChangeContainV2ModuleData, m_riders ) },
		{ "ScuttleDelay",		INI::parseDurationUnsignedInt,	nullptr, offsetof( RiderChangeContainV2ModuleData, m_scuttleFrames ) },
		{ "ScuttleStatus",	INI::parseIndexList,						ModelConditionFlags::getBitNames(), offsetof( RiderChangeContainV2ModuleData, m_scuttleState ) },
		{ "ScuttleOnDeath",	INI::parseBool,									nullptr, offsetof( RiderChangeContainV2ModuleData, m_scuttleOnDeath ) },
		{ "NoScuttleTurnNeutral",	INI::parseBool,						nullptr, offsetof( RiderChangeContainV2ModuleData, m_noScuttleTurnNeutral ) },
		{ nullptr, nullptr, nullptr, 0 }
	};
	p.add(dataFieldParse);
}

//-------------------------------------------------------------------------------------------------
RiderChangeContainV2::RiderChangeContainV2( Thing *thing, const ModuleData *moduleData ) :
								 TransportContain( thing, moduleData )
{
	m_containing = FALSE;
	m_scuttledOnFrame = 0;
	m_killingPilot = FALSE;
	m_adoptingRider = FALSE;
	m_neutralAfterPilotKill = FALSE;
}

//-------------------------------------------------------------------------------------------------
RiderChangeContainV2::~RiderChangeContainV2()
{
}

//-------------------------------------------------------------------------------------------------
Int RiderChangeContainV2::findRiderIndex( const Object* rider ) const
{
	if( !rider )
		return -1;

	const RiderChangeContainV2ModuleData *data = getRiderChangeContainV2ModuleData();
	const Int count = (Int)data->m_riders.size();
	for( Int i = 0; i < count; ++i )
	{
		const ThingTemplate *thing = TheThingFactory->findTemplate( data->m_riders[ i ].m_templateName );
		if( thing && thing->isEquivalentTo( rider->getTemplate() ) )
			return i;
	}
	return -1;
}

//-------------------------------------------------------------------------------------------------
Int RiderChangeContainV2::getContainMax() const
{
	if (getRiderChangeContainV2ModuleData())
		return getRiderChangeContainV2ModuleData()->m_slotCapacity;

	return 0;
}

//-------------------------------------------------------------------------------------------------
Bool RiderChangeContainV2::isValidContainerFor(const Object* rider, Bool checkCapacity) const
{
	//Don't check capacity because our rider will kick the other rider out!
	if( !TransportContain::isValidContainerFor( rider, FALSE ) )
		return FALSE;

	//Scuttled... too late!
	if( m_scuttledOnFrame != 0 )
		return FALSE;

	//Only infantry listed in our rider rows may enter.
	return findRiderIndex( rider ) >= 0;
}

//-------------------------------------------------------------------------------------------------
void RiderChangeContainV2::onContaining( Object *rider, Bool wasSelected )
{
	Object *obj = getObject();
	m_containing = TRUE;

	//NoScuttleTurnNeutral: a neutral, parked vehicle is taken over by whoever boards it.
	if( m_neutralAfterPilotKill )
	{
		m_neutralAfterPilotKill = FALSE;
		obj->clearStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_IMMOBILE ) );

		if( rider->getTeam() && rider->getControllingPlayer() != obj->getControllingPlayer() )
		{
			//Same takeover calls vanilla uses when infantry re-crews an unmanned vehicle. m_adoptingRider
			//stops onCapture() from ejecting the rider who is boarding right now.
			m_adoptingRider = TRUE;
			obj->setCaptured( true );
			obj->defect( rider->getTeam(), 0 );
			m_adoptingRider = FALSE;
		}
	}

	//Remove our existing rider
	if( m_payloadCreated )
	{
		obj->getAI()->aiEvacuateInstantly( TRUE, CMD_FROM_AI );
	}

	//If the rider is currently selected, transfer selection to the container.
	Drawable *containDraw = obj->getDrawable();
	if( containDraw && wasSelected && !containDraw->isSelected() )
	{
		GameMessage *teamMsg = TheMessageStream->appendMessage( GameMessage::MSG_CREATE_SELECTED_GROUP );
		teamMsg->appendBooleanArgument( FALSE );// not creating new team so pass false
		teamMsg->appendObjectIDArgument( obj->getID() );
		TheInGameUI->selectDrawable( containDraw );
		TheInGameUI->setDisplayedMaxWarning( FALSE );
	}

	const Int index = findRiderIndex( rider );
	if( index >= 0 )
	{
		const RiderInfo& info = getRiderChangeContainV2ModuleData()->m_riders[ index ];

		obj->setModelConditionState( info.m_modelConditionFlagType );
		obj->setWeaponSetFlag( info.m_weaponSetFlag );
		obj->setStatus( MAKE_OBJECT_STATUS_MASK( info.m_objectStatusType ) );

		obj->setCommandSetStringOverride( info.m_commandSet );
		TheControlBar->markUIDirty();	// Refresh the UI in case we are selected

		AIUpdateInterface* ai = obj->getAI();
		if( ai )
		{
			ai->chooseLocomotorSet( info.m_locomotorSetType );
		}

		if( obj->getStatusBits().test( OBJECT_STATUS_STEALTHED ) )
		{
			StealthUpdate* stealth = obj->getStealth();
			if( stealth )
			{
				stealth->markAsDetected();
			}
		}

		//Transfer experience from the rider to the vehicle.
		ExperienceTracker *riderTracker = rider->getExperienceTracker();
		ExperienceTracker *bikeTracker = obj->getExperienceTracker();
#if !RETAIL_COMPATIBLE_CRC
		// Same fix as vanilla RiderChangeContain: untrainable riders must not rank up via the vehicle.
		bikeTracker->setTrainable(riderTracker->isTrainable());
#endif
		bikeTracker->setVeterancyLevel( riderTracker->getVeterancyLevel(), FALSE );
		riderTracker->setExperienceAndLevel( 0, FALSE );
	}

	//Extend base class
	TransportContain::onContaining( rider, wasSelected );

	m_containing = FALSE;
}

//-------------------------------------------------------------------------------------------------
void RiderChangeContainV2::onRemoving( Object *rider )
{
	Object *bike = getObject();

	//If the vehicle dies, the rider dies too.
	if( bike->isEffectivelyDead() )
	{
		TheGameLogic->destroyObject( rider );
		return;
	}

	if( m_payloadCreated )
	{
		//Extend base class
		TransportContain::onRemoving( rider );
	}

	const RiderChangeContainV2ModuleData *data = getRiderChangeContainV2ModuleData();
	const Int index = findRiderIndex( rider );
	if( index >= 0 )
	{
		const RiderInfo& info = data->m_riders[ index ];

		bike->clearModelConditionFlags( MAKE_MODELCONDITION_MASK2( info.m_modelConditionFlagType, MODELCONDITION_DOOR_1_CLOSING ) );
		bike->clearWeaponSetFlag( info.m_weaponSetFlag );
		bike->clearStatus( MAKE_OBJECT_STATUS_MASK( info.m_objectStatusType ) );

		//A null player means game teardown -- don't transfer experience then (see vanilla comment).
		if( rider->getControllingPlayer() != nullptr )
		{
			ExperienceTracker *riderTracker = rider->getExperienceTracker();
			ExperienceTracker *bikeTracker = bike->getExperienceTracker();
			bikeTracker->resetTrainable();
			riderTracker->setVeterancyLevel( bikeTracker->getVeterancyLevel(), FALSE );
			bikeTracker->setExperienceAndLevel( 0, FALSE );
		}
	}

	//Rider was killed by KILL_PILOT and ScuttleOnDeath = No: keep the vehicle alive and empty (still
	//mobile, same owner). Veterancy went to the rider above and dies with them.
	if( m_killingPilot )
	{
		bike->setCommandSetStringOverride( AsciiString::TheEmptyString );	// back to the vehicle's own CommandSet
		TheControlBar->markUIDirty();
		return;
	}

	//If we're not replacing the rider, transfer selection to the rider getting off and scuttle.
	if( !m_containing )
	{
		Drawable *containDraw = bike->getDrawable();
		Drawable *riderDraw = rider->getDrawable();
		if( containDraw && riderDraw )
		{
			if( bike->isLocallyControlled() && containDraw->isSelected() )
			{
				GameMessage *teamMsg = TheMessageStream->appendMessage( GameMessage::MSG_CREATE_SELECTED_GROUP );
				teamMsg->appendBooleanArgument( FALSE );// not creating new team so pass false
				teamMsg->appendObjectIDArgument( rider->getID() );
				TheInGameUI->selectDrawable( riderDraw );
				TheInGameUI->setDisplayedMaxWarning( FALSE );

				teamMsg = TheMessageStream->appendMessage( GameMessage::MSG_REMOVE_FROM_SELECTED_GROUP );
				teamMsg->appendObjectIDArgument( bike->getID() );
				TheInGameUI->deselectDrawable( containDraw );
			}

			//Scuttle the vehicle so nobody else can use it.
			m_scuttledOnFrame = TheGameLogic->getFrame();
			bike->setStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_UNSELECTABLE ) );
			bike->setModelConditionState( data->m_scuttleState );
			if( !bike->getAI()->isMoving() )
			{
				bike->setStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_IMMOBILE ) );
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------
UpdateSleepTime RiderChangeContainV2::update()
{
	if( m_scuttledOnFrame != 0 )
	{
		const RiderChangeContainV2ModuleData *data = getRiderChangeContainV2ModuleData();
		UnsignedInt now = TheGameLogic->getFrame();
		if( m_scuttledOnFrame + data->m_scuttleFrames <= now )
		{
			//Tipped over via the scuttle animation -- now sink it without real destruction.
			getObject()->kill( DAMAGE_UNRESISTABLE, DEATH_TOPPLED );
		}
	}
	return TransportContain::update();
}

// ------------------------------------------------------------------------------------------------
/** Called from ActiveBody for DAMAGE_KILLPILOT. With ScuttleOnDeath = Yes (default) we return FALSE and
	* the vanilla combat-bike logic runs (moving: vehicle dies; stopped: rider killed, vehicle scuttles).
	* With ScuttleOnDeath = No, only the rider dies -- whether or not the vehicle was moving -- and the
	* vehicle is left empty (still mobile, same owner, veterancy lost) until a valid rider enters again. */
// ------------------------------------------------------------------------------------------------
Bool RiderChangeContainV2::handleKillPilot( Object *damager )
{
	if( getRiderChangeContainV2ModuleData()->m_scuttleOnDeath )
		return FALSE;

	//Already empty (e.g. rider left while disabled): nothing to kill, and don't blow up the vehicle.
	if( getContainedItemsList()->empty() )
		return TRUE;

	Object *obj = getObject();
	Object *rider = *(getContainedItemsList()->begin());

	AIUpdateInterface *ai = obj->getAI();
	if( ai )
	{
		ai->aiIdle( CMD_FROM_AI );	// stop driving

		m_killingPilot = TRUE;		// onRemoving() skips the scuttle while this is set
		ai->aiEvacuateInstantly( TRUE, CMD_FROM_AI );
		m_killingPilot = FALSE;
	}

	if( damager )
		damager->scoreTheKill( rider );
	rider->kill();

	//Optionally abandon the vehicle: neutral and parked until someone boards it.
	if( getRiderChangeContainV2ModuleData()->m_noScuttleTurnNeutral )
	{
		TheGameLogic->deselectObject( obj, PLAYERMASK_ALL, TRUE );
		obj->setTeam( ThePlayerList->getNeutralPlayer()->getDefaultTeam() );
		obj->setStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_IMMOBILE ) );
		m_neutralAfterPilotKill = TRUE;
	}

	return TRUE;
}

// ------------------------------------------------------------------------------------------------
void RiderChangeContainV2::onCapture( Player *oldOwner, Player *newOwner )
{
	//Taking ownership from the rider who is boarding right now -- don't eject them.
	if( m_adoptingRider )
		return;

	TransportContain::onCapture( oldOwner, newOwner );
}

// ------------------------------------------------------------------------------------------------
void RiderChangeContainV2::unreserveDoorForExit( ExitDoorType exitDoor )
{
	/* nothing -- same as vanilla RiderChangeContain */
}

// ------------------------------------------------------------------------------------------------
Bool RiderChangeContainV2::isExitBusy() const
{
	return FALSE; // same as vanilla RiderChangeContain
}

//-------------------------------------------------------------------------------------------------
Bool RiderChangeContainV2::getContainerPipsToShow(Int& numTotal, Int& numFull)
{
	//No pips: always one rider unless dead.
	numTotal = 0;
	numFull = 0;
	return false;
}

//-------------------------------------------------------------------------------------------------
const Object *RiderChangeContainV2::friend_getRider() const
{
	if( m_containListSize > 0 )
		return m_containList.front();

	return nullptr;
}

// ------------------------------------------------------------------------------------------------
void RiderChangeContainV2::crc( Xfer *xfer )
{
	TransportContain::crc( xfer );
}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version. Unlike vanilla RiderChangeContain, also persists m_scuttledOnFrame so a
	*    vehicle saved mid-scuttle still finishes scuttling (and stays un-enterable) after load, and
	*    m_neutralAfterPilotKill (NoScuttleTurnNeutral). */
// ------------------------------------------------------------------------------------------------
void RiderChangeContainV2::xfer( Xfer *xfer )
{
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	TransportContain::xfer( xfer );

	xfer->xferUnsignedInt( &m_scuttledOnFrame );

	xfer->xferBool( &m_neutralAfterPilotKill );
}

// ------------------------------------------------------------------------------------------------
void RiderChangeContainV2::loadPostProcess()
{
	TransportContain::loadPostProcess();
}
