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
	additional terms, or (at your option) any later version published by
	the Warzone 2100 Project, set out in COPYING.SECTION7 in the
	top-level directory of the source distribution. This notice must be
	kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/
/**
 * @file steering.cpp
 * Steering manager implementation.
 */

#include "steering.h"
#include "utils.h" // for PRECISION

#include "lib/framework/trig.h"

namespace steering
{

SteeringManager::~SteeringManager() = default;

void SteeringManager::addBehavior(std::unique_ptr<ISteeringBehavior> behavior)
{
	_behaviors.emplace_back(std::move(behavior));
	_forces.reserve(_behaviors.size());
}

Vector2i SteeringManager::calculateSteering(const SteeringContext& ctx)
{
	_forces.clear();

	for (const auto& behavior : _behaviors)
	{
		if (behavior->isEnabled(ctx))
		{
			SteeringForce force = behavior->calculate(ctx);
			if (!force.isZero())
			{
				_forces.push_back(force);
			}
		}
	}

	return combineForcesImpl(_forces);
}

uint16_t SteeringManager::calculateSteeringDirection(const SteeringContext& ctx)
{
	Vector2i steering = calculateSteering(ctx);

	// If no steering force, use direction to target
	if (steering.x == 0 && steering.y == 0)
	{
		Vector2i toTarget = ctx.targetPos - ctx.currentPos;
		return iAtan2(toTarget);
	}

	return iAtan2(steering);
}

Vector2i SteeringManager::combineForcesImpl(const std::vector<SteeringForce>& forces)
{
	if (forces.empty())
	{
		return Vector2i(0, 0);
	}

	// Calculate weighted sum of forces
	int64_t totalX = 0;
	int64_t totalY = 0;
	int64_t totalWeight = 0;

	for (const auto& f : forces)
	{
		// Weight the force by its weight value
		// Force is already in fixed-point, weight is also fixed-point
		// Result: force * weight / PRECISION
		int64_t weightedX = static_cast<int64_t>(f.force.x) * f.weight / PRECISION;
		int64_t weightedY = static_cast<int64_t>(f.force.y) * f.weight / PRECISION;

		totalX += weightedX;
		totalY += weightedY;
		totalWeight += f.weight;
	}

	if (totalWeight == 0)
	{
		return Vector2i(0, 0);
	}

	// Normalize by total weight to get average direction
	// Scale back to PRECISION units
	int32_t resultX = static_cast<int64_t>(totalX) * PRECISION / totalWeight;
	int32_t resultY = static_cast<int64_t>(totalY) * PRECISION / totalWeight;

	return Vector2i(resultX, resultY);
}

} // namespace steering
