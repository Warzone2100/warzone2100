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

// Standalone unit tests for src/terrain_surface_math.h (no framework, no
// game dependencies). Build and run:
//   c++ -std=c++20 -Isrc tests/terrain_surface_test.cpp -o terrain_surface_test && ./terrain_surface_test
// or via CMake with -DWZ_BUILD_TERRAIN_SURFACE_TEST=ON (target: terrain_surface_test).
// Exits nonzero on failure.

#include "terrain_surface_math.h"

#include <cstdio>
#include <cmath>
#include <random>

using namespace terrainSurfaceMath;

static int failures = 0;
static int checks = 0;

#define CHECK_NEAR(actual, expected, tolerance, ...) \
	do { \
		checks++; \
		float a_ = (actual), e_ = (expected); \
		if (!(std::fabs(a_ - e_) <= (tolerance))) { \
			failures++; \
			std::printf("FAIL %s:%d: %g != %g (+/- %g): ", __FILE__, __LINE__, a_, e_, (float)(tolerance)); \
			std::printf(__VA_ARGS__); \
			std::printf("\n"); \
		} \
	} while (0)

#define CHECK_TRUE(cond, ...) \
	do { \
		checks++; \
		if (!(cond)) { \
			failures++; \
			std::printf("FAIL %s:%d: ", __FILE__, __LINE__); \
			std::printf(__VA_ARGS__); \
			std::printf("\n"); \
		} \
	} while (0)

static std::mt19937 rng(1234567);

static float randomHeight()
{
	std::uniform_real_distribution<float> dist(0.f, 510.f); // TILE_MAX_HEIGHT range
	return dist(rng);
}

static void fillRandomPatch(float p[4][4])
{
	for (int a = 0; a < 4; a++)
		for (int b = 0; b < 4; b++)
			p[a][b] = randomHeight();
}

// MARK: - 1-D monotone cubic

static void testCubicEndpoints()
{
	for (int trial = 0; trial < 1000; trial++)
	{
		float p0 = randomHeight(), p1 = randomHeight(), p2 = randomHeight(), p3 = randomHeight();
		CHECK_NEAR(monotoneCubic(p0, p1, p2, p3, 0.f), p1, 1e-4f, "endpoint t=0 (trial %d)", trial);
		CHECK_NEAR(monotoneCubic(p0, p1, p2, p3, 1.f), p2, 1e-4f, "endpoint t=1 (trial %d)", trial);
	}
}

static void testCubicBounds()
{
	// never leaves [min(p1,p2), max(p1,p2)] - the no-ringing guarantee
	for (int trial = 0; trial < 2000; trial++)
	{
		float p0 = randomHeight(), p1 = randomHeight(), p2 = randomHeight(), p3 = randomHeight();
		float lo = std::min(p1, p2), hi = std::max(p1, p2);
		for (int k = 0; k <= 64; k++)
		{
			float v = monotoneCubic(p0, p1, p2, p3, k / 64.f);
			CHECK_TRUE(v >= lo - 1e-3f && v <= hi + 1e-3f,
			           "overshoot: %g outside [%g, %g] at t=%g (trial %d)", v, lo, hi, k / 64.f, trial);
		}
	}
}

static void testCubicReproducesLinear()
{
	// linear ramps and constants are reproduced exactly (flat terrain stays flat)
	for (int trial = 0; trial < 200; trial++)
	{
		float base = randomHeight();
		float slope = randomHeight() / 10.f - 25.f;
		for (int k = 0; k <= 16; k++)
		{
			float t = k / 16.f;
			float v = monotoneCubic(base, base + slope, base + 2 * slope, base + 3 * slope, t);
			CHECK_NEAR(v, base + slope * (1.f + t), 2e-3f, "linear ramp (trial %d)", trial);
		}
	}
}

static void testCubicPlateauNextToStep()
{
	// a step [0, 0, 100, 100]: the flat intervals stay exactly flat
	// (plain Catmull-Rom would dip below 0 before the step and bulge above 100 after)
	for (int k = 0; k <= 32; k++)
	{
		float t = k / 32.f;
		CHECK_NEAR(monotoneCubic(0.f, 0.f, 0.f, 100.f, t), 0.f, 1e-4f, "plateau before step");
		CHECK_NEAR(monotoneCubic(0.f, 100.f, 100.f, 100.f, t), 100.f, 1e-4f, "plateau after step");
		float mid = monotoneCubic(0.f, 0.f, 100.f, 100.f, t);
		CHECK_TRUE(mid >= -1e-3f && mid <= 100.f + 1e-3f, "step interval bounded: %g", mid);
	}
}

