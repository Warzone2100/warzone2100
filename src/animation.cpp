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
void Animation<AnimatableData>::start(AnimatableData initialData, AnimatableData finalData)
{
	this->startTime = *this->time;
    this->initialData = initialData;
    this->finalData = finalData;
}

template <class AnimatableData>
void Animation<AnimatableData>::setDuration(uint32_t durationMilliseconds)
{
	this->duration = durationMilliseconds * 0.001 * GAME_TICKS_PER_SEC;
}

template <class AnimatableData>
void Animation<AnimatableData>::update()
{
    if (this->duration > 0) {
        auto deltaTime = *this->time - (int64_t)this->startTime;
        this->progress = MAX(0, MIN(UINT16_MAX, UINT16_MAX * deltaTime / this->duration));
    } else {
        this->progress = UINT16_MAX;
    }

	auto easedProgress = calculateEasing(this->easingType, this->progress);
    this->currentData = this->initialData + (this->finalData - this->initialData) * (easedProgress / (float)UINT16_MAX);
}

template <class AnimatableData>
AnimatableData Animation<AnimatableData>::getCurrent()
{
    return this->currentData;
}

template <class AnimatableData>
bool Animation<AnimatableData>::isFinished()
{
    return this->progress == UINT16_MAX;
}

template class Animation<Vector3f>;
