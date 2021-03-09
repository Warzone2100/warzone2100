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

InputContexts InputContext::contexts = InputContexts();

static const unsigned int MAX_ICONTEXT_PRIORITY = std::numeric_limits<unsigned int>::max();
const InputContext InputContext::ALWAYS_ACTIVE =    { MAX_ICONTEXT_PRIORITY, InputContext::State::ACTIVE,    N_("Global Hotkeys") };
const InputContext InputContext::BACKGROUND =       { 0,                     InputContext::State::ACTIVE,    N_("Other Hotkeys") };
const InputContext InputContext::GAMEPLAY =         { 1,                     InputContext::State::ACTIVE,    N_("Gameplay") };
const InputContext InputContext::RADAR =            { { 2, 0 },              InputContext::State::ACTIVE,    N_("Radar") };
const InputContext InputContext::__DEBUG =          { MAX_ICONTEXT_PRIORITY, InputContext::State::INACTIVE,  N_("Debug") };

static unsigned int inputCtxIndexCounter = 0;
InputContext::InputContext(const ContextPriority priority, const State initialState, const char* const displayName)
	: priority(priority)
	, index(inputCtxIndexCounter++)
	, displayName(displayName)
	, defaultState(initialState)
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
	states[context.index] = newState;
	bDirty = true;
}

bool ContextManager::isActive(const InputContext& context) const
{
	const auto state = states[context.index];
	return state != InputContext::State::INACTIVE;
}

unsigned int ContextManager::getPriority(const InputContext& context) const
{
	switch (states[context.index])
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
	states = std::vector<InputContext::State>(contexts.size(), InputContext::State::INACTIVE);
	for (const InputContext& context : contexts)
	{
		states[context.index] = context.defaultState;
	}
	bDirty = true;
}

void ContextManager::makeAllInactive()
{
	for (const InputContext& context : InputContext::getAllContexts())
	{
		if (context != InputContext::ALWAYS_ACTIVE)
		{
			set(context, InputContext::State::INACTIVE);
		}
	}
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
