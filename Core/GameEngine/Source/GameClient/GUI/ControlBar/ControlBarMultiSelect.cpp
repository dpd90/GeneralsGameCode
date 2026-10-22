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

// FILE: ControlBarMultiSelect.cpp ////////////////////////////////////////////////////////////////
// Author: Colin Day, March 2002
// Desc:   Context-sensitive GUI for when you select multiple objects.  What we do is show
//				 the commands that you can use between them all
///////////////////////////////////////////////////////////////////////////////////////////////////

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include <algorithm>
#include <vector>

#include "Common/GameCommon.h"
#include "Common/MessageStream.h"
#include "Common/Player.h"
#include "Common/ThingTemplate.h"
#include "Common/Upgrade.h"
#include "GameClient/ControlBar.h"
#include "GameClient/Drawable.h"
#include "GameClient/GameClient.h"
#include "GameClient/GadgetPushButton.h"
#include "GameClient/GameWindow.h"
#include "GameClient/InGameUI.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/DozerAIUpdate.h"




//-------------------------------------------------------------------------------------------------
/** Reset the common command data */
//-------------------------------------------------------------------------------------------------
void ControlBar::resetCommonCommandData()
{
	Int i;

	for( i = 0; i < MAX_COMMANDS_PER_SET; i++ )
	{
		m_commonCommands[ i ] = nullptr;
		//Clear out any remnant overlays.
		GadgetButtonDrawOverlayImage( m_commandWindows[ i ], nullptr );
	}

}

//=================================================================================================
// GeneralsMod: Multi-select command UNION
//
// Stock behaviour showed only the INTERSECTION of the selected objects' CommandSets (the same
// button in the same slot for every object). We now show the UNION of every selected object's
// multi-select buttons, in a deterministic order that never depends on the order the objects were
// selected in:
//
//   collect -> filter -> de-dupe -> sort -> place (anchored) -> bind
//
// Ordering key (strict total order, see candidateLess):
//   1. CommandButton  MultiSelectPriority  (high first, default 0, negative = after the default)
//   2. CommandSet     Priority             (high first, default 0)
//   3. native slot in its own CommandSet   (low first, keeps each unit's designed layout)
//   4. CommandSet name, then button name   (alphabetical, final tie-break)
//
// Placement: pinned stock commands (attack move / guard / stop / waypoints) first take their
// native slot, then every other button tries its native slot in sorted order, and whatever is
// left fills the remaining free slots in sorted order. Overflow is dropped (lowest priority).
//
// Each button then commands ONLY the selected objects that own it, see
// ControlBar::appendCommandSubsetMessage() and MSG_SET_COMMAND_SUBSET.
//
// Design: GeneralsMod project doc claude/multiselect-command-union-design.md
//=================================================================================================

namespace
{

	struct MultiSelectCandidate
	{
		const CommandButton *button;
		Int									buttonPri;		///< CommandButton::getMultiSelectPriority()
		Int									setPri;				///< CommandSet::getPriority() of the owning set
		Int									nativeSlot;		///< slot index of the button in its own CommandSet
		AsciiString					setName;			///< tie-break
	};

	//-----------------------------------------------------------------------------------------------
	/** Strict total order over candidates. Distinct buttons never compare equal, so the result of
		* sorting does not depend on the order candidates were collected in (i.e. on selection order). */
	//-----------------------------------------------------------------------------------------------
	bool candidateLess( const MultiSelectCandidate &a, const MultiSelectCandidate &b )
	{
		if( a.buttonPri != b.buttonPri )
			return a.buttonPri > b.buttonPri;			// high first
		if( a.setPri != b.setPri )
			return a.setPri > b.setPri;						// high first
		if( a.nativeSlot != b.nativeSlot )
			return a.nativeSlot < b.nativeSlot;		// designed order

		int c = a.setName.compareNoCase( b.setName );
		if( c != 0 )
			return c < 0;

		c = a.button->getName().compareNoCase( b.button->getName() );
		if( c != 0 )
			return c < 0;

		// only reachable for two distinct button objects with identical names (INI overrides);
		// this is client-side UI layout only, so a pointer compare is an acceptable last resort
		return a.button < b.button;
	}

