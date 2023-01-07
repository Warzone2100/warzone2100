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
constexpr const CostType COST_ZERO = 0; // goal is free

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
			processFlowfield(request);
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

	void setHasLOS(uint16_t idx)
	{
		this->cost[idx] |= (1 << 15);
	}
	void clearLOS(uint8_t x, uint8_t y)
	{
		this->cost[tileTo2Dindex(x, y)] &= ~(1 << 15);
	}
	bool hasLOS(uint16_t idx)
	{
		return (this->cost[idx] & (1 << 15)) > 0;
	}
	bool hasLOS(uint8_t x, uint8_t y)
	{
		return (this->cost[tileTo2Dindex(x, y)]  & (1 << 15)) > 0;
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

// TODO: Remove this in favor of glm.
struct VectorT
{
	float x;
	float y;

	void normalize()
	{
		const float length = std::sqrt(std::pow(x, 2) + std::pow(y, 2));
		if (length != 0) {
			x /= length;
			y /= length;
		}
	}
};

/// auto increment state for flowfield ids. Used on psDroid->sMove.flowfield. 
// 0 means no flowfield exists for the unit.
unsigned int flowfieldIdIncrementor = 1;

/** Contains direction vectors for each map tile */
struct Flowfield
{
	unsigned int id;
	// tile coordinates of movement goal
	uint16_t goal;

	#ifdef DEBUG
	// pointless in release builds
	std::unique_ptr<IntegrationField> integrationField;
	#endif

	std::array<VectorT, FF_MAP_AREA> vectors;

	void setVector(uint8_t x, uint8_t y, VectorT vector) {
		vectors[tileTo2Dindex(x, y)] = vector;
	}
	VectorT getVector(uint8_t x, uint8_t y) const {
		return vectors[tileTo2Dindex(x, y)];
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
	VectorT v = flowfield->getVector(x, y);
	vector = { v.x, v.y };
	return true;
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

void calculateIntegrationField(uint16_t goal, IntegrationField* integrationField, const CostField* costField);
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

	IntegrationField* integrationField = new IntegrationField();
	calculateIntegrationField(request.goal, integrationField, costField);

	auto flowfield = new Flowfield();
	flowfield->id = flowfieldIdIncrementor++;
	flowfield->integrationField = std::unique_ptr<IntegrationField>(integrationField);
	flowfield->goal = request.goal;
	flowfieldById.insert(std::make_pair(flowfield->id, flowfield));
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
							  uint16_t goal,
							  std::set<uint16_t>* stationaryDroids);

void calculateIntegrationField(uint16_t goal,
                               IntegrationField* integrationField,
							   const CostField* costField)
{
	// TODO: here do checking if given tile contains a building (instead of doing that in cost field)
	// TODO: split COST_NOT_PASSABLE into a few constants, for terrain, buildings and maybe sth else
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
	std::priority_queue<Node> openSet;
	openSet.push(Node { 0, goal });
	integrationField->setHasLOS(goal);
	while (!openSet.empty())
	{
		integrateFlowfieldPoints(openSet, integrationField, costField, goal, &stationaryDroids);
		openSet.pop();
	}
}

/** Sets LOS flag for those cells, from which Goal is visible, from all 8 directions.
 * Example : https://howtorts.github.io/2014/01/30/Flow-Fields-LOS.html
 * Without LOS, even in straight line to the goal, flowfield vectors may point in
 * wierd, from human perspective, directions.
*/
void calculateLOS(IntegrationField *integfield, uint16_t at, uint16_t goal)
{
	if (at == goal)
	{
		integfield->setHasLOS(at);
		return;
	}
	ASSERT (integfield->hasLOS(goal), "invariant failed: goal must have LOS");
	// we want signed difference
	int16_t dx, dy;
	Vector2i at_tile = tileFrom2Dindex (at);
	dx = static_cast<int16_t>(xFrom2Dindex(goal)) - static_cast<int16_t>(xFrom2Dindex(at));
	dy = static_cast<int16_t>(yFrom2Dindex(goal)) - static_cast<int16_t>(yFrom2Dindex(at));
	
	int16_t dx_one, dy_one;
	dx_one = dx > 0 ? 1 : -1; // cannot be zero, already checked above
	dy_one = dy > 0 ? 1 : -1;
	
	uint8_t dx_abs, dy_abs;
	dx_abs = std::abs(dx);
	dy_abs = std::abs(dy);
	bool has_los = false;

	// if the cell which 1 closer to goal has LOS, then we *may* have it too
	if (dx_abs >= dy_abs)
	{
		if (integfield->hasLOS(at_tile.x + dx_one, at_tile.y))
			has_los = true;
	}
	if (dy_abs >= dx_abs)
	{
		if (integfield->hasLOS(at_tile.x, at_tile.y + dy_one))
			has_los = true;
	}

	if (dy_abs > 0 && dx_abs > 0)
	{
		// if the diagonal doesn't have LOS, we don't
		if (!integfield->hasLOS(at_tile.x + dx_one, at_tile.y + dy_one))
			has_los = false;
		else if (dx_abs == dy_abs)
		{
			if (COST_NOT_PASSABLE == (integfield->getCost(at_tile.x + dx_one, at_tile.y)) ||
				COST_NOT_PASSABLE == (integfield->getCost(at_tile.x, at_tile.y + dy_one)))
				has_los = false;
		}
	}
	if (has_los) integfield->setHasLOS(at);
	// if (has_los) debug (LOG_FLOWFIELD, "has los %i %i", at_tile.x, at_tile.y);
	return;
}

void integrateFlowfieldPoints(std::priority_queue<Node> &openSet,
                              IntegrationField* integrationField,
							  const CostField* costField,
							  uint16_t goal,
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
		calculateLOS(integrationField, node.index, goal);
		// North
		if (y > 0)
		{
			openSet.push({ integrationCost, tileTo2Dindex(x, y - 1) });
		}
		// East
		if (x < mapWidth)
		{
			openSet.push({ integrationCost, tileTo2Dindex(x + 1, y) });
		}
		// South
		if (y < mapHeight)
		{
			openSet.push({ integrationCost, tileTo2Dindex(x, y + 1) });
		}
		// West
		if (x > 0) 
		{
			openSet.push({ integrationCost, tileTo2Dindex(x - 1, y) });
		}
	}
}

void calculateFlowfield(Flowfield* flowField, IntegrationField* integrationField)
{
	for (int y = 0; y < mapHeight; y++)
	{
		for (int x = 0; x < mapWidth; x++)
		{
			VectorT vector;
			const auto cost = integrationField->getCost(x, y);
			if (cost == COST_NOT_PASSABLE || cost == COST_MIN) {
				// Skip goal and non-passable
				// TODO: probably 0.0 should be only for actual goals, not intermediate goals when crossing sectors
				flowField->setVector(x, y, VectorT { 0.0f, 0.0f });
				continue;
			}
			// if we have LOS, then it's easy: just head straight to the goal
			if (integrationField->hasLOS(x, y))
			{
				uint8_t goalx, goaly; 
				tileFrom2Dindex(flowField->goal, goalx, goaly);
				vector.x = goalx - x;
				vector.y = goaly - y;
				vector.normalize();
				flowField->setVector(x, y, vector);
				continue;
			}
			uint16_t leftCost = integrationField->getCost(x - 1, y);
			uint16_t rightCost = integrationField->getCost(x + 1, y);
			uint16_t topCost = integrationField->getCost(x, y - 1);
			uint16_t bottomCost = integrationField->getCost(x, y + 1);

			// leftCost = leftCost == COST_NOT_PASSABLE ? cost : leftCost;
			// rightCost = rightCost == COST_NOT_PASSABLE ? cost : rightCost;
			// topCost = topCost == COST_NOT_PASSABLE ? cost : topCost;
			// bottomCost = bottomCost == COST_NOT_PASSABLE ? cost : bottomCost;
			
			const bool leftImpassable = leftCost == COST_NOT_PASSABLE;
			const bool rightImpassable = rightCost == COST_NOT_PASSABLE;
			const bool topImpassable = topCost == COST_NOT_PASSABLE;
			const bool bottomImpassable = bottomCost == COST_NOT_PASSABLE;

			/// without the two following fixes, tiles next to an impassable tile ALWAYS
			/// point straight away from the impassable - like an allergic reaction.
			/// (this is because impassable tiles have MAX cost.)

			/// the fix here is for the horizontal axis.
			if (rightImpassable && !leftImpassable)
			{
				rightCost = std::max(leftCost, cost);
			}
			if (leftImpassable && !rightImpassable)
			{
				leftCost = std::max(rightCost, cost);
			}

			/// the fix here is for the vertical axis.
			if (topImpassable && !bottomImpassable)
			{
				topCost = std::max(bottomCost, cost);
			}
			if (bottomImpassable && !topImpassable)
			{
				bottomCost = std::max(topCost, cost);
			}



			// if we are up against a wall, and the two directions along the wall are equally costly,
			// a tie will happen if the path isn't perpendicular to the wall:
			//
			//     x     <--- target
			//
			//   OOOOO   <--- wall
			//     z     <--- tile where a tie will happen (doesn't know whether to go right or left)
			//
			//
			// The tie will be broken by a semi-random number based on whether x+y is odd or even.
			// Note that breaking the tie will not need to be done if the target is below.
			// This is detected by checking if the tieing tiles have lower or greater cost than the z tile.

			bool tieBraker = ((x + y) % 1) == 1;

			bool rightOrLeftIsImpassable = rightImpassable || leftImpassable;
			bool topAndBottomIsPassable = !topImpassable && !bottomImpassable;
			bool topAndBottomHaveEqualCost = topCost == bottomCost;

			if (rightOrLeftIsImpassable && topAndBottomIsPassable && topAndBottomHaveEqualCost && topCost < cost)
			{
				if (tieBraker)
				{
					topCost = 0;
				}
				else
				{
					bottomCost = 0;
				}
			}

			bool topOrBottomIsImpassable = topImpassable || bottomImpassable;
			bool leftAndRightIsPassable = !leftImpassable && !rightImpassable;
			bool leftAndRightHaveEqualCost = leftCost == rightCost;
			
			if (topOrBottomIsImpassable && leftAndRightIsPassable && leftAndRightHaveEqualCost && leftCost < cost)
			{
				if (tieBraker)
				{
					leftCost = 0;
				}
				else
				{
					rightCost = 0;
				}
			}

			
			vector.x = leftCost - rightCost;
			vector.y = topCost - bottomCost;
			vector.normalize();
			// glm::normalize(glm::vec2 {0.5, 0.8});
			// if (std::abs(vector.x) < 0.01f && std::abs(vector.y) < 0.01f) {
			// 	// Local optima. Tilt the vector in any direction.
			// 	vector.x = 0.1f;
			// 	vector.y = 0.1f; // 6,56
			// }

			flowField->setVector(x, y, vector);
		}
	}
}

/** Update a given tile as impossible to cross.
 * TODO also must invalidate cached flowfields, because they
 * were all based on obsolete cost/integration fields!
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
			if(cost != COST_NOT_PASSABLE)
			{
				WzText costText(WzString::fromUtf8(std::to_string(cost)), font_small);
				costText.render(b.x, b.y, WZCOL_TEXT_BRIGHT);
			}
			
			// position

			if(x < 999 && z < 999 && cost != COST_NOT_PASSABLE){
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
	// flowfields
	// std::set<Flowfield*> flowfieldsToDraw;
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
		if (psDroid->sMove.flowfield == 0)
		{
			continue;
		}

		flowfield = flowfieldById.at(psDroid->sMove.flowfield);
		break;
	}
	if (!flowfield) return;
	
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

			auto cost = costField->getCost(x, z);
			if (cost != COST_NOT_PASSABLE)
			{
				iV_PolyLine({
					{ startPointX - 10, height, -startPointY - 10 },
					{ startPointX - 10, height, -startPointY + 10 },
					{ startPointX + 10, height, -startPointY + 10 },
					{ startPointX + 10, height, -startPointY - 10 },
					{ startPointX - 10, height, -startPointY - 10 },
				}, mvp, WZCOL_WHITE);
			}
			// direction
			bool has_los = flowfield->integrationField->hasLOS(x, z);
			iV_PolyLine({
				{ startPointX, height, -startPointY },
				{ startPointX + vector.x * 75, height, -startPointY - vector.y * 75 },
			}, mvp, has_los ? WZCOL_RED : WZCOL_WHITE);

			// integration fields

			const Vector3i integrationFieldText3dCoordinates { (XA + 20), height, -(ZA + 40) };
			Vector2i integrationFieldText2dCoordinates;

			pie_RotateProjectWithPerspective(&integrationFieldText3dCoordinates, mvp, &integrationFieldText2dCoordinates);
			auto integrationCost = flowfield->integrationField->getCost(x, z);
			if (integrationCost != COST_NOT_PASSABLE)
			{
				WzText costText(WzString::fromUtf8 (std::to_string(integrationCost)), font_small);
				costText.render(integrationFieldText2dCoordinates.x, integrationFieldText2dCoordinates.y, WZCOL_TEXT_BRIGHT);
			}
		}
	}
	
	// goal
	uint8_t _goalx, _goaly;
	tileFrom2Dindex (flowfield->goal, _goalx, _goaly);
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

#define VECTOR_FIELD_DEBUG 1
void debugDrawFlowfields(const glm::mat4 &mvp)
{
	if (!isFlowfieldEnabled()) return;
	if (VECTOR_FIELD_DEBUG) debugDrawFlowfield(mvp);
}
