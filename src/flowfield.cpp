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

static bool flowfieldEnabled = false;

void flowfieldEnable() {
	flowfieldEnabled = true;
}

bool isFlowfieldEnabled() {
	return flowfieldEnabled;
}

//

/// This type is ONLY needed for adding vectors as key to eg. a map.
/// because GLM's vector implementation does not include an is-less-than operator overload,
/// which is required by std::map.
struct ComparableVector2i : Vector2i {
	ComparableVector2i(int x, int y) : Vector2i(x, y) {}
	ComparableVector2i(Vector2i value) : Vector2i(value) {}

	inline bool operator<(const ComparableVector2i& b) const {
		if(x < b.x){
			return true;
		}
		if(x > b.x){
			return false;
		}
		if(y < b.y){
			return true;
		}
		return false;
    }
};

#define FF_MAP_WIDTH 256
#define FF_MAP_HEIGHT 256
#define FF_MAP_AREA FF_MAP_WIDTH*FF_MAP_HEIGHT
#define FF_TILE_SIZE 128

constexpr const unsigned short COST_NOT_PASSABLE = std::numeric_limits<unsigned short>::max();
constexpr const unsigned short COST_MIN = 1;

// Decides how much slopes should be avoided
constexpr const float SLOPE_COST_BASE = 0.01f;
// Decides when terrain height delta is considered a slope
constexpr const unsigned short SLOPE_THRESOLD = 4;

