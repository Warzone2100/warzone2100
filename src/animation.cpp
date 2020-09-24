#include "animation.h"

static uint16_t calculateEasing(EasingType easingType, uint16_t progress)
{
    switch (easingType)
    {
        case LINEAR:
            return progress;
        case EASE_IN_OUT:
            return MAX(0, MIN(UINT16_MAX, iCos(UINT16_MAX / 2 + progress / 2) / 2 + (1 << 15)));
    }

    return UINT16_MAX;
}

template <class AnimatableData>
void Animation<AnimatableData>::start(AnimatableData initial, AnimatableData final)
{
	startTime = *time;
    initialData = initial;
    finalData = final;
}

template <class AnimatableData>
void Animation<AnimatableData>::setDuration(uint32_t durationMilliseconds)
{
	duration = durationMilliseconds * 0.001 * GAME_TICKS_PER_SEC;
}

template <class AnimatableData>
void Animation<AnimatableData>::update()
{
    if (duration > 0) {
        auto deltaTime = *time - (int64_t)startTime;
        progress = MAX(0, MIN(UINT16_MAX, UINT16_MAX * deltaTime / duration));
    } else {
        progress = UINT16_MAX;
    }

	auto easedProgress = calculateEasing(easingType, calculateEasing(easingType, progress));
    currentData = initialData + (finalData - initialData) * (easedProgress / (float)UINT16_MAX);
}

template <class AnimatableData>
AnimatableData Animation<AnimatableData>::getCurrent()
{
    return currentData;
}

template <class AnimatableData>
bool Animation<AnimatableData>::isFinished()
{
    return progress == UINT16_MAX;
}

template class Animation<Vector3f>;