// 4-point one-sided derivative estimates - exact for cubics (up to float
// rounding). The interpolant is exactly cubic within an interval (the clamp
// branches are fixed once the 4 lattice samples are).
template <typename F>
static float derivAtStart(F f, float h)
{
	return (-11.f * f(0.f) + 18.f * f(h) - 9.f * f(2.f * h) + 2.f * f(3.f * h)) / (6.f * h);
}
template <typename F>
static float derivAtEnd(F f, float h)
{
	return (11.f * f(1.f) - 18.f * f(1.f - h) + 9.f * f(1.f - 2.f * h) - 2.f * f(1.f - 3.f * h)) / (6.f * h);
}

static void testCubicC1Continuity()
{
	// derivative approaching a shared lattice point from both intervals matches
	for (int trial = 0; trial < 1000; trial++)
	{
		float s[5];
		for (float& v : s) v = randomHeight();
		// the formula is exact for cubics at any step. A large step minimizes
		// the float-rounding amplification (~1/h)
		const float eps = 1.f / 3.f;
		// right end of interval [s1, s2] vs left end of interval [s2, s3]
		float dLeft = derivAtEnd([&](float t) { return monotoneCubic(s[0], s[1], s[2], s[3], t); }, eps);
		float dRight = derivAtStart([&](float t) { return monotoneCubic(s[1], s[2], s[3], s[4], t); }, eps);
		float tol = 0.05f + 1e-2f * std::max(std::fabs(dLeft), std::fabs(dRight));
		CHECK_NEAR(dLeft, dRight, tol, "C1 at shared lattice point (trial %d)", trial);
	}
}

// MARK: - 2-D monotone bicubic

static void testBicubicCorners()
{
	for (int trial = 0; trial < 500; trial++)
	{
		float p[4][4];
		fillRandomPatch(p);
		CHECK_NEAR(monotoneBicubic(p, 0.f, 0.f), p[1][1], 1e-3f, "corner (0,0) (trial %d)", trial);
		CHECK_NEAR(monotoneBicubic(p, 1.f, 0.f), p[2][1], 1e-3f, "corner (1,0) (trial %d)", trial);
		CHECK_NEAR(monotoneBicubic(p, 0.f, 1.f), p[1][2], 1e-3f, "corner (0,1) (trial %d)", trial);
		CHECK_NEAR(monotoneBicubic(p, 1.f, 1.f), p[2][2], 1e-3f, "corner (1,1) (trial %d)", trial);
	}
}

static void testBicubicBounds()
{
	// never leaves [min, max] of the cell's 2x2 corners
	for (int trial = 0; trial < 1000; trial++)
	{
		float p[4][4];
		fillRandomPatch(p);
		float lo = std::min(std::min(p[1][1], p[2][1]), std::min(p[1][2], p[2][2]));
		float hi = std::max(std::max(p[1][1], p[2][1]), std::max(p[1][2], p[2][2]));
		for (int b = 0; b <= 16; b++)
		{
			for (int a = 0; a <= 16; a++)
			{
				float v = monotoneBicubic(p, a / 16.f, b / 16.f);
				CHECK_TRUE(v >= lo - 1e-3f && v <= hi + 1e-3f,
				           "2D overshoot: %g outside [%g, %g] at (%g, %g) (trial %d)", v, lo, hi, a / 16.f, b / 16.f, trial);
			}
		}
	}
}

static void testBicubicCrackFreeAcrossCells()
{
	// two horizontally adjacent cells of a 5x4 lattice: values along the shared
	// edge must agree exactly (crack-free between tiles and sectors)
	for (int trial = 0; trial < 500; trial++)
	{
		float lattice[5][4];
		for (int a = 0; a < 5; a++)
			for (int b = 0; b < 4; b++)
				lattice[a][b] = randomHeight();

		float left[4][4], right[4][4];
		for (int a = 0; a < 4; a++)
		{
			for (int b = 0; b < 4; b++)
			{
				left[a][b] = lattice[a][b];      // cell between lattice x=1..2
				right[a][b] = lattice[a + 1][b]; // cell between lattice x=2..3
			}
		}
		for (int k = 0; k <= 16; k++)
		{
			float ty = k / 16.f;
			float vLeft = monotoneBicubic(left, 1.f, ty);
			float vRight = monotoneBicubic(right, 0.f, ty);
			CHECK_NEAR(vLeft, vRight, 1e-3f, "edge continuity at ty=%g (trial %d)", ty, trial);
		}
	}
}

