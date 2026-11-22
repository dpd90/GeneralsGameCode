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

// FILE: FireUpdateV2.h ///////////////////////////////////////////////////////////////////////////
//
//	GeneralsMod @feature Dimitar 11/11/2026
//	FireUpdateV2 is FireWeaponUpdate (fires a weapon at itself as fast as the weapon allows) extended
//	with an FXList and an ObjectCreationList. Weapon, FX and OCL all happen at the geometric CENTER
//	of the unit (position + half geometry height), not at its feet.
//
//	Timing: nothing happens before InitialDelay. After that the Weapon fires whenever its own reload
//	allows (same as FireWeaponUpdate). FX/OCL:
//	  - Rate > 0: fire every Rate, starting at InitialDelay, independent of the Weapon.
//	  - Rate = 0 (default): fire together with every Weapon shot; with no Weapon set, fire exactly
//	    once, at InitialDelay.
//
//	Behavior = FireUpdateV2 ModuleTag_Pulse
//	  Weapon       = ToxinPoolDamageWeapon
//	  FX           = FX_ToxinPulse
//	  OCL          = OCL_ToxinPuddle
//	  InitialDelay = 500
//	  Rate         = 1000           ; FX/OCL every 1s (0 = with each weapon shot)
//	End
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "GameLogic/Module/UpdateModule.h"
#include "GameLogic/Weapon.h"

class FXList;
class ObjectCreationList;

//-------------------------------------------------------------------------------------------------
class FireUpdateV2ModuleData : public UpdateModuleData
{
public:
	const WeaponTemplate*			m_weaponTemplate;
	const FXList*					m_fxList;
	const ObjectCreationList*		m_ocl;
	UnsignedInt						m_initialDelayFrames;
	UnsignedInt						m_rateFrames;			///< FX/OCL interval; 0 = fire with each weapon shot
	UnsignedInt						m_exclusiveWeaponDelay;	///< If non-zero, any other weapon having fired this recently will keep us from doing anything

	FireUpdateV2ModuleData();

	static void buildFieldParse(MultiIniFieldParse& p);
};

//-------------------------------------------------------------------------------------------------
class FireUpdateV2 : public UpdateModule
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( FireUpdateV2, "FireUpdateV2" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( FireUpdateV2, FireUpdateV2ModuleData )

public:

	FireUpdateV2( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	virtual UpdateSleepTime update() override;

protected:

	Bool isOkayToFire();
	void getCenter( Coord3D& center );
	void doEffects( const Coord3D& center );

	Weapon*			m_weapon;
	UnsignedInt		m_initialDelayFrame;
	UnsignedInt		m_nextEffectFrame;		///< used only when Rate > 0
	Bool			m_effectsFiredOnce;		///< used only when Rate == 0 and no Weapon is set

};
