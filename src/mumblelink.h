#ifndef __INCLUDED_SRC_MUMBLELINK_H__
#define __INCLUDED_SRC_MUMBLELINK_H__

#include "lib/framework/frame.h"
#include "lib/ivis_common/pievector.h"

extern bool InitMumbleLink(void);
extern bool CloseMumbleLink(void);

extern bool UpdateMumbleSoundPos(const Vector3f pos, const Vector3f forward, const Vector3f up);

#endif // __INCLUDED_SRC_MUMBLELINK_H__
