// gcc -o astartest astartest.c -Wall -Werror -g -O0 -I/usr/include/SDL/ `sdl-config --libs --cflags`

#include <assert.h>
#include <stdio.h>
#include <SDL.h>
#include <time.h>

#define mapX 64
#define mapY 48
#define IMPASSABLE 255

int mapWidth = mapX;
int mapHeight = mapY;

#include "../lib/framework/frame.h"
#include "../lib/framework/debug.h"
#include "../lib/framework/types.h"
#include "../lib/framework/macros.h"
#include "../lib/framework/vector.h"
#include "../src/movedef.h"
#include "../src/map.h"

/* Faux declarations */
void fpathHardTableReset(void);

typedef enum _fpath_movetype
{
	FMT_MOVE,		///< Move around all obstacles
	FMT_ATTACK,		///< Assume that we will destroy enemy obstacles
} FPATH_MOVETYPE;

typedef struct _jobNode
{
	PROPULSION_TYPE	propulsion;
	DROID_TYPE	droidType;
	int		destX, destY;
	int		origX, origY;
	UDWORD		droidID;
	struct _jobNode	*next;
	FPATH_MOVETYPE	moveType;
	int		owner;		///< Player owner
} PATHJOB;

typedef enum
{
	ASR_OK,         ///< found a route
	ASR_FAILED,     ///< no route could be found
	ASR_NEAREST,    ///< found a partial route to a nearby position
} ASTAR_RESULT;

static unsigned char testmap[mapX][mapY];

static inline BOOL fpathBaseBlockingTile(SDWORD x, SDWORD y, PROPULSION_TYPE propulsion, int player, FPATH_MOVETYPE moveType)
{
	return testmap[x][y] == IMPASSABLE;
}

#define WZ_TESTING
#include "../src/astar.cpp"

static void printmap(MOVE_CONTROL *move, PATHJOB *job)
{
	int x, y, z;

	for (y = 0; y < mapY; y++)
	{
		for (x = 0; x < mapX; x++)
		{
			bool onPos = false;

			for (z = 0; z < move->numPoints; z++)
			{
				if (move->asPath[z].x == x && move->asPath[z].y == y)
				{
					onPos = true;
				}
			}

			if (x == map_coord(job->origX) && y == map_coord(job->origY))
			{
				fprintf(stdout, "O");
			}
			else if (x == map_coord(job->destX) && y == map_coord(job->destY))
			{
				fprintf(stdout, "X");
			}
			else if (testmap[x][y] == IMPASSABLE)
			{
				assert(!onPos);
				fprintf(stdout, "#");
			}
			else if (onPos)
			{
				fprintf(stdout, "+");
			}
			else
			{
				fprintf(stdout, ".");
			}
		}
		fprintf(stdout, "\n");
	}
}

static void createmap(void)
{
	int x, y;

	srand(123456789);
	for (y = 0; y < mapY; y++)
	{
		int slitPos = (rand() % (mapX - 2)) + 1;	// opening at this position

		for (x = 0; x < mapX; x++)
		{

			if (x == 0 || x == mapX - 1 || y == 0 || y == mapY - 1)
			{
				testmap[x][y] = IMPASSABLE;	// add impassable border
			}
			else if ((y % 4) == 0 && x != slitPos)
			{
				testmap[x][y] = IMPASSABLE;	// add barrier
			}
			else
			{
				testmap[x][y] = 0;		// passable space
			}
		}
	}
}

int main(int argc, char **argv)
{
	ASTAR_RESULT	result;
	PATHJOB		job;
	MOVE_CONTROL	move;
	int		i;
	unsigned int	startTicks, endTicks;
	clock_t		startCpu, endCpu;

	createmap();

	startTicks = SDL_GetTicks();
	startCpu = clock();
	for (i = 0; i < 10000; i++)
	{
		job.propulsion = 0;
		job.droidType = 0;
		job.destX = world_coord(1);
		job.destY = world_coord(1);
		job.origX = world_coord(mapX - 2);
		job.origY = world_coord(mapY - 2);
		job.droidID = 9;
		job.next = NULL;
		job.moveType = 0;
		job.owner = 0;
		move.asPath = NULL;
		move.numPoints = 0;

		result = fpathAStarRoute(&move, &job);
		assert(result == ASR_OK);
		assert(move.numPoints == 214);
	}
	endCpu = clock();
	endTicks = SDL_GetTicks();

	fprintf(stdout, "Found a route with %d points (%u ticks, %u clock)\n", move.numPoints, endTicks - startTicks, (unsigned int)(endCpu - startCpu));
	printmap(&move, &job);

	return 0;
}