	//-----------------------------------------------------------------------------------------------
	/** Stock shared commands that should stay in their usual slot whenever present */
	//-----------------------------------------------------------------------------------------------
	Bool isPinnedCommandType( GUICommandType type )
	{
		switch( type )
		{
			case GUI_COMMAND_ATTACK_MOVE:
			case GUI_COMMAND_GUARD:
			case GUI_COMMAND_GUARD_WITHOUT_PURSUIT:
			case GUI_COMMAND_GUARD_FLYING_UNITS_ONLY:
			case GUI_COMMAND_STOP:
			case GUI_COMMAND_WAYPOINTS:
				return TRUE;
			default:
				return FALSE;
		}
	}

	//-----------------------------------------------------------------------------------------------
	/** Rank of an availability result when several owners disagree: the button shows the most
		* usable state any of its owners has */
	//-----------------------------------------------------------------------------------------------
	Int availabilityRank( CommandAvailability a )
	{
		switch( a )
		{
			case COMMAND_ACTIVE:			return 5;
			case COMMAND_AVAILABLE:		return 4;
			case COMMAND_NOT_READY:		return 3;
			case COMMAND_CANT_AFFORD:	return 2;
			case COMMAND_RESTRICTED:	return 1;
			case COMMAND_HIDDEN:
			default:									return 0;
		}
	}

	//-----------------------------------------------------------------------------------------------
	/** Multi-select portrait: should 'cand' replace 'best'? Same ordering idea as the buttons:
		* highest CommandSet Priority wins; objects with a command set beat objects without one;
		* then CommandSet name, template name (alphabetical) and lowest ObjectID as tie-breaks, so
		* the portrait never depends on selection order. */
	//-----------------------------------------------------------------------------------------------
	Bool portraitCandidateBeats( const Object *cand, const CommandSet *candSet,
															 const Object *best, const CommandSet *bestSet )
	{
		if( best == nullptr )
			return TRUE;

		if( ( candSet != nullptr ) != ( bestSet != nullptr ) )
			return candSet != nullptr;

		if( candSet && bestSet )
		{
			if( candSet->getPriority() != bestSet->getPriority() )
				return candSet->getPriority() > bestSet->getPriority();

			int c = candSet->getName().compareNoCase( bestSet->getName() );
			if( c != 0 )
				return c < 0;
		}

		int c = cand->getTemplate()->getName().compareNoCase( best->getTemplate()->getName() );
		if( c != 0 )
			return c < 0;

		return cand->getID() < best->getID();
	}

	//-----------------------------------------------------------------------------------------------
	/** Selected objects that take part in the command bar */
	//-----------------------------------------------------------------------------------------------
	Bool isGuiRelevant( const Object *obj )
	{
		return obj != nullptr &&
					 !obj->isKindOf( KINDOF_IGNORED_IN_GUI ) &&
					 !obj->getStatusBits().test( OBJECT_STATUS_SOLD );
	}

}

//-------------------------------------------------------------------------------------------------
/** GeneralsMod: true if this button has nothing left to do for this object, so it should not take
	* a slot in the multi-select union. Only completed/inapplicable upgrades for now; an upgrade that
	* is merely in the production queue is NOT spent (it stays visible while researching). */
//-------------------------------------------------------------------------------------------------
Bool ControlBar::isCommandSpent( const CommandButton *command, Object *obj ) const
{
	if( command == nullptr || obj == nullptr )
		return FALSE;

	const UpgradeTemplate *upgradeT = command->getUpgradeTemplate();

	switch( command->getCommandType() )
	{
		case GUI_COMMAND_OBJECT_UPGRADE:
			if( upgradeT == nullptr )
				return FALSE;
			return obj->hasUpgrade( upgradeT ) || !obj->affectedByUpgrade( upgradeT );

		case GUI_COMMAND_PLAYER_UPGRADE:
		{
			if( upgradeT == nullptr )
				return FALSE;
			const Player *player = obj->getControllingPlayer();
			return player != nullptr && player->hasUpgradeComplete( upgradeT );
		}

		default:
			return FALSE;
	}
}

//-------------------------------------------------------------------------------------------------
/** GeneralsMod: true when the selection is two or more GUI-relevant objects that are all dozers
	* sharing one CommandSet. Only then are DOZER_CONSTRUCT buttons offered in multi-select. */
