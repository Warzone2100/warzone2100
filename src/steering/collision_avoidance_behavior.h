// SPDX-License-Identifier: GPL-2.0-or-later

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
/**
 * @file collision_avoidance_behavior.h
 * Collision avoidance steering behavior.
 *
 * This behavior detects nearby obstacles and calculates
 * an avoidance force to prevent collisions.
 *
 * Uses integer-only arithmetic.
 */

#pragma once

#include "steering.h"
#include "lib/framework/vector.h"
#include "lib/wzmaplib/include/wzmaplib/map.h"  // For TILE_UNITS

struct DROID;
struct BASE_OBJECT;

namespace steering
{

/// <summary>
/// Collision avoidance steering behavior.
///
/// This behavior implements collision avoidance by:
/// 1. Detecting obstacles in a look-ahead cone
/// 2. Predicting obstacle positions based on velocity
/// 3. Calculating avoidance forces perpendicular to threats
/// 4. Scaling forces by proximity and urgency
///
/// The algorithm is loosely based on Craig Reynolds' steering behaviors paper (see
/// https://www.researchgate.net/publication/2495826_Steering_Behaviors_For_Autonomous_Characters
/// and https://www.red3d.com/cwr/steer/ for additional guidance),
/// adapted for integer-only arithmetic for deterministic behavior.
/// </summary>
class CollisionAvoidanceBehavior : public ISteeringBehavior
{
public:

	/// Radius to scan for obstacles (world units)
	static constexpr int32_t OBSTACLE_SCAN_RADIUS = TILE_UNITS * 2;


	// Calculate collision avoidance force for the given context.
	SteeringForce calculate(const SteeringContext& ctx) override;

	const char* name() const override { return "CollisionAvoidance"; }

	// Disabled for transporters (they have their own movement logic).
	bool isEnabled(const SteeringContext& ctx) const override;

private:

	/// <summary>
	/// Estimate obstacle velocity.
	///
	/// Uses two guesses:
	/// 1. Actual velocity (moveDir * speed)
	/// 2. Intended velocity (toward target)
	/// </summary>
	/// <param name="obstacle">The obstacle droid</param>
	/// <returns>Estimated velocity vector</returns>
	static Vector2i estimateObstacleVelocity(DROID* obstacle);

	/// <summary>
	/// Check if an object is a valid obstacle for collision avoidance.
	/// </summary>
	/// <param name="obj">The object to check</param>
	/// <param name="ourDroid">Our droid</param>
	/// <returns>`true` if the object should be avoided</returns>
	static bool isValidObstacle(const BASE_OBJECT* obj, const DROID* ourDroid);
};

} // namespace steering
