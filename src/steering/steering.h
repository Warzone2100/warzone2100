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
 * @file steering.h
 * Core steering behaviors interface and types.
 *
 * Provides a modular and extensible steering behavior system
 * for unit movement. All calculations use integer-only arithmetic for
 * deterministic multiplayer synchronization.
 */

#pragma once

#include "lib/framework/vector.h"
#include <memory>
#include <vector>

struct DROID;

namespace steering
{

/// <summary>
/// Steering force vector with weight for blending.
/// The force vector uses fixed-point precision where 65536 represents
/// a magnitude of 1.0.
///
/// The weight is used when combining multiple steering forces together.
/// </summary>
struct SteeringForce
{
	// Force vector in fixed-point precision
	Vector2i force = {0, 0};
	// Weight for blending (in PRECISION units, i.e. weight == 65536 acts as a 1.0 multiplier)
	int32_t weight = 0;

	explicit SteeringForce() = default;
	explicit SteeringForce(Vector2i f, int32_t w) : force(f), weight(w) {}

	/// Check if this force has any effect
	bool isZero() const { return force.x == 0 && force.y == 0 && weight == 0; }
};

/// <summary>
/// Context provided to each steering behavior for calculations.
///
/// Contains all the information a steering behavior might need
/// to calculate its force contribution.
/// </summary>
struct SteeringContext
{
	// The droid currently being steered
	const DROID* droid = nullptr;
	// Current position in world coordinates
	Vector2i currentPos = {0, 0};
	// Target/destination position
	Vector2i targetPos = {0, 0};
	// Current velocity vector
	Vector2i velocity = {0, 0};
	// Current movement direction (0-65535 = 0-360 degrees)
	uint16_t moveDir = 0;
	// Current speed
	int32_t speed = 0;
	// Max speed from droid's propulsion stats
	int32_t maxSpeed = 0;
	// Droid collision radius
	int32_t radius = 0;
};

/// <summary>
/// Abstract interface for steering behaviors.
///
/// Each steering behavior should implement this interface to calculate
/// a steering force contribution.
///
/// Behaviors are combined by the `SteeringManager` using weighted force sum.
/// </summary>
class ISteeringBehavior
{
public:
	virtual ~ISteeringBehavior() = default;

	/// <summary>
	/// Calculate steering force for the given context.
	/// </summary>
	/// <param name="ctx">The steering context containing droid state</param>
	/// <returns>The calculated steering force with weight</returns>
	virtual SteeringForce calculate(const SteeringContext& ctx) = 0;

	/// <summary>
	/// Steering behavior name for debugging.
	/// </summary>
	/// <returns>Null-terminated string with behavior name</returns>
	virtual const char* name() const = 0;

	/// <summary>
	/// Check if this behavior is enabled for the given context.
	///
	/// Override to conditionally disable behaviors based on
	/// droid state (e.g., VTOLs might skip ground collision avoidance).
	/// </summary>
	/// <param name="ctx">The steering context</param>
	/// <returns>`true` if the behavior should be applied</returns>
	virtual bool isEnabled(const SteeringContext& ctx) const { return true; }
};

/// <summary>
/// Manager class for combining multiple steering behaviors.
///
/// The `SteeringManager` owns a collection of steering behaviors
/// and combines their forces using weighted summation. The final
/// result is a direction vector that can be used for movement.
/// </summary>
class SteeringManager
{
public:
	explicit SteeringManager() = default;
	~SteeringManager();

	SteeringManager(const SteeringManager&) = delete;
	SteeringManager& operator=(const SteeringManager&) = delete;
	SteeringManager(SteeringManager&&) = default;
	SteeringManager& operator=(SteeringManager&&) = default;

	/// <summary>
	/// Add a behavior to the manager.
	///
	/// Behaviors are evaluated in the order they are added.
	/// </summary>
	/// <param name="behavior">The steering behavior to add</param>
	void addBehavior(std::unique_ptr<ISteeringBehavior> behavior);

	/// <summary>
	/// Calculate combined steering vector.
	///
	/// Evaluates all enabled behaviors and combines their forces
	/// using weighted summation.
	/// </summary>
	/// <param name="ctx">The steering context</param>
	/// <returns>Combined steering vector in fixed-point precision</returns>
	Vector2i calculateSteering(const SteeringContext& ctx);

	/// <summary>
	/// Calculate combined steering as a direction angle.
	/// Same as `calculateSteering()` but returns the result as an
	/// angle suitable for use with the current movement system.
	/// </summary>
	/// <param name="ctx">The steering context</param>
	/// <returns>Direction angle (0-65535 = 0-360 degrees)</returns>
	uint16_t calculateSteeringDirection(const SteeringContext& ctx);

private:

	// Combine multiple forces using weighted average.
	static Vector2i combineForcesImpl(const std::vector<SteeringForce>& forces);

	// Cached storage for calculated steering forces, used to reduce
	// the number of allocations on the hot path
	std::vector<SteeringForce> _forces;
	std::vector<std::unique_ptr<ISteeringBehavior>> _behaviors;
};

} // namespace steering
