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

// FILE: ContainedTransitionDamageFXV2.h //////////////////////////////////////////////////////////
// Desc:   TransitionDamageFX that also works on contained riders (e.g. an OverlordContain rider).
//
//         Why this exists: TransitionDamageFX only reacts to DamageModuleInterface::
//         onBodyDamageStateChange(), which ActiveBody only calls from attemptDamage()/attemptHealing().
//         A rider inside an OverlordContain never takes those hits itself -- OverlordContain::
//         onBodyDamageStateChange() copies the Overlord's state onto it via BodyModule::setDamageState()
//         -> internalChangeHealth(), which swaps the model (evaluateVisualCondition) but never notifies
//         the rider's damage modules. So a plain TransitionDamageFX on the rider never fires.
//
//         This module takes the EXACT same INI fields as TransitionDamageFX (it reuses
//         TransitionDamageFXModuleData and its parsers) and runs the same effect code. In addition it
//         polls its own body's damage state every CheckInterval and, if the state changed without the
//         normal notification having reached it, runs the transition itself. When it does that while
//         contained, it uses the CONTAINER's last DamageInfo (the hit that actually caused the change),
//         so DamageFXTypes/DamageOCLTypes/DamageParticleTypes filtering and the OCL damage source work
//         the same as they would on the container.
//
//         Normal (non-relayed) notifications update the tracked state first, so a transition is never
//         played twice. Can be used as a drop-in replacement for TransitionDamageFX on any object.
//
// INI: every TransitionDamageFX field, plus optionally:
//     CheckInterval = 100   ; ms between body-state polls (default 100ms, min 1 frame)
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "GameLogic/Module/UpdateModule.h"
#include "GameLogic/Module/DamageModule.h"
#include "GameLogic/Module/TransitionDamageFX.h"

//-------------------------------------------------------------------------------------------------
/** Same data as TransitionDamageFX, plus the poll interval. */
//-------------------------------------------------------------------------------------------------
class ContainedTransitionDamageFXV2ModuleData : public TransitionDamageFXModuleData
{
public:
	UnsignedInt m_checkIntervalFrames;		///< CheckInterval: frames between body-state polls.

	ContainedTransitionDamageFXV2ModuleData();

	static void buildFieldParse(MultiIniFieldParse& p)
	{
		TransitionDamageFXModuleData::buildFieldParse(p);

		static const FieldParse dataFieldParse[] =
		{
			{ "CheckInterval",	INI::parseDurationUnsignedInt,	nullptr, offsetof( ContainedTransitionDamageFXV2ModuleData, m_checkIntervalFrames ) },
			{ 0, 0, 0, 0 }
		};
		p.add(dataFieldParse);
	}
};

//-------------------------------------------------------------------------------------------------
class ContainedTransitionDamageFXV2 : public UpdateModule, public DamageModuleInterface
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( ContainedTransitionDamageFXV2, "ContainedTransitionDamageFXV2" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( ContainedTransitionDamageFXV2, ContainedTransitionDamageFXV2ModuleData )

public:

	ContainedTransitionDamageFXV2( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	static Int getInterfaceMask() { return UpdateModule::getInterfaceMask() | MODULEINTERFACE_DAMAGE; }

	// BehaviorModule
	virtual DamageModuleInterface* getDamage() override { return this; }

	// Module
	virtual void onObjectCreated() override;

	// DamageModuleInterface
	virtual void onDamage( DamageInfo *damageInfo ) override { }
	virtual void onHealing( DamageInfo *damageInfo ) override { }
	virtual void onBodyDamageStateChange( const DamageInfo* damageInfo,
																				BodyDamageType oldState,
																				BodyDamageType newState ) override;

	// UpdateModule
	virtual UpdateSleepTime update() override;

	// Must keep polling while the rider is DISABLED_HELD (plus any other disabled bit it picks up).
	virtual DisabledMaskType getDisabledTypesToProcess() const override { return DISABLEDMASK_ALL; }

protected:

	void doTransitionEffects( const DamageInfo* damageInfo, BodyDamageType oldState, BodyDamageType newState );

	/// we keep a record of attached particle system so we can detach and kill them when we want to
	ParticleSystemID m_particleSystemID[ BODYDAMAGETYPE_COUNT ][ DAMAGE_MODULE_MAX_FX ];

	BodyDamageType		m_lastKnownState;		///< last body state we played transition effects for
	const DamageInfo*	m_relayDamageInfo;	///< non-null only while a polled (relayed) transition is running
};
