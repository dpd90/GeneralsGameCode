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

// FILE: FireOCLBehaviorV2.h /////////////////////////////////////////////////////////////////////////
//
//	GeneralsMod @feature Dimitar 08/09/2026
//	FireOCLBehaviorV2 is GrantStealthBehavior's exact scan pattern (growing radius each frame, up to
//	FinalRadius, then self-destructs) with the payload swapped: instead of granting stealth to every
//	ally found in range, it fires an ObjectCreationList once per object found. Intended use: a special
//	power spawns a short-lived, invisible trigger object (KindOf = NO_COLLIDE IMMOBILE UNATTACKABLE
//	INERT, no real Draw/Body impact -- see the Lazr_WeaponBonus-style pattern) carrying this behavior,
//	which then fires the configured OCL for every matching unit it finds as its radius expands.
//
//	Deliberately not de-duplicated / rate-limited per target: this module is meant to be triggered by a
//	special power with its own long cooldown (e.g. 5 minutes), where firing once per matching unit found
//	during a single expanding-radius sweep is the intended, expected behavior.
//
//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include <vector>
#include "GameClient/ParticleSys.h"
#include "GameLogic/Module/BehaviorModule.h"
#include "GameLogic/Module/UpdateModule.h"
#include "Common/BitFlagsIO.h"

class ParticleSystem;
class ParticleSystemTemplate;
class ObjectCreationList;

//-------------------------------------------------------------------------------------------------
class FireOCLBehaviorV2ModuleData : public UpdateModuleData
{
public:
	Real									m_startRadius;
	Real									m_finalRadius;
	Real									m_radiusGrowRate;
	KindOfMaskType				m_kindOf;					///< Only these types are affected -- defaults to everything
	KindOfMaskType				m_forbiddenKindOf;///< These types are never affected -- defaults to none
	const ParticleSystemTemplate*	m_radiusParticleSystemTmpl;	///< Optional particle system meant to apply to entire effect for entire duration.
	const ObjectCreationList*		m_ocl;							///< Fired once per matching object found in range.

	FireOCLBehaviorV2ModuleData()
	{
		m_finalRadius = 200.0f;
		m_startRadius = 0.0f;
		m_radiusGrowRate = 10.0f;
		m_radiusParticleSystemTmpl = nullptr;
		m_ocl = nullptr;
		SET_ALL_KINDOFMASK_BITS( m_kindOf );
		m_forbiddenKindOf.clear();
	}

	static void buildFieldParse( MultiIniFieldParse& p )
	{
		UpdateModuleData::buildFieldParse( p );

		static const FieldParse dataFieldParse[] =
		{
			{ "StartRadius",						         INI::parseReal,									 nullptr, offsetof( FireOCLBehaviorV2ModuleData, m_startRadius ) },
			{ "FinalRadius",						         INI::parseReal,									 nullptr, offsetof( FireOCLBehaviorV2ModuleData, m_finalRadius ) },
			{ "RadiusGrowRate",						       INI::parseReal,									 nullptr, offsetof( FireOCLBehaviorV2ModuleData, m_radiusGrowRate ) },
			{ "KindOf",						    KindOfMaskType::parseFromINI,					       nullptr, offsetof( FireOCLBehaviorV2ModuleData, m_kindOf ) },
			{ "ForbiddenKindOf",	KindOfMaskType::parseFromINI,					       nullptr, offsetof( FireOCLBehaviorV2ModuleData, m_forbiddenKindOf ) },
			{ "RadiusParticleSystemName",				 INI::parseParticleSystemTemplate, nullptr, offsetof( FireOCLBehaviorV2ModuleData, m_radiusParticleSystemTmpl ) },
			{ "OCL",										         INI::parseObjectCreationList,		 nullptr, offsetof( FireOCLBehaviorV2ModuleData, m_ocl ) },
			{ 0, 0, 0, 0 }
		};

		p.add(dataFieldParse);

	}

};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
class FireOCLBehaviorV2 : public UpdateModule
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( FireOCLBehaviorV2, "FireOCLBehaviorV2" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( FireOCLBehaviorV2, FireOCLBehaviorV2ModuleData )

public:

	FireOCLBehaviorV2( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	virtual UpdateSleepTime update() override;


private:

	void fireOCLAtObject( Object *obj );
	void createEmitters();

	ParticleSystemID m_radiusParticleSystemID;
	Real m_currentScanRadius;

	// GeneralsMod @feature Dimitar 08/09/2026: the radius scan is cumulative (each frame re-queries
	// from radius 0, not just the newly grown ring), so the same object would otherwise be re-found
	// and re-fired on every frame of the sweep. Track objects we've already fired at so each matching
	// object only ever triggers the OCL once for this trigger's whole lifetime. Not xfer'd -- this
	// object is extremely short-lived (a few frames), so a save/load mid-sweep re-firing at an
	// already-hit object is an acceptable, purely cosmetic edge case.
	std::vector<ObjectID> m_firedObjectIDs;
};
