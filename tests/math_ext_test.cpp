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

// Unit tests for solveDifferential2ndOrder() in lib/framework/math_ext.h.
// Build with -DWZ_BUILD_MATH_EXT_TEST=ON (target: math_ext_test). Exits nonzero on failure.
//
// The camera height follower and the war camera call the solver once per frame with that frame's dt.
// It must (a) match the true solution of y'' = -a y - f y',
// (b) give the same result however the time is split into steps,
// and (c) stay finite for every damping, including the critically damped case f^2 = 4a.

#include "lib/framework/math_ext.h"

#include <cstdio>
#include <cmath>

static int failures = 0;
static int checks = 0;

static void checkNear(double actual, double expected, double tolerance, const char *what, double a, double f)
{
	++checks;
	if (!std::isfinite(actual) || std::abs(actual - expected) > tolerance)
	{
		++failures;
		std::printf("FAIL %s (a=%g f=%g): got %.9g, expected %.9g (tolerance %g)\n", what, a, f, actual, expected, tolerance);
	}
}

/// Reference: integrate y'' = -a y - f y' with small RK4 steps.
static void reference(double &y, double &v, double a, double f, double t)
{
	const int n = 200000;
	const double h = t / n;
	auto acc = [&](double yy, double vv) { return -a * yy - f * vv; };
	for (int i = 0; i < n; ++i)
	{
		const double k1y = v, k1v = acc(y, v);
		const double k2y = v + h / 2 * k1v, k2v = acc(y + h / 2 * k1y, v + h / 2 * k1v);
		const double k3y = v + h / 2 * k2v, k3v = acc(y + h / 2 * k2y, v + h / 2 * k2v);
		const double k4y = v + h * k3v, k4v = acc(y + h * k3y, v + h * k3v);
		y += h / 6 * (k1y + 2 * k2y + 2 * k3y + k4y);
		v += h / 6 * (k1v + 2 * k2v + 2 * k3v + k4v);
	}
}

int main()
{
	struct Case { double a, f; };
	const Case cases[] = {
		{12.0, 4.0},                       // underdamped (the camera's constants)
		{16.0, 8.0},                       // exactly critical (f^2/4 - a == 0)
		{12.0, 2.0 * std::sqrt(12.0)},     // critical up to rounding
		{12.0, 2.0 * std::sqrt(12.0) + 1e-7}, // a hair either side of critical
		{12.0, 2.0 * std::sqrt(12.0) - 1e-7},
		{12.0, 7.2},                       // slightly overdamped
		{12.0, 20.0},                      // strongly overdamped
	};
	for (const Case &c : cases)
	{
		const double a = c.a, f = c.f;
		// (a) one step against the reference solution
		float y = -50.f, v = 30.f;
		solveDifferential2ndOrder(&y, &v, a, f, 0.7);
		double ry = -50.0, rv = 30.0;
		reference(ry, rv, a, f, 0.7);
		checkNear(y, ry, 1e-3, "single step position", a, f);
		checkNear(v, rv, 1e-3, "single step velocity", a, f);

		// (b) 84 steps of 1/120 s land where one step of 0.7 s does
		float ys = -50.f, vs = 30.f;
		for (int i = 0; i < 84; ++i)
		{
			solveDifferential2ndOrder(&ys, &vs, a, f, 0.7 / 84);
		}
		checkNear(ys, y, 1e-3, "split steps position", a, f);
		checkNear(vs, v, 1e-3, "split steps velocity", a, f);

		// (c) dt = 0 leaves the state unchanged
		float y0 = 5.f, v0 = -2.f;
		solveDifferential2ndOrder(&y0, &v0, a, f, 0.0);
		checkNear(y0, 5.0, 1e-5, "zero dt position", a, f);
		checkNear(v0, -2.0, 1e-5, "zero dt velocity", a, f);
	}

	std::printf("%d of %d checks passed\n", checks - failures, checks);
	return failures == 0 ? 0 : 1;
}
