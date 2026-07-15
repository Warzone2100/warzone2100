// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
/** @file load_result.h
 * Result types for cooperative loading coroutines (see loading_task.h).
 */

#pragma once

#include <optional>
#include <type_traits>
#include <utility>

#include <tl/expected.hpp>

struct LoadError
{};

template <typename T = void>
using LoadResult = tl::expected<T, LoadError>;

inline LoadResult<> load_ok() noexcept
{
	return {};
}

template <typename T>
inline LoadResult<T> load_ok(T value)
{
	return value;
}

inline auto load_fail() noexcept
{
	return tl::unexpected(LoadError{});
}

/// Lazy promise-side storage: no `LoadResult<T>` until `set()`; consume with `take()`.
template <typename T>
	requires std::is_move_constructible_v<T> || std::is_void_v<T>
struct LoadResultStorage
{
	std::optional<LoadResult<T>> value;

	bool has_outcome() const noexcept
	{
		return value.has_value();
	}

	bool succeeded() const noexcept
	{
		return value && value->has_value();
	}

	void set(LoadResult<T> r) noexcept
	{
		value = std::move(r);
	}

	void set(tl::unexpected<LoadError> error) noexcept
	{
		value = std::move(error);
	}

	template <typename U = T>
		requires (!std::is_void_v<U>)
	void set(U v)
	{
		value = std::move(v);
	}

	LoadResult<T> take() noexcept
	{
		if (!value)
		{
			return load_fail();
		}
		return std::move(*std::exchange(value, std::nullopt));
	}
};
