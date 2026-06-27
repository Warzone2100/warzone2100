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

/** \file
 * Pure interpolation math for the smooth terrain surface (rendering only).
 *
 * This header is deliberately self-contained (no game / framework includes)
 * so it can be unit-tested standalone (tests/terrain_surface_test.cpp).
 *
 * The surface is a monotone (Fritsch-Carlson clamped) bicubic Catmull-Rom
 * interpolation of the tile-corner height lattice, blended per-cell toward
 * the legacy piecewise-linear 4-triangle "fan" surface by a per-corner
 * sharpness factor (1.0 near cliffs = exactly the legacy surface).
 *
 * Guarantees (covered by the unit tests):
 *  - interpolates the corner heights exactly
 *  - C1-continuous across cell (tile) and sector boundaries where sharpness is 0
 *  - never leaves [min, max] of the cell's 2x2 corner heights (no ringing
 *    next to cliffs / steps)
 *  - reproduces linear ramps and plateaus exactly
 *  - evaluation at a cell edge depends only on lattice values whose stencil
 *    both adjacent cells share => crack-free between adjacent cells
 */

#pragma once

#include <algorithm>
#include <cmath>

namespace terrainSurfaceMath
{

/// Fritsch-Carlson-clamped Catmull-Rom tangent at the lattice point between
/// the two secants d01 (before) and d12 (after).
/// Zero at local extrema / plateaus, magnitude limited to 3x the smaller
/// adjacent secant: sufficient for the cubic to stay monotone per interval.
inline float clampedTangent(float d01, float d12)
{
	if (d01 * d12 <= 0.f)
	{
		return 0.f;
	}
	float m = 0.5f * (d01 + d12);
	float bound = 3.f * std::min(std::fabs(d01), std::fabs(d12));
	return std::copysign(std::min(std::fabs(m), bound), m);
}

/// Monotone cubic interpolation of the interval [p1, p2] at t in [0, 1],
/// using the surrounding samples p0 and p3 for the (clamped) tangents.
inline float monotoneCubic(float p0, float p1, float p2, float p3, float t)
{
	float d01 = p1 - p0;
	float d12 = p2 - p1;
	float d23 = p3 - p2;
	float m1 = clampedTangent(d01, d12);
	float m2 = clampedTangent(d12, d23);
	float t2 = t * t;
	float t3 = t2 * t;
	return (2.f * t3 - 3.f * t2 + 1.f) * p1
	     + (t3 - 2.f * t2 + t) * m1
	     + (-2.f * t3 + 3.f * t2) * p2
	     + (t3 - t2) * m2;
}

/// Monotone bicubic interpolation of the central cell of a 4x4 lattice patch.
/// p[x][y] are the lattice samples. The cell spans indices 1..2 in each axis,
/// and (tx, ty) in [0, 1] is the position within that cell.
inline float monotoneBicubic(const float p[4][4], float tx, float ty)
{
	float q0 = monotoneCubic(p[0][0], p[1][0], p[2][0], p[3][0], tx);
	float q1 = monotoneCubic(p[0][1], p[1][1], p[2][1], p[3][1], tx);
	float q2 = monotoneCubic(p[0][2], p[1][2], p[2][2], p[3][2], tx);
	float q3 = monotoneCubic(p[0][3], p[1][3], p[2][3], p[3][3], tx);
	return monotoneCubic(q0, q1, q2, q3, ty);
}

/// The legacy piecewise-linear surface of one tile: 4 triangles fanned from
/// the center vertex (= average of the 4 corners), matching the geometry the
/// renderer has always produced and map_Height() reproduces.
/// h00 is the corner at (0,0), h10 at (1,0), h01 at (0,1), h11 at (1,1).
inline float fanSurface(float h00, float h10, float h01, float h11, float tx, float ty)
{
	float c = 0.25f * (h00 + h10 + h01 + h11);
	// the tile is split into 4 triangles by its diagonals
	if (ty <= tx && ty <= 1.f - tx)
	{
		// bottom triangle: (0,0) (1,0) center
		return h00 + (h10 - h00) * (tx - ty) + (c - h00) * (2.f * ty);
	}
	if (ty >= tx && ty >= 1.f - tx)
	{
		// top triangle: (0,1) (1,1) center
		return h01 + (h11 - h01) * (tx - (1.f - ty)) + (c - h01) * (2.f * (1.f - ty));
	}
	if (tx <= 0.5f)
	{
		// left triangle: (0,0) (0,1) center
		return h00 + (h01 - h00) * (ty - tx) + (c - h00) * (2.f * tx);
	}
	// right triangle: (1,0) (1,1) center
	return h10 + (h11 - h10) * (tx + ty - 1.f) + (c - h10) * (2.f * (1.f - tx));
}

/// Bilinear interpolation of per-corner values (s00 at (0,0), s10 at (1,0), ...)
inline float bilinear(float s00, float s10, float s01, float s11, float tx, float ty)
{
	float b = s00 + (s10 - s00) * tx;
	float t = s01 + (s11 - s01) * tx;
	return b + (t - b) * ty;
}

/// One-sided crease fillet helper: remaps a non-negative height drop d below a
/// plateau (equivalently: rise above a floor) within 2*r of it so the surface
/// leaves the plateau tangentially - g(0) = g'(0) = 0 turns the crease into a
/// C1 junction - and is exactly d beyond (g(2r) = 2r, g'(2r) = 1).
/// Result is in [0, d]: heights only move toward the plateau, never past it,
/// so the plateau side itself is untouched (no "moat" below cliff lips).
inline float filletDrop(float d, float r)
{
	if (r <= 0.f || d >= 2.f * r)
	{
		return d;
	}
	if (d <= 0.f)
	{
		return 0.f;
	}
	const float t = d / r; // in (0, 2)
	return r * t * t * (1.f - 0.25f * t);
}

/// The legacy fan surface with its step creases filleted: the face is eased
/// tangentially into the local plateau top (hi) and floor (lo) levels within a
/// fillet radius of min(rMax, (hi - lo) / 4) - which is zero on flat ground, so
/// this equals fanSurface() away from steps, and the two fillet zones can never
/// overlap. Requires lo <= all corner heights <= hi.
inline float filletedFanSurface(float h00, float h10, float h01, float h11,
                                float hi, float lo, float rMax,
                                float tx, float ty)
{
	const float fan = fanSurface(h00, h10, h01, h11, tx, ty);
	const float r = std::min(rMax, (hi - lo) * 0.25f);
	if (r <= 0.f)
	{
		return fan;
	}
	// round the lip (approach to the plateau top), then the foot (approach to the floor)
	float h = hi - filletDrop(hi - fan, r);
	h = lo + filletDrop(h - lo, r);
	return h;
}

/// The blended surface height for the central cell of a 4x4 lattice patch:
/// monotone bicubic, lerped toward the legacy fan surface by the bilinearly
/// interpolated per-corner sharpness s (s = 1 => exactly the legacy surface).
/// s00 is the sharpness of the corner at cell (0,0) = lattice index (1,1), etc.
inline float blendedSurfaceHeight(const float p[4][4],
                                  float s00, float s10, float s01, float s11,
                                  float tx, float ty)
{
	float smooth = monotoneBicubic(p, tx, ty);
	float sharp = bilinear(s00, s10, s01, s11, tx, ty);
	if (sharp <= 0.f)
	{
		return smooth;
	}
	float fan = fanSurface(p[1][1], p[2][1], p[1][2], p[2][2], tx, ty);
	return smooth + (fan - smooth) * sharp;
}

} // namespace terrainSurfaceMath