static void testBicubicC1AcrossCells()
{
	// C1 across a shared cell edge decomposes: the bicubic's x-derivative at
	// the edge is a fixed function of (a) the q values of the x-pass - equal
	// from both cells (testBicubicCrackFreeAcrossCells) - and (b) the x-pass
	// row derivatives at the edge. So the crossing derivative matches iff the
	// per-row tangents match. Rows ARE pure cubics per cell, exactly testable.
	// (Direct numeric differentiation of the bicubic near the edge is a trap:
	// v(tx) at fixed ty is only piecewise-cubic, since the y-tangent clamp
	// branches can switch within a cell.)
	for (int trial = 0; trial < 500; trial++)
	{
		float lattice[5][4];
		for (int a = 0; a < 5; a++)
			for (int b = 0; b < 4; b++)
				lattice[a][b] = randomHeight();

		const float h = 1.f / 3.f;
		for (int b = 0; b < 4; b++)
		{
			float dLeft = derivAtEnd([&](float tx) {
				return monotoneCubic(lattice[0][b], lattice[1][b], lattice[2][b], lattice[3][b], tx);
			}, h);
			float dRight = derivAtStart([&](float tx) {
				return monotoneCubic(lattice[1][b], lattice[2][b], lattice[3][b], lattice[4][b], tx);
			}, h);
			float tol = 0.05f + 1e-2f * std::max(std::fabs(dLeft), std::fabs(dRight));
			CHECK_NEAR(dLeft, dRight, tol, "row tangent across cells (row %d, trial %d)", b, trial);
		}
	}
}

// MARK: - Legacy fan surface

static void testFanSurface()
{
	for (int trial = 0; trial < 500; trial++)
	{
		float h00 = randomHeight(), h10 = randomHeight(), h01 = randomHeight(), h11 = randomHeight();
		// corners exact
		CHECK_NEAR(fanSurface(h00, h10, h01, h11, 0.f, 0.f), h00, 1e-4f, "fan corner 00 (trial %d)", trial);
		CHECK_NEAR(fanSurface(h00, h10, h01, h11, 1.f, 0.f), h10, 1e-4f, "fan corner 10 (trial %d)", trial);
		CHECK_NEAR(fanSurface(h00, h10, h01, h11, 0.f, 1.f), h01, 1e-4f, "fan corner 01 (trial %d)", trial);
		CHECK_NEAR(fanSurface(h00, h10, h01, h11, 1.f, 1.f), h11, 1e-4f, "fan corner 11 (trial %d)", trial);
		// center = average of corners
		CHECK_NEAR(fanSurface(h00, h10, h01, h11, 0.5f, 0.5f), 0.25f * (h00 + h10 + h01 + h11), 1e-3f,
		           "fan center (trial %d)", trial);
		// linear along the outer edges
		for (int k = 0; k <= 8; k++)
		{
			float t = k / 8.f;
			CHECK_NEAR(fanSurface(h00, h10, h01, h11, t, 0.f), h00 + (h10 - h00) * t, 1e-3f, "fan bottom edge (trial %d)", trial);
			CHECK_NEAR(fanSurface(h00, h10, h01, h11, t, 1.f), h01 + (h11 - h01) * t, 1e-3f, "fan top edge (trial %d)", trial);
			CHECK_NEAR(fanSurface(h00, h10, h01, h11, 0.f, t), h00 + (h01 - h00) * t, 1e-3f, "fan left edge (trial %d)", trial);
			CHECK_NEAR(fanSurface(h00, h10, h01, h11, 1.f, t), h10 + (h11 - h10) * t, 1e-3f, "fan right edge (trial %d)", trial);
		}
		// continuous across the diagonals (sample near both sides)
		for (int k = 1; k < 8; k++)
		{
			float t = k / 8.f;
			const float eps = 1e-4f;
			CHECK_NEAR(fanSurface(h00, h10, h01, h11, t, t - eps),
			           fanSurface(h00, h10, h01, h11, t, t + eps), 0.5f, "fan diagonal continuity (trial %d)", trial);
			CHECK_NEAR(fanSurface(h00, h10, h01, h11, t, 1.f - t - eps),
			           fanSurface(h00, h10, h01, h11, t, 1.f - t + eps), 0.5f, "fan anti-diagonal continuity (trial %d)", trial);
		}
		// planar corners => reproduces the plane
		float a = randomHeight() / 100.f, b = randomHeight() / 100.f, c = randomHeight();
		for (int k = 0; k < 16; k++)
		{
			float tx = (k % 4) / 3.f, ty = (k / 4) / 3.f;
			CHECK_NEAR(fanSurface(c, c + a, c + b, c + a + b, tx, ty), c + a * tx + b * ty, 1e-3f,
			           "fan planar (trial %d)", trial);
		}
	}
}

