#pragma once

#include "../../levels.h"

#define MAX_LEVEL_NAME_SIZE	(256)

extern char	aLevelName[MAX_LEVEL_NAME_SIZE + 1];	//256];			// vital! the wrf file to use.
extern bool	bLimiterLoaded;

void SPinit(LEVEL_TYPE gameType);
void loadOK();
