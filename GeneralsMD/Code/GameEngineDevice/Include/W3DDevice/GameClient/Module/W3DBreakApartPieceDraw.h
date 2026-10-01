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

// FILE: W3DBreakApartPieceDraw.h ///////////////////////////////////////////////////////////////
// GeneralsMod @feature Dimitar 19/09/2026, gained a real ModuleData 01/10/2026
// Desc: Draw module for a BreakApartDeathBehaviorV2 debris piece. Every instance starts with
//       nothing loaded, and is told, exactly once, right after the Object it belongs to is
//       spawned, which single subobject of which reference model to display, via DrawModule::
//       setBreakApartPiece() (see Common/DrawModule.h for why that hand-off is a non-pure virtual
//       on the shared DrawModule base rather than a dedicated interface: BreakApartDeathBehaviorV2.cpp,
//       the caller, is a z_gameengine file and must never #include anything from this class's own
//       GameEngineDevice header).
//
//       GeneralsMod @fix Dimitar 01/10/2026: this module originally carried NO ModuleData at all
//       (MAKE_STANDARD_MODULE_MACRO, not _WITH_MODULE_DATA -- same shape W3DDebrisDraw still uses).
//       That meant it inherited Module::friend_newModuleData()'s default, which parses every
//       instance's INI block against a raw nullptr FieldParse table -- so ANY field placed inside a
//       `Draw = W3DBreakApartPieceDraw ModuleTag_X ... End` block crashed instantly
//       (INI::findFieldParse() dereferences parseTable->token, and parseTable was nullptr), not
//       just an unrecognized one. Gained a real W3DBreakApartPieceDrawModuleData below, with
//       exactly one field so far (ParticleSystem) -- fixes that crash for good, not just for this
//       one field.
//
//       GeneralsMD-only: registered in the SHARED Core/GameEngineDevice/.../W3DModuleFactory.cpp
//       under an #if RTS_ZEROHOUR guard (see that file), same as W3DPersistentAnimModelDraw/
//       W3DWheeledTankDraw -- this header/source pair does not exist on the plain Generals build
//       at all.
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "Common/GameType.h"
#include "Common/DrawModule.h"
#include "GameClient/ParticleSys.h"	///< GeneralsMod @feature Dimitar 01/10/2026: ParticleSystemID/ParticleSystemTemplate/TheParticleSystemManager for the new ParticleSystem field below
#include "WWMath/matrix3d.h"	///< GeneralsMod @feature Dimitar 19/09/2026: need the full Matrix3D type (not just DrawModule.h's forward declare) for the m_pieceOffset member below

// FORWARD REFERENCES /////////////////////////////////////////////////////////////////////////////
class Thing;
class RenderObjClass;
class Shadow;

//-------------------------------------------------------------------------------------------------
// GeneralsMod @feature Dimitar 01/10/2026: this module's own ModuleData -- currently just the one
// ParticleSystem field. boneName/particleSystemTemplate follow the exact same two-token INI shape
// as W3DModelDraw's own ParticleSysBone field (`<BoneName> <TemplateName>`, see parseParticleSysBone
// in W3DModelDraw.cpp) for consistency with the rest of this engine, with one deliberate difference:
// since this module only ever shows ONE resolved bone/subobject at a time (unlike W3DModelDraw,
// which can have many ParticleSysBone entries across a whole visible model), "None" as the bone name
// (case-insensitive, also what an unresolved/misspelled bone name degrades to) means "don't tie this
// to any specific bone -- attach it at the piece's own root" instead of being an error, which is
// also exactly what the field looks like when written as `ParticleSystem = None <TemplateName>`.
class W3DBreakApartPieceDrawModuleData : public ModuleData
{
public:

	AsciiString							m_particleSystemBoneName;		///< lowered; empty or "none" means attach at the piece's own root, not a specific bone
	const ParticleSystemTemplate*	m_particleSystemTemplate;	///< nullptr (the default) disables the whole feature -- no particle system is ever created

	W3DBreakApartPieceDrawModuleData()
	{
		m_particleSystemTemplate = nullptr;
	}
	virtual ~W3DBreakApartPieceDrawModuleData() { }

	static void buildFieldParse(MultiIniFieldParse& p);

};

