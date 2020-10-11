#include "animation.h"

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
