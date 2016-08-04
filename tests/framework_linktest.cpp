#include "lib/framework/wzglobal.h"
#include "lib/framework/types.h"
#include "lib/framework/frame.h"

// --- dummy rendering library implementation ----

void wzToggleFullscreen()
{
}

bool wzIsFullscreen()
{
	return false;
}

void wzFatalDialog(char const*)
{
}

int wzGetTicks()
{
	return 1;
}

void inputInitialise()
{
}

// --- end linking hacks ---

int main(void)
{
	frameInitialise();
	frameUpdate();
	frameShutDown();
	return 0;
}