//-------------------------------------------------------------------------------------------------
Bool ControlBar::isMultiDozerSelection() const
{
	const DrawableList *selectedDrawables = TheInGameUI->getAllSelectedDrawables();
	if( selectedDrawables == nullptr )
		return FALSE;

	Int count = 0;
	AsciiString commandSetName;

	for( DrawableListCIt it = selectedDrawables->begin(); it != selectedDrawables->end(); ++it )
	{
		const Drawable *draw = *it;
		const Object *obj = draw ? draw->getObject() : nullptr;
		if( !isGuiRelevant( obj ) )
			continue;

		if( !obj->isKindOf( KINDOF_DOZER ) )
			return FALSE;

		if( count == 0 )
			commandSetName = obj->getCommandSetString();
		else if( commandSetName.compare( obj->getCommandSetString() ) != 0 )
			return FALSE;

		++count;
	}

	return count >= 2;
}

//-------------------------------------------------------------------------------------------------
/** GeneralsMod: pick the selected dozer that should take the next build order: a dozer with no
	* build task pending and not disabled, nearest to pos (if given), lowest ObjectID as tie-break.
	* Client-side choice; the chosen ID travels in MSG_DOZER_CONSTRUCT so logic stays deterministic. */
//-------------------------------------------------------------------------------------------------
Object *ControlBar::chooseMultiSelectBuilder( const Coord3D *pos ) const
{
	const DrawableList *selectedDrawables = TheInGameUI->getAllSelectedDrawables();
	if( selectedDrawables == nullptr )
		return nullptr;

	// a dozer handed an order in the last ~2 seconds may not show its build task yet (the order is
	// still travelling through the network), so prefer a different dozer when there is one
	const UnsignedInt now = TheGameLogic ? TheGameLogic->getFrame() : 0;
	const UnsignedInt recentWindow = 2 * LOGICFRAMES_PER_SECOND;

	Object *best = nullptr;
	Real bestDistSqr = 0.0f;
	Object *bestRecent = nullptr;
	Real bestRecentDistSqr = 0.0f;

	for( DrawableListCIt it = selectedDrawables->begin(); it != selectedDrawables->end(); ++it )
	{
		Drawable *draw = *it;
		Object *obj = draw ? draw->getObject() : nullptr;
		if( !isGuiRelevant( obj ) || !obj->isKindOf( KINDOF_DOZER ) || obj->isDisabled() )
			continue;

		AIUpdateInterface *ai = obj->getAIUpdateInterface();
		DozerAIInterface *dozerAI = ai ? ai->getDozerAIInterface() : nullptr;
		if( dozerAI == nullptr || dozerAI->isTaskPending( DOZER_TASK_BUILD ) )
			continue;

		Real distSqr = 0.0f;
		if( pos )
		{
			const Coord3D *p = obj->getPosition();
			Real dx = p->x - pos->x;
			Real dy = p->y - pos->y;
			distSqr = dx * dx + dy * dy;
		}

		Bool recent = FALSE;
		for( Int r = 0; r < MAX_RECENT_MULTI_BUILDERS; r++ )
		{
			if( m_recentBuilderIDs[ r ] == obj->getID() && now - m_recentBuilderFrames[ r ] < recentWindow )
			{
				recent = TRUE;
				break;
			}
		}

		Object *&slot = recent ? bestRecent : best;
		Real &slotDist = recent ? bestRecentDistSqr : bestDistSqr;
		if( slot == nullptr || distSqr < slotDist ||
				( distSqr == slotDist && obj->getID() < slot->getID() ) )
		{
			slot = obj;
			slotDist = distSqr;
		}
	}

	return best ? best : bestRecent;
}

//-------------------------------------------------------------------------------------------------
/** GeneralsMod: remember a dozer that was just given a multi-select build order */
//-------------------------------------------------------------------------------------------------
void ControlBar::noteMultiSelectBuilderUsed( ObjectID id )
{
	m_recentBuilderIDs[ m_nextRecentBuilder ] = id;
	m_recentBuilderFrames[ m_nextRecentBuilder ] = TheGameLogic ? TheGameLogic->getFrame() : 0;
	m_nextRecentBuilder = ( m_nextRecentBuilder + 1 ) % MAX_RECENT_MULTI_BUILDERS;
}