// Propulsion mapping FOR READING DATA ONLY! See below.
const std::map<PROPULSION_TYPE, int> propulsionToIndex
{
	// All these share the same flowfield, because they are different types of ground-only
	{PROPULSION_TYPE_WHEELED, 0},
	{PROPULSION_TYPE_TRACKED, 0},
	{PROPULSION_TYPE_LEGGED, 0},
	{PROPULSION_TYPE_HALF_TRACKED, 0},
	//////////////////////////////////
	{PROPULSION_TYPE_PROPELLOR, 1},
	{PROPULSION_TYPE_HOVER, 2},
	{PROPULSION_TYPE_LIFT, 3}
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
	/// Target position
	Vector2i goal;
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

std::vector<ComparableVector2i> toComparableVectors(std::vector<Vector2i> values){
	std::vector<ComparableVector2i> result;

	for(auto value : values){
		result.push_back(*new ComparableVector2i(value));
	}

	return result;
}

// If the path finding system is shutdown or not
static volatile bool ffpathQuit = false;

// threading stuff
static WZ_THREAD        *ffpathThread = nullptr;
static WZ_MUTEX         *ffpathMutex = nullptr;
static WZ_SEMAPHORE     *ffpathSemaphore = nullptr;
static std::list<FLOWFIELDREQUEST> flowfieldRequests;
static std::map<std::pair<ComparableVector2i, PROPULSION_TYPE>, bool> flowfieldCurrentlyActiveRequests;
std::mutex flowfieldMutex;

void processFlowfield(FLOWFIELDREQUEST request);

void calculateFlowfieldAsync(unsigned int targetX, unsigned int targetY, PROPULSION_TYPE propulsion) {
	Vector2i goal { map_coord(targetX), map_coord(targetY) };

	FLOWFIELDREQUEST request;
	request.goal = goal;
	request.propulsion = propulsion;
	
	if(flowfieldCurrentlyActiveRequests.count(std::make_pair(ComparableVector2i(goal), propulsion))){
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

			flowfieldCurrentlyActiveRequests.insert(std::make_pair(std::make_pair(ComparableVector2i(request.goal), request.propulsion), true));
			wzMutexUnlock(ffpathMutex);
			processFlowfield(request);
			wzMutexLock(ffpathMutex);
			flowfieldCurrentlyActiveRequests.erase(std::make_pair(ComparableVector2i(request.goal), request.propulsion));
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

inline unsigned int coordinateToArrayIndex(unsigned short x, unsigned short y) { return y * FF_MAP_WIDTH + x; }
inline Vector2i arrayIndexToCoordinate(unsigned int index) { return Vector2i { index % FF_MAP_WIDTH, index / FF_MAP_WIDTH }; }

struct IntegrationField {
	unsigned short cost[FF_MAP_AREA];
	void setCost(unsigned int x, unsigned int y, unsigned short value){
		this->cost[coordinateToArrayIndex(x, y)] = value;
	}
	unsigned short getCost(unsigned int x, unsigned int y){
		return this->cost[coordinateToArrayIndex(x, y)];
	}
	void setCost(unsigned int index, unsigned short value){
		this->cost[index] = value;
	}
	unsigned short getCost(unsigned int index){
		return this->cost[index];
	}
};

struct CostField {
	unsigned short cost[FF_MAP_AREA];
	void setCost(unsigned int x, unsigned int y, unsigned short value){
		this->cost[coordinateToArrayIndex(x, y)] = value;
	}
	unsigned short getCost(unsigned int x, unsigned int y){
		return this->cost[coordinateToArrayIndex(x, y)];
	}
	void setCost(unsigned int index, unsigned short value){
		this->cost[index] = value;
	}
	unsigned short getCost(unsigned int index){
		return this->cost[index];
	}
};

// TODO: Remove this in favor of glm.
struct VectorT {
	float x;
	float y;

	void normalize() {
		const float length = std::sqrt(std::pow(x, 2) + std::pow(y, 2));

		if (length != 0) {
			x /= length;
			y /= length;
		}
	}
};

/// auto increment state for flowfield ids. Used on psDroid->sMove.flowfield. 0 means no flowfield exists for the unit.
unsigned int flowfieldIdIncrementor = 1;

struct Flowfield {
	unsigned int id;
	std::vector<Vector2i> goals;
	std::unique_ptr<IntegrationField> integrationField;
	std::array<VectorT, FF_MAP_AREA> vectors;

	void setVector(unsigned short x, unsigned short y, VectorT vector) {
		vectors[coordinateToArrayIndex(x, y)] = vector;
	}
	VectorT getVector(unsigned short x, unsigned short y) const {
		return vectors[coordinateToArrayIndex(x, y)];
	}
};

// Cost fields for ground, hover and lift movement types
std::array<std::unique_ptr<CostField>, 4> costFields {
	std::unique_ptr<CostField>(new CostField()),
	std::unique_ptr<CostField>(new CostField()),
	std::unique_ptr<CostField>(new CostField()),
	std::unique_ptr<CostField>(new CostField()),
};

// Flow field cache for ground, hover and lift movement types
std::array<std::unique_ptr<std::map<std::vector<ComparableVector2i>, std::unique_ptr<Flowfield>>>, 4> flowfieldCaches {
	std::unique_ptr<std::map<std::vector<ComparableVector2i>, std::unique_ptr<Flowfield>>>(new std::map<std::vector<ComparableVector2i>, std::unique_ptr<Flowfield>>()),
	std::unique_ptr<std::map<std::vector<ComparableVector2i>, std::unique_ptr<Flowfield>>>(new std::map<std::vector<ComparableVector2i>, std::unique_ptr<Flowfield>>()),
	std::unique_ptr<std::map<std::vector<ComparableVector2i>, std::unique_ptr<Flowfield>>>(new std::map<std::vector<ComparableVector2i>, std::unique_ptr<Flowfield>>()),
	std::unique_ptr<std::map<std::vector<ComparableVector2i>, std::unique_ptr<Flowfield>>>(new std::map<std::vector<ComparableVector2i>, std::unique_ptr<Flowfield>>())
};

bool tryGetFlowfieldForTarget(unsigned int targetX, unsigned int targetY, PROPULSION_TYPE propulsion, unsigned int &flowfieldId){
	const auto flowfieldCache = flowfieldCaches[propulsionToIndex.at(propulsion)].get();

	std::vector<ComparableVector2i> goals { { map_coord(targetX), map_coord(targetY) } };

 	// this check is already done in fpath.cpp.
	// TODO: we should perhaps refresh the flowfield instead of just bailing here.
	if (!flowfieldCache->count(goals)) {
		return false;
	}

	const auto flowfield = flowfieldCache->at(goals).get();

	flowfieldId = flowfield->id;

	return true;
}

std::map<unsigned int, Flowfield*> flowfieldById;
bool tryGetFlowfieldVector(unsigned int flowfieldId, int x, int y, Vector2f& vector){
	if(!flowfieldById.count(flowfieldId)){
		return false;
	}

	auto flowfield = flowfieldById.at(flowfieldId);

	auto v = flowfield->getVector(x, y);
	vector = { v.x, v.y };

	return true;
}

struct Node {
	unsigned short predecessorCost;
	unsigned int index;

	bool operator<(const Node& other) const {
		// We want top element to have lowest cost
		return predecessorCost > other.predecessorCost;
	}
};

void calculateIntegrationField(const std::vector<ComparableVector2i>& points, IntegrationField* integrationField, CostField* costField);
void calculateFlowfield(Flowfield* flowField, IntegrationField* integrationField);

void processFlowfield(FLOWFIELDREQUEST request) {

	// NOTE for us noobs!!!! This function is executed on its own thread!!!!

	const auto flowfieldCache = flowfieldCaches[propulsionToIndex.at(request.propulsion)].get();
	const auto costField = costFields[propulsionToIndex.at(request.propulsion)].get();

	std::vector<ComparableVector2i> goals { request.goal }; // TODO: multiple goals for formations

 	// this check is already done in fpath.cpp.
	// TODO: we should perhaps refresh the flowfield instead of just bailing here.
	if (flowfieldCache->count(goals)) {
		return;
	}

	IntegrationField* integrationField = new IntegrationField();
	calculateIntegrationField(goals, integrationField, costField);

	auto flowfield = new Flowfield();
	flowfield->id = flowfieldIdIncrementor++;
	flowfield->integrationField = std::unique_ptr<IntegrationField>(integrationField);
	flowfield->goals = { request.goal };
	flowfieldById.insert(std::make_pair(flowfield->id, flowfield));
	calculateFlowfield(flowfield, integrationField);

	{
		std::lock_guard<std::mutex> lock(flowfieldMutex);
		flowfieldCache->insert(std::make_pair(goals, std::unique_ptr<Flowfield>(flowfield)));
	}
}

void integrateFlowfieldPoints(std::priority_queue<Node>& openSet, IntegrationField* integrationField, CostField* costField, std::set<ComparableVector2i>* stationaryDroids);

void calculateIntegrationField(const std::vector<ComparableVector2i>& points, IntegrationField* integrationField, CostField* costField) {
	// TODO: here do checking if given tile contains a building (instead of doing that in cost field)
	// TODO: split COST_NOT_PASSABLE into a few constants, for terrain, buildings and maybe sth else
	for (unsigned int x = 0; x < mapWidth; x++) {
		for (unsigned int y = 0; y < mapHeight; y++) {
			integrationField->setCost(x, y, COST_NOT_PASSABLE);
		}
	}

	std::unique_ptr<std::set<ComparableVector2i>> stationaryDroids = std::unique_ptr<std::set<ComparableVector2i>>(new std::set<ComparableVector2i>());

	for (unsigned i = 0; i < MAX_PLAYERS; i++)
	{
		for (DROID *psCurr = apsDroidLists[i]; psCurr != nullptr; psCurr = psCurr->psNext)
		{
			if(psCurr->sMove.Status == MOVEINACTIVE){
				stationaryDroids->insert({ map_coord(psCurr->pos.x), map_coord(psCurr->pos.y) });
			}
		}
	}

	// Thanks to priority queue, we get the water ripple effect - closest tile first.
	// First we go where cost is the lowest, so we don't discover better path later.
	std::priority_queue<Node> openSet;

	for (auto& point : points) {
		openSet.push({ 0, coordinateToArrayIndex(point.x, point.y) });
	}

	while (!openSet.empty()) {
		integrateFlowfieldPoints(openSet, integrationField, costField, stationaryDroids.get());
		openSet.pop();
	}
}

void integrateFlowfieldPoints(std::priority_queue<Node>& openSet, IntegrationField* integrationField, CostField* costField, std::set<ComparableVector2i>* stationaryDroids) {
	const Node& node = openSet.top();
	auto cost = costField->getCost(node.index);

	if (cost == COST_NOT_PASSABLE) {
		return;
	}

	auto coordinate = arrayIndexToCoordinate(node.index);

	if(stationaryDroids->count({ coordinate })){
		return;
	}

	// Go to the goal, no matter what
	if (node.predecessorCost == 0) {
		cost = COST_MIN;
	}

	const unsigned short integrationCost = node.predecessorCost + cost;
	const unsigned short oldIntegrationCost = integrationField->getCost(node.index);

	if (integrationCost < oldIntegrationCost) {
		integrationField->setCost(node.index, integrationCost);

		// North
		if(coordinate.y > 0){
			openSet.push({ integrationCost, coordinateToArrayIndex(coordinate.x, coordinate.y - 1) });
		}
		// East
		if(coordinate.x < mapWidth){
			openSet.push({ integrationCost, coordinateToArrayIndex(coordinate.x + 1, coordinate.y) });
		}
		// South
		if(coordinate.y < mapHeight){
			openSet.push({ integrationCost, coordinateToArrayIndex(coordinate.x, coordinate.y + 1) });
		}
		// West
		if(coordinate.x > 0){
			openSet.push({ integrationCost, coordinateToArrayIndex(coordinate.x - 1, coordinate.y) });
		}
	}
}

void calculateFlowfield(Flowfield* flowField, IntegrationField* integrationField) {
	for (int y = 0; y < mapHeight; y++) {
		for (int x = 0; x < mapWidth; x++) {
			const auto cost = integrationField->getCost(x, y);
			if (cost == COST_NOT_PASSABLE || cost == COST_MIN) {
				// Skip goal and non-passable
				// TODO: probably 0.0 should be only for actual goals, not intermediate goals when crossing sectors
				flowField->setVector(x, y, VectorT { 0.0f, 0.0f });
				continue;
			}

			unsigned short leftCost = integrationField->getCost(x - 1, y);
			unsigned short rightCost = integrationField->getCost(x + 1, y);
			unsigned short topCost = integrationField->getCost(x, y - 1);
			unsigned short bottomCost = integrationField->getCost(x, y + 1);

			const bool leftImpassable = leftCost == COST_NOT_PASSABLE;
			const bool rightImpassable = rightCost == COST_NOT_PASSABLE;
			const bool topImpassable = topCost == COST_NOT_PASSABLE;
			const bool bottomImpassable = bottomCost == COST_NOT_PASSABLE;

			/// without the two following fixes, tiles next to an impassable tile ALWAYS
			/// point straight away from the impassable - like an allergic reaction.
			/// (this is because impassable tiles have MAX cost.)

			/// the fix here is for the horizontal axis.
			if(rightImpassable && !leftImpassable){
				rightCost = std::max(leftCost, cost);
			}
			if(leftImpassable && !rightImpassable){
				leftCost = std::max(rightCost, cost);
			}

			/// the fix here is for the vertical axis.
			if(topImpassable && !bottomImpassable){
				topCost = std::max(bottomCost, cost);
			}
			if(bottomImpassable && !topImpassable){
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

			if(rightOrLeftIsImpassable && topAndBottomIsPassable && topAndBottomHaveEqualCost && topCost < cost){
				if(tieBraker) {
					topCost = 0;
				} else {
					bottomCost = 0;
				}
			}

			bool topOrBottomIsImpassable = topImpassable || bottomImpassable;
			bool leftAndRightIsPassable = !leftImpassable && !rightImpassable;
			bool leftAndRightHaveEqualCost = leftCost == rightCost;
			
			if(topOrBottomIsImpassable && leftAndRightIsPassable && leftAndRightHaveEqualCost && leftCost < cost){
				if(tieBraker) {
					leftCost = 0;
				} else {
					rightCost = 0;
				}
			}

			VectorT vector;
			vector.x = leftCost - rightCost;
			vector.y = topCost - bottomCost;
			vector.normalize();

			// if (std::abs(vector.x) < 0.01f && std::abs(vector.y) < 0.01f) {
			// 	// Local optima. Tilt the vector in any direction.
			// 	vector.x = 0.1f;
			// 	vector.y = 0.1f; // 6,56
			// }

			flowField->setVector(x, y, vector);
		}
	}
}

unsigned short calculateTileCost(unsigned short x, unsigned short y, PROPULSION_TYPE propulsion)
{
	// TODO: Current impl forbids VTOL from flying over short buildings
	if (!fpathBlockingTile(x, y, propulsion))
	{
		int pMax, pMin;
		getTileMaxMin(x, y, &pMax, &pMin);

		const auto delta = static_cast<unsigned short>(pMax - pMin);

		if (propulsion != PROPULSION_TYPE_LIFT && delta > SLOPE_THRESOLD)
		{
			// Yes, the cost is integer and we do not care about floating point tail
			return std::max(COST_MIN, static_cast<unsigned short>(SLOPE_COST_BASE * delta));
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
			for (auto&& propType : propulsionToIndexUnique)
			{
				auto cost = calculateTileCost(x, y, propType.first);
				costFields[propType.second]->setCost(x, y, cost);
			}
		}
	}
}

void destroyCostFields()
{
	// ?
}

void destroyflowfieldCaches() {
	for (auto&& pair : propulsionToIndexUnique) {
		flowfieldCaches[pair.second]->clear();
	}
}

void debugDrawFlowfield(const glm::mat4 &mvp) {
	const auto playerXTile = map_coord(player.p.x);
	const auto playerZTile = map_coord(player.p.z);
	
	const auto& costField = costFields[propulsionToIndex.at(PROPULSION_TYPE_WHEELED)];

	for (auto deltaX = -6; deltaX <= 6; deltaX++)
	{
		const auto x = playerXTile + deltaX;

		if(x < 0){
			continue;
		}
		
		for (auto deltaZ = -6; deltaZ <= 6; deltaZ++)
		{
			const auto z = playerZTile + deltaZ;

			if(z < 0){
				continue;
			}

			const float XA = world_coord(x);
			const float XB = world_coord(x + 1);
			const float ZA = world_coord(z);
			const float ZB = world_coord(z + 1);
			const float YAA = map_TileHeight(x, z);
			const float YBA = map_TileHeight(x + 1, z);
			const float YAB = map_TileHeight(x, z + 1);
			const float YBB = map_TileHeight(x + 1, z + 1);
			
			float height = map_TileHeight(x, z);

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

			pie_RotateProject(&a, mvp, &b);
			auto cost = costField->getCost(x, z);
			if(cost != COST_NOT_PASSABLE){
				WzText costText(std::to_string(cost), font_small);
				costText.render(b.x, b.y, WZCOL_TEXT_BRIGHT);
			}
			
			// position

			if(x < 999 && z < 999 && cost != COST_NOT_PASSABLE){
				char positionString[7];
				ssprintf(positionString, "%i,%i", x, z);
				const Vector3i positionText3dCoords = { (XA + 20), height, -(ZB - 20) };
				Vector2i positionText2dCoords;

				pie_RotateProject(&positionText3dCoords, mvp, &positionText2dCoords);
				WzText positionText(positionString, font_small);
				positionText.render(positionText2dCoords.x, positionText2dCoords.y, WZCOL_LBLUE);
			}
	 	}
	}

	// flowfields

	std::set<Flowfield*> flowfieldsToDraw;

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

		flowfieldsToDraw.insert(flowfieldById.at(psDroid->sMove.flowfield));
	}

	for(auto flowfield : flowfieldsToDraw){
		for (auto deltaX = -6; deltaX <= 6; deltaX++)
		{
			const auto x = playerXTile + deltaX;

			if(x < 0){
				continue;
			}
			
			for (auto deltaZ = -6; deltaZ <= 6; deltaZ++)
			{
				const auto z = playerZTile + deltaZ;

				if(z < 0){
					continue;
				}
				
				const float XA = world_coord(x);
				const float ZA = world_coord(z);

				auto vector = flowfield->getVector(x, z);
				
				auto startPointX = XA + FF_TILE_SIZE / 2;
				auto startPointY = ZA + FF_TILE_SIZE / 2;

				auto height = map_TileHeight(x, z) + 10;

				// origin

				auto cost = costField->getCost(x, z);
				if(cost != COST_NOT_PASSABLE){
					iV_PolyLine({
						{ startPointX - 10, height, -startPointY - 10 },
						{ startPointX - 10, height, -startPointY + 10 },
						{ startPointX + 10, height, -startPointY + 10 },
						{ startPointX + 10, height, -startPointY - 10 },
						{ startPointX - 10, height, -startPointY - 10 },
					}, mvp, WZCOL_WHITE);
				}
				
				// direction

				iV_PolyLine({
					{ startPointX, height, -startPointY },
					{ startPointX + vector.x * 75, height, -startPointY - vector.y * 75 },
				}, mvp, WZCOL_WHITE);

				// integration fields

				const Vector3i integrationFieldText3dCoordinates = { (XA + 20), height, -(ZA + 40) };
				Vector2i integrationFieldText2dCoordinates;

				pie_RotateProject(&integrationFieldText3dCoordinates, mvp, &integrationFieldText2dCoordinates);
				auto integrationCost = flowfield->integrationField->getCost(x, z);
				if(integrationCost != COST_NOT_PASSABLE){
					WzText costText(std::to_string(integrationCost), font_small);
					costText.render(integrationFieldText2dCoordinates.x, integrationFieldText2dCoordinates.y, WZCOL_TEXT_BRIGHT);
				}
			}
		}

		// goal

		for(auto goal : flowfield->goals){
			auto goalX = world_coord(goal.x) + FF_TILE_SIZE / 2;
			auto goalY = world_coord(goal.y) + FF_TILE_SIZE / 2;
			auto height = map_TileHeight(goal.x, goal.y) + 10;
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

void debugDrawFlowfields(const glm::mat4 &mvp) {
	if (!isFlowfieldEnabled()) return;

	if (VECTOR_FIELD_DEBUG) {
		debugDrawFlowfield(mvp);
	}
}
