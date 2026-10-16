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

#include "framepacer.h"

#include <algorithm>
#include <cmath>
#include <iterator>

static constexpr size_t kMeasuredWindow = 64;      ///< measured deltas kept for estimating the refresh period
static constexpr size_t kPresentedWindow = 32;     ///< display intervals kept
static constexpr size_t kMinSamples = 16;          ///< samples needed before measured periods are used
static constexpr double kSnapTolerance = 0.2;      ///< distance from a whole number of periods that still fits, in periods
static constexpr double kStartFitFraction = 0.85;  ///< share of recent samples that must fit to start rounding
static constexpr double kStopFitFraction = 0.65;   ///< share of recent samples below which rounding stops
static constexpr double kPeriodSmoothing = 0.02;   ///< per-frame weight of a new period estimate
static constexpr double kPeriodChange = 0.02;      ///< relative change taken as a new refresh rate
static constexpr double kPhaseGain = 0.05;         ///< share of each offset from the grid moved into the phase
static constexpr int kSpreadMaxExtra = 2;          ///< most extra periods spread over the following frames

void FramePacer::Ring::push(double v, size_t capacity)
{
	if (values.size() < capacity)
	{
		values.push_back(v);
	}
	else
	{
		values[next] = v;
	}
	next = (next + 1) % capacity;
}

double FramePacer::Ring::mean() const
{
	if (values.empty())
	{
		return 0.0;
	}
	double sum = 0.0;
	for (double v : values)
	{
		sum += v;
	}
	return sum / static_cast<double>(values.size());
}

double FramePacer::Ring::percentile(double p) const
{
	if (values.empty())
	{
		return 0.0;
	}
	std::vector<double> sorted(values);
	const size_t k = std::min(sorted.size() - 1, static_cast<size_t>(p * static_cast<double>(sorted.size() - 1) + 0.5));
	std::nth_element(sorted.begin(), sorted.begin() + static_cast<std::ptrdiff_t>(k), sorted.end());
	return sorted[k];
}

void FramePacer::reset()
{
	havePhase_ = false;
	spreadFilled_ = false;
}

void FramePacer::setVsync(bool vsync)
{
	if (vsync != vsync_)
	{
		vsync_ = vsync;
		havePhase_ = false;
		spreadFilled_ = false;
	}
}

void FramePacer::setNominalRefreshPeriod(double seconds)
{
	nominalRefresh_ = (seconds > 0.0 && seconds < maxDelta) ? seconds : 0.0;
}

void FramePacer::addPresentation(uint64_t presentId, double displayTime)
{
	if (havePresentation_ && presentId <= lastPresentId_)
	{
		return; // duplicate or out of order
	}
	if (havePresentation_ && presentId == lastPresentId_ + 1)
	{
		const double interval = displayTime - lastPresentTime_;
		if (interval > 0.0 && interval < maxDelta)
		{
			presentedIntervals_.push(interval, kPresentedWindow);
		}
	}
	havePresentation_ = true;
	lastPresentId_ = presentId;
	lastPresentTime_ = displayTime;
}

double FramePacer::fitFraction(double period, double *refinedPeriod) const
{
	if (period <= 0.0 || measuredIntervals_.values.empty())
	{
		return 0.0;
	}
	size_t fit = 0;
	double sumIntervals = 0.0;
	double sumPeriods = 0.0;
	for (double v : measuredIntervals_.values)
	{
		const double k = std::round(v / period);
		if (k >= 1.0 && std::abs(v - k * period) <= kSnapTolerance * period)
		{
			++fit;
			sumIntervals += v;
			sumPeriods += k;
		}
	}
	if (refinedPeriod)
	{
		// Intervals of frames that made their vblanks add up to whole periods, as the wobble cancels out
		*refinedPeriod = (sumPeriods > 0.0) ? sumIntervals / sumPeriods : period;
	}
	return static_cast<double>(fit) / static_cast<double>(measuredIntervals_.values.size());
}

FramePacer::Estimate FramePacer::estimateRefresh() const
{
	Estimate best;
	if (measuredIntervals_.values.size() < kMinSamples)
	{
		best.period = nominalRefresh_;
		best.fitFraction = nominalRefresh_ > 0.0 ? 1.0 : 0.0;
		return best;
	}

	struct Candidate { double period; bool fromPresentation; };
	Candidate candidates[4];
	size_t n = 0;
	if (presentedIntervals_.values.size() >= kMinSamples)
	{
		// Display intervals are whole periods (a missed vblank shows as two)
		const double median = presentedIntervals_.percentile(0.5);
		double sumIntervals = 0.0, sumPeriods = 0.0;
		for (double v : presentedIntervals_.values)
		{
			const double k = std::round(v / median);
			if (k >= 1.0 && std::abs(v - k * median) <= kSnapTolerance * median)
			{
				sumIntervals += v;
				sumPeriods += k;
			}
		}
		candidates[n++] = {sumPeriods > 0.0 ? sumIntervals / sumPeriods : median, true};
	}
	if (nominalRefresh_ > 0.0)
	{
		candidates[n++] = {nominalRefresh_, false};
	}
	candidates[n++] = {measuredIntervals_.percentile(0.5), false};
	candidates[n++] = {measuredIntervals_.percentile(0.1), false}; // frames alternating between 1 and 2 periods

	for (size_t i = 0; i < n; ++i)
	{
		double period = candidates[i].period;
		double fraction = 0.0;
		if (candidates[i].fromPresentation)
		{
			fraction = fitFraction(period, nullptr);
		}
		else
		{
			// A percentile of noisy deltas only admits the deltas near its own multiples,
			// which biases the refined value. Repeating converges on the period the deltas cluster around.
			for (int round = 0; round < 3; ++round)
			{
				double refined = period;
				fraction = fitFraction(period, &refined);
				period = refined;
			}
		}
		// A later candidate has to fit clearly better than an earlier, more trustworthy one
		if (best.period <= 0.0 || fraction > best.fitFraction + 0.05)
		{
			best.period = period;
			best.fitFraction = fraction;
			best.fromPresentation = candidates[i].fromPresentation;
		}
	}
	return best;
}

