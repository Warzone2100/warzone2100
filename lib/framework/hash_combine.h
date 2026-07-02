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
/** @file hash_combine.h
 * Boost-style `hash_combine` helper for mixing values into a `std::size_t` seed.
 */


#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

inline void hash_combine(std::size_t& seed) { }

template <typename T, typename... Rest>
inline void hash_combine(std::size_t& seed, const T& v, Rest... rest)
{
	std::hash<T> hasher;
#if SIZE_MAX >= UINT64_MAX
	seed ^= hasher(v) + 0x9e3779b97f4a7c15L + (seed << 6) + (seed >> 2);
#else
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
#endif
	hash_combine(seed, rest...);
}
