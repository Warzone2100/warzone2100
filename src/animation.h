/*
	This file is part of Warzone 2100.
	Copyright (C) 2020  Warzone 2100 Project

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
 * @file
 * Definitions for animation functions.
 */

#include "stdint.h"
#include "lib/gamelib/gtime.h"

#ifndef __INCLUDED_SRC_ANIMATION_H__
#define __INCLUDED_SRC_ANIMATION_H__

enum EasingType
{
	LINEAR,
	EASE_IN_OUT,
	EASE_IN,
	EASE_OUT,
};

template <class AnimatableData>
class Animation
{
public:
	Animation(uint32_t const &time, EasingType easingType = LINEAR, uint16_t duration = 0) : time(time), easingType(easingType), duration(duration) {}
	virtual ~Animation() {}
	virtual void start();
	void update();
	bool isActive() const;
	const AnimatableData &getCurrent() const;
	const AnimatableData &getFinalData() const;
	Animation<AnimatableData> &setInitialData(AnimatableData initial);
	Animation<AnimatableData> &setFinalData(AnimatableData final);
	Animation<AnimatableData> &setEasing(EasingType easing);
	Animation<AnimatableData> &setDuration(uint32_t durationMilliseconds);

protected:
	uint16_t getEasedProgress() const;

	uint32_t const &time;
	EasingType easingType;
	uint32_t duration;
	uint32_t startTime = 0;
	uint16_t progress = UINT16_MAX;
	AnimatableData initialData;
	AnimatableData finalData;
	AnimatableData currentData;
};

class RotationAnimation: public Animation<Vector3f>
{
public:
	RotationAnimation(uint32_t const &time, EasingType easingType = LINEAR, uint16_t duration = 0): Animation(time, easingType, duration) {}
	void start() override;
};

#endif // __INCLUDED_SRC_ANIMATION_H__
