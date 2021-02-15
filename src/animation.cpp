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
 * Animation functions.
 */

#include "animation.h"

ValueTracker* ValueTracker::startTracking(int value)
{
	this->initial = value;
	this->target = value;
	this->targetDelta = 0;
	this->current = value;
	this->startTime = graphicsTime;
	this->_reachedTarget = false;
	return this;
}
ValueTracker* ValueTracker::stopTracking()
{
	this->initial = 0;
	this->current = 0;
	this->startTime = 0;
	this->_reachedTarget = false;
	return this;
}
bool ValueTracker::isTracking()
{
	return this->startTime != 0;
}
ValueTracker* ValueTracker::setSpeed(int value)
{
	this->speed = value;
	return this;
}
ValueTracker* ValueTracker::setTargetDelta(int value)
{
	this->targetDelta = value;
	this->target = this->initial + value;
	this->_reachedTarget = false;
	return this;
}
ValueTracker* ValueTracker::setTarget(int value)
{
	this->targetDelta = value - this->initial;
	this->target = value;
	this->_reachedTarget = false;
	return this;
}
ValueTracker* ValueTracker::update()
{
	if(this->_reachedTarget)
	{
		return this;
	}

	if(std::abs(this->target - this->current) < 1)
	{
		this->_reachedTarget = true;
		return this;
	}

	this->current = (this->initial + this->targetDelta - this->current) * realTimeAdjustedIncrement(this->speed) + this->current;
	return this;
}
int ValueTracker::getCurrent()
{
	if(this->_reachedTarget)
	{
		return this->target;
	}
	return this->current;
}
int ValueTracker::getCurrentDelta()
{
	if(this->_reachedTarget)
	{
		return this->targetDelta;
	}
	return this->current - this->initial;
}
int ValueTracker::getInitial()
{
	return this->initial;
}
int ValueTracker::getTarget()
{
	return this->target;
}
int ValueTracker::getTargetDelta()
{
	return this->targetDelta;
}
bool ValueTracker::reachedTarget()
{
	return this->_reachedTarget;
}

static uint16_t calculateEasing(EasingType easingType, uint16_t progress)
{
	switch (easingType)
	{
	case LINEAR:
		return progress;
	case EASE_IN_OUT:
		return MAX(0, MIN(UINT16_MAX, iCos(UINT16_MAX / 2 + progress / 2) / 2 + (1 << 15)));
	case EASE_IN:
		return (progress * (uint32_t)progress) / UINT16_MAX;
	case EASE_OUT:
		return 2 * progress - (progress * (uint32_t)progress) / (UINT16_MAX - 1);
	}

	return UINT16_MAX;
}

template <class AnimatableData>
void Animation<AnimatableData>::start()
{
	startTime = time;
	progress = 0;
}

template <class AnimatableData>
void Animation<AnimatableData>::update()
{
	if (duration > 0)
	{
		auto deltaTime = time - (int64_t)startTime;
		progress = MAX(0, MIN(UINT16_MAX, UINT16_MAX * deltaTime / duration));
	}
	else
	{
		progress = UINT16_MAX;
	}

	currentData = initialData + (finalData - initialData) * (getEasedProgress() / (float)UINT16_MAX);
}

template <class AnimatableData>
bool Animation<AnimatableData>::isActive() const
{
	return progress < UINT16_MAX;
}

template <class AnimatableData>
const AnimatableData &Animation<AnimatableData>::getCurrent() const
{
	return currentData;
}

template <class AnimatableData>
const AnimatableData &Animation<AnimatableData>::getFinalData() const
{
	return finalData;
}

template <class AnimatableData>
uint16_t Animation<AnimatableData>::getEasedProgress() const
{
	return calculateEasing(easingType, progress);
}

template <class AnimatableData>
Animation<AnimatableData> &Animation<AnimatableData>::setInitialData(AnimatableData initial)
{
	initialData = initial;
	currentData = initial;
	return *this;
}

template <class AnimatableData>
Animation<AnimatableData> &Animation<AnimatableData>::setFinalData(AnimatableData final)
{
	finalData = final;
	return *this;
}

template <class AnimatableData>
Animation<AnimatableData> &Animation<AnimatableData>::setEasing(EasingType easing)
{
	easingType = easing;
	return *this;
}

template <class AnimatableData>
Animation<AnimatableData> &Animation<AnimatableData>::setDuration(uint32_t durationMilliseconds)
{
	duration = durationMilliseconds * 0.001 * GAME_TICKS_PER_SEC;
	return *this;
}

/**
 * Find the angle equivalent to `from` in the interval between `to - 180°` and to `to + 180°`.
 *
 * For example:
 * - if `from` is `10°` and `to` is `350°`, it will return `370°`.
 * - if `from` is `350°` and `to` is `0°`, it will return `-10°`.
 *
 * Useful while animating a rotation, to always animate the shortest angle delta.
 */
int32_t calculateRelativeAngle(uint16_t from, uint16_t to)
{
	return to + (int16_t)(from - to);
}

void RotationAnimation::start()
{
	finalData = Vector3f((uint16_t)finalData.x, (uint16_t)finalData.y, (uint16_t)finalData.z);
	initialData = Vector3f(
		calculateRelativeAngle(initialData.x, finalData.x),
		calculateRelativeAngle(initialData.y, finalData.y),
		calculateRelativeAngle(initialData.z, finalData.z)
	);
	Animation::start();
}

template class Animation<Vector3f>;
template class Animation<float>;
