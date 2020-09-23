#include "stdint.h"
#include "lib/gamelib/gtime.h"

#ifndef __INCLUDED_SRC_ANIMATION_H__
#define __INCLUDED_SRC_ANIMATION_H__

enum EasingType
{
    LINEAR,
    EASE_IN_OUT
};

template <class AnimatableData>
class Animation
{
public:
    Animation(const uint32_t *time, EasingType easingType = LINEAR): time(time), easingType(easingType) {}
    void start(AnimatableData initialData, AnimatableData finalData);
    void setDuration(uint32_t durationMilliseconds);
    void update();
    AnimatableData getCurrent();
    bool isFinished();

private:
	const uint32_t *time;
	uint32_t startTime = 0;
	uint32_t duration = 0;
	uint16_t progress = 0;
    EasingType easingType;
    AnimatableData initialData;
    AnimatableData finalData;
    AnimatableData currentData;
};

#endif // __INCLUDED_SRC_ANIMATION_H__