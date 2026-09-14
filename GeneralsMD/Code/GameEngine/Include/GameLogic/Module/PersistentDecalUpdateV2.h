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

// FILE: PersistentDecalUpdateV2.h ///////////////////////////////////////////////////////////////
//
//	GeneralsMod @feature Dimitar 14/09/2026
//	Paints a single, permanent ground decal under this object -- same underlying mechanism as the
//	vanilla KindOf=FS_FAKE / Upgrade_AmericaChemicalSuits tricks (Drawable::setTerrainDecal(), via
//	W3DModelDraw's object-bound TheProjectedShadowManager->addDecal(RenderObjClass*, ...) -- which
//	is why this decal automatically skips its own terrain re-projection work when the unit is
//	off-screen/occluded (Is_Really_Visible()) or shrouded (enableShadowInvisible()), same as the
//	unit's own shadow -- see Drawable::setPersistentDecal()), except the texture name and size come
//	from THIS module's own INI-parsed data instead of a fixed compiled-in TerrainDecalType/array
//	slot or a ThingTemplate-level field -- any modder can point at an arbitrary texture with zero
//	engine changes.
//
//	Deliberately minimal (no fade, no resize, no throb, no revert): upgradeImplementation() paints
//	the decal exactly once and the object goes back to permanent sleep -- update() should in
//	practice never actually run. The decal is NOT explicitly cleaned up on death: W3DModelDraw's
//	own destructor already releases m_terrainDecal like it does for the chem suit/fake structure
//	cases, so nothing extra is needed here. The unit's own regular shadow (W3DModelDraw's separate
//	m_shadow member) is completely untouched by any of this -- m_terrainDecal/m_shadow are two
//	independent Shadow* members, so a unit keeps its normal shadow at the same time as this decal.
//
//	Survives save/load for free, same as the vanilla chem suit decal: Object::initObject() (run by
//	ThingFactory::newObject() on every object construction, including save-game reload) calls
//	Object::updateUpgradeModules(), which replays attemptUpgrade()/upgradeImplementation() for any
//	UpgradeMux module still showing !isAlreadyUpgraded() -- true for a freshly-constructed module,
//	since m_upgradeExecuted defaults false until that object's own xfer() runs a moment later. By
//	the time objects are recreated on load, ThePlayerList (CHUNK_Players) has already been restored
//	(GameState.cpp registers it before CHUNK_GameLogic), so a player-wide TriggeredBy upgrade is
//	already satisfied and the decal gets repainted right there. Nothing needs to be xferred for this.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#define DEFINE_SHADOW_NAMES	// GeneralsMod @feature Dimitar 14/09/2026: need TheShadowNames for the Style field below
#include "GameClient/Shadow.h"
#include "GameLogic/Module/CreateModule.h"
#include "GameLogic/Module/UpdateModule.h"
#include "GameLogic/Module/UpgradeModule.h"

//-------------------------------------------------------------------------------------------------
class PersistentDecalUpdateV2ModuleData : public UpdateModuleData
{
public:
	UpgradeMuxData	m_upgradeMuxData;
	AsciiString			m_textureName;	///< decal texture, e.g. "MyDecal.tga" -- no MaxDecalCount-style cap, unlike DecalUpdateV2
	Real						m_sizeX;				///< world-space decal width
	Real						m_sizeY;				///< world-space decal height
	Bool						m_startsActive;	///< StartsActive -- if Yes (default), the decal is painted immediately on creation, no upgrade needed at all; set No to make TriggeredBy the only trigger
	ShadowType			m_style;				///< Style -- SHADOW_ALPHA_DECAL (default) or SHADOW_ADDITIVE_DECAL, see TheShadowNames in Shadow.h
	Bool						m_onlyVisibleToOwningPlayer;	///< OnlyVisibleToOwningPlayer -- if Yes, only the owning player's own client ever paints this decal at all (checked once, same semantics/limitation as RadiusDecalTemplate's field of the same name -- does not react to a later capture)

