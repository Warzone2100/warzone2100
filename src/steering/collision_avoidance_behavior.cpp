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
 * @file collision_avoidance_behavior.cpp
 * Collision avoidance steering behavior implementation.
 */

#include "collision_avoidance_behavior.h"
#include "steering.h"
#include "utils.h" // for PRECISION

#include "lib/framework/trig.h"
#include "lib/framework/vector.h"

#include "src/droid.h"
#include "src/droiddef.h"
#include "src/mapgrid.h"
#include "src/map.h"
#include "src/fpath.h"
#include "src/move.h"
#include "src/objects.h"

#include <algorithm>

namespace steering
{

SteeringForce CollisionAvoidanceBehavior::calculate(const SteeringContext& ctx)
{
	int32_t numObstacles = 0;
	int32_t distTotal = 0;
	Vector2i totalDir(0, 0);

	// Can't avoid anything if we can't move
	if (ctx.maxSpeed == 0)
	{
		return SteeringForce(Vector2i(0, 0), 0);
	}

	// Vector to target (for blending)
	Vector2i toTarget = ctx.targetPos - ctx.currentPos;

	// Scan nearby objects for obstacles
	static GridList gridList;  // static to avoid allocations
	gridList = gridStartIterate(ctx.currentPos.x, ctx.currentPos.y, OBSTACLE_SCAN_RADIUS);
	for (GridIterator gi = gridList.begin(); gi != gridList.end(); ++gi)
	{
		BASE_OBJECT* obj = *gi;

		// Skip invalid obstacles
		if (!isValidObstacle(obj, ctx.droid))
		{
			continue;
		}

		DROID* obstacle = castDroid(obj);
		if (!obstacle)
		{
			continue;
		}

		// Get obstacle properties
		PROPULSION_STATS* obstaclePropStats = obstacle->getPropulsionStats();
		int32_t obstacleRadius = moveObjRadius(obstacle);
		int32_t combinedRadius = ctx.radius + obstacleRadius;

		// Estimate obstacle velocity
		Vector2i obstacleVel = estimateObstacleVelocity(obstacle);
		// Find the guessed obstacle speed and direction, clamped to half our speed.
		int32_t obstacleSpeedGuess = std::min(iHypot(obstacleVel), ctx.maxSpeed / 2);
		uint16_t obstDirectionGuess = iAtan2(obstacleVel);

		// Position of obstacle relative to us
		Vector2i obstaclePos = obstacle->pos.xy();
		Vector2i diff = obstaclePos - ctx.currentPos;

		// Predict where the obstacle will be when we get close
		int32_t distToObstacle = iHypot(diff);
		// Find very approximate position of obstacle relative to us when we get close, based on our guesses.
		int64_t predictionFactor = std::max(distToObstacle - combinedRadius * 2 / 3, 0) * obstacleSpeedGuess / ctx.maxSpeed;
		Vector2i deltaDiff = iSinCosR(obstDirectionGuess, predictionFactor);

		// Don't assume obstacle can go through blocking tiles
		if (!fpathBlockingTile(map_coord(obstacle->pos.x + deltaDiff.x),
		                       map_coord(obstacle->pos.y + deltaDiff.y),
		                       obstaclePropStats->propulsionType))
		{
			diff += deltaDiff;
		}

		// Check if obstacle is ahead of us (in direction of movement)
		if (dot(diff, toTarget) < 0)
		{
			// Object is behind us relative to target
			continue;
		}

		// Calculate distance metrics
		int32_t centreDist = std::max(iHypot(diff), 1);
		int32_t effectiveDist = std::max(centreDist - combinedRadius, 1);

		// Accumulate avoidance direction
		// The formula: diff * PRECISION / (centreDist * effectiveDist)
		// This gives more weight to closer obstacles
		totalDir += diff * PRECISION / (centreDist * effectiveDist);
		distTotal += PRECISION / effectiveDist;
		++numObstacles;
	}

	// No obstacles to avoid
	if ((totalDir.x == 0 && totalDir.y == 0) || numObstacles == 0)
	{
		return SteeringForce(Vector2i(0, 0), 0);
	}

	// Average the accumulated direction
	totalDir = Vector2i(totalDir.x / numObstacles, totalDir.y / numObstacles);
	distTotal /= numObstacles;

	// Create perpendicular avoid vector (choose side toward target)
	Vector2i perp = orthogonalCW(totalDir);
	Vector2i avoid = dot(toTarget, perp) < 0 ? -perp : perp;

	// Normalize both destination and avoid vectors
	Vector2i destNorm = toTarget * 32767 / (iHypot(toTarget) + 1);
	// avoid.x and avoid.y are up to 65536, so we can multiply by at most 32767 here without potential overflow.
	Vector2i avoidNorm = avoid * 32767 / (iHypot(avoid) + 1);

	// Calculate blend ratio based on proximity
	int32_t ratio = std::min(distTotal * ctx.radius / 2, PRECISION);

	// Blend destination and avoid vectors
	Vector2i result = destNorm * (PRECISION - ratio) + avoidNorm * ratio;

	int32_t weight = PRECISION; // currently 1.0 magnitude to keep existing behavior
	// Maybe calculate weight based on threat level?
	// // Maximum avoidance weight (cap to prevent over-reaction)
	// static constexpr int32_t MAX_AVOIDANCE_WEIGHT = 49152; // 0.75 in fixed-point
	// int32_t weight = std::min(ratio, MAX_AVOIDANCE_WEIGHT);

	return SteeringForce(result, weight);
}

bool CollisionAvoidanceBehavior::isEnabled(const SteeringContext& ctx) const
{
	if (!ctx.droid)
	{
		return false;
	}
	// Transporters have their own movement logic
	return !ctx.droid->isTransporter();
}

Vector2i CollisionAvoidanceBehavior::estimateObstacleVelocity(DROID* obstacle)
{
	// Velocity guess 1: Guess the velocity the droid is actually moving at.
	Vector2i velocityGuess1 = iSinCosR(obstacle->sMove.moveDir, obstacle->sMove.speed);

	// Velocity guess 2: Guess the velocity the droid wants to move at.
	Vector2i obstaclePos = obstacle->pos.xy();
	Vector2i targetDiff = obstacle->sMove.target - obstaclePos;
	int32_t targetDist = iHypot(targetDiff);

	const PROPULSION_STATS* propStats = obstacle->getPropulsionStats();
	int32_t maxSpeed = propStats->maxSpeed;

	// Scale intended speed by distance (slower when close to target)
	int32_t intendedSpeed = maxSpeed * std::min(targetDist, OBSTACLE_SCAN_RADIUS) / OBSTACLE_SCAN_RADIUS;
	Vector2i velocityGuess2 = iSinCosR(iAtan2(targetDiff), intendedSpeed);

	// If blocked, assume no intended movement
	if (moveBlocked(obstacle))
	{
		velocityGuess2 = Vector2i(0, 0);
	}

	// Average the two guesses
	return (velocityGuess1 + velocityGuess2) / 2;
}

bool CollisionAvoidanceBehavior::isValidObstacle(const BASE_OBJECT* obj, const DROID* ourDroid)
{
	// Skip ourselves
	if (obj == static_cast<const BASE_OBJECT*>(ourDroid))
	{
		return false;
	}

	// Only consider droids
	const DROID* obstacle = castDroid(const_cast<BASE_OBJECT*>(obj));
	if (!obstacle)
	{
		return false;
	}

	// VTOL droids only avoid each other and don't affect ground droids
	if (ourDroid->isVtol() != obstacle->isVtol())
	{
		return false;
	}

	// Don't avoid transporters
	if (obstacle->isTransporter())
	{
		return false;
	}

	// Don't avoid people on the other side - run over them
	if (obstacle->droidType == DROID_PERSON && obstacle->player != ourDroid->player)
	{
		return false;
	}

	return true;
}

} // namespace steering
