// Must be before some stuff from headers from flowfield.h, otherwise "std has no member 'mutex'"
// That simply means someone else have messed up
#include <mutex>

#include "flowfield.h"

#include <future>
#include <map>
#include <set>
#include <typeinfo>
#include <vector>
#include <memory>
#include <chrono>

#include "lib/framework/debug.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/ivis_opengl/piematrix.h"

#include "display3d.h"
#include "map.h"
#include "lib/framework/wzapp.h"
#include <glm/gtx/transform.hpp>
#include "lib/framework/opengl.h"
#include "lib/ivis_opengl/piedef.h"
#include "lib/ivis_opengl/piefunc.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/pieclip.h"
#include "lib/wzmaplib/include/wzmaplib/map.h"

static bool flowfieldEnabled = false;
// just for debug, remove for release builds
#ifdef DEBUG
	#define DEFAULT_EXTENT_X 2
	#define DEFAULT_EXTENT_Y 2
#else
	#define DEFAULT_EXTENT_X 1
	#define DEFAULT_EXTENT_Y 1
#endif
void flowfieldEnable() {
	flowfieldEnabled = true;
}

bool isFlowfieldEnabled() {
	return flowfieldEnabled;
}

inline uint16_t tileTo2Dindex (uint8_t x, uint8_t y)
{
	return (x << 8 | y);
}

inline uint16_t tileTo2Dindex (Vector2i tile)
{
	return (tile.x << 8 | tile.y);
}

inline Vector2i tileFrom2Dindex (uint16_t idx)
{
	return Vector2i {(idx) >> 8, (idx & 0x00FF)};
}


inline void tileFrom2Dindex (uint16_t idx, uint8_t &x, uint8_t &y)
{
	x = (idx) >> 8;
	y = (idx & 0x00FF);
}

inline uint8_t xFrom2Dindex (uint16_t idx)
{
	return (idx >> 8);
}

inline uint8_t yFrom2Dindex (uint16_t idx)
{
	return (idx & 0x00FF);
}

// ignore the first one when iterating!
// 8 neighbours for each cell
static const int neighbors[DIR_TO_VEC_SIZE][2] = {
	{-0xDEAD, -0xDEAD}, // DIR_NONE
	{-1, -1}, {0, -1}, {+1, -1},
	{-1,  0},          {+1,  0},
	{-1, +1}, {0, +1}, {+1, +1}
};

#define FF_MAP_WIDTH 256
#define FF_MAP_HEIGHT 256
#define FF_MAP_AREA FF_MAP_WIDTH*FF_MAP_HEIGHT
#define FF_TILE_SIZE 128

/** Simple cost type*/
using CostType = uint8_t;

/** Integrated cost type */
using ICostType = uint16_t;

constexpr const CostType COST_NOT_PASSABLE = std::numeric_limits<uint8_t>::max();
// constexpr const uint16_t COST_NOT_PASSABLE_
constexpr const CostType COST_MIN = 1; // default cost 
// constexpr const CostType COST_ZERO = 0; // goal is free

// Decides how much slopes should be avoided
constexpr const float SLOPE_COST_BASE = 0.01f;
// Decides when terrain height delta is considered a slope
constexpr const uint16_t SLOPE_THRESOLD = 4;

const int propulsionIdx2[] = {
	0,//PROPULSION_TYPE_WHEELED,
	0,//PROPULSION_TYPE_TRACKED,
	0,//PROPULSION_TYPE_LEGGED,
	2,//PROPULSION_TYPE_HOVER,
	3,//PROPULSION_TYPE_LIFT,
	1,//PROPULSION_TYPE_PROPELLOR,
	0,//PROPULSION_TYPE_HALF_TRACKED,
};
// Propulsion used in for-loops FOR WRITING DATA. We don't want to process "0" index multiple times.
const std::map<PROPULSION_TYPE, int> propulsionToIndexUnique
{
	{PROPULSION_TYPE_WHEELED, 0},
	{PROPULSION_TYPE_PROPELLOR, 1},
	{PROPULSION_TYPE_HOVER, 2},
	{PROPULSION_TYPE_LIFT, 3}
};

void initCostFields();
void destroyCostFields();
void destroyflowfieldCaches();

struct FLOWFIELDREQUEST
{
	/// Target position: 2 bytes, highest byte is X, lowest byte is Y
	uint16_t goal;
	PROPULSION_TYPE propulsion;
};