	PersistentDecalUpdateV2ModuleData()
	{
		m_sizeX = 10.0f;
		m_sizeY = 10.0f;
		m_startsActive = TRUE;
		m_style = SHADOW_ALPHA_DECAL;
		m_onlyVisibleToOwningPlayer = FALSE;
	}

	static void buildFieldParse(MultiIniFieldParse& p)
	{
		static const FieldParse dataFieldParse[] =
		{
			{ "Texture",	INI::parseAsciiString,	nullptr, offsetof( PersistentDecalUpdateV2ModuleData, m_textureName ) },
			{ "SizeX",		INI::parseReal,					nullptr, offsetof( PersistentDecalUpdateV2ModuleData, m_sizeX ) },
			{ "SizeY",		INI::parseReal,					nullptr, offsetof( PersistentDecalUpdateV2ModuleData, m_sizeY ) },
			{ "StartsActive",	INI::parseBool,					nullptr, offsetof( PersistentDecalUpdateV2ModuleData, m_startsActive ) },
			{ "Style",			INI::parseBitString32,	TheShadowNames, offsetof( PersistentDecalUpdateV2ModuleData, m_style ) },
			{ "OnlyVisibleToOwningPlayer",	INI::parseBool,		nullptr, offsetof( PersistentDecalUpdateV2ModuleData, m_onlyVisibleToOwningPlayer ) },
			{ 0, 0, 0, 0 }
		};

		UpdateModuleData::buildFieldParse(p);
		p.add(dataFieldParse);
		p.add(UpgradeMuxData::getFieldParse(), offsetof( PersistentDecalUpdateV2ModuleData, m_upgradeMuxData ));
	}
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
class PersistentDecalUpdateV2 : public UpdateModule, public UpgradeMux, public CreateModuleInterface
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( PersistentDecalUpdateV2, "PersistentDecalUpdateV2" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( PersistentDecalUpdateV2, PersistentDecalUpdateV2ModuleData )

public:

	PersistentDecalUpdateV2( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	static Int getInterfaceMask() { return UpdateModule::getInterfaceMask() | MODULEINTERFACE_UPGRADE | MODULEINTERFACE_CREATE; }

	// BehaviorModule
	virtual UpgradeModuleInterface* getUpgrade() override { return this; }
	virtual CreateModuleInterface* getCreate() override { return this; }

	// UpdateModule
	virtual UpdateSleepTime update() override;

	// ObjectModule -- GeneralsMod @feature Dimitar 14/09/2026: re-evaluate OnlyVisibleToOwningPlayer
	// when the unit is captured (vehicles can be hijacked) -- see the .cpp for the full rationale.
	virtual void onCapture( Player *oldOwner, Player *newOwner ) override;

	// CreateModuleInterface -- GeneralsMod @feature Dimitar 14/09/2026: lets StartsActive paint the
	// decal immediately with no upgrade at all. See the .cpp for why this can't be done directly in
	// the constructor instead (the Drawable isn't bound to the Object yet at that point).
	virtual void onCreate() override;
	virtual void onBuildComplete() override {}
	virtual Bool shouldDoOnBuildComplete() const override { return false; }

protected:

	// UpgradeMux functions.  Mux standing, of course, for Majorly Ugly Xhitcode
	virtual void upgradeImplementation() override;
	virtual void getUpgradeActivationMasks(UpgradeMaskType& activation, UpgradeMaskType& conflicting) const override
	{
		getPersistentDecalUpdateV2ModuleData()->m_upgradeMuxData.getUpgradeActivationMasks(activation, conflicting);
	}
	virtual void performUpgradeFX() override
	{
		getPersistentDecalUpdateV2ModuleData()->m_upgradeMuxData.performUpgradeFX(getObject());
	}
	virtual void processUpgradeRemoval() override
	{
		getPersistentDecalUpdateV2ModuleData()->m_upgradeMuxData.muxDataProcessUpgradeRemoval(getObject());
	}
	virtual Bool requiresAllActivationUpgrades() const override
	{
		return getPersistentDecalUpdateV2ModuleData()->m_upgradeMuxData.m_requiresAllTriggers;
	}
	virtual Bool isSubObjectsUpgrade() override { return false; }
};
