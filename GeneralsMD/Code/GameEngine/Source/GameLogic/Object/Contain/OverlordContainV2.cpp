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

// FILE: OverlordContainV2.cpp ////////////////////////////////////////////////////////////////////
// Desc: Like OverlordContain, but every rider currently mounted on us -- not just the first one --
// is drawn, sinks experience, gets the stealth grant, follows body-damage-state changes, and
// changes hands with us on capture. See OverlordContainV2.h for the full rationale.
///////////////////////////////////////////////////////////////////////////////////////////////////

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine
#include "Common/Player.h"
#include "Common/Xfer.h"
#include "Common/ThingTemplate.h"
#include "Common/ThingFactory.h"
#include "GameClient/ControlBar.h"
#include "GameClient/Drawable.h"
#include "GameLogic/ExperienceTracker.h"
#include "GameLogic/Module/BodyModule.h"
#include "GameLogic/Module/OverlordContainV2.h"
#include "GameLogic/Object.h"
#include "GameLogic/PartitionManager.h"




// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
OverlordContainV2ModuleData::OverlordContainV2ModuleData()
{
	m_experienceSinkForRider = TRUE;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void OverlordContainV2ModuleData::buildFieldParse(MultiIniFieldParse& p)
{
  TransportContainModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
    { "PayloadTemplateName",  INI::parseAsciiStringVectorAppend, nullptr, offsetof(OverlordContainV2ModuleData, m_payloadTemplateNameData) },
    { "ExperienceSinkForRider",  INI::parseBool, nullptr, offsetof(OverlordContainV2ModuleData, m_experienceSinkForRider) },

		{ nullptr, nullptr, nullptr, 0 }
	};
  p.add(dataFieldParse);
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
OverlordContainV2::OverlordContainV2( Thing *thing, const ModuleData *moduleData ) :
								 TransportContain( thing, moduleData )
{
	m_redirectionActivated = FALSE;

  m_payloadCreated = FALSE;

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
OverlordContainV2::~OverlordContainV2()
{

}


void OverlordContainV2::onObjectCreated()
{
  OverlordContainV2::createPayload();
}

//-------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void OverlordContainV2::createPayload()
{
	OverlordContainV2ModuleData* self = (OverlordContainV2ModuleData*)getOverlordContainV2ModuleData();


  // Any number of different passengers, of any mix of KindOfs, can be loaded here at init time
	Object* object = getObject();
	ContainModuleInterface *contain = object->getContain();
	if( contain )
  {
		contain->enableLoadSounds( FALSE );

	  TemplateNameListV2 list = self->m_payloadTemplateNameData;
	  TemplateNameIteratorV2 iter = list.begin();
	  while ( iter != list.end() )
	  {
		  const ThingTemplate* temp = TheThingFactory->findTemplate( *iter );
		  if (temp)
		  {
			  Object* payload = TheThingFactory->newObject( temp, object->getTeam() );

			  if( contain->isValidContainerFor( payload, true ) )
			  {
				  contain->addToContain( payload );
			  }
			  else
			  {
				  DEBUG_CRASH( ( "OverlordContainV2::createPayload: %s is full, or not valid for the payload %s!", object->getName().str(), (*iter).str() ) );
			  }

      }

      ++iter;
    }

		contain->enableLoadSounds( TRUE );

  }

	m_payloadCreated = TRUE;

}

// ------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void OverlordContainV2::onBodyDamageStateChange( const DamageInfo* damageInfo,
																				BodyDamageType oldState,
																				BodyDamageType newState)  ///< state change callback
{
	// I can't use any convenience functions, as they will all get routed to whoever I may be
	// redirecting to. I want just my actual riders.
	// Oh, and I don't want this function trying to do death.  That is more complicated and will be
	// handled on my death.
	if( newState == BODY_RUBBLE )
		return;

	for( ContainedItemsList::const_iterator it = m_containList.begin(); it != m_containList.end(); ++it )
	{
		Object *myGuy = *it;
		if( myGuy )
			myGuy->getBodyModule()->setDamageState( newState );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ContainModuleInterface *OverlordContainV2::getRedirectedContain() const
{
	// Naturally, I can not use a redirectible convenience function
	// to answer if I am redirecting yet.

	// Only ever redirect when exactly one rider is aboard -- the moment a second one boards, we stop
	// pretending to be "him" and just behave like an ordinary multi-slot transport whose occupants
	// all stay visibly mounted.
	if( m_containListSize != 1 )
		return nullptr;

	if( !m_redirectionActivated )
		return nullptr;// Shut off early to allow death to happen without my bunker having
	// trouble finding me to say goodbye as messages get sucked up the pipe to him.

	Object *myGuy = m_containList.front();
	if( myGuy )
		return myGuy->getContain();

	return nullptr;// Or say no if they have no contain.
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------



//-------------------------------------------------------------------------------------------------
void OverlordContainV2::onDie( const DamageInfo *damageInfo )
{
	// Do you mean me the Overlord, or my behavior of passing stuff on to my one redirected rider?
	if( getRedirectedContain() == nullptr )
	{
		TransportContain::onDie( damageInfo );
		return;
	}
	//Everything is fine if I am empty, carrying more than one rider, or carrying a regular guy.  If I
	// have a redirected contain set up, then I need to handle the order of death explicitly, or
	// things will become confused when I stop redirecting in the middle of the process.  Or I will
	// get confused as my commands get sucked up the pipe.

	// So this is an extend that lets me control the order of death.

	deactivateRedirectedContain();
	Object *myGuy = m_containList.front();
	myGuy->kill();

	TransportContain::onDie( damageInfo );
}

//-------------------------------------------------------------------------------------------------
void OverlordContainV2::onDelete()
{
	// Do you mean me the Overlord, or my behavior of passing stuff on to my one redirected rider?
	if( getRedirectedContain() == nullptr )
	{
		TransportContain::onDelete();
		return;
	}

	// Without my throwing the redirect switch, teardown deletion will get confused and fire off a bunch of asserts
	getRedirectedContain()->removeAllContained();

	deactivateRedirectedContain();
	removeAllContained();

	TransportContain::onDelete();
}

// ------------------------------------------------------------------------------------------------
void OverlordContainV2::onCapture( Player *oldOwner, Player *newOwner )
{
	if( m_containListSize < 1 )
		return;

	// Every rider currently mounted on us needs to change hands with us. (In the classic
	// single-redirected-rider case he will, in turn, kick or keep whatever he's carrying himself,
	// exactly as OverlordContain always did.)
	for( ContainedItemsList::const_iterator it = m_containList.begin(); it != m_containList.end(); ++it )
	{
		Object *myGuy = *it;
		if( myGuy )
			myGuy->setTeam( newOwner->getDefaultTeam() );
	}
}

//-------------------------------------------------------------------------------------------------
Bool OverlordContainV2::isGarrisonable() const
{
	if( getRedirectedContain() == nullptr )
		return FALSE;

	return getRedirectedContain()->isGarrisonable();
}

//-------------------------------------------------------------------------------------------------
Bool OverlordContainV2::isKickOutOnCapture()
{
	if( getRedirectedContain() == nullptr )
		return FALSE;// Me the Overlord doesn't want to

	return getRedirectedContain()->isKickOutOnCapture();
}

//-------------------------------------------------------------------------------------------------
void OverlordContainV2::addToContainList( Object *obj )
{
	// Do you mean me the Overlord, or my behavior of passing stuff on to my one redirected rider?
	if( getRedirectedContain() == nullptr )
	{
		TransportContain::addToContainList( obj );
		return;
	}

	getRedirectedContain()->addToContainList( obj );
}

//-------------------------------------------------------------------------------------------------
void OverlordContainV2::addToContain( Object *obj )
{
	// Do you mean me the Overlord, or my behavior of passing stuff on to my one redirected rider?
	if( getRedirectedContain() == nullptr )
	{
		TransportContain::addToContain( obj );
		return;
	}

	getRedirectedContain()->addToContain( obj );

}

//-------------------------------------------------------------------------------------------------
/** Remove 'obj' from the m_containList of objects in this module.
	* This will trigger an onRemoving event for the object that this module
	* is a part of and an onRemovedFrom event for the object being removed */
//-------------------------------------------------------------------------------------------------
void OverlordContainV2::removeFromContain( Object *obj, Bool exposeStealthUnits )
{
	// Do you mean me the Overlord, or my behavior of passing stuff on to my one redirected rider?
	if( getRedirectedContain() == nullptr )
	{
		TransportContain::removeFromContain( obj, exposeStealthUnits );
		return;
	}

	getRedirectedContain()->removeFromContain( obj, exposeStealthUnits );

}

//-------------------------------------------------------------------------------------------------
/** Remove all contained objects from the contained list */
//-------------------------------------------------------------------------------------------------
void OverlordContainV2::removeAllContained( Bool exposeStealthUnits )
{
	// Do you mean me the Overlord, or my behavior of passing stuff on to my one redirected rider?
	if( getRedirectedContain() == nullptr )
	{
		TransportContain::removeAllContained( exposeStealthUnits );
		return;
	}

	const ContainedItemsList *fullList = getRedirectedContain()->getContainedItemsList();

	Object *obj;
	ContainedItemsList::const_iterator it;
	it = (*fullList).begin();
	while( it != (*fullList).end() )
	{
		obj = *it;
		it++;
		removeFromContain( obj, exposeStealthUnits );
	}
}

//-------------------------------------------------------------------------------------------------
/** Iterate the contained list and call the callback on each of the objects */
//-------------------------------------------------------------------------------------------------
void OverlordContainV2::iterateContained( ContainIterateFunc func, void *userData, Bool reverse )
{
	// Do you mean me the Overlord, or my behavior of passing stuff on to my one redirected rider?
	if( getRedirectedContain() == nullptr )
	{
		TransportContain::iterateContained( func, userData, reverse );
		return;
	}

	getRedirectedContain()->iterateContained( func, userData, reverse );
}

//-------------------------------------------------------------------------------------------------
void OverlordContainV2::onContaining( Object *obj, Bool wasSelected )
{
	// Do you mean me the Overlord, or my behavior of passing stuff on to my one redirected rider?
	if( getRedirectedContain() == nullptr )
	{
		TransportContain::onContaining( obj, wasSelected );


    if ( obj->isKindOf( KINDOF_PORTABLE_STRUCTURE ) )
    {
  		activateRedirectedContain();//Am now carrying something (only takes effect while I have exactly one rider)

			// And this contain style explicitly sucks XP from our little friend.
			if( getOverlordContainV2ModuleData()->m_experienceSinkForRider  &&  obj->getExperienceTracker() )
				obj->getExperienceTracker()->setExperienceSink(getObject()->getID());


      if ( obj->isKindOf( KINDOF_PORTABLE_STRUCTURE ) && getObject()->testStatus( OBJECT_STATUS_STEALTHED ) )
      {
        StealthUpdate *myStealth =  obj->getStealth();
        if ( myStealth )
        {
          myStealth->receiveGrant( true );
          // note to anyone... once stealth is granted to this gattlingcannon ( or such )
          // let its own stealthupdate govern the allowedtostealth cases
          // a portable structure never gets removed, so...
        }
      }




    }


    return;
	}

	OpenContain::onContaining( obj, wasSelected );

	getRedirectedContain()->onContaining( obj, wasSelected );

}

//-------------------------------------------------------------------------------------------------
void OverlordContainV2::killAllContained()
{
	// This is a game call meant to clear actual passengers.  We don't want it to kill our turret.  That'd be weird.
	if( getRedirectedContain() )
	{
		getRedirectedContain()->killAllContained();
	}
}

//-------------------------------------------------------------------------------------------------
void OverlordContainV2::onRemoving( Object *obj )
{
	// Do you mean me the Overlord, or my behavior of passing stuff on to my one redirected rider?
	if( getRedirectedContain() == nullptr )
	{
		TransportContain::onRemoving( obj );
		return;
	}

	OpenContain::onRemoving(obj);

	getRedirectedContain()->onRemoving( obj );

}

//-------------------------------------------------------------------------------------------------
Bool OverlordContainV2::isValidContainerFor(const Object* obj, Bool checkCapacity) const
{
	// Do you mean me the Overlord, or my behavior of passing stuff on to my one redirected rider?
	if( getRedirectedContain() == nullptr )
		return TransportContain::isValidContainerFor( obj, checkCapacity );

	return getRedirectedContain()->isValidContainerFor( obj, checkCapacity );
}

//-------------------------------------------------------------------------------------------------
UnsignedInt OverlordContainV2::getContainCount() const
{
	ContainModuleInterface* redir = getRedirectedContain();

	// Do you mean me the Overlord, or my behavior of passing stuff on to my one redirected rider?
	if( redir == nullptr )
		return TransportContain::getContainCount();

	return redir->getContainCount();
}

//-------------------------------------------------------------------------------------------------
Bool OverlordContainV2::getContainerPipsToShow(Int& numTotal, Int& numFull)
{
	// Do you mean me the Overlord, or my behavior of passing stuff on to my one redirected rider?
	if( getRedirectedContain() == nullptr )
	{
		numTotal = 0;
		numFull = 0;
		return false;
	}
	else
	{
		return getRedirectedContain()->getContainerPipsToShow(numTotal, numFull);
	}
}

//-------------------------------------------------------------------------------------------------
Int OverlordContainV2::getContainMax() const
{
	// Do you mean me the Overlord, or my behavior of passing stuff on to my one redirected rider?
	if( getRedirectedContain() == nullptr )
		return TransportContain::getContainMax();

	return getRedirectedContain()->getContainMax();
}

//-------------------------------------------------------------------------------------------------
const ContainedItemsList* OverlordContainV2::getContainedItemsList() const
{
	// Do you mean me the Overlord, or my behavior of passing stuff on to my one redirected rider?
	if( getRedirectedContain() == nullptr )
		return TransportContain::getContainedItemsList();

	return getRedirectedContain()->getContainedItemsList();
}

//-------------------------------------------------------------------------------------------------
Bool OverlordContainV2::isEnclosingContainerFor( const Object *obj ) const
{
	// All of this redirection stuff makes it so that while I am normally a transport
	// for Overlord subObjects, once I have a passenger, _I_ become a transport of their type.
	// So, the answer to this question depends on if it is my passenger asking, or theirs.
	// As always, I can't use convenience functions that get redirected on a ? like this.
	//
	// Unlike single-rider OverlordContain (which only ever exposes m_containList.front()), ANY of
	// our directly-mounted riders count as "visibly exposed" here -- that's what lets each one's own
	// W3DDependencyModelDraw pick up its AttachToBoneInContainer bone instead of being treated as
	// sealed inside a closed transport.
	for( ContainedItemsList::const_iterator it = m_containList.begin(); it != m_containList.end(); ++it )
	{
		if( *it == obj )
			return FALSE;
	}

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
Bool OverlordContainV2::isDisplayedOnControlBar() const
{
	// Do you mean me the Overlord, or my behavior of passing stuff on to my one redirected rider?
	if( getRedirectedContain() == nullptr )
		return FALSE;//No need to call up inheritance, this is a module based question, and I say no.

	return getRedirectedContain()->isDisplayedOnControlBar();
}

//-------------------------------------------------------------------------------------------------
const Object *OverlordContainV2::friend_getRider() const
{
// The draw order dependency bug for riders means that our draw module needs to cheat to get around
	// it.  This keeps returning just the first rider, for the handful of subsystems (disabled-status
	// propagation, stealth imitation, etc.) that still only expect one "primary" passenger; the draw
	// modules themselves use friend_getVisibleRiders() below to get all of them.

 	if( m_containListSize > 0 )
 		return m_containList.front();


	return nullptr;
}

//-------------------------------------------------------------------------------------------------
void OverlordContainV2::friend_getVisibleRiders( std::vector<const Object*>& riders ) const
{
	// Every object directly mounted on us should be drawn -- this is the whole point of V2.
	// (When we're in the classic single-redirected-rider case, m_containList still only ever has
	// that one entry in it -- whatever HE is carrying lives in HIS OWN contain list, not ours -- so
	// this is correct and safe in both cases.)
	for( ContainedItemsList::const_iterator it = m_containList.begin(); it != m_containList.end(); ++it )
	{
		if( *it )
			riders.push_back( *it );
	}
}

//-------------------------------------------------------------------------------------------------
void OverlordContainV2::activateRedirectedContain()
{
	m_redirectionActivated = TRUE;
}

//-------------------------------------------------------------------------------------------------
void OverlordContainV2::deactivateRedirectedContain()
{
	m_redirectionActivated = FALSE;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// if my object gets selected, then my visible passengers should, too
// this gets called from
void OverlordContainV2::clientVisibleContainedFlashAsSelected()
{
	// THIS OVERRIDES GRAHAMS NASTY OVERRIDE THING
	// SO WE CAN FLASH THE PORTABLE BUNKER INSTEAD OF ITS OCCUPANTS
	const ContainedItemsList* items = TransportContain::getContainedItemsList();

	if( items )
	{
		ContainedItemsList::const_iterator it;
		it = items->begin();

		while( it != items->end() )
		{
			Object *object = *it;
			if ( object && object->isKindOf( KINDOF_PORTABLE_STRUCTURE ) )
			{
				Drawable *draw = object->getDrawable();
				if ( draw )
				{
					draw->flashAsSelected(); //WOW!
				}
			}

			++it;
		}
	}

}


Bool OverlordContainV2::isPassengerAllowedToFire( ObjectID id ) const
{
	Object *passenger = TheGameLogic->findObjectByID(id);

	if(passenger != nullptr)
	{
		//only allow infantry, and turrets and such.  no vehicles.
		if(passenger->isKindOf(KINDOF_INFANTRY) == FALSE && passenger->isKindOf(KINDOF_PORTABLE_STRUCTURE) == FALSE)
			return FALSE;
	}


  if ( getObject() && getObject()->getContainedBy() ) // nested containment voids firing, always
    return FALSE;

  return TransportContain::isPassengerAllowedToFire();
}



// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void OverlordContainV2::crc( Xfer *xfer )
{

	// extend base class
	TransportContain::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void OverlordContainV2::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	TransportContain::xfer( xfer );

	// redirection activated
	xfer->xferBool( &m_redirectionActivated );

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void OverlordContainV2::loadPostProcess()
{

	// extend base class
	TransportContain::loadPostProcess();

}
