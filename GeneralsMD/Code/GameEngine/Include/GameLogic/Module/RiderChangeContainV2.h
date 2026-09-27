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

// FILE: RiderChangeContainV2.h ///////////////////////////////////////////////////////////////////
// GeneralsMod @feature Dimitar 26/09/2026
//
// Desc: RiderChangeContain with an unlimited rider list instead of the fixed Rider1..Rider8 slots.
//       Behaves exactly like vanilla RiderChangeContain (swap rider on entry, transfer veterancy,
//       scuttle the vehicle when the rider leaves), but every rider row is declared with the same
//       repeatable field, and each statement appends one more row:
//
//   Behavior = RiderChangeContainV2 ModuleTag_Riders
//     Rider = GLAInfantryRebel          RIDER1  WEAPON_RIDER1  STATUS_RIDER1  CommandSet_A  SET_NORMAL
//     Rider = GLAInfantryRPGTrooper     RIDER2  WEAPON_RIDER2  STATUS_RIDER2  CommandSet_B  SET_NORMAL
//     ...
//     Rider = SomeOtherInfantry         RIDER32 WEAPON_RIDER32 STATUS_RIDER32 CommandSet_X  SET_NORMAL
//     ScuttleDelay  = 1000
//     ScuttleStatus = TOPPLED
//     ScuttleOnDeath = Yes   ; No = KILL_PILOT kills only the rider; the vehicle stays alive, empty,
//                            ;      mobile and with the same owner (veterancy is lost) until a valid
//                            ;      rider enters again.
//     NoScuttleTurnNeutral = No ; Only used when ScuttleOnDeath = No. Yes = after KILL_PILOT the empty
//                            ;      vehicle turns neutral and immobile (veterancy still lost). The
//                            ;      next valid rider to board takes ownership of it and it can move again.
//                            ;      (Infantry of any player may board it: needs AllowNeutralInside = Yes,
//                            ;      which is the default.)
//                            ;      A rider leaving voluntarily still scuttles, as before.
//     ; ...plus every normal TransportContain field (Slots, ExitBone, etc.)
//   End
//
//   Row format is identical to vanilla RiderN: template, model condition, weapon set flag,
//   object status, command set override, locomotor set.
//
//   The number of rows is unlimited. The number of DISTINCT flags is not: the engine now has
//   RIDER1..RIDER32 model conditions, WEAPON_RIDER1..WEAPON_RIDER32 weapon set flags and
//   STATUS_RIDER1..STATUS_RIDER32 object statuses. Rows may share flags.
//
//   Note: vanilla code gives STATUS_RIDER8 a hardcoded meaning (Chinook / AIStates use it as a
//   "busy" marker), so avoid STATUS_RIDER8 on rider rows, exactly as with vanilla RiderChangeContain.
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "GameLogic/Module/TransportContain.h"
#include "GameLogic/Module/RiderChangeContain.h"	// RiderInfo + RiderChangeContainModuleData::parseRiderInfo

//-------------------------------------------------------------------------------------------------
class RiderChangeContainV2ModuleData : public TransportContainModuleData
{
public:

	std::vector<RiderInfo>	m_riders;
	UnsignedInt							m_scuttleFrames;
	ModelConditionFlagType	m_scuttleState;
	Bool										m_scuttleOnDeath;		///< FALSE: KILL_PILOT only kills the rider; the vehicle survives empty and mobile
	Bool										m_noScuttleTurnNeutral;	///< with ScuttleOnDeath = No: the survivor turns neutral + immobile until re-boarded

	RiderChangeContainV2ModuleData();

	static void buildFieldParse(MultiIniFieldParse& p);
	static void parseRider( INI* ini, void *instance, void *store, const void* /*userData*/ );

};

//-------------------------------------------------------------------------------------------------
class RiderChangeContainV2 : public TransportContain
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( RiderChangeContainV2, "RiderChangeContainV2" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( RiderChangeContainV2, RiderChangeContainV2ModuleData )

public:

	RiderChangeContainV2( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	virtual Bool isValidContainerFor( const Object* obj, Bool checkCapacity) const override;

	virtual void onContaining( Object *obj, Bool wasSelected ) override;		///< object now contains 'obj'
	virtual void onRemoving( Object *obj ) override;			///< object no longer contains 'obj'
	virtual UpdateSleepTime update() override;							///< called once per frame

	virtual Bool isRiderChangeContain() const override { return TRUE; }
	virtual const Object *friend_getRider() const override;

	virtual Int getContainMax() const override;

	virtual Bool isExitBusy() const override;
	virtual void unreserveDoorForExit( ExitDoorType exitDoor ) override;
	virtual Bool isDisplayedOnControlBar() const override {return TRUE;}

	virtual Bool getContainerPipsToShow( Int& numTotal, Int& numFull ) override;

	virtual Bool handleKillPilot( Object *damager ) override;	///< DAMAGE_KILLPILOT hook (see ScuttleOnDeath)
	virtual void onCapture( Player *oldOwner, Player *newOwner ) override;

private:

	/// Index into m_riders of the row matching this rider's template, or -1 if none.
	Int findRiderIndex( const Object* rider ) const;

	UnsignedInt m_scuttledOnFrame;
	Bool m_containing; //doesn't require xfer.
	Bool m_killingPilot; //TRUE only during handleKillPilot()'s evacuate call; doesn't require xfer.
	Bool m_adoptingRider; //TRUE only while a neutral vehicle defects to its new rider's team; doesn't require xfer.
	Bool m_neutralAfterPilotKill; //NoScuttleTurnNeutral: vehicle is neutral + immobile, waiting for a new rider.

};
