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

// Unit tests for FramePacer (lib/gamelib/framepacer.h).
// Build with -DWZ_BUILD_FRAMEPACER_TEST=ON (target: framepacer_test). Exits nonzero on failure.

#include "lib/gamelib/framepacer.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

static int failures = 0;
static int checks = 0;

static void check(bool ok, const char *what, double actual = 0.0, double expected = 0.0)
{
	++checks;
	if (!ok)
	{
		++failures;
		std::printf("FAIL %s (got %.9g, expected %.9g)\n", what, actual, expected);
	}
}

/// Present-return times for a vsynced display, where frame i is shown at vblank v[i].
/// The CPU sees present return at that vblank plus a wobble,
/// or with lag > 0 at the vblank of frame i - lag (as when it waits for a swapchain image that frame releases).
struct Sim
{
	double period;
	std::vector<double> measured;   ///< measured deltas fed to the pacer
	std::vector<double> displayed;  ///< true display intervals
};

static Sim makeSim(double period, int frames, const std::vector<int> &vblanksPerFrame, double wobble, unsigned seed, int lag = 0)
{
	std::mt19937 rng(seed);
	std::uniform_real_distribution<double> noise(-wobble, wobble);
	Sim s;
	s.period = period;
	std::vector<double> vblanks;
	double vblank = 0.0, prevSeen = 0.0;
	for (int i = 0; i < frames + lag; ++i)
	{
		const int k = vblanksPerFrame[static_cast<size_t>(i) % vblanksPerFrame.size()];
		vblank += k * period;
		vblanks.push_back(vblank);
	}
	for (int i = 0; i < frames; ++i)
	{
		const size_t seenFrame = static_cast<size_t>(std::max(0, i - lag));
		const double seen = vblanks[seenFrame] + noise(rng) + (i < lag ? i * period : 0.0);
		if (i > 0)
		{
			s.measured.push_back(seen - prevSeen);
			s.displayed.push_back(vblanks[static_cast<size_t>(i)] - vblanks[static_cast<size_t>(i - 1)]);
		}
		prevSeen = seen;
	}
	return s;
}

struct Result
{
	double maxError = 0.0;   ///< largest |delta - display interval| after warm-up
	double maxDelta = 0.0;   ///< largest delta after warm-up
	double minDelta = 1.0;   ///< smallest delta after warm-up
	double totalDrift = 0.0; ///< |sum of deltas - sum of display intervals|
	bool allSnapped = true;
};

static Result run(FramePacer &pacer, const Sim &s, size_t warmup = 100)
{
	Result r;
	double sumDelta = 0.0, sumShown = 0.0;
	pacer.update(-1.0);
	for (size_t i = 0; i < s.measured.size(); ++i)
	{
		const double d = pacer.update(s.measured[i]);
		if (i >= warmup)
		{
			r.maxError = std::max(r.maxError, std::abs(d - s.displayed[i]));
			r.maxDelta = std::max(r.maxDelta, d);
			r.minDelta = std::min(r.minDelta, d);
			r.allSnapped = r.allSnapped && pacer.snapped();
			sumDelta += d;
			sumShown += s.displayed[i];
		}
	}
	r.totalDrift = std::abs(sumDelta - sumShown);
	return r;
}

