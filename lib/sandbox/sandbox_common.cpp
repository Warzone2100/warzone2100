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

#include "sandbox.h"
#include "sandbox_internal.h"

#include <cstring>

namespace wz::sandbox
{

static Status lastStatus;

void harden()
{
	backendHarden();
}

Status confine(const Plan& plan, Tier requested)
{
	lastStatus = backendConfine(plan, requested);
	return lastStatus;
}

const Status& status()
{
	return lastStatus;
}

bool parseTier(const char* text, Tier& out)
{
	if (text == nullptr)
	{
		return false;
	}
	if (strcmp(text, "off") == 0 || strcmp(text, "0") == 0)
	{
		out = Tier::Off;
		return true;
	}
	if (strcmp(text, "write") == 0)
	{
		out = Tier::WriteConfined;
		return true;
	}
	if (strcmp(text, "strict") == 0)
	{
		out = Tier::Strict;
		return true;
	}
	return false;
}

const char* tierName(Tier t)
{
	switch (t)
	{
	case Tier::Off:
		return "off";
	case Tier::WriteConfined:
		return "write";
	case Tier::Strict:
		return "strict";
	}
	return "off";
}

} // namespace wz::sandbox
