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
static constexpr double kUnroundedStallMin = 0.05; ///< shortest unrounded delta that can count as a stall, in seconds
static constexpr size_t kLateWindow = 32;          ///< recent late-frame predictions checked against the counted periods
static constexpr double kLateMargin = 0.00125;     ///< time a frame still needs after predictLate() to make a vblank, in seconds
static constexpr double kUnroundedLateMin = 0.5;   ///< least extra time that counts as late when not rounding, in typical frame intervals

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
		roundedLate_ = LateCheck();
		unroundedLate_ = LateCheck();
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
		// Display intervals are whole periods (a missed vblank shows as two).
		// When most frames take several vblanks, the median is a multiple of the period.
		const double median = presentedIntervals_.percentile(0.5);
		double period = median, bestFit = -1.0;
		for (int divisor = 1; divisor <= 4; ++divisor)
		{
			const double base = median / divisor;
			double sumIntervals = 0.0, sumPeriods = 0.0;
			size_t fits = 0;
			for (double v : presentedIntervals_.values)
			{
				const double k = std::round(v / base);
				if (k >= 1.0 && std::abs(v - k * base) <= kSnapTolerance * base)
				{
					sumIntervals += v;
					sumPeriods += k;
					++fits;
				}
			}
			const double fit = static_cast<double>(fits) / static_cast<double>(presentedIntervals_.values.size());
			if (fits > 0 && fit > bestFit + 0.05)
			{
				period = sumIntervals / sumPeriods;
				bestFit = fit;
			}
		}
		candidates[n++] = {period, true};
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
		// A later candidate has to fit clearly better than an earlier, more trustworthy one.
		// A whole fraction of a period that already fits only fits better by admitting samples between vblanks.
		const double ratio = (best.period > 0.0) ? best.period / period : 0.0;
		const bool fractionOfBest = std::round(ratio) >= 2.0 && std::abs(ratio - std::round(ratio)) <= kSnapTolerance;
		if (fractionOfBest && best.fitFraction >= kStopFitFraction)
		{
			continue;
		}
		if (best.period <= 0.0 || fraction > best.fitFraction + 0.05)
		{
			best.period = period;
			best.fitFraction = fraction;
			best.fromPresentation = candidates[i].fromPresentation;
		}
	}
	return best;
}

void FramePacer::LateCheck::judge(bool hit)
{
	hits.push(hit ? 1.0 : 0.0, kLateWindow);
	if (hits.values.size() >= kMinSamples && hits.mean() >= kStartFitFraction)
	{
		predicting = true;
	}
	else if (hits.mean() < kStopFitFraction)
	{
		predicting = false;
	}
}

void FramePacer::judgeLate(double extra, double candidate, double slack, bool rounded)
{
	// A queue that absorbs a late frame usually takes its extra time back with an early sample soon after,
	// so a prediction is judged once the following frames have had the chance
	if (judgeCount_ == kJudgeFrames)
	{
		const size_t oldest = judgeNext_;
		if (judgeCandidate_[oldest] > 0.0)
		{
			double net = judgeExtra_[oldest] + std::min(0.0, extra);
			for (size_t i = 1; i < kJudgeFrames; ++i)
			{
				net += std::min(0.0, judgeExtra_[(oldest + i) % kJudgeFrames]);
			}
			const bool hit = net >= judgeCandidate_[oldest] - judgeSlack_[oldest];
			(judgeRounded_[oldest] ? roundedLate_ : unroundedLate_).judge(hit);
		}
	}
	else
	{
		++judgeCount_;
	}
	judgeExtra_[judgeNext_] = extra;
	judgeCandidate_[judgeNext_] = candidate;
	judgeSlack_[judgeNext_] = slack;
	judgeRounded_[judgeNext_] = rounded;
	judgeNext_ = (judgeNext_ + 1) % kJudgeFrames;
}

