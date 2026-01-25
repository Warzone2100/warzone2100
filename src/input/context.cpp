/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <limits>
#include <list>
#include <functional>

#include "context.h"
#include "debugmappings.h"
#include "lib/framework/frame.h"
#include "../hci.h" // for intMode

extern bool isMouseOverRadar();

static bool isInDesignScreen()
{
	return intMode == INTMODE::INT_DESIGN;
}

const ContextId InputContext::ALWAYS_ACTIVE         = "ALWAYS_ACTIVE";
const ContextId InputContext::BACKGROUND            = "BACKGROUND";
const ContextId InputContext::GAMEPLAY              = "GAMEPLAY";
const ContextId InputContext::RADAR                 = "RADAR";
const ContextId InputContext::DEBUG_MISC            = "DEBUG_MISC";
const ContextId InputContext::DEBUG_LEVEL_EDITOR    = "DEBUG_LEVEL_EDITOR";
const ContextId InputContext::DEBUG_HAS_SELECTION   = "DEBUG_HAS_SELECTION";

static const InputContext& nullContext()
{
	static const InputContext NULL_CONTEXT = InputContext("__NULL", false, 0, InputContext::State::INACTIVE, "null");
	return NULL_CONTEXT;
}

void registerDefaultContexts(ContextManager& contextManager, DebugInputManager& dbgInputManager)
{
	static const unsigned int MAX_ICONTEXT_PRIORITY = std::numeric_limits<unsigned int>::max();
	const InputContext alwaysActive   = { InputContext::ALWAYS_ACTIVE,          true,  MAX_ICONTEXT_PRIORITY,         InputContext::State::ACTIVE,    N_("Global Keys")          };
	const InputContext background     = { InputContext::BACKGROUND,             false, 0,                             InputContext::State::ACTIVE,    N_("Other Keys")           };
	const InputContext gameplay       = { InputContext::GAMEPLAY,               false, 1,                             InputContext::State::ACTIVE,    N_("Gameplay Keys")           };
	const InputContext radar          = { InputContext::RADAR,                  false, { 2, 0 },                      InputContext::State::ACTIVE,    N_("Radar Keys"),             []() { return isMouseOverRadar() && !isInDesignScreen(); } };
	const InputContext debug          = { InputContext::DEBUG_MISC,             false, { MAX_ICONTEXT_PRIORITY, 0 },  InputContext::State::INACTIVE,  N_("Debug Keys"),             [&dbgInputManager]() { return dbgInputManager.isDebugPrioritized(); } };
	const InputContext debugLvlEditor = { InputContext::DEBUG_LEVEL_EDITOR,     false, { MAX_ICONTEXT_PRIORITY, 0 },  InputContext::State::INACTIVE,  N_("Debug (level editor)"),   [&dbgInputManager]() { return dbgInputManager.isDebugPrioritized(); } };
	const InputContext debugSelection = { InputContext::DEBUG_HAS_SELECTION,    false, { MAX_ICONTEXT_PRIORITY, 0 },  InputContext::State::INACTIVE,  N_("Debug (selection)"),      [&dbgInputManager]() { return dbgInputManager.isDebugPrioritized(); } };

	contextManager.registerContext(nullContext());
	contextManager.registerContext(alwaysActive);
	contextManager.registerContext(background);
	contextManager.registerContext(gameplay);
	contextManager.registerContext(radar);
	contextManager.registerContext(debug);
	contextManager.registerContext(debugLvlEditor);
	contextManager.registerContext(debugSelection);
}

InputContext::InputContext(const ContextId id, const bool bIsAlwaysActive, const ContextPriority priority, const State initialState, const char* const displayName)
	: InputContext(id, bIsAlwaysActive, priority, initialState, displayName, []() { return false; })
{
}

static unsigned int inputCtxIndexCounter = 0;
InputContext::InputContext(const ContextId id, const bool bIsAlwaysActive, const ContextPriority priority, const State initialState, const char* const displayName, const PriorityCondition condition)
	: id(id)
	, bIsAlwaysActive(bIsAlwaysActive)
	, priority(priority)
	, index(inputCtxIndexCounter++)
	, displayName(displayName)
	, defaultState(initialState)
	, condition(condition)
{
}

const std::string InputContext::getDisplayName() const
{
	return displayName;
}

bool InputContext::isAlwaysActive() const
{
	return bIsAlwaysActive;
}

bool operator==(const InputContext& lhs, const InputContext& rhs)
{
	return lhs.index == rhs.index;
}

bool operator!=(const InputContext& lhs, const InputContext& rhs)
{
	return !(lhs == rhs);
}

const InputContext& ContextManager::get(const ContextId& contextId) const
{
	const auto found = contexts.find(contextId);
	if (found != contexts.cend())
	{
		return found->second;
	}
	else
	{
		debug(LOG_WARNING, "Tried to get missing input context \"%s\". Returning null context instead!", contextId);
		return nullContext();
	}
}

void ContextManager::set(const ContextId& contextId, const InputContext::State newState)
{
	const InputContext& context = get(contextId);

	auto& state = states.back();
	const InputContext::State oldState = state[context.index];
	if (oldState != newState)
	{
		state[context.index] = newState;
		bDirty = true;
	}
}

bool ContextManager::isActive(const ContextId& contextId) const
{
	const InputContext& context = get(contextId);

	ASSERT_OR_RETURN(false, !states.empty(), "No InputContext::States available");
	const auto& state = states.back();
	return state[context.index] != InputContext::State::INACTIVE;
}

unsigned int ContextManager::getPriority(const ContextId contextId) const
{
	const InputContext& context = get(contextId);

	switch (states.back()[context.index])
	{
	case InputContext::State::PRIORITIZED:
		return context.priority.prioritized;
	case InputContext::State::ACTIVE:
		return context.priority.active;
	case InputContext::State::INACTIVE:
	default:
		return 0;
	}
}

void ContextManager::resetStates()
{
	auto state = std::vector<InputContext::State>(contexts.size(), InputContext::State::INACTIVE);
	for (const auto& keyValuePair : contexts)
	{
		const InputContext& context = keyValuePair.second;
		state[context.index] = context.defaultState;
	}
	states.clear();
	states.push_back(state);

	bDirty = true;
}

void ContextManager::makeAllInactive()
{
	for (const auto& keyValuePair : contexts)
	{
		const InputContext& context = keyValuePair.second;
		if (!context.isAlwaysActive())
		{
			set(context.id, InputContext::State::INACTIVE);
		}
	}

	// No need to set dirty as set() handles setting it if necessary
}

void ContextManager::updatePriorityStatus()
{
	for (const auto& keyValuePair : contexts)
	{
		const InputContext& context = keyValuePair.second;
		if (isActive(context.id))
		{
			set(context.id, context.condition()
				? InputContext::State::PRIORITIZED
				: InputContext::State::ACTIVE);
		}
	}
}

void ContextManager::registerContext(InputContext context)
{
	contexts.insert({ context.id, context });
}

void ContextManager::pushState()
{
	const auto copyOfCurrentState = std::vector<InputContext::State>(states.back());
	states.push_back(copyOfCurrentState);
	bDirty = true;
}

void ContextManager::popState()
{
	ASSERT_OR_RETURN(, states.size() > 1, "Unbalanced push / pop");
	states.pop_back();
	bDirty = true;
}


bool ContextManager::isDirty() const
{
	return bDirty;
}

void ContextManager::clearDirty()
{
	bDirty = false;
}
