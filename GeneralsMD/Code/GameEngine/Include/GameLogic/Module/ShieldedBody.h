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

// FILE: ShieldedBody.h ////////////////////////////////////////////////////////////////////////
//
//	GeneralsMod @feature Dimitar 13/09/2026
//	Just like ActiveBody, but lets a companion module (e.g. ShieldGeneratorUpdateV2) temporarily
//	freeze the BodyDamageType (PRISTINE/DAMAGED/REALLYDAMAGED/RUBBLE) that this object reports to
//	the rest of the engine, independent of whatever its real current/max health is doing at that
//	moment.
//
//	Why this exists: BodyDamageType is a pure function of currentHealth/maxHealth (see
//	ActiveBody::calcDamageState() in ActiveBody.cpp), and two separate engine systems read it
//	directly every time it changes: Locomotor::getMaxSpeedForCondition() (and turn rate/acceleration/
//	lift) call getDamageState() every frame to decide movement penalties, and
//	ActiveBody::evaluateVisualCondition() reports it to the Drawable to drive the DAMAGED/
//	REALLYDAMAGED model conditions. Any module that temporarily changes health or max health --
//	such as a "shield" that grants extra hit points for a while -- can therefore cause a unit to
//	visibly (and mechanically, via movement speed) snap in and out of REALLYDAMAGED purely as an
//	artifact of the temporary health change, not because anything really changed about how hurt it
//	is. Freezing the reported state for the duration of that effect avoids the snap, without
//	touching the real health/damage-state math at all -- death, rubble collapse, and all other
//	real consequences of actual health changes are completely unaffected; only what gets reported
//	to Locomotor and the Drawable is held fixed.
//
//	This module does not add any usable capability by itself (freezing a state to itself is not
//	something the object initiates) -- another module reaches it via Object::getBodyModule() and a
//	getModuleNameKey() check against NAMEKEY("ShieldedBody") (findModule() itself is protected on
//	Object, so this is the public-surface equivalent of Object::findUpdateModule() /
//	findDamageModule()'s "match by name key, then cast" convention), then calls freezeDamageState()
//	/ unfreezeDamageState() directly -- see ShieldGeneratorUpdateV2.cpp for the reference caller. An
//	object not carrying ShieldedBody as its Body (i.e. still plain ActiveBody, or another ActiveBody
//	subclass) simply won't be found by that lookup, and any caller doing it this way should treat
//	that as a normal, harmless case to skip -- this is meant to be an opt-in Body type for whichever
//	specific unit templates need it (e.g. Body = ShieldedBody instead of Body = ActiveBody), not a
//	replacement for ActiveBody everywhere.
//
//	Usage:
//
//	Body = ShieldedBody ModuleTag_01
//	  MaxHealth = 500.0     ; same fields as ActiveBody -- nothing new to configure here
//	End
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "GameLogic/Module/ActiveBody.h"

//-------------------------------------------------------------------------------------------------
class ShieldedBody : public ActiveBody
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( ShieldedBody, "ShieldedBody" )
	MAKE_STANDARD_MODULE_MACRO( ShieldedBody )	///< reuses ActiveBodyModuleData as-is -- no new INI fields

public:

	ShieldedBody( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	// ActiveBody
	virtual BodyDamageType getDamageState() const override;		///< returns the frozen snapshot while frozen, otherwise defers to ActiveBody::getDamageState()
	virtual void evaluateVisualCondition() override;						///< reports getDamageState() (frozen-or-real, per above) to the Drawable instead of the raw live state

	// Freeze/unfreeze -- called by whatever module is granting this object a temporary health
	// change and doesn't want it to affect the reported damage state for the duration. Safe to
	// call redundantly (freezing while already frozen, or unfreezing while not frozen, are both
	// harmless no-ops) -- callers are not expected to track whether they've already called these.
	void freezeDamageState();		///< snapshots the CURRENT real damage state and holds it
	void unfreezeDamageState();	///< resumes live reporting, and immediately re-syncs the visual once against the real (current) state

private:

	Bool						m_damageStateFrozen;	///< if true, getDamageState() returns m_frozenDamageState instead of the real live value
	BodyDamageType	m_frozenDamageState;	///< the snapshot taken at freezeDamageState() time; meaningless while !m_damageStateFrozen

};