// MARK: - Crease fillet

static void testFilletDrop()
{
	const float r = 20.f;
	// identity beyond the fillet zone (and continuous at its boundary)
	CHECK_NEAR(filletDrop(100.f, r), 100.f, 1e-4f, "far field identity");
	CHECK_NEAR(filletDrop(2.f * r, r), 2.f * r, 1e-3f, "zone boundary value");
	// pinned at the plateau, clamped for out-of-range input
	CHECK_NEAR(filletDrop(0.f, r), 0.f, 1e-6f, "zero at plateau");
	CHECK_NEAR(filletDrop(-5.f, r), 0.f, 1e-6f, "clamped for negative drop");
	CHECK_NEAR(filletDrop(50.f, 0.f), 50.f, 1e-6f, "disabled at r=0");
	// tangent to the plateau (g'(0) = 0) and C1 at the zone boundary (g'(2r) = 1)
	const float eps = 1e-2f;
	CHECK_NEAR(filletDrop(eps, r) / eps, 0.f, 0.01f, "tangent at plateau");
	CHECK_NEAR((filletDrop(2.f * r, r) - filletDrop(2.f * r - eps, r)) / eps, 1.f, 0.01f, "C1 at zone boundary");
	// one-sided and monotone: 0 <= fillet(d) <= d, nondecreasing
	float prev = 0.f;
	for (int k = 0; k <= 100; k++)
	{
		float d = k * 0.5f;
		float f = filletDrop(d, r);
		CHECK_TRUE(f >= -1e-4f && f <= d + 1e-4f, "one-sided: %g not in [0, %g]", f, d);
		CHECK_TRUE(f >= prev - 1e-4f, "monotone: %g < %g at d=%g", f, prev, d);
		prev = f;
	}
}

static void testFilletedFanSurface()
{
	const float rMax = 20.f;
	// flat ground (hi == lo): exactly the fan surface
	for (int k = 0; k < 16; k++)
	{
		float tx = (k % 4) / 3.f, ty = (k / 4) / 3.f;
		CHECK_NEAR(filletedFanSurface(80.f, 80.f, 80.f, 80.f, 80.f, 80.f, rMax, tx, ty), 80.f, 1e-3f, "flat identity");
	}
	// a cliff cell: face from hi=200 (x=0 side) down to lo=0 (x=1 side)
	const float hi = 200.f, lo = 0.f;
	for (int k = 0; k <= 8; k++)
	{
		float ty = k / 8.f;
		// corner/edge exactness at plateau and floor
		CHECK_NEAR(filletedFanSurface(hi, lo, hi, lo, hi, lo, rMax, 0.f, ty), hi, 1e-3f, "lip edge exact");
		CHECK_NEAR(filletedFanSurface(hi, lo, hi, lo, hi, lo, rMax, 1.f, ty), lo, 1e-3f, "foot edge exact");
		// mid-face far from both fillet zones: unchanged
		CHECK_NEAR(filletedFanSurface(hi, lo, hi, lo, hi, lo, rMax, 0.5f, ty),
		           fanSurface(hi, lo, hi, lo, 0.5f, ty), 1e-3f, "mid-face identity");
	}
	// stays within [lo, hi] and only moves heights toward the nearer plateau
	for (int a = 0; a <= 16; a++)
	{
		for (int b = 0; b <= 16; b++)
		{
			float tx = a / 16.f, ty = b / 16.f;
			float fan = fanSurface(hi, lo, hi, lo, tx, ty);
			float h = filletedFanSurface(hi, lo, hi, lo, hi, lo, rMax, tx, ty);
			CHECK_TRUE(h >= lo - 1e-3f && h <= hi + 1e-3f, "fillet bounds: %g", h);
			// lip zone raises toward hi, foot zone lowers toward lo, else identity
			if (hi - fan < 2.f * rMax)
			{
				CHECK_TRUE(h >= fan - 1e-3f, "lip fillet one-sided: %g < %g", h, fan);
			}
			else if (fan - lo < 2.f * rMax)
			{
				CHECK_TRUE(h <= fan + 1e-3f, "foot fillet one-sided: %g > %g", h, fan);
			}
			else
			{
				CHECK_NEAR(h, fan, 1e-3f, "identity between fillet zones");
			}
		}
	}
	// short step (hi - lo < 4*rMax): fillet radius shrinks, corners stay exact
	CHECK_NEAR(filletedFanSurface(30.f, 0.f, 30.f, 0.f, 30.f, 0.f, rMax, 0.f, 0.5f), 30.f, 1e-3f, "short step lip exact");
	CHECK_NEAR(filletedFanSurface(30.f, 0.f, 30.f, 0.f, 30.f, 0.f, rMax, 1.f, 0.5f), 0.f, 1e-3f, "short step foot exact");
}

