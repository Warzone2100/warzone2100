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

#pragma once

#include "lib/framework/wzglobal.h" // for WZ_ZSTD_COMPRESSION_ADAPTER_ENABLED

#include <string>
#include <system_error>

/// <summary>
/// Custom error category class, which acts exactly as `std::system_category()` except
/// that the error messages are always translated to english representation (locale-agnostic behavior).
///
/// Please see the bug https://github.com/microsoft/STL/issues/3254 for the explanation
/// as to why we would need to use a custom error category (at least on Windows).
///
/// NOTE: Currently, for OSes other than Windows, error messages are obtained via
/// `strerror()` function, which is influenced by the `LC_MESSAGES` locale category.
/// This shouldn't be a problem, though, as opposed to Windows, which suffers from the
/// aforementioned bug in MSVC STL.
/// </summary.
class GenericSystemErrorCategory : public std::error_category
{
public:

	constexpr GenericSystemErrorCategory() = default;

	const char* name() const noexcept override
	{
		return "generic system (locale-safe)";
	}

	/// This method provides locale-agnostic mapping from the underlying POSIX (or Windows)
	/// error codes to the human-readable error message strings.
	std::string message(int ev) const override;
	/// This method overload is needed to support the comparison of `std::error_code:s`
	/// bound to our custom `GenericSystemErrorCategory` to the values
	/// of `std::errc` enumeration (which transparently map to `std::error_condition:s`
	/// bound to the `std::system_category()`).
	std::error_condition default_error_condition(int ev) const noexcept override;
};

/// <summary>
/// Custom error category which maps error codes from `getaddrinfo()` function to
/// the appropriate error messages.
/// </summary>
class GetaddrinfoErrorCategory : public std::error_category
{
public:

	constexpr GetaddrinfoErrorCategory() = default;

	const char* name() const noexcept override
	{
		return "getaddrinfo";
	}

	std::string message(int ev) const override;
};

/// <summary>
/// Custom error category which maps some of the error codes from zlib to
/// the appropriate error messages.
/// </summary>
class ZlibErrorCategory : public std::error_category
{
public:

	constexpr ZlibErrorCategory() = default;

	const char* name() const noexcept override
	{
		return "zlib";
	}

	std::string message(int ev) const override;
};

#ifdef WZ_ZSTD_COMPRESSION_ADAPTER_ENABLED
/// <summary>
/// Custom error category which maps zstd error codes (values from
/// the `ZSTD_ErrorCode` enum) to human-readable error messages via
/// `ZSTD_getErrorString()`.
///
/// Callers should convert a `size_t` returned by streaming zstd functions
/// to the stable enum value with `ZSTD_getErrorCode(res)` before passing
/// it to `make_zstd_error_code()`.
/// </summary>
class ZstdErrorCategory : public std::error_category
{
public:

	constexpr ZstdErrorCategory() = default;

	const char* name() const noexcept override
	{
		return "zstd";
	}

	std::string message(int ev) const override;
};
#endif // WZ_ZSTD_COMPRESSION_ADAPTER_ENABLED

const std::error_category& generic_system_error_category();
const std::error_category& getaddrinfo_error_category();
const std::error_category& zlib_error_category();
#ifdef WZ_ZSTD_COMPRESSION_ADAPTER_ENABLED
const std::error_category& zstd_error_category();
#endif

std::error_code make_network_error_code(int ev);
std::error_code make_getaddrinfo_error_code(int ev);
std::error_code make_zlib_error_code(int ev);
#ifdef WZ_ZSTD_COMPRESSION_ADAPTER_ENABLED
std::error_code make_zstd_error_code(int ev);
#endif