void FramePacer::spreadFill(int periods)
{
	for (int &v : spread_)
	{
		v = periods;
	}
	spreadNext_ = 0;
	spreadSum_ = periods * static_cast<int>(kSpreadFrames);
	spreadFilled_ = true;
}

int FramePacer::spreadMedian() const
{
	int sorted[kSpreadFrames];
	std::copy(std::begin(spread_), std::end(spread_), sorted);
	std::nth_element(sorted, sorted + kSpreadFrames / 2, sorted + kSpreadFrames);
	return sorted[kSpreadFrames / 2];
}

double FramePacer::update(double measuredDelta)
{
	snapped_ = false;
	if (measuredDelta < 0.0)
	{
		measured_ = 0.0;
		delta_ = 0.0;
		havePhase_ = false;
		spreadFilled_ = false;
		return delta_;
	}
	measured_ = std::min(measuredDelta, maxDelta);
	if (measuredDelta < maxDelta)
	{
		measuredIntervals_.push(measuredDelta, kMeasuredWindow);
	}

	const Estimate estimate = estimateRefresh();
	refreshFromPresentation_ = estimate.fromPresentation;
	if (estimate.period > 0.0)
	{
		// Estimates are noisy, while the refresh rate only changes in steps (ex. another display or mode)
		if (refresh_ <= 0.0 || std::abs(estimate.period - refresh_) > kPeriodChange * refresh_)
		{
			refresh_ = estimate.period;
		}
		else
		{
			refresh_ += kPeriodSmoothing * (estimate.period - refresh_);
		}
	}

	// Place this sample on a grid of whole refresh periods, counting the periods since the last sample.
	// The grid's phase follows the samples, so:
	// - present-time "wobble" stays within a period
	// - a late sample followed by an early one counts as two periods then none
	int periods = 0;
	if (refresh_ <= 0.0 || measuredDelta >= maxDelta)
	{
		havePhase_ = false;
	}
	else if (!havePhase_)
	{
		havePhase_ = true;
		phase_ = 0.0;
		periods = std::max(1, static_cast<int>(std::lround(measured_ / refresh_)));
	}
	else
	{
		const double position = phase_ + measured_ / refresh_;
		const double nearest = std::floor(position + 0.5);
		const double offset = position - nearest;
		phaseFits_.push(std::abs(offset) <= kSnapTolerance ? 1.0 : 0.0, kMeasuredWindow);
		phase_ = offset - kPhaseGain * std::clamp(offset, -kSnapTolerance, kSnapTolerance);
		periods = static_cast<int>(nearest);
	}

	// Samples that keep to the grid mean presentation is locked to the display
	const double fit = (phaseFits_.values.size() >= kMinSamples) ? phaseFits_.mean() : estimate.fitFraction;
	if (!vsync_ || refresh_ <= 0.0 || !havePhase_)
	{
		snapping_ = false;
	}
	else if (fit >= kStartFitFraction)
	{
		snapping_ = true;
	}
	else if (fit < kStopFitFraction)
	{
		snapping_ = false;
	}

	if (!snapping_)
	{
		spreadFilled_ = false;
		delta_ = measured_;
		return delta_;
	}
	snapped_ = true;

	// Animate by the average over the last few frames (see the file comment),
	// except for a stall of more than kSpreadMaxExtra extra periods, which is shown at once
	if (!spreadFilled_)
	{
		spreadFill(std::max(1, periods));
	}
	int immediate = 0;
	const int typical = spreadMedian();
	if (periods > typical + kSpreadMaxExtra)
	{
		immediate = periods - typical;
		periods = typical;
	}
	spreadSum_ += periods - spread_[spreadNext_];
	spread_[spreadNext_] = periods;
	spreadNext_ = (spreadNext_ + 1) % kSpreadFrames;

	delta_ = (static_cast<double>(spreadSum_) / static_cast<double>(kSpreadFrames) + immediate) * refresh_;
	if (delta_ < 0.0 || delta_ > maxDelta)
	{
		delta_ = std::clamp(delta_, 0.0, maxDelta);
		spreadFilled_ = false;
	}
	return delta_;
}
