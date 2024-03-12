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

#ifndef __INCLUDED_SRC_INPUT_CONTEXT_H__
#define __INCLUDED_SRC_INPUT_CONTEXT_H__

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

void registerDefaultContexts(class ContextManager& contextManager, class DebugInputManager& dbgInputManager);

struct ContextPriority
{
	const unsigned int prioritized;
	const unsigned int active;

	ContextPriority(unsigned int value)
		: ContextPriority(value, value)
	{
	}
	ContextPriority(unsigned int prioritized, unsigned int active)
		: prioritized(prioritized)
		, active(active)
	{
	}
};

typedef const char* ContextId;

struct InputContext
{
	/* Always enabled. Actions in this context will always execute. Bindings with collisions to ALWAYS_ACTIVE keybinds are not valid, as they would never get executed */
	static const ContextId ALWAYS_ACTIVE;
	/* Background-level keybinds. These run at the lowest priority, and are not executed if _anything_ else is bound to the keys */
	static const ContextId BACKGROUND;
	/* Generic gameplay (main viewport) keybindings. Unit commands etc. */
	static const ContextId GAMEPLAY;
	/* Active while player hovers over the radar. */
	static const ContextId RADAR;

	/* Debug only, "level editor" mappings */
	static const ContextId DEBUG_LEVEL_EDITOR;
	/* Debug only, active when a unit is selected */
	static const ContextId DEBUG_HAS_SELECTION;
	/* Debug only, other mappings */
	static const ContextId DEBUG_MISC;


	enum class State
	{
		/*
		 The input context is prioritized. e.g. `InputContext::RADAR` is prioritized when player hovers
		 over the radar, prioritizing any bindings belonging to that context.
		 */
		PRIORITIZED,

		/*
		 The input context is active. Any bindings will use the lower `active` priority.
		 */
		ACTIVE,

		/*
		 The input context is inactive. Bindings in the context are ignored.
		 */
		INACTIVE,
	};

	/* Predicate for checking if the context should be prioritized. */
	typedef std::function<bool()> PriorityCondition;

	/* Display name to be shown in e.g. edit keymap options */
	const std::string getDisplayName() const;

	/* Returns boolean indicating whether or not the context is always active. Always active contexts
	   cannot be disabled e.g. via `ContextManager::makeAllInactive()`. This also means that any mappings
	   in always active contexts will conflict with mappings from other contexts (as they would override
	   them anyway). */
	bool isAlwaysActive() const;

public:
	InputContext(const ContextId id, const bool bIsAlwaysActive, const ContextPriority priority, const State initialState, const char* const displayName);

	InputContext(const ContextId id, const bool bIsAlwaysActive, const ContextPriority priority, const State initialState, const char* const displayName, const PriorityCondition condition);

private:
	const ContextId id;
	const bool bIsAlwaysActive;
	const ContextPriority priority;
	const unsigned int index;
	const std::string displayName;
	const State defaultState;
	const PriorityCondition condition;

	friend bool operator==(const InputContext& lhs, const InputContext& rhs);
	friend class ContextManager;
};

bool operator==(const InputContext& lhs, const InputContext& rhs);
bool operator!=(const InputContext& lhs, const InputContext& rhs);


class ContextManager
{
public:
	/* Resets all context states to their defaults. */
	void resetStates();

	/* Makes all contexts inactive. Used to disable key mappings while in design screen. */
	void makeAllInactive();

	/* Pushes the current state of the input contexts to a state stack. More specifically, creates
	   a copy of the current context states and pushes the copy at the top of the stack. */
	void pushState();

	/* Pops state of the input contexts from the state stack. */
	void popState();

	/* Gets the information about the context with the given ID */
	const InputContext& get(const ContextId& contextId) const;

	/* Updates the state of a single context. Use this to prioritize/enable/disable contexts. */
	void set(const ContextId& contextId, const InputContext::State state);

	/*
	 Gets the status of the context. If this returns `false`, any bindings belonging to the context
	 should not be processed.
	 */
	bool isActive(const ContextId& context) const;

	/* Priority of the context when resolving collisions. Context with highest priority wins */
	unsigned int getPriority(const ContextId context) const;

	/* Updates the priority status (ACTIVE/PRIORITIZED) of contexts based on their conditions */
	void updatePriorityStatus();

public:
	void registerContext(InputContext context);

private:
	bool isDirty() const;

	void clearDirty();

	std::unordered_map<std::string, InputContext> contexts;
	std::vector<std::vector<InputContext::State>> states;
	bool bDirty;

	friend class InputManager;
};

#endif // __INCLUDED_SRC_INPUT_CONTEXT_H__
