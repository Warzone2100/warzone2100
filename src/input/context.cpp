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
#include "lib/framework/frame.h"
#include "../hci.h" // for intMode

InputContexts InputContext::contexts = InputContexts();

extern bool isMouseOverRadar();

static bool isInDesignScreen()
{
	return intMode == INTMODE::INT_DESIGN;
}

static const unsigned int MAX_ICONTEXT_PRIORITY = std::numeric_limits<unsigned int>::max();
const InputContext InputContext::ALWAYS_ACTIVE =    { true,  MAX_ICONTEXT_PRIORITY, InputContext::State::ACTIVE,    N_("Global Hotkeys") };
const InputContext InputContext::BACKGROUND =       { false, 0,                     InputContext::State::ACTIVE,    N_("Other Hotkeys")  };
const InputContext InputContext::GAMEPLAY =         { false, 1,                     InputContext::State::ACTIVE,    N_("Gameplay")       };
const InputContext InputContext::RADAR =            { false, { 2, 0 },              InputContext::State::ACTIVE,    N_("Radar"),         []() { return isMouseOverRadar() && !isInDesignScreen(); }};
const InputContext InputContext::__DEBUG =          { false, MAX_ICONTEXT_PRIORITY, InputContext::State::INACTIVE,  N_("Debug")          };

InputContext::InputContext(const bool bIsAlwaysActive, const ContextPriority priority, const State initialState, const char* const displayName)
	: InputContext(bIsAlwaysActive, priority, initialState, displayName, []() { return false; })
{
}

static unsigned int inputCtxIndexCounter = 0;
InputContext::InputContext(const bool bIsAlwaysActive, const ContextPriority priority, const State initialState, const char* const displayName, const PriorityCondition condition)
	: bIsAlwaysActive(bIsAlwaysActive)
	, priority(priority)
	, index(inputCtxIndexCounter++)
	, displayName(displayName)
	, defaultState(initialState)
	, condition(condition)
{
	contexts.push_back(std::ref(*this));
}

const InputContexts InputContext::getAllContexts()
{
	return contexts;
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


void ContextManager::set(const InputContext& context, const InputContext::State newState)
{
	auto& state = states.back();
	const InputContext::State oldState = state[context.index];
	if (oldState != newState)
	{
		state[context.index] = newState;
		bDirty = true;
	}
}

bool ContextManager::isActive(const InputContext& context) const
{
	const auto& state = states.back();
	return state[context.index] != InputContext::State::INACTIVE;
}

unsigned int ContextManager::getPriority(const InputContext& context) const
{
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
	const auto contexts = InputContext::getAllContexts();
	auto state = std::vector<InputContext::State>(contexts.size(), InputContext::State::INACTIVE);
	for (const InputContext& context : contexts)
	{
		state[context.index] = context.defaultState;
	}
	states.clear();
	states.push_back(state);

	bDirty = true;
}

void ContextManager::makeAllInactive()
{
	for (const InputContext& context : InputContext::getAllContexts())
	{
		if (!context.isAlwaysActive())
		{
			set(context, InputContext::State::INACTIVE);
		}
	}

	// No need to set dirty as set() handles setting it if necessary
}

void ContextManager::updatePriorityStatus()
{
	for (const InputContext& context : InputContext::getAllContexts())
	{
		if (isActive(context))
		{
			set(context, context.condition()
				? InputContext::State::PRIORITIZED
				: InputContext::State::ACTIVE);
		}
	}
}

void ContextManager::pushState()
{
	const auto copyOfCurrentState = std::vector<InputContext::State>(states.back());
	states.push_back(copyOfCurrentState);
	bDirty = true;
}

void ContextManager::popState()
{
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