int main()
{
	const double hz120 = 1.0 / 120.0;

	{
		// Every frame on time, with present returns wobbling by up to 0.8 ms
		FramePacer pacer;
		const Sim s = makeSim(hz120, 2000, {1}, 0.0008, 1);
		const Result r = run(pacer, s);
		check(r.allSnapped, "120 Hz wobble: snapped");
		check(r.maxError < 2e-5, "120 Hz wobble: delta within 0.02 ms of the display interval", r.maxError, 0.0);
		check(r.totalDrift < 0.25 * hz120, "120 Hz wobble: no drift", r.totalDrift, 0.0);
		check(std::abs(pacer.refreshPeriod() - hz120) < 5e-6, "120 Hz wobble: period", pacer.refreshPeriod(), hz120);
	}
	{
		// One in 20 frames shown for two periods, seen by the CPU two frames later
		std::vector<int> pattern(20, 1);
		pattern[7] = 2;
		FramePacer pacer;
		const Sim s = makeSim(hz120, 2000, pattern, 0.0006, 2, 2);
		const Result r = run(pacer, s);
		check(r.allSnapped, "missed vblanks: snapped");
		check(r.maxDelta < hz120 * (1.0 + 1.0 / 8.0) + 2e-5, "missed vblanks: extra period spread, no jump", r.maxDelta, hz120 * 1.125);
		check(r.minDelta > hz120 - 2e-5, "missed vblanks: never less than a period", r.minDelta, hz120);
		check(r.totalDrift < 0.25 * hz120, "missed vblanks: no drift", r.totalDrift, 0.0);
	}
	{
		// The same, seen on the frame itself (CPU not waiting on the display)
		std::vector<int> pattern(20, 1);
		pattern[7] = 2;
		FramePacer pacer;
		const Sim s = makeSim(hz120, 2000, pattern, 0.0006, 9);
		const Result r = run(pacer, s);
		check(r.maxDelta < hz120 * (1.0 + 1.0 / 8.0) + 2e-5, "missed vblanks, no lag: spread", r.maxDelta, hz120 * 1.125);
		check(r.totalDrift < 0.25 * hz120, "missed vblanks, no lag: no drift", r.totalDrift, 0.0);
	}
	{
		// Sim tick frames that present late but are absorbed by the swapchain:
		// every 12th sample comes 0.6 periods late and the next one early again, while the display shows every period.
		FramePacer pacer;
		Sim s = makeSim(hz120, 2400, {1}, 0.0006, 11);
		for (size_t i = 12; i + 1 < s.measured.size(); i += 12)
		{
			s.measured[i] += 0.6 * hz120;
			s.measured[i + 1] -= 0.6 * hz120;
		}
		pacer.update(-1.0);
		bool allSnapped = true;
		double maxDelta = 0.0, minDelta = 1.0, sumDelta = 0.0, sumShown = 0.0;
		for (size_t i = 0; i < s.measured.size(); ++i)
		{
			if (i >= 3)
			{
				pacer.addPresentation(i - 3, static_cast<double>(i - 3) * hz120); // reported three frames late
			}
			const double d = pacer.update(s.measured[i]);
			if (i >= 100)
			{
				allSnapped = allSnapped && pacer.snapped();
				maxDelta = std::max(maxDelta, d);
				minDelta = std::min(minDelta, d);
				sumDelta += d;
				sumShown += s.displayed[i];
			}
		}
		check(allSnapped, "absorbed late frames: snapped");
		check(maxDelta < hz120 * (1.0 + 1.0 / 8.0) + 2e-5, "absorbed late frames: at most an eighth of a period over", maxDelta, hz120 * 1.125);
		check(minDelta > hz120 * (1.0 - 1.0 / 8.0) - 2e-5, "absorbed late frames: at most an eighth of a period under", minDelta, hz120 * 0.875);
		check(std::abs(sumDelta - sumShown) < hz120, "absorbed late frames: no drift", std::abs(sumDelta - sumShown), 0.0);
	}
	{
		// Variable refresh rate with the frame time sweeping between 9 and 14 ms: not rounded.
		FramePacer pacer;
		pacer.setNominalRefreshPeriod(hz120);
		std::mt19937 rng(12);
		std::uniform_real_distribution<double> noise(-0.0003, 0.0003);
		pacer.update(-1.0);
		bool anySnapped = false;
		double maxDiff = 0.0;
		for (int i = 0; i < 3000; ++i)
		{
			const double m = 0.0115 + 0.0025 * std::sin(2.0 * 3.14159265358979 * i / 120.0) + noise(rng);
			const double out = pacer.update(m);
			if (i >= 64)
			{
				anySnapped = anySnapped || pacer.snapped();
				maxDiff = std::max(maxDiff, std::abs(out - m));
			}
		}
		check(!anySnapped, "varying VRR frame rate: not snapped");
		check(maxDiff < 1e-9, "varying VRR frame rate: deltas are the measured ones", maxDiff, 0.0);
	}
	{
		// A steady frame rate under variable refresh rate is shown like a fixed refresh rate at that rate.
		for (double frameTime : {1.0 / 90.0, 1.0 / 100.0})
		{
			FramePacer pacer;
			pacer.setNominalRefreshPeriod(hz120);
			std::mt19937 rng(13);
			std::uniform_real_distribution<double> noise(-0.0003, 0.0003);
			pacer.update(-1.0);
			double maxDiff = 0.0, sumDelta = 0.0, sumMeasured = 0.0;
			for (int i = 0; i < 2000; ++i)
			{
				const double m = frameTime + noise(rng);
				const double out = pacer.update(m);
				if (i >= 64)
				{
					maxDiff = std::max(maxDiff, std::abs(out - frameTime));
					sumDelta += out;
					sumMeasured += m;
				}
			}
			check(maxDiff < 0.0004, "steady VRR frame rate: deltas within the noise of the frame time", maxDiff, 0.0);
			check(std::abs(sumDelta - sumMeasured) < frameTime, "steady VRR frame rate: no drift", std::abs(sumDelta - sumMeasured), 0.0);
		}
	}
	{
		// A stall of several periods
		FramePacer pacer;
		const Sim s = makeSim(hz120, 400, {1}, 0.0006, 10);
		pacer.update(-1.0);
		for (size_t i = 0; i < 300; ++i)
		{
			pacer.update(s.measured[i]);
		}
		const double d = pacer.update(6.0 * hz120 + 0.0003);
		check(pacer.snapped(), "stall: snapped");
		check(std::abs(d - 6.0 * hz120) < 2e-5, "stall: six periods shown at once", d, 6.0 * hz120);
		check(std::abs(pacer.lastMotionDelta() - 3.0 * hz120) < 2e-5, "stall: motion left at a slow frame", pacer.lastMotionDelta(), 3.0 * hz120);
	}
	{
		// Missed vblanks and slow frames keep all their motion
		FramePacer pacer;
		const Sim s = makeSim(hz120, 2000, {1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 3, 1}, 0.0006, 12);
		pacer.update(-1.0);
		double maxDiff = 0.0;
		for (size_t i = 0; i < s.measured.size(); ++i)
		{
			pacer.update(s.measured[i]);
			maxDiff = std::max(maxDiff, std::abs(pacer.lastMotionDelta() - pacer.lastDelta()));
		}
		check(maxDiff < 1e-12, "slow frames: motion is the delta", maxDiff, 0.0);
	}
	{
		// Without vsync, sim tick frames keep their motion and a stall is left out
		FramePacer pacer;
		pacer.setVsync(false);
		pacer.update(-1.0);
		double maxDiff = 0.0;
		for (int i = 0; i < 600; ++i)
		{
			const double m = (i % 30 == 0) ? 0.015 : 0.0033;
			pacer.update(m);
			maxDiff = std::max(maxDiff, std::abs(pacer.lastMotionDelta() - m));
		}
		check(maxDiff < 1e-12, "vsync off, sim ticks: motion is the measured delta", maxDiff, 0.0);
		const double d = pacer.update(0.1);
		check(std::abs(d - 0.1) < 1e-12, "vsync off, stall: delta is the measured one", d, 0.1);
		check(std::abs(pacer.lastMotionDelta() - 0.05) < 1e-12, "vsync off, stall: motion left at the floor", pacer.lastMotionDelta(), 0.05);
	}
	{
		// 119.88 Hz with a nominal 120 Hz
		const double p = 1.0 / 119.88;
		FramePacer pacer;
		pacer.setNominalRefreshPeriod(hz120);
		const Sim s = makeSim(p, 20000, {1}, 0.0007, 3);
		const Result r = run(pacer, s, 200);
		check(std::abs(pacer.refreshPeriod() - p) < 5e-6, "119.88 Hz: period refined from the measurements", pacer.refreshPeriod(), p);
		check(r.totalDrift < 0.3 * p, "119.88 Hz: no drift over 20000 frames", r.totalDrift, 0.0);
		check(r.maxError < 0.01 * p, "119.88 Hz: deltas within 1% of the display interval", r.maxError, 0.0);
	}
	{
		// About 80 fps on a 120 Hz display, with frames alternating between one and two periods
		FramePacer pacer;
		const Sim s = makeSim(hz120, 2000, {1, 2}, 0.0006, 4);
		const Result r = run(pacer, s);
		check(r.allSnapped, "alternating 1/2 periods: snapped");
		check(r.maxDelta < 1.5 * hz120 + 2e-5 && r.minDelta > 1.5 * hz120 - 2e-5, "alternating 1/2 periods: 1.5 periods each", r.maxDelta, 1.5 * hz120);
		check(r.totalDrift < hz120, "alternating 1/2 periods: no drift", r.totalDrift, 0.0);
	}
	{
		// 60 fps on a 120 Hz display
		FramePacer pacer;
		const Sim s = makeSim(hz120, 2000, {2}, 0.0008, 5);
		const Result r = run(pacer, s);
		check(r.maxError < 2e-5, "every frame two periods: deltas", r.maxError, 0.0);
	}
	{
		// Variable refresh rate or irregular timing
		FramePacer pacer;
		std::mt19937 rng(6);
		std::uniform_real_distribution<double> d(0.007, 0.0145);
		pacer.update(-1.0);
		bool anySnapped = false;
		double maxDiff = 0.0;
		for (int i = 0; i < 2000; ++i)
		{
			const double m = d(rng);
			const double out = pacer.update(m);
			if (i >= 64)
			{
				anySnapped = anySnapped || pacer.snapped();
				maxDiff = std::max(maxDiff, std::abs(out - m));
			}
		}
		check(!anySnapped, "irregular timing: not snapped");
		check(maxDiff < 1e-9, "irregular timing: deltas are the measured ones", maxDiff, 0.0);
	}
	{
		// Without vsync
		FramePacer pacer;
		pacer.setVsync(false);
		const Sim s = makeSim(hz120, 500, {1}, 0.0008, 7);
		pacer.update(-1.0);
		double maxDiff = 0.0;
		for (double m : s.measured)
		{
			maxDiff = std::max(maxDiff, std::abs(pacer.update(m) - m));
		}
		check(maxDiff < 1e-9, "vsync off: deltas are the measured ones", maxDiff, 0.0);
	}
	{
		// Presentation times available
		FramePacer pacer;
		const Sim s = makeSim(hz120, 400, {1}, 0.0008, 8);
		pacer.update(-1.0);
		double t = 0.0;
		for (size_t i = 0; i < s.measured.size(); ++i)
		{
			pacer.update(s.measured[i]);
			if (i >= 3)
			{
				pacer.addPresentation(i - 3, t); // reported three frames late
				t += hz120;
			}
		}
		check(pacer.usingPresentationTimes(), "presentation times used for the period");
		check(std::abs(pacer.refreshPeriod() - hz120) < 1e-7, "period from presentation times", pacer.refreshPeriod(), hz120);
	}
	{
		// Warm-up with a nominal period
		FramePacer pacer;
		pacer.setNominalRefreshPeriod(hz120);
		check(pacer.update(-1.0) == 0.0, "reset: first delta is 0");
		const double d = pacer.update(hz120 + 0.0007);
		check(std::abs(d - hz120) < 1e-9, "nominal period used while warming up", d, hz120);
		check(pacer.update(0.5) == FramePacer::maxDelta, "long stall clamped", pacer.lastDelta(), FramePacer::maxDelta);
	}

	std::printf("%d of %d checks passed\n", checks - failures, checks);
	return failures == 0 ? 0 : 1;
}
