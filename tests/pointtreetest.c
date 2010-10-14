#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "../src/pointtree.h"

#define NUM_POINTS 100000
#define NUM_SLOW_CHECKS 1
#define NUM_FAST_CHECKS 100000

int32_t points[NUM_POINTS][3];
extern int doDump;

#define CHECK(x, s, ...) do { if (!(x)) { printf("tests/pointtreetest.c:%d: Check %s failed: "s"\n", __LINE__, #x, ##__VA_ARGS__); abort(); } } while(0)
#define CHECKd(x, s, ...) do { if (!(x)) { printf("tests/pointtreetest.c:%d: Check %s failed: "s"\n", __LINE__, #x, ##__VA_ARGS__); doDump = true; pointTreeQuery(pointTree, x, y, r); abort(); } } while(0)

int randPos()
{
	return -500 + rand()%1000;
}

int randRange()
{
	return rand()%100;
}

int randRangeSmall()
{
	return rand()%30;
}

int main()
{
	int i, n, pointsFound = 0;
	srand(time(NULL));

	POINT_TREE *pointTree = pointTreeCreate();
	CHECK(pointTree != NULL, "Huh? Out of memory..?");

	// Make random points.
	for (i = 0; i != NUM_POINTS; ++i)
	{
		points[i][0] = randPos();
		points[i][1] = randPos();
		points[i][2] = 0;
		pointTreeInsert(pointTree, points[i], points[i][0], points[i][1]);
	}

	pointTreeSort(pointTree);

	for (n = 0; n != NUM_SLOW_CHECKS; ++n)
	{
		int32_t x = randPos();
		int32_t y = randPos();
		int32_t r = randRange();
		void **ret = pointTreeQuery(pointTree, x, y, r);
		for (i = 0; ret[i] != NULL; ++i)
		{
			int32_t *p = (int32_t *)ret[i];
			double dx = x - p[0], dy = y - p[1];
			double dist = sqrt(dx*dx + dy*dy);
			CHECKd(dist < r*4*sqrt(2) + 1, "Found point %p, but was too far. Wanted %d, found %lf, expected max %lf, dx = %lf, dy = %lf", p, r, dist, r*4*sqrt(2), dx, dy);
			++p[2];  // Mark point as found.
			++pointsFound;
			CHECKd(p[2] == 1, "Point found more than once.");
		}
		for (i = 0; i != NUM_POINTS; ++i)
		{
			double dx = x - points[i][0], dy = y - points[i][1];
			double dist = sqrt(dx*dx + dy*dy);
			CHECKd(points[i][2] || dist > r + 0.00001, "Point not found, and was within the radius.");
			points[i][2] = 0;  // Unmark point.
		}
	}

	printf("Found %d points total (in detailed check), with no problems.\n", pointsFound);

	pointsFound = 0;
	// Warning, duplicate code.
	for (n = 0; n != NUM_FAST_CHECKS; ++n)
	{
		int32_t x = randPos();
		int32_t y = randPos();
		int32_t r = randRangeSmall();
		void **ret = pointTreeQuery(pointTree, x, y, r);
		for (i = 0; ret[i] != NULL; ++i)
		{
			int32_t *p = (int32_t *)ret[i];
			double dx = x - p[0], dy = y - p[1];
			double dist = sqrt(dx*dx + dy*dy);
			CHECKd(dist < r*4*sqrt(2) + 1, "Found point %p, but was too far. Wanted %d, found %lf, expected max %lf, dx = %lf, dy = %lf", p, r, dist, r*4*sqrt(2), dx, dy);
			//Slow check only: ++p[2];  // Mark point as found.
			++pointsFound;
			//Slow check only: CHECKd(p[2] == 1, "Point found more than once.");
		}
		/*Slow check only: for (i = 0; i != NUM_POINTS; ++i)
		{
			double dx = x - points[i][0], dy = y - points[i][1];
			double dist = sqrt(dx*dx + dy*dy);
			CHECKd(points[i][2] || dist > r + 0.00001, "Point not found, and was within the radius.");
			points[i][2] = 0;  // Unmark point.
		}*/
	}

	printf("Found %d points total (in fast check), with no problems.\n", pointsFound);

	int32_t x = randPos();
	int32_t y = randPos();
	int32_t r = randRange();
	doDump = true;
	pointTreeQuery(pointTree, x, y, r);
	printf("Dumped image of random query to tests/pointtree.ppm, for your viewing pleasure.\n");
}