void flowfieldInit() {
	if (!isFlowfieldEnabled()) return;

	if(mapWidth == 0 || mapHeight == 0){
		// called by both stageTwoInitialise() and stageThreeInitialise().
		// in the case of both these being called, map will be unavailable the first time.
		return;
	}

	initCostFields();
}

void flowfieldDestroy() {
	if (!isFlowfieldEnabled()) return;

	destroyCostFields();
	destroyflowfieldCaches();
}


// If the path finding system is shutdown or not
static volatile bool ffpathQuit = false;

// threading stuff
static WZ_THREAD        *ffpathThread = nullptr;
static WZ_MUTEX         *ffpathMutex = nullptr;
static WZ_SEMAPHORE     *ffpathSemaphore = nullptr;
static std::list<FLOWFIELDREQUEST> flowfieldRequests;
static std::map<std::pair<uint16_t, PROPULSION_TYPE>, bool> flowfieldCurrentlyActiveRequests;
std::mutex flowfieldMutex;

void processFlowfield(FLOWFIELDREQUEST request);

void calculateFlowfieldAsync(unsigned int targetX, unsigned int targetY, PROPULSION_TYPE propulsion) {
	uint16_t goal  = tileTo2Dindex ( map_coord(targetX), map_coord(targetY) );
	debug (LOG_FLOWFIELD, "calculating path to %i %i (=%i)", map_coord(targetX), map_coord(targetY), goal);
	FLOWFIELDREQUEST request;
	request.goal = goal;
	request.propulsion = propulsion;
	
	if(flowfieldCurrentlyActiveRequests.count(std::make_pair(goal, propulsion)))
	{
		debug (LOG_FLOWFIELD, "already requested this exact flowfield.");
		return; // already requested this exact flowfield. patience is golden.
	}

	wzMutexLock(ffpathMutex);

	bool isFirstRequest = flowfieldRequests.empty();
	flowfieldRequests.push_back(request);
	
	wzMutexUnlock(ffpathMutex);
	
	if (isFirstRequest)
	{
		wzSemaphorePost(ffpathSemaphore);  // Wake up processing thread.
	}
}

/** This runs in a separate thread */
static int ffpathThreadFunc(void *)
{
	wzMutexLock(ffpathMutex);

	while (!ffpathQuit)
	{
		if (flowfieldRequests.empty())
		{
			wzMutexUnlock(ffpathMutex);
			wzSemaphoreWait(ffpathSemaphore);  // Go to sleep until needed.
			wzMutexLock(ffpathMutex);
			continue;
		}

		if(!flowfieldRequests.empty())
		{
			// Copy the first request from the queue.
			auto request = std::move(flowfieldRequests.front());
			flowfieldRequests.pop_front();

			flowfieldCurrentlyActiveRequests.insert(
				std::make_pair(
					std::make_pair(request.goal, request.propulsion),
					true));
			wzMutexUnlock(ffpathMutex);
			debug (LOG_FLOWFIELD, "started processing flowfield request %i", request.goal);
			auto start = std::chrono::high_resolution_clock::now();
			processFlowfield(request);
			auto end = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
			debug (LOG_FLOWFIELD, "processing took %li", duration.count());
			wzMutexLock(ffpathMutex);
			flowfieldCurrentlyActiveRequests.erase(std::make_pair(request.goal, request.propulsion));
		}
	}
	wzMutexUnlock(ffpathMutex);
	return 0;
}

// initialise the findpath module
bool ffpathInitialise()
{
	// The path system is up
	ffpathQuit = false;

	if (!ffpathThread)
	{
		ffpathMutex = wzMutexCreate();
		ffpathSemaphore = wzSemaphoreCreate(0);
		ffpathThread = wzThreadCreate(ffpathThreadFunc, nullptr);
		wzThreadStart(ffpathThread);
	}

	return true;
}


