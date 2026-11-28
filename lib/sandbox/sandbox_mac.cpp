// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version, as published
	by the Warzone 2100 Project. They are provided in COPYING.SECTION7 in
	the top-level directory of the source distribution. This notice must
	be kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
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

void backendHarden(Tier requested)
{
	(void)requested;
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
