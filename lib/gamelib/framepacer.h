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
 *  Frame pacing: turns measured frame intervals into the intervals that the display shows.
 *
 *  With vsync, samples are placed on a grid of whole refresh periods and the periods between them are counted, and frames are animated by the average over the last few frames
 *  (a late sample followed by an early one counts as two periods then none, and a missed vblank only shows up in the measured interval a few frames later).
 *
 *  The sum of the deltas stays in step with the measured time, and a stall of several periods is shown at once.
 *  The motion delta leaves out the part of a stall beyond a slow frame.
 *
 *  The refresh period comes from either the reported display intervals, the measured intervals, or the backend's nominal value.
 *  Without vsync, or when the samples don't keep to the grid (ex. variable refresh rate), the measured interval is used as-is.
 *
 *  Has no dependencies on the rest of the game (see tests/framepacer_test.cpp).
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

class FramePacer
{
public:
	/// Forget the previous frame, so the next update() returns 0 (ex. after loading or focus loss).
	/// The refresh period estimate is kept.
	void reset();

	/// Whether presentation is synchronized to the display (vsync or adaptive vsync).
	/// Rounding is only applied when it is.
	void setVsync(bool vsync);

	/// The display's nominal refresh period in seconds, if the backend knows it (0 if unknown).
	/// Only used until enough intervals have been measured.
	void setNominalRefreshPeriod(double seconds);

	/// A frame reached the display.
	/// presentId increases by one per present.
	/// displayTime is in seconds on any clock (only differences between reports are used).
	/// Reports may arrive several frames late.
	void addPresentation(uint64_t presentId, double displayTime);

	/// Called once per frame with the measured time in seconds since the previous call
	/// (negative for the first frame after reset()). Returns the delta to animate by.
	double update(double measuredDelta);

	/// The last delta without the part of a stall beyond a slow frame, for motion like the camera's.
	double lastMotionDelta() const { return motion_; }

	// Diagnostics, valid after update()
	double lastMeasuredDelta() const { return measured_; }
	double lastDelta() const { return delta_; }
	double refreshPeriod() const { return refresh_; }   ///< 0 when unknown
	bool snapped() const { return snapped_; }            ///< whether the last delta was rounded
	bool usingPresentationTimes() const { return refreshFromPresentation_; }
	double drift() const { return phase_ * refresh_; }   ///< offset of the last sample from the period grid

	/// Longest delta update() returns, in seconds
	static constexpr double maxDelta = 1.0 / 6.0;

private:
	struct Ring
	{
		std::vector<double> values;
		size_t next = 0;
		void push(double v, size_t capacity);
		double mean() const;
		double percentile(double p) const;
	};

	struct Estimate
	{
		double period = 0.0;       ///< refresh period, 0 if none
		double fitFraction = 0.0;  ///< share of recent measured deltas near a whole number of periods
		bool fromPresentation = false;
	};
	Estimate estimateRefresh() const;
	double fitFraction(double period, double *refinedPeriod) const;
	void spreadFill(int periods);
	int spreadMedian() const;

	static constexpr size_t kSpreadFrames = 8;

	bool vsync_ = true;
	double nominalRefresh_ = 0.0;
	Ring measuredIntervals_;      ///< recent measured deltas
	Ring presentedIntervals_;     ///< recent display intervals between consecutive presents
	bool havePresentation_ = false;
	uint64_t lastPresentId_ = 0;
	double lastPresentTime_ = 0.0;

	int spread_[kSpreadFrames] = {};  ///< periods per frame over the last kSpreadFrames frames
	size_t spreadNext_ = 0;           ///< next slot of spread_ to overwrite
	int spreadSum_ = 0;               ///< sum of spread_
	bool spreadFilled_ = false;       ///< spread_ holds values for the current run of rounded frames

	bool havePhase_ = false;      ///< phase_ follows the current run of samples
	double phase_ = 0.0;          ///< position of the last sample relative to the period grid, in periods
	Ring phaseFits_;              ///< 1 for each recent sample within kSnapTolerance of the grid, else 0

	double measured_ = 0.0;
	double delta_ = 0.0;
	double motion_ = 0.0;
	double refresh_ = 0.0;
	bool snapped_ = false;
	bool snapping_ = false;       ///< recent samples keep to the period grid
	bool refreshFromPresentation_ = false;
};
