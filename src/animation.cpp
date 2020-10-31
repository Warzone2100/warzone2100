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

AngleAnimation* AngleAnimation::setInitial(Vector3i value){
	this->initialX = (UWORD)value.x;
	this->initialY = (UWORD)value.y;
	this->initialZ = (UWORD)value.z;
	return this;
}
AngleAnimation* AngleAnimation::setDuration(int milliseconds){
	this->duration = milliseconds;
	return this;
}
AngleAnimation* AngleAnimation::startNow(){
	this->startTime = graphicsTime;
	return this;
}
AngleAnimation* AngleAnimation::setTarget(int x, int y, int z){
	this->targetX = (UWORD)x;
	this->targetY = (UWORD)y;
	this->targetZ = (UWORD)z;
	return this;
}
AngleAnimation* AngleAnimation::update(){
	float t = (graphicsTime - this->startTime) / (float)this->duration;

	t = t < 0.5 ? 2 * t * t : t * (4 - 2 * t) - 1;

	// the SWORD here is the secret juice. It takes the target delta
	// (target minus initial) and makes it wrap to the other side if > 180.
	this->currentX = (this->initialX + (SWORD)(this->targetX - this->initialX) * t);
	this->currentY = (this->initialY + (SWORD)(this->targetY - this->initialY) * t);
	this->currentZ = (this->initialZ + (SWORD)(this->targetZ - this->initialZ) * t);

	return this;
}
Vector3i AngleAnimation::getCurrent(){
	return { this->currentX, this->currentY, this->currentZ };
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
		return (progress * (uint32_t)progress) / (UINT16_MAX * (uint32_t)UINT16_MAX);
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
	return calculateEasing(easingType, calculateEasing(easingType, progress));
}

template <class AnimatableData>
Animation<AnimatableData> &Animation<AnimatableData>::setInitialData(AnimatableData initial)
{
	initialData = initial;
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

template class Animation<Vector3f>;
template class Animation<float>;
