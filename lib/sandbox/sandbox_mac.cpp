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

/**
 * The macOS backend, which reports status.
 *
 * The App Sandbox entitlements in platforms/macos/Resources/Warzone.entitlements confine the app from
 * launch, and the Hardened Runtime is a property of the signature, so there is nothing to apply here.
 */

#include "sandbox_internal.h"

#include <cstdlib>

namespace wz::sandbox
{

void backendHarden()
{
}

Status backendConfine(const Plan& plan, Tier requested)
{
	(void)plan;
	(void)requested;

	Status result;
	if (getenv("APP_SANDBOX_CONTAINER_ID") == nullptr)
	{
		result.detail = "no app sandbox container";
		return result;
	}

	// The container confines writes, and the requested tier has no bearing on it.
	result.active = true;
	result.effective = Tier::WriteConfined;
	result.backend = "macos-app-sandbox";
	result.detail = "app sandbox container";
	return result;
}

} // namespace wz::sandbox