//-------------------------------------------------------------------------------------------------
/** GeneralsMod: when several objects are selected, restrict the NEXT command message to the
	* selected objects whose current CommandSet contains exactly this button (pointer compare).
	* Must be called immediately before the command message is appended. Objects that are
	* KINDOF_IGNORED_IN_GUI (e.g. mob members) always come along, as they did before. */
//-------------------------------------------------------------------------------------------------
void ControlBar::appendCommandSubsetMessage( const CommandButton *command )
{
	if( command == nullptr || TheInGameUI->getSelectCount() <= 1 )
		return;

	const DrawableList *selectedDrawables = TheInGameUI->getAllSelectedDrawables();
	if( selectedDrawables == nullptr )
		return;

	std::vector<ObjectID> owners;
	Int total = 0;

	for( DrawableListCIt it = selectedDrawables->begin(); it != selectedDrawables->end(); ++it )
	{
		const Drawable *draw = *it;
		const Object *obj = draw ? draw->getObject() : nullptr;
		if( obj == nullptr )
			continue;

		++total;

		if( obj->isKindOf( KINDOF_IGNORED_IN_GUI ) )
		{
			owners.push_back( obj->getID() );
			continue;
		}

		const CommandSet *commandSet = findCommandSet( obj->getCommandSetString() );
		if( commandSet && commandSet->hasCommandButton( command ) )
			owners.push_back( obj->getID() );
	}

	// nobody owns it (e.g. a shortcut button) or everybody does: plain selection is already right
	if( owners.empty() || (Int)owners.size() == total )
		return;

	// GameMessage argument counts are an UnsignedByte; an absurdly large subset falls back to the
	// stock behaviour (whole selection) rather than overflowing
	if( owners.size() > 250 )
	{
		DEBUG_LOG(( "appendCommandSubsetMessage: %d owners exceeds the message argument limit, using the whole selection", (Int)owners.size() ));
		return;
	}

	GameMessage *msg = TheMessageStream->appendMessage( GameMessage::MSG_SET_COMMAND_SUBSET );
	for( std::vector<ObjectID>::const_iterator it = owners.begin(); it != owners.end(); ++it )
		msg->appendObjectIDArgument( *it );
}

//-------------------------------------------------------------------------------------------------
/** Populate the visible command bar with the UNION of the multi-select commands of all the
	* objects that are selected in the UI (GeneralsMod, see the block comment above) */
