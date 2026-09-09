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

// FILE: OverlordContainV2.h //////////////////////////////////////////////////////////////////////
// Desc: Like OverlordContain, but supports more than one simultaneously-visible rider.
//
// OverlordContain's "redirect transport queries to my one rider" trick (so a single rider that is
// itself a container -- e.g. a turret that garrisons infantry -- transparently becomes the
// Overlord's own inventory) only makes sense when there is exactly one rider. This module keeps
// that trick for the classic one-rider case, but the instant a second rider boards, redirection is
// switched off and this container behaves like a normal multi-slot TransportContain whose
// occupants all stay visibly mounted (each drawn via its own W3DDependencyModelDraw
// AttachToBoneInContainer bone), all get ExperienceSinkForRider / stealth-grant / damage-state /
// on-capture treatment, and are governed by the usual Slots / AllowInsideKindOf rules -- so riders
// can be any mix of KindOfs, not just one specific unit type.
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "GameLogic/Module/TransportContain.h"
#include "GameLogic/GameLogic.h"


typedef std::vector<AsciiString> TemplateNameListV2;
typedef std::vector<AsciiString>::const_iterator TemplateNameIteratorV2;

//-------------------------------------------------------------------------------------------------
class OverlordContainV2ModuleData : public TransportContainModuleData
{
public:

	OverlordContainV2ModuleData();

	TemplateNameListV2 m_payloadTemplateNameData;
	Bool m_experienceSinkForRider;


	static void buildFieldParse(MultiIniFieldParse& p);
	static void parseInitialPayload( INI* ini, void *instance, void *store, const void* /*userData*/ );
};

//-------------------------------------------------------------------------------------------------
class OverlordContainV2 : public TransportContain
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( OverlordContainV2, "OverlordContainV2" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( OverlordContainV2, OverlordContainV2ModuleData )

	virtual void onBodyDamageStateChange( const DamageInfo* damageInfo,
																				BodyDamageType oldState,
																				BodyDamageType newState) override;  ///< state change callback
public:

	OverlordContainV2( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	virtual OpenContain *asOpenContain() override { return this; }  ///< treat as open container
	virtual Bool isGarrisonable() const override;	///< can this unit be Garrisoned? (ick)
  virtual Bool isBustable() const override { return false;};	///< can this container get busted by bunkerbuster? (ick)
	virtual Bool isHealContain() const override { return false; } ///< true when container only contains units while healing (not a transport!)
	virtual Bool isTunnelContain() const override { return FALSE; }
	virtual Bool isImmuneToClearBuildingAttacks() const override { return true; }
  virtual Bool isSpecialOverlordStyleContainer() const override {return TRUE;}
	virtual Bool isPassengerAllowedToFire( ObjectID id = INVALID_ID ) const override;	///< Hey, can I shoot out of this container?


	virtual void onDie( const DamageInfo *damageInfo ) override;  ///< the die callback
	virtual void onDelete() override;	///< Last possible moment cleanup
	virtual void onCapture( Player *oldOwner, Player *newOwner ) override; // Every mounted rider changes hands with us; a lone redirected rider still cascades his own capture too
	virtual void onObjectCreated() override;

	// Contain stuff we need to override to redirect on a condition
	virtual void onContaining( Object *obj, Bool wasSelected ) override;		///< object now contains 'obj'
	virtual void onRemoving( Object *obj ) override;			///< object no longer contains 'obj'

	virtual Bool isValidContainerFor(const Object* obj, Bool checkCapacity) const override;
	virtual void addToContain( Object *obj ) override;				///< add 'obj' to contain list
	virtual void addToContainList( Object *obj ) override;		///< The part of AddToContain that inheritors can override (Can't do whole thing because of all the private stuff involved)
	virtual void removeFromContain( Object *obj, Bool exposeStealthUnits = FALSE ) override;	///< remove 'obj' from contain list
	virtual void removeAllContained( Bool exposeStealthUnits = FALSE ) override;				///< remove all objects on contain list
	virtual Bool isEnclosingContainerFor( const Object *obj ) const override;	///< Does this type of Contain Visibly enclose its contents?
	virtual Bool isDisplayedOnControlBar() const override;///< Does this container display its contents on the ControlBar?
	virtual Bool isKickOutOnCapture() override;// The bunker may want to, but we certainly don't
	virtual void killAllContained() override;				///< kill all objects inside.  For us, this does not mean our rider(s)

	// contain list access
	virtual void iterateContained( ContainIterateFunc func, void *userData, Bool reverse ) override;
	virtual UnsignedInt getContainCount() const override;
	virtual Int getContainMax() const override;
	virtual const ContainedItemsList* getContainedItemsList() const override;

	// Friend for our Draw module only.
	virtual const Object *friend_getRider() const override; ///< Damn.  The draw order dependency bug for riders means that our draw module needs to cheat to get around it.
	virtual void friend_getVisibleRiders( std::vector<const Object*>& riders ) const override; ///< Same cheat, but every rider that should be drawn, not just the first.

	///< if my object gets selected, then my visible passengers should, too
	///< this gets called from
	virtual void clientVisibleContainedFlashAsSelected() override;

	virtual Bool getContainerPipsToShow(Int& numTotal, Int& numFull) override;
	virtual void createPayload() override;

private:
	/**< While exactly one rider is aboard and that rider is itself a container, we redirect
	transport queries to him (so, e.g., troops he garrisons show up as if riding in us). The instant
	a second rider boards, this returns null unconditionally and we behave like a plain multi-slot
	TransportContain instead. If this returns null, we are either empty, carrying more than one
	rider, or carrying a single non-container rider.
	*/
	ContainModuleInterface *getRedirectedContain() const; ///< And this gets what we are redirecting to.
	void activateRedirectedContain();///< I need to shut this off since I can talk directly to my bunker, but he can never directly see me
	void deactivateRedirectedContain();
  void parseInitialPayload( INI* ini, void *instance, void *store, const void* /*userData*/ );

	Bool m_redirectionActivated;

};