void ffpathShutdown()
{
	if (ffpathThread)
	{
		// Signal the path finding thread to quit
		ffpathQuit = true;
		wzSemaphorePost(ffpathSemaphore);  // Wake up thread.

		wzThreadJoin(ffpathThread);
		ffpathThread = nullptr;
		wzMutexDestroy(ffpathMutex);
		ffpathMutex = nullptr;
		wzSemaphoreDestroy(ffpathSemaphore);
		ffpathSemaphore = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////

/** Cost integration field
 * Highest bit will be used for Line-of-sight flag.
 * so we have 15bits for actual integrated cost
*/
struct IntegrationField
{
	uint16_t goal;
	uint8_t goalXExtent;
	uint8_t goalYExtent;
	IntegrationField (uint16_t goal_, uint8_t goalXExtent_, uint8_t goalYExtent_) :
		goal(goal_), goalXExtent(goalXExtent_), goalYExtent(goalYExtent_) {}
	ICostType cost[FF_MAP_AREA] = {0};
	ICostType getCost(uint8_t x, uint8_t y) const
	{
		return this->cost[tileTo2Dindex(x, y)] & ~(1 << 15);
	}
	ICostType getCost(uint16_t index) const
	{
		return this->cost[index] & ~(1 << 15);
	}

	/** Set cost and overrides flags. */
	void setCost(uint8_t x, uint8_t y, ICostType value)
	{
		ASSERT (value <= 0x7FFF, "cost is too high! Highest bit is reserved for flags");
		this->cost[tileTo2Dindex(x, y)] = value;
	}
	/** Set cost and overrides flags. */
	void setCost(uint16_t index, ICostType value)
	{
		ASSERT (value <= 0x7FFF, "cost is too high! Highest bit is reserved for flags");
		this->cost[index] = value;
	}
};

/** Cost of movement for each map tile */
struct CostField
{
	CostType cost[FF_MAP_AREA];
	void setCost(uint8_t x, uint8_t y, CostType value)
	{
		this->cost[tileTo2Dindex(x, y)] = value;
	}
	CostType getCost(uint8_t x, uint8_t y) const
	{
		return this->cost[tileTo2Dindex(x, y)];
	}
	void setCost(uint16_t index, CostType value)
	{
		this->cost[index] = value;
	}
	CostType getCost(uint16_t index) const
	{
		return this->cost[index];
	}
};

/// auto increment state for flowfield ids. Used on psDroid->sMove.flowfieldId. 
// 0 means no flowfield exists for the unit.
unsigned int flowfieldIdIncrementor = 1;

/** Contains direction vectors for each map tile */
struct Flowfield
{
	unsigned int id;
	// top-left corner of the goal area
	uint16_t goal;
	// goal can be an area, which starts at "goal",
	// and of TotalSurface = (goalXExtent) x (goalYExtent)
	// a simple goal sized 1 tile will have both extents equal to 1
	// a square goal sized 4 will have both extends equal to 2
	// vertical line of len = 2 will have goalYExtent equal to 2, and goalXextent equal to 1
	uint8_t goalXExtent = 1;
	uint8_t goalYExtent = 1;
	#ifdef DEBUG
	// pointless in release builds because is only need for "debugDrawFlowfield"
	std::unique_ptr<IntegrationField> integrationField;
	#endif
	std::array<Directions, FF_MAP_AREA> dirs;
	// TODO use bitfield
	std::array<bool, FF_MAP_AREA> impassable;
	void setImpassable (uint8_t x, uint8_t y)
	{
		uint16_t tile = tileTo2Dindex(x, y);
		impassable[tile] = true;
	}
	bool isImpassable (uint8_t x, uint8_t y) const
	{
		return impassable[tileTo2Dindex(x, y)];
	}
	void setDir (uint8_t x, uint8_t y, Directions dir)
	{
		dirs[tileTo2Dindex(x, y)] = dir;
	}
	Vector2f getVector(uint8_t x, uint8_t y) const
	{
		auto idx = tileTo2Dindex(x, y);
		return dirToVec[(int) dirs[idx]];
	}
};

using FlowFieldUptr = std::unique_ptr<Flowfield>;
using CostFieldUptr = std::unique_ptr<CostField>;

// Cost fields for ground, hover and lift movement types
std::array<CostFieldUptr, 4> costFields
{
	CostFieldUptr(new CostField()), // PROPULSION_TYPE_WHEELED
	CostFieldUptr(new CostField()), // PROPULSION_TYPE_PROPELLOR
	CostFieldUptr(new CostField()), // PROPULSION_TYPE_HOVER
	CostFieldUptr(new CostField()), // PROPULSION_TYPE_LIFT
};

// Flow field cache for ground, hover and lift movement types
// key: Map Array index
std::array<std::unique_ptr<std::map<uint16_t, FlowFieldUptr>>, 4> flowfieldCaches 
{
	std::unique_ptr<std::map<uint16_t, FlowFieldUptr>>(new std::map<uint16_t, FlowFieldUptr>()),
	std::unique_ptr<std::map<uint16_t, FlowFieldUptr>>(new std::map<uint16_t, FlowFieldUptr>()),
	std::unique_ptr<std::map<uint16_t, FlowFieldUptr>>(new std::map<uint16_t, FlowFieldUptr>()),
	std::unique_ptr<std::map<uint16_t, FlowFieldUptr>>(new std::map<uint16_t, FlowFieldUptr>())
};

bool tryGetFlowfieldForTarget(unsigned int targetX,
                              unsigned int targetY,
							  PROPULSION_TYPE propulsion,
							  unsigned int &flowfieldId)
{
	// caches needs to be updated each time there a structure built / destroyed

	const auto flowfieldCache = flowfieldCaches[propulsionIdx2[propulsion]].get();
	Vector2i goal { map_coord(targetX), map_coord(targetY) };

	// this check is already done in fpath.cpp.
	// TODO: we should perhaps refresh the flowfield instead of just bailing here.
	if (!flowfieldCache->count(tileTo2Dindex(goal)))
	{
		return false;
	}
	const auto flowfield = flowfieldCache->at(tileTo2Dindex(goal)).get();
	flowfieldId = flowfield->id;
	return true;
}

std::map<unsigned int, Flowfield*> flowfieldById;
bool tryGetFlowfieldVector(unsigned int flowfieldId, uint8_t x, uint8_t y, Vector2f& vector)
{
	if(!flowfieldById.count(flowfieldId)) return false;
	auto flowfield = flowfieldById.at(flowfieldId);
	Vector2f v = flowfield->getVector(x, y);
	vector = { v.x, v.y };
	return true;
}

bool flowfieldIsImpassable(unsigned int flowfieldId, uint8_t x, uint8_t y)
{
	auto flowfield = flowfieldById.at(flowfieldId);
	return flowfield->isImpassable(x, y);
}

struct Node
{
	uint16_t predecessorCost;
	uint16_t index;

	bool operator<(const Node& other) const {
		// We want top element to have lowest cost
		return predecessorCost > other.predecessorCost;
	}
};

inline bool isInsideGoal (uint16_t goal, uint8_t x, uint8_t y, uint8_t extentx, uint8_t extenty)
{
	return x < xFrom2Dindex(goal) + extentx &&
				x >= xFrom2Dindex(goal) &&
			    y < yFrom2Dindex(goal) + extenty &&
				y >= yFrom2Dindex(goal);
}

void calculateIntegrationField(IntegrationField* integrationField, const CostField* costField);
void calculateFlowfield(Flowfield* flowField, IntegrationField* integrationField);

void processFlowfield(FLOWFIELDREQUEST request)
{
	// NOTE for us noobs!!!! This function is executed on its own thread!!!!
	static_assert(PROPULSION_TYPE_NUM == 7, "new propulsions need to handled!!");
	const auto costField = costFields[propulsionIdx2[request.propulsion]].get();
	const auto flowfieldCache = flowfieldCaches[propulsionIdx2[request.propulsion]].get();
 	// this check is already done in fpath.cpp.
	// TODO: we should perhaps refresh the flowfield instead of just bailing here.
	if (flowfieldCache->count(request.goal)) return;

	IntegrationField* integrationField = new IntegrationField(request.goal, DEFAULT_EXTENT_X, DEFAULT_EXTENT_Y);
	calculateIntegrationField(integrationField, costField);

	auto flowfield = new Flowfield();
	flowfield->id = flowfieldIdIncrementor++;
	flowfield->integrationField = std::unique_ptr<IntegrationField>(integrationField);
	flowfield->goal = request.goal;
	flowfield->goalXExtent = integrationField->goalXExtent;
	flowfield->goalYExtent = integrationField->goalYExtent;
	flowfieldById.insert(std::make_pair(flowfield->id, flowfield));
	debug (LOG_FLOWFIELD, "calculating flowfield %i, at x=%i (len %i) y=%i(len %i)", flowfield->id, 
		xFrom2Dindex(request.goal), flowfield->goalXExtent,
		yFrom2Dindex(request.goal), flowfield->goalYExtent);
	calculateFlowfield(flowfield, integrationField);
	// store the result, this will be checked by fpath.cpp
	{
		std::lock_guard<std::mutex> lock(flowfieldMutex);
		flowfieldCache->insert(std::make_pair(request.goal, std::unique_ptr<Flowfield>(flowfield)));
	}
}

void integrateFlowfieldPoints(std::priority_queue<Node> &openSet,
                              IntegrationField* integrationField,
							  const CostField* costField,
							  std::set<uint16_t>* stationaryDroids);

void calculateIntegrationField(IntegrationField* integrationField,
							   const CostField* costField)
{
	// TODO: ? maybe, split COST_NOT_PASSABLE into a few constants, for terrain, buildings and maybe sth else
	for (int x = 0; x < mapWidth; x++)
	{
		for (int y = 0; y < mapHeight; y++)
		{
			integrationField->setCost(x, y, COST_NOT_PASSABLE);
		}
	}

	std::set<uint16_t> stationaryDroids;
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		for (DROID *psCurr = apsDroidLists[i]; psCurr != nullptr; psCurr = psCurr->psNext)
		{
			if(psCurr->sMove.Status == MOVEINACTIVE){
				stationaryDroids.insert(tileTo2Dindex ({ map_coord(psCurr->pos.x), map_coord(psCurr->pos.y) }));
			}
		}
	}

	// Thanks to priority queue, we get the water ripple effect - closest tile first.
	// First we go where cost is the lowest, so we don't discover better path later.
	uint8_t _goalx, _goaly;
	tileFrom2Dindex (integrationField->goal, _goalx, _goaly);
	std::priority_queue<Node> openSet;
	for (int dx = 0; dx < integrationField->goalXExtent; dx++)
	{
		for (int dy = 0; dy < integrationField->goalYExtent; dy++)
		{
			//integrationField->setCost(_goalx + dx, _goaly + dy, COST_MIN);
			openSet.push(Node { 0, tileTo2Dindex(_goalx + dx, _goaly + dy) });
		}
	}
	while (!openSet.empty())
	{
		integrateFlowfieldPoints(openSet, integrationField, costField, &stationaryDroids);
		openSet.pop();
	}
}

const std::array<Directions, 3> quad1_vecs = {
	Directions::DIR_4,
	Directions::DIR_2,
	Directions::DIR_1
};
const std::array<Directions, 3> quad2_vecs = {
	Directions::DIR_1,
	Directions::DIR_0,
	Directions::DIR_3
};
const std::array<Directions, 3> quad3_vecs = {
	Directions::DIR_3,
	Directions::DIR_5,
	Directions::DIR_6
};
const std::array<Directions, 3> quad4_vecs = {
	Directions::DIR_6,
	Directions::DIR_7,
	Directions::DIR_4
};

template <typename T>
uint8_t minIndex (T a, T b, T c)
{
	auto min = std::min(a, std::min(b, c));
	return min == a ? 0 : (min == b ? 1 : 2);
}

template <typename T>
uint8_t minIndex (T a, T b, T c, T d)
{
	auto min = std::min(a, std::min(b, std::min(c, d)));
	return min == a ? 0 : (min == b ? 1 : (min == c ? 2 : 3));
}

uint8_t minIndex (uint16_t a[], size_t a_len)
{
	uint16_t min_so_far = std::numeric_limits<uint16_t>::max();
	uint8_t min_idx = a_len;
	for (int i = 0; i < a_len; i++)
	{
		if (a[i] < min_so_far)
		{
			min_so_far = a[i];
			min_idx = i;
		}
	}
	const int straight_idx[4] = {
		(int) ((int) Directions::DIR_1 - (int) Directions::DIR_0),
		(int) ((int) Directions::DIR_4 - (int) Directions::DIR_0),
		(int) ((int) Directions::DIR_6 - (int) Directions::DIR_0),
		(int) ((int) Directions::DIR_3 - (int) Directions::DIR_0)
	};
	// prefer straight lines over diagonals
	for (int i = 0; i < 4; i ++)
	{
		if (min_so_far == a[straight_idx[i]])
		{
			return straight_idx[i];
		}
	}
	return min_idx;
}

Directions findClosestDirection (Quadrant q, Vector2f real)
{
	float lena, lenb, lenc;
	uint8_t minidx;
	#define BODY(QUAD_VEC) lena = glm::length(real - dirToVec[(int) QUAD_VEC [0]]); \
		lenb = glm::length(real - dirToVec[(int) QUAD_VEC [1]]); \
		lenc = glm::length(real - dirToVec[(int) QUAD_VEC [2]]); \
		minidx = minIndex(lena, lenb, lenc); \
		return QUAD_VEC [minidx]

	switch (q)
	{
	case Quadrant::Q1:
		BODY(quad1_vecs);
	case Quadrant::Q2:
		BODY(quad2_vecs);
	case Quadrant::Q3:
		BODY(quad3_vecs);
	case Quadrant::Q4:
		BODY(quad4_vecs);
	default:
		ASSERT(false, "stoopid compiler");
		return Directions::DIR_0;
	}
}

/** Find which direction fits more precisely toward the goal.
 * Returns approximate Direction
 */
Directions findClosestDirection (Vector2f real)
{
	if (real.x >= 0 && real.y < 0)
	{
		return findClosestDirection (Quadrant::Q1, real);
	}
	else if (real.x < 0 && real.y  < 0)
	{
		return findClosestDirection (Quadrant::Q2, real);
	}
	else if (real.x < 0 && real.y >= 0)
	{
		return findClosestDirection (Quadrant::Q3, real);
	}
	else
	{
		return findClosestDirection (Quadrant::Q4, real);
	}
}

/** Wave-front expansion, from a tile to all 8 of its neighbors */
void integrateFlowfieldPoints(std::priority_queue<Node> &openSet,
                              IntegrationField* integrationField,
							  const CostField* costField,
							  std::set<uint16_t>* stationaryDroids)
{
	const Node& node = openSet.top();
	auto cost = costField->getCost(node.index);

	if (cost == COST_NOT_PASSABLE) return;
	if (stationaryDroids->count(node.index)) return;

	// Go to the goal, no matter what
	if (node.predecessorCost == 0)
	{
		cost = COST_MIN;
	}

	const ICostType integrationCost = node.predecessorCost + cost;
	const ICostType oldIntegrationCost = integrationField->getCost(node.index);
	uint8_t x, y;
	tileFrom2Dindex (node.index, x, y);
	if (integrationCost < oldIntegrationCost)
	{
		integrationField->setCost(node.index, integrationCost);
		for (int neighb = (int) Directions::DIR_0; neighb < DIR_TO_VEC_SIZE; neighb++)
		{
			uint8_t neighbx, neighby;
			neighbx = x + neighbors[neighb][0];
			neighby = y + neighbors[neighb][1];
			if (neighby > 0 && neighby < mapHeight &&
			    neighbx > 0 && neighbx < mapWidth)
			{
				openSet.push({ integrationCost, tileTo2Dindex(neighbx, neighby) });
			}
		}
	}
}

void calculateFlowfield(Flowfield* flowField, IntegrationField* integrationField)
{
	for (int y = 0; y < mapHeight; y++)
	{
		for (int x = 0; x < mapWidth; x++)
		{
			if (integrationField->getCost(x, y) == COST_NOT_PASSABLE) {
				// Skip goal and non-passable
				// TODO: probably 0.0 should be only for actual goals, not intermediate goals when crossing sectors
				flowField->setImpassable(x, y);
			}
		}
	}
	static_assert((int) Directions::DIR_NONE == 0, "Invariant failed");
	static_assert(DIR_TO_VEC_SIZE == 9, "dirToVec must be sync with Directions!");

	for (int y = 0; y < mapHeight; y++)
	{
		for (int x = 0; x < mapWidth; x++)
		{
			const auto cost = integrationField->getCost(x, y);
			if (cost == COST_NOT_PASSABLE) 
			{
				continue;
			}
			if (isInsideGoal (flowField->goal, x, y, flowField->goalXExtent, flowField->goalYExtent))
			{
				continue;
				flowField->setDir(x, y, Directions::DIR_NONE);
			}
			uint16_t costs[8] = {0};
			// we don't care about DIR_NONE
			for (int neighb = (int) Directions::DIR_0; neighb < DIR_TO_VEC_SIZE; neighb++)
			{
				// substract 1, because Directions::DIR_0 is 1
				costs[neighb - 1] = integrationField->getCost(x + neighbors[neighb][0], y + neighbors[neighb][1]);
			}
			uint8_t minCostIdx = minIndex (costs, 8);
			ASSERT (minCostIdx >= 0 && minCostIdx < 8, "invariant failed");
			// yes, add one because DIR_NONE == 0 and DIR_0  == 1
			Directions dir = (Directions) (minCostIdx + (int) Directions::DIR_0);
			flowField->setDir(x, y, dir);
		}
	}
}

/** Update a given tile as impossible to cross.
 * TODO also must invalidate cached flowfields, because they
 * were all based on obsolete cost/integration fields!
 * Is there any benefit in updating Integration Field instead of Cost Field?
 */
void markTileAsImpassable(uint8_t x, uint8_t y, PROPULSION_TYPE prop)
{
	costFields[propulsionToIndexUnique.at(prop)]->setCost(x, y, COST_NOT_PASSABLE);
}

void markTileAsDefaultCost(uint8_t x, uint8_t y, PROPULSION_TYPE prop)
{
	costFields[propulsionToIndexUnique.at(prop)]->setCost(x, y, COST_MIN);
}

CostType calculateTileCost(uint16_t x, uint16_t y, PROPULSION_TYPE propulsion)
{
	// TODO: Current impl forbids VTOL from flying over short buildings
	if (!fpathBlockingTile(x, y, propulsion))
	{
		int pMax, pMin;
		getTileMaxMin(x, y, &pMax, &pMin);

		const auto delta = static_cast<uint16_t>(pMax - pMin);

		if (propulsion != PROPULSION_TYPE_LIFT && delta > SLOPE_THRESOLD)
		{
			// Yes, the cost is integer and we do not care about floating point tail
			return std::max(COST_MIN, static_cast<CostType>(SLOPE_COST_BASE * delta));
		}
		else
		{
			return COST_MIN;
		}
	}

	return COST_NOT_PASSABLE;
}

void initCostFields()
{
	for (int x = 0; x < mapWidth; x++)
	{
		for (int y = 0; y < mapHeight; y++)
		{
			auto cost_0 = calculateTileCost(x, y, PROPULSION_TYPE_WHEELED);
			auto cost_1 = calculateTileCost(x, y, PROPULSION_TYPE_PROPELLOR);
			auto cost_2 = calculateTileCost(x, y, PROPULSION_TYPE_HOVER);
			auto cost_3 = calculateTileCost(x, y, PROPULSION_TYPE_LIFT);
			costFields[propulsionIdx2[PROPULSION_TYPE_WHEELED]]->setCost(x, y, cost_0);
			costFields[propulsionIdx2[PROPULSION_TYPE_PROPELLOR]]->setCost(x, y, cost_1);
			costFields[propulsionIdx2[PROPULSION_TYPE_HOVER]]->setCost(x, y, cost_2);
			costFields[propulsionIdx2[PROPULSION_TYPE_LIFT]]->setCost(x, y, cost_3);
		}
	}
	debug (LOG_FLOWFIELD, "init cost field done.");
}

void destroyCostFields()
{
	// ?
}

void destroyflowfieldCaches() 
{
	// for (auto&& pair : propulsionToIndexUnique) {
	// 	flowfieldCaches[pair.second]->clear();
	// }
}

void debugDrawFlowfield(const glm::mat4 &mvp) 
{
	const auto playerXTile = map_coord(playerPos.p.x);
	const auto playerZTile = map_coord(playerPos.p.z);
	Flowfield* flowfield = nullptr;
	for (DROID *psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		if (!psDroid->selected)
		{
			continue;
		}
		if (psDroid->sMove.Status != MOVEPOINTTOPOINT)
		{
			continue;
		}
		if (psDroid->sMove.flowfieldId == 0)
		{
			continue;
		}

		flowfield = flowfieldById.at(psDroid->sMove.flowfieldId);
		break;
	}
	if (!flowfield) return;
	
	const auto& costField = costFields[propulsionIdx2[PROPULSION_TYPE_WHEELED]];
	for (auto deltaX = -6; deltaX <= 6; deltaX++)
	{
		const auto x = playerXTile + deltaX;

		if(x < 0) continue;
		
		for (auto deltaZ = -6; deltaZ <= 6; deltaZ++)
		{
			const auto z = playerZTile + deltaZ;

			if(z < 0) continue;

			const int XA = world_coord(x);
			const int XB = world_coord(x + 1);
			const int ZA = world_coord(z);
			const int ZB = world_coord(z + 1);
			const int YAA = map_TileHeight(x, z);
			const int YBA = map_TileHeight(x + 1, z);
			const int YAB = map_TileHeight(x, z + 1);
			const int YBB = map_TileHeight(x + 1, z + 1);
			
			int height = map_TileHeight(x, z);

			// tile
			iV_PolyLine({
				{ XA, YAA, -ZA },
				{ XA, YAB, -ZB },
				{ XB, YBB, -ZB },
				{ XB, YBA, -ZA },
				{ XA, YAA, -ZA },
			}, mvp, WZCOL_GREY);

			// cost
			const Vector3i a = { (XA + 20), height, -(ZA + 20) };
			Vector2i b;
			pie_RotateProjectWithPerspective(&a, mvp, &b);
			auto cost = costField->getCost(x, z);
			if(!flowfield->isImpassable(x, z))
			{
				WzText costText(WzString::fromUtf8(std::to_string(cost)), font_small);
				costText.render(b.x, b.y, WZCOL_TEXT_BRIGHT);
				// position
				if(x < 999 && z < 999){
					char positionString[7];
					ssprintf(positionString, "%i,%i", x, z);
					const Vector3i positionText3dCoords = { (XA + 20), height, -(ZB - 20) };
					Vector2i positionText2dCoords;

					pie_RotateProjectWithPerspective(&positionText3dCoords, mvp, &positionText2dCoords);
					WzText positionText(positionString, font_small);
					positionText.render(positionText2dCoords.x, positionText2dCoords.y, WZCOL_RED);
				}
			}
	 	}
	}
	// flowfields

	for (auto deltaX = -6; deltaX <= 6; deltaX++)
	{
		const int x = playerXTile + deltaX;
		
		if (x < 0) continue;

		for (auto deltaZ = -6; deltaZ <= 6; deltaZ++)
		{
			const int z = playerZTile + deltaZ;

			if (z < 0) continue;
			
			
			const float XA = world_coord(x);
			const float ZA = world_coord(z);

			auto vector = flowfield->getVector(x, z);
			
			auto startPointX = XA + FF_TILE_SIZE / 2;
			auto startPointY = ZA + FF_TILE_SIZE / 2;

			auto height = map_TileHeight(x, z) + 10;

			// origin
			if (!flowfield->isImpassable(x, z))
			{
				iV_PolyLine({
					{ startPointX - 10, height, -startPointY - 10 },
					{ startPointX - 10, height, -startPointY + 10 },
					{ startPointX + 10, height, -startPointY + 10 },
					{ startPointX + 10, height, -startPointY - 10 },
					{ startPointX - 10, height, -startPointY - 10 },
				}, mvp, WZCOL_WHITE);
				
				// direction
				iV_PolyLine({
					{ startPointX, height, -startPointY },
					{ startPointX + vector.x * 75, height, -startPointY - vector.y * 75 },
				}, mvp, WZCOL_WHITE);
			}

			// integration fields
			const Vector3i integrationFieldText3dCoordinates { (XA + 20), height, -(ZA + 40) };
			Vector2i integrationFieldText2dCoordinates;

			pie_RotateProjectWithPerspective(&integrationFieldText3dCoordinates, mvp, &integrationFieldText2dCoordinates);
			auto integrationCost = flowfield->integrationField->getCost(x, z);
			if (!flowfield->isImpassable(x, z))
			{
				WzText costText(WzString::fromUtf8 (std::to_string(integrationCost)), font_small);
				costText.render(integrationFieldText2dCoordinates.x, integrationFieldText2dCoordinates.y, WZCOL_TEXT_BRIGHT);
			}
		}
	}
	for (int dx = 0; dx < flowfield->goalXExtent; dx++)
	{
		for (int dy = 0; dy < flowfield->goalYExtent; dy++)
		{
			uint8_t _goalx, _goaly;
			tileFrom2Dindex (flowfield->goal, _goalx, _goaly);
			_goalx += dx;
			_goaly += dy;
			auto goalX = world_coord(_goalx) + FF_TILE_SIZE / 2;
			auto goalY = world_coord(_goaly) + FF_TILE_SIZE / 2;
			auto height = map_TileHeight(_goalx, _goaly) + 10;
			iV_PolyLine({
				{ goalX - 10 + 3, height, -goalY - 10 + 3 },
				{ goalX - 10 + 3, height, -goalY + 10 + 3 },
				{ goalX + 10 + 3, height, -goalY + 10 + 3 },
				{ goalX + 10 + 3, height, -goalY - 10 + 3 },
				{ goalX - 10 + 3, height, -goalY - 10 + 3 },
			}, mvp, WZCOL_RED);
		}
	}
}

#define VECTOR_FIELD_DEBUG 1
void debugDrawFlowfields(const glm::mat4 &mvp)
{
	if (!isFlowfieldEnabled()) return;
	if (VECTOR_FIELD_DEBUG) debugDrawFlowfield(mvp);
}