//-------------------------------------------------------------------------------------------------
void ControlBar::populateMultiSelect()
{
	// GeneralsMod: the portrait shows the highest-priority selected unit (see portraitCandidateBeats)
	Object *portraitObj = nullptr;
	const CommandSet *portraitSet = nullptr;

	// first reset the common command data
	resetCommonCommandData();

	// by default, hide all the controls in the command section
	for( Int i = 0; i < MAX_COMMANDS_PER_SET; i++ )
	{
		if (m_commandWindows[ i ])
		{
			m_commandWindows[ i ]->winHide( TRUE );
		}
	}

	// sanity
	DEBUG_ASSERTCRASH( TheInGameUI->getSelectCount() > 1,
										 ("populateMultiSelect: Can't populate multiselect context cause there are only '%d' things selected",
										  TheInGameUI->getSelectCount()) );

	// get the list of drawable IDs from the in game UI
	const DrawableList *selectedDrawables = TheInGameUI->getAllSelectedDrawables();

	// sanity
	DEBUG_ASSERTCRASH( selectedDrawables->empty() == FALSE, ("populateMultiSelect: Drawable list is empty") );

	// construct buttons only make sense when every selected object is a dozer with the same set
	const Bool multiDozer = isMultiDozerSelection();

	//
	// COLLECT + FILTER + DE-DUPE
	//
	std::vector<MultiSelectCandidate> candidates;

	for( DrawableListCIt it = selectedDrawables->begin(); it != selectedDrawables->end(); ++it )
	{
		Drawable *draw = *it;
		Object *obj = draw ? draw->getObject() : nullptr;

		// ignored-in-GUI objects and objects being sold can't contribute commands
		if( !isGuiRelevant( obj ) )
			continue;

		const CommandSet *commandSet = findCommandSet( obj->getCommandSetString() );

		// portrait: keep the highest-priority unit
		if( portraitCandidateBeats( obj, commandSet, portraitObj, portraitSet ) )
		{
			portraitObj = obj;
			portraitSet = commandSet;
		}

		// an object without a command set simply contributes nothing (stock wiped everything)
		if( commandSet == nullptr )
			continue;

		for( Int i = 0; i < MAX_COMMANDS_PER_SET; i++ )
		{
			// our implementation doesn't necessarily make use of the max possible command buttons
			if( !m_commandWindows[ i ] )
				continue;

			const CommandButton *command = commandSet->getCommandButton( i );
			if( command == nullptr )
				continue;

			GUICommandType type = command->getCommandType();

			// per-passenger exit buttons are single-selection only; Evacuate covers multi-select
			if( type == GUI_COMMAND_EXIT_CONTAINER )
				continue;

			if( type == GUI_COMMAND_DOZER_CONSTRUCT || type == GUI_COMMAND_DOZER_CONSTRUCT_CANCEL )
			{
				// offered (even without OK_FOR_MULTI_SELECT) only for an all-dozer, same-set selection
				if( !multiDozer )
					continue;
			}
			else if( type != GUI_COMMAND_ATTACK_MOVE &&
							 BitIsSet( command->getOptions(), OK_FOR_MULTI_SELECT ) == FALSE )
			{
				// attack move keeps its stock exception: if anybody has it, it's shown
				continue;
			}

			// nothing left to do for this object (completed upgrade etc.)
			if( isCommandSpent( command, obj ) )
				continue;

			MultiSelectCandidate cand;
			cand.button			= command;
			cand.buttonPri	= command->getMultiSelectPriority();
			cand.setPri			= commandSet->getPriority();
			cand.nativeSlot	= i;
			cand.setName		= commandSet->getName();

			// de-dupe by button pointer, keeping whichever candidate sorts first (order independent)
			std::vector<MultiSelectCandidate>::iterator existing = candidates.begin();
			for( ; existing != candidates.end(); ++existing )
				if( existing->button == command )
					break;

			if( existing == candidates.end() )
				candidates.push_back( cand );
			else if( candidateLess( cand, *existing ) )
				*existing = cand;
		}
	}

	//
	// SORT
	//
	std::sort( candidates.begin(), candidates.end(), candidateLess );

	//
	// PLACE (anchored packing)
	//
	const Int count = (Int)candidates.size();
	std::vector<Int> slotOf( count, -1 );
	Bool slotUsed[ MAX_COMMANDS_PER_SET ];
	for( Int s = 0; s < MAX_COMMANDS_PER_SET; s++ )
		slotUsed[ s ] = FALSE;

	// pass 0: pinned stock commands claim their native slot
	for( Int k = 0; k < count; k++ )
	{
		if( !isPinnedCommandType( candidates[ k ].button->getCommandType() ) )
			continue;
		Int s = candidates[ k ].nativeSlot;
		if( !slotUsed[ s ] )
		{
			slotUsed[ s ] = TRUE;
			slotOf[ k ] = s;
		}
	}

	// pass 1: everything else tries its native slot, in priority order
	for( Int k = 0; k < count; k++ )
	{
		if( slotOf[ k ] >= 0 )
			continue;
		Int s = candidates[ k ].nativeSlot;
		if( !slotUsed[ s ] )
		{
			slotUsed[ s ] = TRUE;
			slotOf[ k ] = s;
		}
	}

	// pass 2: leftovers fill the remaining free slots, in priority order; overflow is dropped
	for( Int k = 0; k < count; k++ )
	{
		if( slotOf[ k ] >= 0 )
			continue;
		for( Int s = 0; s < MAX_COMMANDS_PER_SET; s++ )
		{
			if( m_commandWindows[ s ] && !slotUsed[ s ] )
			{
				slotUsed[ s ] = TRUE;
				slotOf[ k ] = s;
				break;
			}
		}
	}

	//
	// BIND - in priority order, so the higher-priority button wins any shared hotkey
	// (switchToContext() has already reset the hotkey manager)
	//
	for( Int k = 0; k < count; k++ )
	{
		Int s = slotOf[ k ];
		if( s < 0 )
			continue;

		const CommandButton *command = candidates[ k ].button;
		m_commonCommands[ s ] = command;
		m_commandWindows[ s ]->winHide( FALSE );
		m_commandWindows[ s ]->winEnable( TRUE );
		setControlCommand( m_commandWindows[ s ], command );
	}

	// set the portrait image
	setPortraitByObject( portraitObj );

}

