/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project

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

#include "netjoin.h"

std::string JoinConnectionDescription::connectiontype_to_string(JoinConnectionDescription::JoinConnectionType type)
{
	switch (type)
	{
	case JoinConnectionDescription::JoinConnectionType::TCP_DIRECT: return "tcp";
#ifdef WZ_GNS_NETWORK_BACKEND_ENABLED
	case JoinConnectionDescription::JoinConnectionType::GNS_DIRECT: return "gns";
#endif
	}
	return {}; // silence compiler warning
}

optional<JoinConnectionDescription::JoinConnectionType> JoinConnectionDescription::connectiontype_from_string(const std::string& str)
{
	if (str == "tcp")
	{
		return JoinConnectionDescription::JoinConnectionType::TCP_DIRECT;
	}
#ifdef WZ_GNS_NETWORK_BACKEND_ENABLED
	if (str == "gns")
	{
		return JoinConnectionDescription::JoinConnectionType::GNS_DIRECT;
	}
#endif
	return nullopt;
}
