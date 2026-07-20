// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2025  Warzone 2100 Project

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

#include <string>
#include <system_error>

namespace gns
{

class GNSGeneralErrorCategory : public std::error_category
{
public:

	constexpr GNSGeneralErrorCategory() = default;

	const char* name() const noexcept override
	{
		return "GNS general error codes (described by EResult)";
	}

	std::string message(int ev) const override;
	std::error_condition default_error_condition(int ev) const noexcept override;
};

const std::error_category& gns_general_error_category();

std::error_code make_gns_error_code(int ev);

} // namespace gns