// MARK: - Blended surface

static void testBlendedSharpnessExtremes()
{
	for (int trial = 0; trial < 500; trial++)
	{
		float p[4][4];
		fillRandomPatch(p);
		for (int b = 0; b <= 8; b++)
		{
			for (int a = 0; a <= 8; a++)
			{
				float tx = a / 8.f, ty = b / 8.f;
				// sharpness 1 everywhere => exactly the legacy fan surface
				CHECK_NEAR(blendedSurfaceHeight(p, 1.f, 1.f, 1.f, 1.f, tx, ty),
				           fanSurface(p[1][1], p[2][1], p[1][2], p[2][2], tx, ty), 1e-3f,
				           "blend s=1 == fan (trial %d)", trial);
				// sharpness 0 everywhere => exactly the smooth surface
				CHECK_NEAR(blendedSurfaceHeight(p, 0.f, 0.f, 0.f, 0.f, tx, ty),
				           monotoneBicubic(p, tx, ty), 1e-3f,
				           "blend s=0 == smooth (trial %d)", trial);
			}
		}
	}
}

static void testBlendedBounds()
{
	// both blend inputs respect the 2x2 cell bounds, so the blend does too
	for (int trial = 0; trial < 500; trial++)
	{
		float p[4][4];
		fillRandomPatch(p);
		float s00 = (trial % 3) / 2.f, s10 = (trial % 5) / 4.f, s01 = (trial % 7) / 6.f, s11 = (trial % 2) / 1.f;
		float lo = std::min(std::min(p[1][1], p[2][1]), std::min(p[1][2], p[2][2]));
		float hi = std::max(std::max(p[1][1], p[2][1]), std::max(p[1][2], p[2][2]));
		for (int b = 0; b <= 8; b++)
		{
			for (int a = 0; a <= 8; a++)
			{
				float v = blendedSurfaceHeight(p, s00, s10, s01, s11, a / 8.f, b / 8.f);
				CHECK_TRUE(v >= lo - 1e-3f && v <= hi + 1e-3f,
				           "blended overshoot: %g outside [%g, %g] (trial %d)", v, lo, hi, trial);
			}
		}
	}
}

int main()
{
	testCubicEndpoints();
	testCubicBounds();
	testCubicReproducesLinear();
	testCubicPlateauNextToStep();
	testCubicC1Continuity();
	testBicubicCorners();
	testBicubicBounds();
	testBicubicCrackFreeAcrossCells();
	testBicubicC1AcrossCells();
	testFanSurface();
	testFilletDrop();
	testFilletedFanSurface();
	testBlendedSharpnessExtremes();
	testBlendedBounds();

	std::printf("%s: %d checks, %d failures\n", failures == 0 ? "PASS" : "FAIL", checks, failures);
	return failures == 0 ? 0 : 1;
}
