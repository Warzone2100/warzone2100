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
	void start();
	void update();
	bool isActive() const;
	const AnimatableData &getCurrent() const;
	const AnimatableData &getFinalData() const;
	Animation<AnimatableData> &setInitialData(AnimatableData initial);
	Animation<AnimatableData> &setFinalData(AnimatableData final);
	Animation<AnimatableData> &setEasing(EasingType easing);
	Animation<AnimatableData> &setDuration(uint32_t durationMilliseconds);

private:
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

#endif // __INCLUDED_SRC_ANIMATION_H__