//-------------------------------------------------------------------------------------------------
/** Update logic for the multi select context sensitive GUI. GeneralsMod: each button is evaluated
	* only against the selected objects that OWN it, and shows the most usable state among them. */
//-------------------------------------------------------------------------------------------------
void ControlBar::updateContextMultiSelect()
{
	// sanity
	DEBUG_ASSERTCRASH( TheInGameUI->getSelectCount() > 1,
										 ("updateContextMultiSelect: TheInGameUI only has '%d' things selected",
										  TheInGameUI->getSelectCount()) );

	// get the list of drawable IDs from the in game UI
	const DrawableList *selectedDrawables = TheInGameUI->getAllSelectedDrawables();

	// sanity
	DEBUG_ASSERTCRASH( selectedDrawables->empty() == FALSE, ("updateContextMultiSelect: Drawable list is empty") );

	// resolve each relevant object's command set once per update
	std::vector<Object *> objects;
	std::vector<const CommandSet *> commandSets;
	for( DrawableListCIt it = selectedDrawables->begin(); it != selectedDrawables->end(); ++it )
	{
		Drawable *draw = *it;
		Object *obj = draw ? draw->getObject() : nullptr;
		if( obj == nullptr || obj->isKindOf( KINDOF_IGNORED_IN_GUI ) )
			continue;

		objects.push_back( obj );
		commandSets.push_back( findCommandSet( obj->getCommandSetString() ) );
	}

	const size_t objectCount = objects.size();

	for( Int i = 0; i < MAX_COMMANDS_PER_SET; i++ )
	{
		GameWindow *win = m_commandWindows[ i ];

		// our implementation doesn't necessarily make use of the max possible command buttons
		if( !win )
			continue;

		// don't consider hidden windows
		if( win->winIsHidden() == TRUE )
			continue;

		// get the command
		const CommandButton *command = (const CommandButton *)GadgetButtonGetData( win );
		if( command == nullptr )
			continue;

		CommandAvailability best = COMMAND_HIDDEN;
		Int bestRank = -1;
		Int owners = 0;

		for( size_t j = 0; j < objectCount; j++ )
		{
			if( commandSets[ j ] == nullptr || !commandSets[ j ]->hasCommandButton( command ) )
				continue;

			++owners;
			CommandAvailability a = getCommandAvailability( command, objects[ j ], win );
			Int r = availabilityRank( a );
			if( r > bestRank )
			{
				bestRank = r;
				best = a;
			}
		}

		// nobody owns it any more this frame (command set just changed): stock-style fallback
		if( owners == 0 )
		{
			for( size_t j = 0; j < objectCount; j++ )
			{
				CommandAvailability a = getCommandAvailability( command, objects[ j ], win );
				Int r = availabilityRank( a );
				if( r > bestRank )
				{
					bestRank = r;
					best = a;
				}
			}
		}

		win->winClearStatus( WIN_STATUS_NOT_READY );
		win->winClearStatus( WIN_STATUS_ALWAYS_COLOR );

		switch( best )
		{
			case COMMAND_HIDDEN:
				win->winHide( TRUE );
				break;
			case COMMAND_RESTRICTED:
				win->winEnable( FALSE );
				break;
			case COMMAND_NOT_READY:
				win->winEnable( FALSE );
				win->winSetStatus( WIN_STATUS_NOT_READY );
				break;
			case COMMAND_CANT_AFFORD:
				win->winEnable( FALSE );
				win->winSetStatus( WIN_STATUS_ALWAYS_COLOR );
				break;
			default:
				win->winEnable( TRUE );
				break;
		}

		//If button is a CHECK_LIKE, then update it's status now.
		if( BitIsSet( command->getOptions(), CHECK_LIKE ) )
		{
			GadgetCheckLikeButtonSetVisualCheck( win, best == COMMAND_ACTIVE );
		}
	}

}
