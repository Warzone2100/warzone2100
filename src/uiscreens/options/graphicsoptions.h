#pragma once

#include "../common.h"

bool runGraphicsOptionsMenu();
void startGraphicsOptionsMenu();


// Graphics options, shared for in-game options menu use
char const* graphicsOptionsFmvmodeString();
char const* graphicsOptionsScanlinesString();
char const* graphicsOptionsSubtitlesString();
char const* graphicsOptionsShadowsString();
char const* graphicsOptionsRadarString();
char const* graphicsOptionsRadarJumpString();
char const* graphicsOptionsScreenShakeString();
void seqFMVmode();
void seqScanlineMode();
