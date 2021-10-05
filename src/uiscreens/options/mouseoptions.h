#pragma once

#include "../common.h"

void startMouseOptionsMenu();
bool runMouseOptionsMenu();


// Mouse options, shared for in-game options menu use
char const* mouseOptionsMflipString();
char const* mouseOptionsTrapString();
char const* mouseOptionsMbuttonsString();
char const* mouseOptionsMmrotateString();
char const* mouseOptionsCursorModeString();
char const* mouseOptionsScrollEventString();
void seqScrollEvent();
