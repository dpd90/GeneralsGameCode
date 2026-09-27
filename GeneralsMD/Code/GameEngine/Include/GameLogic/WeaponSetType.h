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

// WeaponSetType.h ////////////////////////////////////////////////////////////////////////////////
// Part of header detangling
// JKMCD Aug 2002

#pragma once

//-------------------------------------------------------------------------------------------------
// IMPORTANT NOTE: you should endeavor to set up states such that the most "normal"
// state is defined by the bit being off. That is, the typical "normal" condition
// has all condition flags set to zero.
//
// IMPORTANT NOTE #2: if you add or modify this list, be sure to update TheWeaponSetNames,
// *and* TheWeaponSetTypeToModelConditionTypeMap!
//
enum WeaponSetType CPP_11(: Int)
{
	// The access and use of this enum has the bit shifting built in, so this is a 0,1,2,3,4,5 enum
	WEAPONSET_VETERAN		= 0,
	WEAPONSET_ELITE,
	WEAPONSET_HERO,
	WEAPONSET_PLAYER_UPGRADE,			// This weapon set flag comes from a purchased upgrade to the player
	WEAPONSET_CRATEUPGRADE_ONE,
	WEAPONSET_CRATEUPGRADE_TWO,
	WEAPONSET_VEHICLE_HIJACK,
	WEAPONSET_CARBOMB,
	WEAPONSET_MINE_CLEARING_DETAIL,
	WEAPONSET_RIDER1, //Kris: Added these for different combat-bike riders
	WEAPONSET_RIDER2,
	WEAPONSET_RIDER3,
	WEAPONSET_RIDER4,
	WEAPONSET_RIDER5,
	WEAPONSET_RIDER6,
	WEAPONSET_RIDER7,
	WEAPONSET_RIDER8,

	// GeneralsMod @feature Dimitar 11/09/2026: lets a module keep a real weapon
	// (e.g. a healing weapon) off a unit's normally-orderable weapon slot by default, only swapping
	// it in via a WeaponSet{ Conditions = HEALING_DETAIL ... } block while the module itself has this
	// flag set -- same pattern WEAPONSET_MINE_CLEARING_DETAIL uses for DozerMineDisarmingWeapon.
	WEAPONSET_HEALING_DETAIL,

	// GeneralsMod @feature Dimitar 26/09/2026: RIDER9-32 for RiderChangeContainV2
	WEAPONSET_RIDER9,
	WEAPONSET_RIDER10,
	WEAPONSET_RIDER11,
	WEAPONSET_RIDER12,
	WEAPONSET_RIDER13,
	WEAPONSET_RIDER14,
	WEAPONSET_RIDER15,
	WEAPONSET_RIDER16,
	WEAPONSET_RIDER17,
	WEAPONSET_RIDER18,
	WEAPONSET_RIDER19,
	WEAPONSET_RIDER20,
	WEAPONSET_RIDER21,
	WEAPONSET_RIDER22,
	WEAPONSET_RIDER23,
	WEAPONSET_RIDER24,
	WEAPONSET_RIDER25,
	WEAPONSET_RIDER26,
	WEAPONSET_RIDER27,
	WEAPONSET_RIDER28,
	WEAPONSET_RIDER29,
	WEAPONSET_RIDER30,
	WEAPONSET_RIDER31,
	WEAPONSET_RIDER32,

	WEAPONSET_COUNT
};