//-------------------------------------------------------------------------------------------------
class W3DBreakApartPieceDraw : public DrawModule
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( W3DBreakApartPieceDraw, "W3DBreakApartPieceDraw" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( W3DBreakApartPieceDraw, W3DBreakApartPieceDrawModuleData )

public:

	W3DBreakApartPieceDraw( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	/// the draw method
	virtual void doDrawModule(const Matrix3D* transformMtx) override;

	virtual void setShadowsEnabled(Bool enable) override { }			///< no shadow support in this first pass -- see .cpp
	virtual void releaseShadows() override { }
	virtual void allocateShadows() override { }

	virtual void setFullyObscuredByShroud(Bool fullyObscured) override { }
	virtual void reactToTransformChange(const Matrix3D* oldMtx, const Coord3D* oldPos, Real oldAngle) override;
	virtual void reactToGeometryChange() override { }

	// GeneralsMod @feature Dimitar 19/09/2026: the real entry point -- see DrawModule::
	// setBreakApartPiece()'s own comment in Common/DrawModule.h. One-shot: a second call while
	// m_renderObject is already set is silently ignored, same re-entrancy guard shape as
	// W3DDebrisDraw::setModelName(). Default argument deliberately NOT repeated here (it's on the
	// DrawModule base declaration, the only one that matters for calls made through a DrawModule*,
	// which is how every real caller reaches this).
	virtual void setBreakApartPiece(const AsciiString& modelName, const AsciiString& boneName, const Matrix3D* liveBoneWorldTransform) override;

	// GeneralsMod @feature Dimitar 19/09/2026: the complementary "remainder" entry point -- see
	// DrawModule::setBreakApartRemainder()'s own comment in Common/DrawModule.h. Same one-shot
	// guard shape as setBreakApartPiece() above (a second call while m_renderObject is already set
	// is silently ignored) -- the two entry points are mutually exclusive on any one instance in
	// practice (BreakApartDeathBehaviorV2 only ever calls one or the other on a given spawned
	// Object, never both), but sharing the guard costs nothing and keeps that safe either way.
	virtual void setBreakApartRemainder(const AsciiString& modelName, const std::vector<AsciiString>& hiddenBoneNames) override;

private:

	RenderObjClass*						m_renderObject;										///< W3D Render object for this drawable -- a private clone of BreakApartModel, never shared with the reference-model cache BreakApartModelHelper keeps for the DieModule's own hierarchy queries

	// GeneralsMod @feature Dimitar 19/09/2026: the fixed delta (captured once, in setBreakApartPiece())
	// between the debris Drawable's own transform and this piece's bone-aligned root transform --
	// identity when no live bone transform was available (old behavior: just mirror the Drawable's
	// transform every frame). doDrawModule()/reactToTransformChange() recompose
	// currentDrawableTransform * m_pieceOffset every frame instead of a bare copy, so the bone
	// alignment established at spawn time (including any turret/pitch rotation captured then)
	// survives the piece being subsequently flung/tumbled by PhysicsBehavior.
	Matrix3D									m_pieceOffset;

	// GeneralsMod @feature Dimitar 01/10/2026: tracks the ParticleSystem created from this module's
	// own ParticleSystem field (see W3DBreakApartPieceDrawModuleData above), if any -- INVALID_PARTICLE_SYSTEM_ID
	// when the field wasn't configured, or the particle system failed to create. Needed so the
	// destructor can explicitly destroy() it -- unlike W3DModelDraw (which tracks a whole vector of
	// these and destroys them on ConditionState transitions), this module's piece never changes
	// state after spawn, so its only cleanup moment is its own destruction.
	ParticleSystemID					m_particleSystemID;

	// GeneralsMod @feature Dimitar 01/10/2026: shared by setBreakApartPiece()/setBreakApartRemainder()
	// -- both are this module's one-shot "now show something" entry points, and either one should
	// pick up the configured ParticleSystem field the same way. Reads getW3DBreakApartPieceDrawModuleData()
	// and m_renderObject directly (both must already be set by the time this runs); no-op if the
	// module data's own template is nullptr (the field was never set in INI).
	void attachConfiguredParticleSystem();

};
