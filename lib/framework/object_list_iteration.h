/*
	This file is part of Warzone 2100.
	Copyright (C) 2024  Warzone 2100 Project

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
/** @file
 *  Basic routines for list iteration of various in-game objects (resources, sounds, droids, etc.).
 */
#pragma once

#include <list>
#include <type_traits>
#include <iterator>
#include <functional>

enum class IterationResult
{
	BREAK_ITERATION,
	CONTINUE_ITERATION
};

/// <summary>
/// Utility class used to determine how to invoke `Callable`
/// in the `mutating_list_iterate` function depending on its signature.
///
/// Currently two callable signatures are supported:
/// * `IterationResult(ObjectType*)`
/// * `IterationResult(std::list<ObjectType*>::iterator)`
///
/// The latter overload is convenient when one needs to erase from or
/// insert into the list being iterated directly inside the handler's body,
/// avoiding additional lookups to obtain an iterator to the current element.
/// </summary>
/// <typeparam name="Callable">
/// Can either be a function pointer or a lambda expression (or any other callable).
/// </typeparam>
template <typename Callable>
struct LoopBodyHandlerCallStrategy
{
	// Since we are constrained by C++14, we'll need to use SFINAE to determine correct
	// specialization of `Invoke` to call depending on input type of handler function.
	//
	// Here we use a bunch of compile-time tests to determine
	// whether our callable is convertible to `std::function` of a compatible signature.
	//
	// This is the most simple way to constrain and choose a correct overload
	// of `Invoke` function depending on the callable signature.
	//
	// Choosing the correct overload is done via `std::enable_if<Cond, T>` helper from <type_traits>,
	// which basically provides `using type = T` only when `Cond` is `true`.
	template <typename ObjectType>
	static constexpr bool handler_accepts_ptr = std::is_convertible<
		Callable,
		std::function<IterationResult(ObjectType*)>>::value;
	template <typename ObjectType>
	static constexpr bool handler_accepts_iter = std::is_convertible<
		Callable,
		std::function<IterationResult(typename std::list<ObjectType*>::iterator)>>::value;

	// `Invoke` overload for Callable taking a list iterator as the argument
	template <typename ObjectType>
	static std::enable_if_t<handler_accepts_iter<ObjectType>, IterationResult>
		Invoke(Callable handler, typename std::list<ObjectType*>::iterator iter)
	{
		return handler(iter);
	}

	// `Invoke` overload for Callable taking a pointer to `ObjectType` as the argument
	template <typename ObjectType>
	static std::enable_if_t<handler_accepts_ptr<ObjectType>, IterationResult>
		Invoke(Callable handler, typename std::list<ObjectType*>::iterator iter)
	{
		return handler(*iter);
	}
};

// Common iteration helper for lists of game objects
// with an ability to execute loop body handlers which can
// possibly invalidate the any iterator in the range `[begin(), currentIterator]`.
template <typename ObjectType, typename MaybeErasingLoopBodyHandler>
void mutating_list_iterate(std::list<ObjectType*>& list, MaybeErasingLoopBodyHandler handler)
{
	using HandlerCallStrategy = LoopBodyHandlerCallStrategy<MaybeErasingLoopBodyHandler>;

	static_assert(
		   HandlerCallStrategy::template handler_accepts_ptr<ObjectType>
		|| HandlerCallStrategy::template handler_accepts_iter<ObjectType>,
		"Unsupported loop body handler signature: "
		"should return IterationResult and take either an ObjectType* or an iterator");

	if (list.empty())
	{
		return;
	}

	typename std::remove_reference_t<decltype(list)>::iterator it = list.begin(), itNext;
	while (it != list.end())
	{
		itNext = std::next(it);
		// Can possibly invalidate `it` and anything before it.
		const auto res = HandlerCallStrategy::template Invoke<ObjectType>(handler, it);
		if (res == IterationResult::BREAK_ITERATION)
		{
			break;
		}
		it = itNext;
	}
}