void FramePacer::spreadFill(int periods)
{
	for (int &v : spread_)
	{
		v = periods;
	}
	for (int &v : predictedSpread_)
	{
		v = 0;
	}
	spreadNext_ = 0;
	predictedSum_ = 0;
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
		motion_ = 0.0;
		havePhase_ = false;
		spreadFilled_ = false;
		predicted_ = 0;
		predictedSeconds_ = 0.0;
		candidateSeconds_ = 0.0;
		judgeCount_ = 0;
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

	// The time this frame's motion already got from predictLate() is taken back from the following frames
	const int predicted = predicted_;
	const double predictedSeconds = predictedSeconds_;
	const double candidateSeconds = candidateSeconds_;
	const bool candidateRounded = candidateRounded_;
	const double slack = 0.5 * (candidateRounded ? refresh_ : candidateSeconds);
	predicted_ = 0;
	predictedSeconds_ = 0.0;
	candidateSeconds_ = 0.0;

	if (!snapping_)
	{
		spreadFilled_ = false;
		delta_ = measured_;
		double allowance = maxDelta;
		double extra = 0.0;
		if (measuredIntervals_.values.size() >= kMinSamples)
		{
			const double typical = measuredIntervals_.percentile(0.5);
			allowance = std::max((1 + kSpreadMaxExtra) * typical, kUnroundedStallMin);
			extra = measured_ - typical;
		}
		judgeLate(extra, candidateSeconds, slack, candidateRounded);
		if (candidateSeconds > 0.0 && !candidateRounded)
		{
			lateRemaining_.push(std::max(0.0, measured_ - lastElapsed_), kLateWindow);
		}
		motion_ = std::max(0.0, std::min(delta_, allowance) - predictedSeconds);
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
	judgeLate((periods - typical) * refresh_, candidateSeconds, slack, candidateRounded);
	// Whole periods are taken back in step with the average, and anything else at once
	double paidBack = predictedSeconds - predicted * refresh_;
	int spreadPredicted = predicted;
	if (periods > typical + kSpreadMaxExtra)
	{
		immediate = periods - typical;
		periods = typical;
		paidBack = predictedSeconds;
		spreadPredicted = 0;
	}
	spreadSum_ += periods - spread_[spreadNext_];
	spread_[spreadNext_] = periods;
	predictedSum_ += spreadPredicted - predictedSpread_[spreadNext_];
	predictedSpread_[spreadNext_] = spreadPredicted;
	spreadNext_ = (spreadNext_ + 1) % kSpreadFrames;

	delta_ = (static_cast<double>(spreadSum_) / static_cast<double>(kSpreadFrames) + immediate) * refresh_;
	if (delta_ < 0.0 || delta_ > maxDelta)
	{
		delta_ = std::clamp(delta_, 0.0, maxDelta);
		spreadFilled_ = false;
	}
	const int leftOut = std::max(0, immediate - kSpreadMaxExtra);
	const double takenBack = static_cast<double>(predictedSum_) / static_cast<double>(kSpreadFrames) * refresh_ + paidBack;
	motion_ = std::max(0.0, delta_ - leftOut * refresh_ - takenBack);
	return delta_;
}

double FramePacer::predictLate(double elapsed)
{
	predicted_ = 0;
	predictedSeconds_ = 0.0;
	candidateSeconds_ = 0.0;
	prediction_ = 0.0;
	if (elapsed < 0.0)
	{
		return 0.0;
	}
	lastElapsed_ = elapsed;
	if (snapping_ && spreadFilled_ && refresh_ > 0.0)
	{
		// The vblank a typical frame is shown at has already passed
		const int extra = static_cast<int>(std::floor(phase_ + (elapsed + kLateMargin) / refresh_)) + 1 - spreadMedian();
		if (extra <= 0)
		{
			return 0.0;
		}
		const int candidate = std::min(extra, kSpreadMaxExtra);
		candidateSeconds_ = candidate * refresh_;
		candidateRounded_ = true;
		if (!roundedLate_.predicting)
		{
			return 0.0;
		}
		predicted_ = candidate;
		predictedSeconds_ = candidateSeconds_;
	}
	else
	{
		// Without rounding a frame is shown when it is presented,
		// for a late frame about as long after this call as recent late frames took
		if (snapping_ || measuredIntervals_.values.size() < kMinSamples)
		{
			return 0.0;
		}
		const double typical = measuredIntervals_.percentile(0.5);
		const double extra = elapsed + lateRemaining_.percentile(0.5) - typical;
		if (extra <= kUnroundedLateMin * typical)
		{
			return 0.0;
		}
		candidateSeconds_ = std::min(extra, kUnroundedStallMin);
		candidateRounded_ = false;
		if (!unroundedLate_.predicting)
		{
			return 0.0;
		}
		predictedSeconds_ = candidateSeconds_;
	}
	prediction_ = predictedSeconds_;
	return prediction_;
}
