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
#include <list>

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

struct InputContext;
typedef std::list<std::reference_wrapper<InputContext>> InputContexts;

struct InputContext
{
	/* Always enabled. Actions in this context will always execute. Bindings with collisions to ALWAYS_ACTIVE keybinds are not valid, as they would never get executed */
	static const InputContext ALWAYS_ACTIVE;
	/* Background-level keybinds. These run at the lowest priority, and are not executed if _anything_ else is bound to the keys */
	static const InputContext BACKGROUND;
	/* Generic gameplay (main viewport) keybindings. Unit commands etc. */
	static const InputContext GAMEPLAY;
	/* Active while player hovers over the radar. */
	static const InputContext RADAR;

	/* For debug-only bindings. */
	static const InputContext __DEBUG;

	static const InputContexts getAllContexts();

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

	/* Display name to be shown in e.g. edit keymap options */
	const std::string getDisplayName() const;

private:
	static InputContexts contexts;

	InputContext(const ContextPriority priority, const State initialState, const char* const displayName);

	const ContextPriority priority;
	const unsigned int index;
	const std::string displayName;
	const State defaultState;

	friend bool operator==(const InputContext& lhs, const InputContext& rhs);
	friend class InputManager;
};

bool operator==(const InputContext& lhs, const InputContext& rhs);
bool operator!=(const InputContext& lhs, const InputContext& rhs);

#endif // __INCLUDED_SRC_INPUT_CONTEXT_H__
