/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/**
 * @file display3d.c
 * Draws the 3D view.
 * Originally by Alex McLean & Jeremy Sallis, Pumpkin Studios, EIDOS INTERACTIVE
 */

#include "lib/framework/frame.h"
#include "lib/framework/math_ext.h"
#include "lib/framework/stdio_ext.h"

/* Includes direct access to render library */
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piedraw.h"
#include "lib/ivis_opengl/pietypes.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/framework/fixedpoint.h"
#include "lib/ivis_opengl/piefunc.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/ivis_opengl/imd.h"
#include "lib/ivis_opengl/pieclip.h"

#include "lib/gamelib/gtime.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "lib/netplay/netplay.h"

#ifndef GLM_ENABLE_EXPERIMENTAL
	#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_interpolation.hpp>

#include "loop.h"
#include "atmos.h"
#include "map.h"
#include "droid.h"
#include "group.h"
#include "visibility.h"
#include "geometry.h"
#include "messagedef.h"
#include "miscimd.h"
#include "effects.h"
#include "edit3d.h"
#include "feature.h"
#include "hci.h"
#include "display.h"
#include "intdisplay.h"
#include "radar.h"
#include "display3d.h"
#include "lighting.h"
#include "console.h"
#include "projectile.h"
#include "bucket3d.h"
#include "message.h"
#include "component.h"
#include "warcam.h"
#include "order.h"
#include "scores.h"
#include "multiplay.h"
#include "advvis.h"
#include "cmddroid.h"
#include "terrain.h"
#include "profiling.h"
#include "warzoneconfig.h"
#include "multistat.h"
#include "animation.h"
#include "faction.h"
#include "wzcrashhandlingproviders.h"
#include "shadowcascades.h"
#include "profiling.h"


/********************  Prototypes  ********************/

static void displayDelivPoints(const glm::mat4& viewMatrix, const glm::mat4 &perspectiveViewMatrix);
static void displayProximityMsgs(const glm::mat4& viewMatrix, const glm::mat4 &perspectiveViewMatrix);
static void displayDynamicObjects(const glm::mat4 &viewMatrix, const glm::mat4 &perspectiveViewMatrix);
static void displayStaticObjects(const glm::mat4 &viewMatrix, const glm::mat4 &perspectiveViewMatrix);
static void displayFeatures(const glm::mat4 &viewMatrix, const glm::mat4 &perspectiveViewMatrix);
static UDWORD	getTargettingGfx();
static void	drawGroupNumber(BASE_OBJECT *psObject);
static void	trackHeight(int desiredHeight);
static void	renderSurroundings(const glm::mat4& projectionMatrix, const glm::mat4 &skyboxViewMatrix);
static void	locateMouse();
static bool	renderWallSection(STRUCTURE *psStructure, const glm::mat4 &viewMatrix, const glm::mat4 &perspectiveViewMatrix);
static void	drawDragBox();
static void	calcFlagPosScreenCoords(SDWORD *pX, SDWORD *pY, SDWORD *pR, const glm::mat4 &perspectiveViewModelMatrix);
static void	drawTiles(iView *player, LightingData& lightData, LightMap& lightmap, ILightingManager& lightManager);
static void	display3DProjectiles(const glm::mat4 &viewMatrix, const glm::mat4 &perspectiveViewMatrix);
static void	drawDroidAndStructureSelections();
static void	drawDroidSelections();
static void	drawStructureSelections();
static void displayBlueprints(const glm::mat4 &viewMatrix, const glm::mat4 &perspectiveViewMatrix);
static void	processSensorTarget();
static void	processDestinationTarget();
static bool	eitherSelected(DROID *psDroid);
static void	structureEffects();
static void	showDroidSensorRanges();
static void	showSensorRange2(BASE_OBJECT *psObj);
static void	drawRangeAtPos(SDWORD centerX, SDWORD centerY, SDWORD radius);
static void	addConstructionLine(DROID *psDroid, STRUCTURE *psStructure, const glm::mat4 &viewMatrix);
static void	doConstructionLines(const glm::mat4 &viewMatrix);
static void	drawDroidCmndNo(DROID *psDroid);
static void	drawDroidOrder(const DROID *psDroid);
static void	drawDroidRank(DROID *psDroid);
static void	drawDroidSensorLock(DROID *psDroid);
static	int	calcAverageTerrainHeight(int tileX, int tileZ);
static	int	calculateCameraHeight(int height);
static void	updatePlayerAverageCentreTerrainHeight();
bool	doWeDrawProximitys();
static PIELIGHT getBlueprintColour(STRUCT_STATES state);

static void NetworkDisplayImage(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
extern bool writeGameInfo(const char *pFileName); // Used to help debug issues when we have fatal errors & crash handler testing.

static WzText txtLevelName;
static WzText txtDebugStatus;
static WzText txtCurrentTime;
static WzText txtShowFPS;
static WzText txtUnits;
// show Samples text
static WzText txtShowSamples_Que;
static WzText txtShowSamples_Lst;
static WzText txtShowSamples_Act;
// show Orders text
static WzText txtShowOrders;
// show Droid visible/draw counts text
static WzText droidText;

static gfx_api::buffer* pScreenTriangleVBO = nullptr;


/********************  Variables  ********************/
// Should be cleaned up properly and be put in structures.

// Initialised at start of drawTiles().
// In model coordinates where x is east, y is up and z is north, rather than world coordinates where x is east, y is south and z is up.
// To get the real camera position, still need to add Vector3i(player.p.x, 0, player.p.z).
static Vector3i actualCameraPosition;

static bool	bRangeDisplay = false;
static SDWORD	rangeCenterX, rangeCenterY, rangeRadius;
static bool	bDrawProximitys = true;
bool	godMode;
bool	showGateways = false;
bool	showPath = false;

// Skybox data
static float wind = 0.0f;
static float windSpeed = 0.0f;
static float skybox_scale = 10000.0f;

/// When to display HP bars
UWORD barMode;

/// Have we made a selection by clicking the mouse? - used for dragging etc
bool	selectAttempt = false;

/// Vectors that hold the player and camera directions and positions
iView	playerPos;

/// How far away are we from the terrain
static float distance;

/// Stores the screen coordinates of the transformed terrain tiles
static Vector3i tileScreenInfo[VISIBLE_YTILES + 1][VISIBLE_XTILES + 1];
static bool tileScreenVisible[VISIBLE_YTILES + 1][VISIBLE_XTILES + 1] = {false};

/// Records the present X and Y values for the current mouse tile (in tiles)
SDWORD mouseTileX, mouseTileY;
Vector2i mousePos(0, 0);

/// Do we want the radar to be rendered
bool radarOnScreen = true;
bool radarPermitted = true;

bool radarVisible()
{
	if (radarOnScreen && radarPermitted && getWidgetsStatus())
	{
		return true;
	}
	else
	{
		return false;
	}
}

/// Show unit/building gun/sensor range
bool rangeOnScreen = false;  // For now, most likely will change later!  -Q 5-10-05   A very nice effect - Per

/// Tactical UI: show/hide target origin icon
bool tuiTargetOrigin = false;

/// Temporary values for the terrain render - center of grid to be rendered
static int playerXTile, playerZTile;

/// The cached value of frameGetFrameNumber()
static UDWORD currentGameFrame;
/// The box used for multiple selection - present screen coordinates
static QUAD dragQuad;

/** Number of tiles visible
 * \todo This should become dynamic! (A function of resolution, angle and zoom maybe.)
 */
const Vector2i visibleTiles(VISIBLE_XTILES, VISIBLE_YTILES);

/// The tile we use for drawing the bottom of a body of water
static unsigned int underwaterTile = WATER_TILE;
/** The tile we use for drawing rubble
 * \note Unused.
 */
static unsigned int rubbleTile = BLOCKING_RUBBLE_TILE;

/** Show how many frames we are rendering per second
 * default OFF, turn ON via console command 'showfps'
 */
bool showFPS = false;       //
/** Show how many samples we are rendering per second
 * default OFF, turn ON via console command 'showsamples'
 */
bool showUNITCOUNT = false;
/** Show how many kills/deaths (produced units) made
 * default OFF, turn ON via console command 'showunits'
 */
bool showSAMPLES = false;
/**  Show the current selected units order / action
 *  default OFF, turn ON via console command 'showorders'
 */
bool showORDERS = false;
/**  Show the drawn/undrawn counts for droids
  * default OFF, turn ON by flipping it here
  */
bool showDROIDcounts = false;

/**  Speed of blueprints animation (moving from one tile to another)
  * default 20, change in config
  */
int BlueprintTrackAnimationSpeed = 20;

/** When we have a connection issue, we will flash a message on screen
*/
static const char *errorWaiting = nullptr;
static uint32_t lastErrorTime = 0;

#define NETWORK_FORM_ID 0xFAAA
#define NETWORK_BUT_ID 0xFAAB
/** When enabled, this causes a segfault in the game, to test out the crash handler */
bool CauseCrash = false;

/** tells us in realtime, what droid is doing (order / action)
*/
char DROIDDOING[512];

/// Geometric offset which will be passed to pie_SetGeometricOffset
static const int geoOffset = 192;

/// The average terrain height for the center of the area the camera is looking at
static int averageCentreTerrainHeight;

/** The time at which a sensor target was last assigned
 * Used to draw a visual effect.
 */
static UDWORD	lastTargetAssignation = 0;
/** The time at which an order concerning a destination was last given
 * Used to draw a visual effect.
 */
static UDWORD	lastDestAssignation = 0;
static bool	bSensorTargetting = false;
static bool	bDestTargetting = false;
static BASE_OBJECT *psSensorObj = nullptr;
static UDWORD	destTargetX, destTargetY;
static UDWORD	destTileX = 0, destTileY = 0;

struct Blueprint
{
	Blueprint()
		: stats(nullptr)
		, pos({0,0,0})
		, dir(0)
		, index(0)
		, state(SS_BLUEPRINT_INVALID)
		, player(selectedPlayer)
	{}
	Blueprint(STRUCTURE_STATS const *stats, Vector3i pos, uint16_t dir, uint32_t index, STRUCT_STATES state, uint8_t player)
		: stats(stats)
		, pos(pos)
		, dir(dir)
		, index(index)
		, state(state)
		, player(player)
	{}
	int compare(Blueprint const &b) const
	{
		if (stats->ref != b.stats->ref)
		{
			return stats->ref < b.stats->ref ? -1 : 1;
		}
		if (pos.x != b.pos.x)
		{
			return pos.x < b.pos.x ? -1 : 1;
		}
		if (pos.y != b.pos.y)
		{
			return pos.y < b.pos.y ? -1 : 1;
		}
		if (pos.z != b.pos.z)
		{
			return pos.z < b.pos.z ? -1 : 1;
		}
		if (dir != b.dir)
		{
			return dir < b.dir ? -1 : 1;
		}
		if (index != b.index)
		{
			return index < b.index ? -1 : 1;
		}
		if (state != b.state)
		{
			return state < b.state ? -1 : 1;
		}
		return 0;
	}
	bool operator <(Blueprint const &b) const
	{
		return compare(b) < 0;
	}
	bool operator ==(Blueprint const &b) const
	{
		return compare(b) == 0;
	}
	optional<STRUCTURE> buildBlueprint() const
	{
		return ::buildBlueprint(stats, pos, dir, index, state, player);
	}
	void renderBlueprint(const glm::mat4 &viewMatrix, const glm::mat4 &perspectiveViewMatrix) const
	{
		if (clipXY(pos.x, pos.y))
		{
			auto optStruct = buildBlueprint();
			ASSERT_OR_RETURN(, optStruct.has_value(), "buildBlueprint returned nullopt");
			renderStructure(&*optStruct, viewMatrix, perspectiveViewMatrix);
		}
	}

	STRUCTURE_STATS const *stats;
	Vector3i pos;
	uint16_t dir;
	uint32_t index;
	STRUCT_STATES state;
	uint8_t player;
};

static std::vector<Blueprint> blueprints;

#define	TARGET_TO_SENSOR_TIME	((4*(GAME_TICKS_PER_SEC))/5)
#define	DEST_TARGET_TIME	(GAME_TICKS_PER_SEC/4)

/// The distance the selection box will pulse
static const float BOX_PULSE_SIZE = 30;

/// the opacity at which building blueprints will be drawn
static const int BLUEPRINT_OPACITY = 120;

/********************  Functions  ********************/

void display3dScreenSizeDidChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight)
{
	if (psWScreen == nullptr) return;
	resizeRadar(); // recalculate radar position
}

static void queueDroidPowerBarsRects(DROID *psDroid, bool drawBox, BatchedMultiRectRenderer& batchedMultiRectRenderer, size_t rectGroup);
static void queueDroidEnemyHealthBarsRects(DROID *psDroid, BatchedMultiRectRenderer& batchedMultiRectRenderer, size_t rectGroup);
static void queueWeaponReloadBarRects(BASE_OBJECT *psObj, WEAPON *psWeap, int weapon_slot, BatchedMultiRectRenderer& batchedMultiRectRenderer, size_t rectGroup);
static void queueStructureHealth(STRUCTURE *psStruct, BatchedMultiRectRenderer& batchedMultiRectRenderer, size_t rectGroup);
static void queueStructureBuildProgress(STRUCTURE *psStruct, BatchedMultiRectRenderer& batchedMultiRectRenderer, size_t rectGroup);
static void drawStructureTargetOriginIcon(STRUCTURE *psStruct, int weapon_slot);

class BatchedObjectStatusRenderer
{
public:
	void addDroidSelectionsToRender(uint32_t player, BASE_OBJECT *psClickedOn, bool bMouseOverOwnDroid)
	{
		bool bDrawAll = barMode == BAR_DROIDS || barMode == BAR_DROIDS_AND_STRUCTURES;

		// first, for all droids, queue the various rects
		for (DROID *psDroid : apsDroidLists[player])
		{
			if (psDroid->sDisplay.frameNumber == currentGameFrame)
			{
				/* If it's selected and on screen or it's the one the mouse is over */
				if (bDrawAll ||
					eitherSelected(psDroid) ||
					(bMouseOverOwnDroid && psDroid == (DROID *) psClickedOn) ||
					droidUnderRepair(psDroid)
					)
				{
					queueDroidPowerBarsRects(psDroid, psDroid->selected, batchedMultiRectRenderer, 0);

					for (unsigned i = 0; i < psDroid->numWeaps; i++)
					{
						queueWeaponReloadBarRects((BASE_OBJECT *)psDroid, &psDroid->asWeaps[i], i, batchedMultiRectRenderer, 1);
					}

					droidsToDraw.push_back(psDroid);
				}
			}
		}
	}

	void addEnemyDroidHealthToRender(DROID *psDroid)
	{
		queueDroidEnemyHealthBarsRects(psDroid, batchedMultiRectRenderer, 4);
	}

	void addEnemyStructureHealthToRender(STRUCTURE* psStruct)
	{
		queueStructureHealth(psStruct, batchedMultiRectRenderer, 4);
		if (psStruct->status == SS_BEING_BUILT)
		{
			queueStructureBuildProgress(psStruct, batchedMultiRectRenderer, 4);
		}
	}

	void addStructureSelectionsToRender(uint32_t player, BASE_OBJECT *psClickedOn, bool bMouseOverOwnStructure)
	{
		/* Go thru' all the buildings */
		for (STRUCTURE *psStruct : apsStructLists[player])
		{
			if (psStruct->sDisplay.frameNumber == currentGameFrame)
			{
				/* If it's selected */
				if (psStruct->selected ||
					(barMode == BAR_DROIDS_AND_STRUCTURES && psStruct->pStructureType->type != REF_WALL && psStruct->pStructureType->type != REF_WALLCORNER) ||
					(bMouseOverOwnStructure && psStruct == (STRUCTURE *)psClickedOn)
				   )
				{
					queueStructureHealth(psStruct, batchedMultiRectRenderer, 2);

					for (unsigned i = 0; i < psStruct->numWeaps; i++)
					{
						queueWeaponReloadBarRects((BASE_OBJECT *)psStruct, &psStruct->asWeaps[i], i, batchedMultiRectRenderer, 3);
						// drawStructureTargetOriginIcon(psStruct, i); // in the future, use instanced rendering here? (for now, add to structsToDraw below and handle later)
					}

					structsToDraw.push_back(psStruct);
				}

				if (psStruct->status == SS_BEING_BUILT)
				{
					queueStructureBuildProgress(psStruct, batchedMultiRectRenderer, 2);
				}
			}
		}
	}

	void addStructureTargetingGfxToRender()
	{
		// Future todo: make this instanced as well?
		renderStructureTargetingGfx = true;
	}

	void drawAllSelections()
	{
		// droid selection / health draw
		// 1. box + power bars rects
		batchedMultiRectRenderer.drawRects(0);
		// 2. droid rank images
		for (DROID *psDroid : droidsToDraw)
		{
			/* Write the droid rank out */
			if ((psDroid->sDisplay.screenX + psDroid->sDisplay.screenR) > 0
				&&	(psDroid->sDisplay.screenX - psDroid->sDisplay.screenR) < pie_GetVideoBufferWidth()
				&&	(psDroid->sDisplay.screenY + psDroid->sDisplay.screenR) > 0
				&&	(psDroid->sDisplay.screenY - psDroid->sDisplay.screenR) < pie_GetVideoBufferHeight())
			{
				drawDroidRank(psDroid);
				drawDroidSensorLock(psDroid);
				drawDroidCmndNo(psDroid);
				drawGroupNumber(psDroid);
			}
		}
		// 3. weapon reload bar
		batchedMultiRectRenderer.drawRects(1);

		// structure selection / health draw
		// 1. structure health / build progress rects
		batchedMultiRectRenderer.drawRects(2);
		// 2. structure target origin icons
		for (STRUCTURE *psStruct : structsToDraw)
		{
			for (unsigned i = 0; i < psStruct->numWeaps; i++)
			{
				drawStructureTargetOriginIcon(psStruct, i);
			}
			if (psStruct->isFactory())
			{
				drawGroupNumber(psStruct);
			}
		}
		// 3. structure weapon reload bars
		batchedMultiRectRenderer.drawRects(3);

		if (renderStructureTargetingGfx)
		{
			SDWORD scrX, scrY;
			for (uint32_t i = 0; i < MAX_PLAYERS; i++)
			{
				for (const STRUCTURE* psStruct : apsStructLists[i])
				{
					/* If it's targetted and on-screen */
					if (psStruct->flags.test(OBJECT_FLAG_TARGETED)
						&& psStruct->sDisplay.frameNumber == currentGameFrame)
					{
						scrX = psStruct->sDisplay.screenX;
						scrY = psStruct->sDisplay.screenY;
						iV_DrawImage(IntImages, getTargettingGfx(), scrX, scrY);
					}
				}
			}
		}

		// draw last rects
		batchedMultiRectRenderer.drawRects(4);
	}

	void initialize()
	{
		batchedMultiRectRenderer.initialize();
		// 2 for droid selection rect groups, + 2 for structure selection rect groups, + 1 for "mouse over" rect groups (drawn last)
		batchedMultiRectRenderer.resizeRectGroups(5);
	}

	void clear()
	{
		droidsToDraw.clear();
		structsToDraw.clear();
		renderStructureTargetingGfx = false;
		batchedMultiRectRenderer.clear();
	}

	void reset()
	{
		clear();
		batchedMultiRectRenderer.reset();
	}
private:
	std::vector<DROID *> droidsToDraw;
	std::vector<STRUCTURE *> structsToDraw;
	bool renderStructureTargetingGfx = false;
	BatchedMultiRectRenderer batchedMultiRectRenderer;
};

static BatchedObjectStatusRenderer batchedObjectStatusRenderer;

float interpolateAngleDegrees(int a, int b, float t)
{
	if(a > 180)
	{
		a -= 360;
	}
	if(b > 180)
	{
		b -= 360;
	}

	float d = b - a;

	return a + d * t;
}

bool drawShape(iIMDShape *strImd, UDWORD timeAnimationStarted, int colour, PIELIGHT buildingBrightness, int pieFlag, int pieFlagData, const glm::mat4& modelMatrix, const glm::mat4& viewMatrix, float stretchDepth)
{
	glm::mat4 modifiedModelMatrix = modelMatrix;
	int animFrame = 0; // for texture animation
	if (strImd->numFrames > 0)	// Calculate an animation frame
	{
		animFrame = getModularScaledGraphicsTime(strImd->animInterval, strImd->numFrames);
	}
	if (strImd->objanimframes)
	{
		int elapsed = graphicsTime - timeAnimationStarted;
		if (elapsed < 0)
		{
			elapsed = 0; // Animation hasn't started yet.
		}

		const int frame = (elapsed / strImd->objanimtime) % strImd->objanimframes;
		ASSERT(frame < strImd->objanimframes, "Bad index %d >= %d", frame, strImd->objanimframes);

		const ANIMFRAME &state = strImd->objanimdata.at(frame);

		if (state.scale.x == -1.0f) // disabled frame, for implementing key frame animation
		{
			return false;
		}

		if (strImd->interpolate == 1)
		{
			const float frameFraction = static_cast<float>(fmod(elapsed / (float)strImd->objanimtime, strImd->objanimframes) - frame);
			const int nextFrame = (frame + 1) % strImd->objanimframes;
			const ANIMFRAME &nextState = strImd->objanimdata.at(nextFrame);

			modifiedModelMatrix *=
				glm::interpolate(glm::translate(glm::vec3(state.pos)), glm::translate(glm::vec3(nextState.pos)), frameFraction) *
				glm::rotate(RADIANS(interpolateAngleDegrees(state.rot.pitch / DEG(1), nextState.rot.pitch / DEG(1), frameFraction)), glm::vec3(1.f, 0.f, 0.f)) *
				glm::rotate(RADIANS(interpolateAngleDegrees(state.rot.direction / DEG(1), nextState.rot.direction / DEG(1), frameFraction)), glm::vec3(0.f, 1.f, 0.f)) *
				glm::rotate(RADIANS(interpolateAngleDegrees(state.rot.roll / DEG(1), nextState.rot.roll / DEG(1), frameFraction)), glm::vec3(0.f, 0.f, 1.f)) *
				glm::scale(state.scale);
		}
		else
		{
			modifiedModelMatrix *= glm::translate(glm::vec3(state.pos)) *
				glm::rotate(UNDEG(state.rot.pitch), glm::vec3(1.f, 0.f, 0.f)) *
				glm::rotate(UNDEG(state.rot.direction), glm::vec3(0.f, 1.f, 0.f)) *
				glm::rotate(UNDEG(state.rot.roll), glm::vec3(0.f, 0.f, 1.f)) *
				glm::scale(state.scale);
		}
	}

	return pie_Draw3DShape(strImd, animFrame, colour, buildingBrightness, pieFlag, pieFlagData, modifiedModelMatrix, viewMatrix, stretchDepth, true);
}

static void setScreenDispWithPerspective(SCREEN_DISP_DATA *sDisplay, const glm::mat4 &perspectiveViewModelMatrix)
{
	Vector3i zero(0, 0, 0);
	Vector2i s(0, 0);

	pie_RotateProjectWithPerspective(&zero, perspectiveViewModelMatrix, &s);
	sDisplay->screenX = s.x;
	sDisplay->screenY = s.y;
}

void setSkyBox(const char *page, float mywind, float myscale)
{
	windSpeed = mywind;
	wind = 0.0f;
	skybox_scale = myscale;
	pie_Skybox_Texture(page);
}

static inline void rotateSomething(int &x, int &y, uint16_t angle)
{
	int64_t cra = iCos(angle), sra = iSin(angle);
	int newX = (x * cra - y * sra) >> 16;
	int newY = (x * sra + y * cra) >> 16;
	x = newX;
	y = newY;
}

static Blueprint getTileBlueprint(int mapX, int mapY)
{
	Vector2i mouse(world_coord(mapX) + TILE_UNITS / 2, world_coord(mapY) + TILE_UNITS / 2);

	for (std::vector<Blueprint>::const_iterator blueprint = blueprints.begin(); blueprint != blueprints.end(); ++blueprint)
	{
		const Vector2i size = blueprint->stats->size(blueprint->dir) * TILE_UNITS;
		if (abs(mouse.x - blueprint->pos.x) < size.x / 2 && abs(mouse.y - blueprint->pos.y) < size.y / 2)
		{
			return *blueprint;
		}
	}

	return Blueprint(nullptr, Vector3i(), 0, 0, SS_BEING_BUILT, selectedPlayer);
}

STRUCTURE *getTileBlueprintStructure(int mapX, int mapY)
{
	static optional<STRUCTURE> optStruct;

	Blueprint blueprint = getTileBlueprint(mapX, mapY);
	if (blueprint.state == SS_BLUEPRINT_PLANNED)
	{
		if (optStruct)
		{
			// Delete previously returned structure, if any.
			optStruct.reset();
		}
		auto b = blueprint.buildBlueprint();
		// Don't use `operator=(optional&&)` to avoid declaring custom copy/move-assignment operators
		// for `STRUCTURE` (more specifically, for `SIMPLE_OBJECT`, which has some const-qualified data members).
		if (b)
		{
			optStruct.emplace(*b);
		}
		else
		{
			optStruct.reset();
		}
		return optStruct.has_value() ? &optStruct.value() : nullptr;  // This blueprint was clicked on.
	}

	return nullptr;
}

STRUCTURE_STATS const *getTileBlueprintStats(int mapX, int mapY)
{
	return getTileBlueprint(mapX, mapY).stats;
}

bool anyBlueprintTooClose(STRUCTURE_STATS const *stats, Vector2i pos, uint16_t dir)
{
	for (std::vector<Blueprint>::const_iterator blueprint = blueprints.begin(); blueprint != blueprints.end(); ++blueprint)
	{
		if ((blueprint->state == SS_BLUEPRINT_PLANNED || blueprint->state == SS_BLUEPRINT_PLANNED_BY_ALLY)
		    && isBlueprintTooClose(stats, pos, dir, blueprint->stats, blueprint->pos, blueprint->dir))
		{
			return true;
		}
	}
	return false;
}

void clearBlueprints()
{
	blueprints.clear();
}

static PIELIGHT selectionBrightness()
{
	int brightVar;
	if (!gamePaused())
	{
		brightVar = getModularScaledGraphicsTime(990, 110);
		if (brightVar > 55)
		{
			brightVar = 110 - brightVar;
		}
	}
	else
	{
		brightVar = 55;
	}
	return pal_SetBrightness(200 + brightVar);
}

static PIELIGHT structureBrightness(STRUCTURE *psStructure)
{
	PIELIGHT buildingBrightness;

	if (psStructure->isBlueprint())
	{
		buildingBrightness = getBlueprintColour(psStructure->status);
	}
	else
	{
		buildingBrightness = pal_SetBrightness(static_cast<UBYTE>(200 - 100 / 65536.f * getStructureDamage(psStructure)));

		/* If it's selected, then it's brighter */
		if (psStructure->selected)
		{
			buildingBrightness = selectionBrightness();
		}
		if (!getRevealStatus())
		{
			buildingBrightness = pal_SetBrightness(avGetObjLightLevel((BASE_OBJECT *)psStructure, buildingBrightness.byte.r));
		}
		if (!hasSensorOnTile(mapTile(map_coord(psStructure->pos.x), map_coord(psStructure->pos.y)), selectedPlayer))
		{
			buildingBrightness.byte.r /= 2;
			buildingBrightness.byte.g /= 2;
			buildingBrightness.byte.b /= 2;
		}
	}
	return buildingBrightness;
}


/// Show all droid movement parts by displaying an explosion at every step
static void showDroidPaths()
{
	if ((graphicsTime / 250 % 2) != 0)
	{
		return;
	}

	if (selectedPlayer >= MAX_PLAYERS)
	{
		return; // no-op for now
	}

	for (const DROID *psDroid : apsDroidLists[selectedPlayer])
	{
		if (psDroid->selected && psDroid->sMove.Status != MOVEINACTIVE)
		{
			const int len = psDroid->sMove.asPath.size();
			for (int i = std::max(psDroid->sMove.pathIndex - 1, 0); i < len; i++)
			{
				Vector3i pos;

				ASSERT(worldOnMap(psDroid->sMove.asPath[i].x, psDroid->sMove.asPath[i].y), "Path off map!");
				pos.x = psDroid->sMove.asPath[i].x;
				pos.z = psDroid->sMove.asPath[i].y;
				pos.y = map_Height(pos.x, pos.z) + 16;

				effectGiveAuxVar(80);
				addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_LASER, false, nullptr, 0);
			}
		}
	}
}

/// Displays an image for the Network Issue button
static void NetworkDisplayImage(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();
	UWORD ImageID;
	CONNECTION_STATUS status = (CONNECTION_STATUS)UNPACKDWORD_TRI_A(psWidget->UserData);

	ASSERT(psWidget->type == WIDG_BUTTON, "Not a button");

	// cheap way to do a button flash
	if ((realTime / 250) % 2 == 0)
	{
		ImageID = UNPACKDWORD_TRI_B(psWidget->UserData);
	}
	else
	{
		ImageID = UNPACKDWORD_TRI_C(psWidget->UserData);
	}

	if (NETcheckPlayerConnectionStatus(status, NET_ALL_PLAYERS))
	{
		unsigned c = 0;
		char players[MAX_PLAYERS + 1];
		PlayerMask playerMaskMapped = 0;
		for (unsigned n = 0; n < MAX_PLAYERS; ++n)
		{
			if (NETcheckPlayerConnectionStatus(status, n))
			{
				playerMaskMapped |= 1 << NetPlay.players[n].position;
			}
		}
		for (unsigned n = 0; n < MAX_PLAYERS; ++n)
		{
			if ((playerMaskMapped & 1 << n) != 0)
			{
				STATIC_ASSERT(MAX_PLAYERS <= 32);  // If increasing MAX_PLAYERS, check all the 1<<playerNumber shifts, since the 1 is usually a 32-bit type.
				players[c++] = "0123456789ABCDEFGHIJKLMNOPQRSTUV"[n];
			}
		}
		players[c] = '\0';
		const unsigned width = iV_GetTextWidth(players, font_regular) + 10;
		const unsigned height = iV_GetTextHeight(players, font_regular) + 10;
		iV_SetTextColour(WZCOL_TEXT_BRIGHT);
		iV_DrawText(players, x - width, y + height, font_regular);
	}

	iV_DrawImage(IntImages, ImageID, x, y);
}

static void setupConnectionStatusForm()
{
	static unsigned          prevStatusMask = 0;

	const int separation = 3;
	unsigned statusMask = 0;
	unsigned total = 0;

	for (unsigned i = 0; i < CONNECTIONSTATUS_NORMAL; ++i)
	{
		if (NETcheckPlayerConnectionStatus((CONNECTION_STATUS)i, NET_ALL_PLAYERS))
		{
			statusMask |= 1 << i;
			++total;
		}
	}

	if (prevStatusMask != 0 && statusMask != prevStatusMask)
	{
		// Remove the icons.
		for (unsigned i = 0; i < CONNECTIONSTATUS_NORMAL; ++i)
		{
			if ((statusMask & 1 << i) != 0)
			{
				widgDelete(psWScreen, NETWORK_BUT_ID + i);   // kill button
			}
		}
		widgDelete(psWScreen, NETWORK_FORM_ID);  // kill form

		prevStatusMask = 0;
	}

	if (prevStatusMask == 0 && statusMask != 0)
	{
		unsigned n = 0;
		// Create the basic form
		W_FORMINIT sFormInit;
		sFormInit.formID = 0;
		sFormInit.id = NETWORK_FORM_ID;
		sFormInit.style = WFORM_PLAIN;
		sFormInit.calcLayout = LAMBDA_CALCLAYOUT_SIMPLE({
			psWidget->move((int)(pie_GetVideoBufferWidth() - 52), 80);
		});
		sFormInit.width = 36;
		sFormInit.height = (24 + separation) * total - separation;
		if (!widgAddForm(psWScreen, &sFormInit))
		{
			//return false;
		}

		/* Now add the buttons */
		for (unsigned i = 0; i < CONNECTIONSTATUS_NORMAL; ++i)
		{
			if ((statusMask & 1 << i) == 0)
			{
				continue;
			}

			//set up default button data
			W_BUTINIT sButInit;
			sButInit.formID = NETWORK_FORM_ID;
			sButInit.id = NETWORK_BUT_ID + i;
			sButInit.width = 36;
			sButInit.height = 24;

			//add button
			sButInit.style = WBUT_PLAIN;
			sButInit.x = 0;
			sButInit.y = (24 + separation) * n;
			sButInit.pDisplay = NetworkDisplayImage;
			// Note we would set the image to be different based on which issue it is.
			switch (i)
			{
			default:
				ASSERT(false, "Bad connection status value.");
				sButInit.pTip = "Bug";
				sButInit.UserData = PACKDWORD_TRI(0, IMAGE_DESYNC_HI, IMAGE_PLAYER_LEFT_LO);
				break;
			case CONNECTIONSTATUS_PLAYER_LEAVING:
				sButInit.pTip = _("Player left");
				sButInit.UserData = PACKDWORD_TRI(i, IMAGE_PLAYER_LEFT_HI, IMAGE_PLAYER_LEFT_LO);
				break;
			case CONNECTIONSTATUS_PLAYER_DROPPED:
				sButInit.pTip = _("Player dropped");
				sButInit.UserData = PACKDWORD_TRI(i, IMAGE_DISCONNECT_LO, IMAGE_DISCONNECT_HI);
				break;
			case CONNECTIONSTATUS_WAITING_FOR_PLAYER:
				sButInit.pTip = _("Waiting for other players");
				sButInit.UserData = PACKDWORD_TRI(i, IMAGE_WAITING_HI, IMAGE_WAITING_LO);

				break;
			case CONNECTIONSTATUS_DESYNC:
				sButInit.pTip = _("Out of sync");
				sButInit.UserData = PACKDWORD_TRI(i, IMAGE_DESYNC_HI, IMAGE_DESYNC_LO);
				break;
			}

			if (!widgAddButton(psWScreen, &sButInit))
			{
				//return false;
			}

			++n;
		}

		prevStatusMask = statusMask;
	}
}

/// Render the 3D world
void draw3DScene()
{
	WZ_PROFILE_SCOPE(draw3DScene);
	wzPerfBegin(PERF_START_FRAME, "Start 3D scene");

	/* What frame number are we on? */
	currentGameFrame = frameGetFrameNumber();

	// Tell shader system what the time is
	pie_SetShaderTime(graphicsTime);

	/* Build the drag quad */
	if (dragBox3D.status == DRAG_RELEASED)
	{
		dragQuad.coords[0].x = dragBox3D.x1; // TOP LEFT
		dragQuad.coords[0].y = dragBox3D.y1;

		dragQuad.coords[1].x = dragBox3D.x2; // TOP RIGHT
		dragQuad.coords[1].y = dragBox3D.y1;

		dragQuad.coords[2].x = dragBox3D.x2; // BOTTOM RIGHT
		dragQuad.coords[2].y = dragBox3D.y2;

		dragQuad.coords[3].x = dragBox3D.x1; // BOTTOM LEFT
		dragQuad.coords[3].y = dragBox3D.y2;
	}

	pie_Begin3DScene();
	/* Set 3D world origins */
	pie_SetGeometricOffset(rendSurface.width / 2, geoOffset);

	updateFogDistance(distance);

	// Set light manager
	{
		if (war_getPointLightPerPixelLighting() && getTerrainShaderQuality() == TerrainShaderQuality::NORMAL_MAPPING)
		{
			setLightingManager(std::make_unique<renderingNew::LightingManager>());
		}
		else
		{
			setLightingManager(std::make_unique<rendering1999::LightingManager>());
		}
	}

	/* Now, draw the terrain */
	drawTiles(&playerPos, getCurrentLightingData(), getCurrentLightmapData(), getCurrentLightingManager());

	wzPerfBegin(PERF_MISC, "3D scene - misc and text");

	pie_BeginInterface();

	/* Show the drag Box if necessary */
	drawDragBox();

	/* Have we released the drag box? */
	if (dragBox3D.status == DRAG_RELEASED)
	{
		dragBox3D.status = DRAG_INACTIVE;
	}

	drawDroidAndStructureSelections();

	pie_SetFogStatus(false);
	iV_SetTextColour(WZCOL_TEXT_BRIGHT);

	/* Dont remove this folks!!!! */
	if (errorWaiting)
	{
		// print the error message if none have been printed for one minute
		if (lastErrorTime == 0 || lastErrorTime + (60 * GAME_TICKS_PER_SEC) < realTime)
		{
			char trimMsg[255];
			audio_PlayBuildFailedOnce();
			ssprintf(trimMsg, "Error! (Check your logs!): %.78s", errorWaiting);
			addConsoleMessage(trimMsg, DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
			errorWaiting = nullptr;
			lastErrorTime = realTime;
		}
	}
	else
	{
		errorWaiting = debugLastError();
	}
	if (showSAMPLES)		//Displays the number of sound samples we currently have
	{
		unsigned int width, height;
		std::string Qbuf, Lbuf, Abuf;

		Qbuf = astringf("Que: %04u", audio_GetSampleQueueCount());
		Lbuf = astringf("Lst: %04u", audio_GetSampleListCount());
		Abuf = astringf("Act: %04u", sound_GetActiveSamplesCount());
		txtShowSamples_Que.setText(WzString::fromUtf8(Qbuf), font_regular);
		txtShowSamples_Lst.setText(WzString::fromUtf8(Lbuf), font_regular);
		txtShowSamples_Act.setText(WzString::fromUtf8(Abuf), font_regular);

		width = txtShowSamples_Que.width() + 11;
		height = txtShowSamples_Que.height();

		txtShowSamples_Que.render(pie_GetVideoBufferWidth() - width, height + 2, WZCOL_TEXT_BRIGHT);
		txtShowSamples_Lst.render(pie_GetVideoBufferWidth() - width, height + 48, WZCOL_TEXT_BRIGHT);
		txtShowSamples_Act.render(pie_GetVideoBufferWidth() - width, height + 59, WZCOL_TEXT_BRIGHT);
	}
	if (showFPS)
	{
		std::string fps = astringf("FPS: %d", frameRate());
		txtShowFPS.setText(WzString::fromUtf8(fps), font_regular);
		const unsigned width = txtShowFPS.width() + 10;
		const unsigned height = 9; //txtShowFPS.height();
		txtShowFPS.render(pie_GetVideoBufferWidth() - width, pie_GetVideoBufferHeight() - height, WZCOL_TEXT_BRIGHT);
	}
	if (showUNITCOUNT && selectedPlayer < MAX_PLAYERS)
	{
		std::string killdiff = astringf("Units: %u lost / %u built / %u killed", missionData.unitsLost, missionData.unitsBuilt, getSelectedPlayerUnitsKilled());
		txtUnits.setText(WzString::fromUtf8(killdiff), font_regular);
		const unsigned width = txtUnits.width() + 10;
		const unsigned height = 9; //txtUnits.height();
		txtUnits.render(pie_GetVideoBufferWidth() - width - ((showFPS) ? txtShowFPS.width() + 10 : 0), pie_GetVideoBufferHeight() - height, WZCOL_TEXT_BRIGHT);
	}
	if (showORDERS)
	{
		unsigned int height;
		txtShowOrders.setText(DROIDDOING, font_regular);
		height = txtShowOrders.height();
		txtShowOrders.render(0, pie_GetVideoBufferHeight() - height, WZCOL_TEXT_BRIGHT);
	}
	if (showDROIDcounts && selectedPlayer < MAX_PLAYERS)
	{
		int visibleDroids = 0;
		int undrawnDroids = 0;
		for (const DROID *psDroid : apsDroidLists[selectedPlayer])
		{
			if (psDroid->sDisplay.frameNumber != currentGameFrame)
			{
				++undrawnDroids;
				continue;
			}
			++visibleDroids;
		}
		char droidCounts[255];
		sprintf(droidCounts, "Droids: %d drawn, %d undrawn", visibleDroids, undrawnDroids);
		droidText.setText(droidCounts, font_regular);
		droidText.render(pie_GetVideoBufferWidth() - droidText.width() - 10, droidText.height() + 2, WZCOL_TEXT_BRIGHT);
	}

	setupConnectionStatusForm();

	if (getWidgetsStatus() && !gamePaused())
	{
		char buildInfo[255];
		bool showMs = (gameTimeGetMod() < Rational(1, 4));
		getAsciiTime(buildInfo, showMs ? graphicsTime : gameTime, showMs);
		txtLevelName.render(RET_X + 134, 410 + E_H, WZCOL_TEXT_MEDIUM);
		const DebugInputManager& dbgInputManager = gInputManager.debugManager();
		if (dbgInputManager.debugMappingsAllowed())
		{
			txtDebugStatus.render(RET_X + 134, 436 + E_H, WZCOL_TEXT_MEDIUM);
		}
		txtCurrentTime.setText(buildInfo, font_small);
		txtCurrentTime.render(RET_X + 134, 422 + E_H, WZCOL_TEXT_MEDIUM);
	}

	while (playerPos.r.y > DEG(360))
	{
		playerPos.r.y -= DEG(360);
	}

	/* If we don't have an active camera track, then track terrain height! */
	if (!getWarCamStatus())
	{
		/* Move the autonomous camera if necessary */
		updatePlayerAverageCentreTerrainHeight();
		trackHeight(calculateCameraHeight(averageCentreTerrainHeight));
	}
	else
	{
		processWarCam();
	}

	processSensorTarget();
	processDestinationTarget();

	structureEffects(); // add fancy effects to structures

	showDroidSensorRanges(); //shows sensor data for units/droids/whatever...-Q 5-10-05
	if (CauseCrash)
	{
		char *crash = nullptr;
#ifdef DEBUG
		ASSERT(false, "Yes, this is a assert.  This should not happen on release builds! Use --noassert to bypass in debug builds.");
		debug(LOG_WARNING, " *** Warning!  You have compiled in debug mode! ***");
#endif
		writeGameInfo("WZdebuginfo.txt");		//also test writing out this file.
		debug(LOG_FATAL, "Forcing a segfault! (crash handler test)");
		// and here comes the crash
		if (!crashHandlingProviderTestCrash())
		{
#if defined(WZ_CC_GNU) && !defined(WZ_CC_INTEL) && !defined(WZ_CC_CLANG) && (7 <= __GNUC__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wnull-dereference"
#endif
			*crash = 0x3; // deliberate null-dereference
#if defined(WZ_CC_GNU) && !defined(WZ_CC_INTEL) && !defined(WZ_CC_CLANG) && (7 <= __GNUC__)
# pragma GCC diagnostic pop
#endif
		}
#if defined(__EMSCRIPTEN__)
		abort();
#endif
		exit(-1);	// should never reach this, but just in case...
	}
	//visualize radius if needed
	if (bRangeDisplay)
	{
		drawRangeAtPos(rangeCenterX, rangeCenterY, rangeRadius);
	}

	if (showPath)
	{
		showDroidPaths();
	}

	wzPerfEnd(PERF_MISC);
}


/***************************************************************************/
bool	doWeDrawProximitys()
{
	return (bDrawProximitys);
}
/***************************************************************************/
void	setProximityDraw(bool val)
{
	bDrawProximitys = val;
}
/***************************************************************************/
/// Calculate the average terrain height for the area directly below the tile
static int calcAverageTerrainHeight(int tileX, int tileZ)
{
	int numTilesAveraged = 0;

	/**
	 * We track the height here - so make sure we get the average heights
	 * of the tiles directly underneath us
	 */
	int result = 0;
	for (int i = -4; i <= 4; i++)
	{
		for (int j = -4; j <= 4; j++)
		{
			if (tileOnMap(tileX + j, tileZ + i))
			{
				/* Get a pointer to the tile at this location */
				MAPTILE *psTile = mapTile(tileX + j, tileZ + i);

				result += psTile->height;
				numTilesAveraged++;
			}
		}
	}
	if (numTilesAveraged == 0) // might be if off map
	{
		return ELEVATION_SCALE * TILE_UNITS;
	}

	/**
	 * Work out the average height.
	 * We use this information to keep the player camera above the terrain.
	 */
	MAPTILE *psTile = mapTile(tileX, tileZ);

	result /= numTilesAveraged;
	if (result < psTile->height)
	{
		result = psTile->height;
	}

	return result;
}

static void updatePlayerAverageCentreTerrainHeight()
{
	averageCentreTerrainHeight = calcAverageTerrainHeight(playerXTile, playerZTile);
}

inline bool quadIntersectsWithScreen(const QUAD & quad)
{
	const int width = pie_GetVideoBufferWidth();
	const int height = pie_GetVideoBufferHeight();
	for (int iCoordIdx = 0; iCoordIdx < 4; ++iCoordIdx)
	{
		if ((quad.coords[iCoordIdx].x < 0) || (quad.coords[iCoordIdx].x > width))
		{
			continue;
		}
		if ((quad.coords[iCoordIdx].y < 0) || (quad.coords[iCoordIdx].y > height))
		{
			continue;
		}

		return true; // corner (x,y) is within the screen bounds
	}

	return false;
}

glm::mat4 getBiasedShadowMapMVPMatrix(glm::mat4 lightOrthoMatrix, const glm::mat4& lightViewMatrix)
{
	glm::mat4 biasMatrix(
		0.5, 0.0, 0.0, 0.0,
		0.0, 0.5, 0.0, 0.0,
		0.0, 0.0, 0.5, 0.0,
		0.5, 0.5, 0.5, 1.0
	);

	glm::mat4 shadowMatrix = lightOrthoMatrix * lightViewMatrix;
	return biasMatrix * shadowMatrix;
}

/// Draw the terrain and all droids, missiles and other objects on it
static void drawTiles(iView *player, LightingData& lightData, LightMap& lightmap, ILightingManager& lightManager)
{
	WZ_PROFILE_SCOPE(drawTiles);
	// draw terrain

	auto currShadowMode = pie_getShadowMode();

	/* ---------------------------------------------------------------- */
	/* Do boundary and extent checking                                  */
	/* ---------------------------------------------------------------- */

	/* Find our position in tile coordinates */
	playerXTile = map_coord(player->p.x);
	playerZTile = map_coord(player->p.z);

	/* ---------------------------------------------------------------- */
	/* Set up the geometry                                              */
	/* ---------------------------------------------------------------- */

	/* ---------------------------------------------------------------- */

	const glm::mat4 baseViewMatrix =
		glm::translate(glm::vec3(0.f, 0.f, distance)) *
		glm::scale(glm::vec3(pie_GetResScalingFactor() / 100.f)) *
		glm::rotate(UNDEG(player->r.z), glm::vec3(0.f, 0.f, 1.f)) *
		glm::rotate(UNDEG(player->r.x), glm::vec3(1.f, 0.f, 0.f)) *
		glm::rotate(UNDEG(player->r.y), glm::vec3(0.f, 1.f, 0.f)) *
		glm::translate(glm::vec3(0, -player->p.y, 0));

	// Calculate shadow mapping cascades
	glm::vec3 lightInvDir = getTheSun();
	size_t numShadowCascades = std::min<size_t>(gfx_api::context::get().numDepthPasses(), WZ_MAX_SHADOW_CASCADES);
	std::vector<Cascade> shadowCascades;
	if (currShadowMode == ShadowMode::Shadow_Mapping)
	{
		calculateShadowCascades(player, distance, baseViewMatrix, lightInvDir, numShadowCascades, shadowCascades);
	}

	// Incorporate all the view transforms into viewMatrix
	const glm::mat4 viewMatrix = baseViewMatrix * glm::translate(glm::vec3(-player->p.x, 0, player->p.z));

	const glm::mat4 perspectiveMatrix = pie_PerspectiveGet();
	const glm::mat4 perspectiveViewMatrix = perspectiveMatrix * viewMatrix;

	const glm::vec3 cameraPos = (glm::inverse(viewMatrix) * glm::vec4(0,0,0,1)).xyz(); // `actualCameraPosition` is not accurate enough due to int calc

	actualCameraPosition = Vector3i(0, 0, 0);

	/* Set the camera position */
	actualCameraPosition.z -= static_cast<int>(distance);

	// Now, scale the world according to what resolution we're running in
	actualCameraPosition.z /= std::max<int>(static_cast<int>(pie_GetResScalingFactor() / 100.f), 1);

	/* Rotate for the player */
	rotateSomething(actualCameraPosition.x, actualCameraPosition.y, -player->r.z);
	rotateSomething(actualCameraPosition.y, actualCameraPosition.z, -player->r.x);
	rotateSomething(actualCameraPosition.z, actualCameraPosition.x, -player->r.y);

	/* Translate */
	actualCameraPosition.y -= -player->p.y;

	// this also determines the length of the shadows
	const Vector3f theSun = (viewMatrix * glm::vec4(getTheSun(), 0.f)).xyz();
	pie_BeginLighting(theSun);

	// Reset all lighting data
	lightData.lights.clear();
	lightManager.SetFrameStart();

	// update the fog of war... FIXME: Remove this
	const glm::mat4 tileCalcPerspectiveViewMatrix = perspectiveMatrix * baseViewMatrix;
	auto currTerrainShaderType = getTerrainShaderType();
	{
		WZ_PROFILE_SCOPE(init_lightmap);
		for (int i = -visibleTiles.y / 2, idx = 0; i <= visibleTiles.y / 2; i++, ++idx)
		{
			/* Go through the x's */
			for (int j = -visibleTiles.x / 2, jdx = 0; j <= visibleTiles.x / 2; j++, ++jdx)
			{
				Vector2i screen(0, 0);
				Position pos;

				pos.x = world_coord(j);
				pos.z = -world_coord(i);
				pos.y = 0;

				if (tileOnMap(playerXTile + j, playerZTile + i))
				{
					MAPTILE* psTile = mapTile(playerXTile + j, playerZTile + i);

					pos.y = map_TileHeight(playerXTile + j, playerZTile + i);
					auto color = pal_SetBrightness((currTerrainShaderType == TerrainShaderType::SINGLE_PASS) ? 0 : static_cast<UBYTE>(psTile->level));
					lightmap(playerXTile + j, playerZTile + i) = color;
				}
				tileScreenInfo[idx][jdx].z = pie_RotateProjectWithPerspective(&pos, tileCalcPerspectiveViewMatrix, &screen);
				tileScreenInfo[idx][jdx].x = screen.x;
				tileScreenInfo[idx][jdx].y = screen.y;
			}
		}
	}

	// Determine whether each tile in the drawable range is actually visible on-screen
	// (used for more accurate clipping elsewhere)
	{
		WZ_PROFILE_SCOPE(tile_Culling);
		for (int idx = 0; idx < visibleTiles.y; ++idx)
		{
			for (int jdx = 0; jdx < visibleTiles.x; ++jdx)
			{
				QUAD quad;

				quad.coords[0].x = tileScreenInfo[idx + 0][jdx + 0].x;
				quad.coords[0].y = tileScreenInfo[idx + 0][jdx + 0].y;

				quad.coords[1].x = tileScreenInfo[idx + 0][jdx + 1].x;
				quad.coords[1].y = tileScreenInfo[idx + 0][jdx + 1].y;

				quad.coords[2].x = tileScreenInfo[idx + 1][jdx + 1].x;
				quad.coords[2].y = tileScreenInfo[idx + 1][jdx + 1].y;

				quad.coords[3].x = tileScreenInfo[idx + 1][jdx + 0].x;
				quad.coords[3].y = tileScreenInfo[idx + 1][jdx + 0].y;

				tileScreenVisible[idx][jdx] = quadIntersectsWithScreen(quad);
			}
		}
	}

	wzPerfEnd(PERF_START_FRAME);

	pie_StartMeshes();

	/* This is done here as effects can light the terrain - pause mode problems though */
	wzPerfBegin(PERF_EFFECTS, "3D scene - effects");
	processEffects(perspectiveViewMatrix, lightData);
	atmosUpdateSystem();
	avUpdateTiles();
	wzPerfEnd(PERF_EFFECTS);

	// The lightmap need to be ready at this point
	{
		WZ_PROFILE_SCOPE(LightingManager_ComputeFrameData);
		lightManager.ComputeFrameData(lightData, lightmap, perspectiveViewMatrix);
	}

	// prepare terrain for drawing
	perFrameTerrainUpdates(lightmap);

	// and prepare for rendering the models
	wzPerfBegin(PERF_MODEL_INIT, "Draw 3D scene - model init");

	/* ---------------------------------------------------------------- */
	/* Calculate & batch all mesh / object instances to be drawn        */
	/* ---------------------------------------------------------------- */
	displayStaticObjects(viewMatrix, perspectiveViewMatrix); // may be bucket render implemented
	displayFeatures(viewMatrix, perspectiveViewMatrix);
	displayDynamicObjects(viewMatrix, perspectiveViewMatrix); // may be bucket render implemented
	if (doWeDrawProximitys())
	{
		displayProximityMsgs(viewMatrix, perspectiveViewMatrix);
	}
	displayDelivPoints(viewMatrix, perspectiveViewMatrix);
	display3DProjectiles(viewMatrix, perspectiveViewMatrix); // may be bucket render implemented
	wzPerfEnd(PERF_MODEL_INIT);

	wzPerfBegin(PERF_PARTICLES, "3D scene - particles");
	atmosDrawParticles(viewMatrix, perspectiveViewMatrix);
	wzPerfEnd(PERF_PARTICLES);

	bucketRenderCurrentList(viewMatrix, perspectiveViewMatrix);

	gfx_api::context::get().debugStringMarker("Draw 3D scene - blueprints");
	displayBlueprints(viewMatrix, perspectiveViewMatrix);

	/* ---------------------------------------------------------------- */
	/* Actually render / draw everything                                */
	/* ---------------------------------------------------------------- */

	pie_UpdateLightmap(getTerrainLightmapTexture(), getModelUVLightmapMatrix());
	pie_FinalizeMeshes(currentGameFrame);


	// shadow/depth-mapping passes
	ShadowCascadesInfo shadowCascadesInfo;
	shadowCascadesInfo.shadowMapSize = gfx_api::context::get().getDepthPassDimensions(0); // Note: Currently assumes that every depth pass has the same dimensions
	for (size_t i = 0; i < std::min<size_t>(shadowCascades.size(), WZ_MAX_SHADOW_CASCADES); ++i)
	{
		shadowCascadesInfo.shadowMVPMatrix[i] = getBiasedShadowMapMVPMatrix(shadowCascades[i].projectionMatrix, shadowCascades[i].viewMatrix);
		shadowCascadesInfo.shadowCascadeSplit[i] = shadowCascades[i].splitDepth;
	}

	if (currShadowMode == ShadowMode::Shadow_Mapping)
	{
		WZ_PROFILE_SCOPE(ShadowMapping);
		for (size_t i = 0; i < numShadowCascades; ++i)
		{
			gfx_api::context::get().beginDepthPass(i);
			pie_DrawAllMeshes(currentGameFrame, shadowCascades[i].projectionMatrix, shadowCascades[i].viewMatrix, shadowCascadesInfo, true);
			gfx_api::context::get().endCurrentDepthPass();
		}
	}
	// start main render pass


	gfx_api::context::get().beginSceneRenderPass();

	// now we are about to draw the terrain
	wzPerfBegin(PERF_TERRAIN, "3D scene - terrain");
	pie_SetFogStatus(true);
	drawTerrain(perspectiveViewMatrix, viewMatrix, cameraPos, -getTheSun(), shadowCascadesInfo);
	wzPerfEnd(PERF_TERRAIN);

	// draw skybox
	// NOTE: Must come *after* drawTerrain *if* using the fallback (old) terrain shaders
	wzPerfBegin(PERF_SKYBOX, "3D scene - skybox");
	renderSurroundings(pie_SkyboxPerspectiveGet(), baseViewMatrix);
	wzPerfEnd(PERF_SKYBOX);

	wzPerfBegin(PERF_WATER, "3D scene - water");
	// prepare for the water and the lightmap
	pie_SetFogStatus(true);
	// also, make sure we can use world coordinates directly
	drawWater(perspectiveViewMatrix, cameraPos, -getTheSun());
	wzPerfEnd(PERF_WATER);

	wzPerfBegin(PERF_MODELS, "3D scene - models");
	{
		WZ_PROFILE_SCOPE(pie_DrawAllMeshes);
		pie_DrawAllMeshes(currentGameFrame, perspectiveMatrix, viewMatrix, shadowCascadesInfo, false);
	}
	wzPerfEnd(PERF_MODELS);

	if (!gamePaused())
	{
		doConstructionLines(viewMatrix);
	}
	locateMouse();

	{
		WZ_PROFILE_SCOPE(endSceneRenderPass);
		gfx_api::context::get().endSceneRenderPass();
	}

	// Draw the scene to the default framebuffer
	{
		WZ_PROFILE_SCOPE(copyToFBO);
		gfx_api::WorldToScreenPSO::get().bind();
		gfx_api::WorldToScreenPSO::get().bind_constants({1.0f});
		gfx_api::WorldToScreenPSO::get().bind_vertex_buffers(pScreenTriangleVBO);
		gfx_api::WorldToScreenPSO::get().bind_textures(gfx_api::context::get().getSceneTexture());
		gfx_api::WorldToScreenPSO::get().draw(3, 0);
		gfx_api::WorldToScreenPSO::get().unbind_vertex_buffers(pScreenTriangleVBO);
	}
}

/// Initialise the fog, skybox and some other stuff
bool init3DView()
{
	setTheSun(getDefaultSunPosition());

	/* There are no drag boxes */
	dragBox3D.status = DRAG_INACTIVE;

	/* Get all the init stuff out of here? */
	initWarCam();

	/* Init the game messaging system */
	initConsoleMessages();

	atmosInitSystem();

	// default skybox, will override in script if not satisfactory
	setSkyBox("texpages/page-25-sky-arizona.png", 0.0f, 10000.0f);

	// distance is not saved, so initialise it now
	distance = war_GetMapZoom(); // distance

	if(pie_GetFogEnabled()){
		pie_SetFogStatus(true);
	}

	// Set the initial fog distance
	updateFogDistance(distance);

	setDefaultFogColour();

	playerPos.r.z = 0; // roll
	playerPos.r.y = 0; // rotation
	playerPos.r.x = DEG(360 + INITIAL_STARTING_PITCH); // angle

	if (!initTerrain())
	{
		return false;
	}

	txtLevelName.setText(WzString::fromUtf8(mapNameWithoutTechlevel(getLevelName())), font_small);
	txtDebugStatus.setText("DEBUG ", font_small);

	batchedObjectStatusRenderer.initialize();


	if (!pScreenTriangleVBO)
	{
		// vertex attributes for a single triangle that covers the screen
		gfx_api::gfxFloat screenTriangleVertices[] = {
			-1.f, -1.f,
			3.f, -1.f,
			-1.f, 3.f
		};
		pScreenTriangleVBO = gfx_api::context::get().create_buffer_object(gfx_api::buffer::usage::vertex_buffer, gfx_api::context::buffer_storage_hint::static_draw, "screenTriangleVertices");
		pScreenTriangleVBO->upload(sizeof(screenTriangleVertices), screenTriangleVertices);
	}

	return true;
}

void shutdown3DView()
{
	txtLevelName = WzText();
	txtDebugStatus = WzText();
	txtCurrentTime = WzText();
	txtShowFPS = WzText();
	txtUnits = WzText();
	// show Samples text
	txtShowSamples_Que = WzText();
	txtShowSamples_Lst = WzText();
	txtShowSamples_Act = WzText();
	// show Orders text
	txtShowOrders = WzText();
	// show Droid visible/draw counts text
	droidText = WzText();

	batchedObjectStatusRenderer.clear(); // NOTE: *NOT* reset() - see shutdown3DView_FullReset below for why
}

void shutdown3DView_FullReset()
{
	// Because of when levReleaseMissionData can be called (especially when using certain cheats in campaign like "ascend" or "let me win")
	// Only call "reset" (which also resets BatchedMultiRectRenderer and actually frees GPU buffers)
	// in this function, which is called from stageTwoShutdown (as opposed to stageThreeShutdown, which is called from levReleaseMissionData)...
	batchedObjectStatusRenderer.reset();

	delete pScreenTriangleVBO;
	pScreenTriangleVBO = nullptr;
}

/// set the view position from save game
void disp3d_setView(iView *newView)
{
	playerPos = *newView;
}

/// reset the camera rotation (used for save games <= 10)
void disp3d_oldView()
{
	playerPos.r.y = OLD_INITIAL_ROTATION; // rotation
	playerPos.p.y = OLD_START_HEIGHT; // height
}

/// get the view position for save game
void disp3d_getView(iView *newView)
{
	*newView = playerPos;
}

/// Are the current world coordinates within the processed range of tiles on the screen?
/// (Warzone has a maximum range of tiles around the current player camera position that it will display.)
bool quickClipXYToMaximumTilesFromCurrentPosition(SDWORD x, SDWORD y)
{
	// +2 for edge of visibility fading (see terrain.cpp)
	if (std::abs(x - playerPos.p.x) < world_coord(visibleTiles.x / 2 + 2) &&
		std::abs(y - playerPos.p.z) < world_coord(visibleTiles.y / 2 + 2))
	{
		return (true);
	}
	else
	{
		return (false);
	}
}

/// Are the current tile coordinates visible on screen?
bool clipXY(SDWORD x, SDWORD y)
{
	// +2 for edge of visibility fading (see terrain.cpp)
	if (std::abs(x - playerPos.p.x) < world_coord(visibleTiles.x / 2 + 2) &&
	    std::abs(y - playerPos.p.z) < world_coord(visibleTiles.y / 2 + 2))
	{
		// additional check using the tileScreenVisible matrix
		int mapX = map_coord(x - playerPos.p.x) + visibleTiles.x / 2;
		int mapY = map_coord(y - playerPos.p.z) + visibleTiles.y / 2;
		if (mapX < 0 || mapY < 0)
		{
			return false;
		}
		if (mapX > visibleTiles.x || mapY > visibleTiles.y)
		{
			return false;
		}
		return tileScreenVisible[mapY][mapX];
	}
	else
	{
		return (false);
	}
}

bool clipXYZNormalized(const Vector3i &normalizedPosition, const glm::mat4 &perspectiveViewMatrix)
{
	Vector2i pixel(0, 0);
	pie_RotateProjectWithPerspective(&normalizedPosition, perspectiveViewMatrix, &pixel);
	return pixel.x >= 0 && pixel.y >= 0 && pixel.x < pie_GetVideoBufferWidth() && pixel.y < pie_GetVideoBufferHeight();
}

/// Are the current 3d game-world coordinates visible on screen?
/// (Does not take into account occlusion)
bool clipXYZ(int x, int y, int z, const glm::mat4 &perspectiveViewMatrix)
{
	Vector3i position;
	position.x = x;
	position.z = -(y);
	position.y = z;

	return clipXYZNormalized(position, perspectiveViewMatrix);
}

bool clipShapeOnScreen(const iIMDShape *pIMD, const glm::mat4 &perspectiveViewModelMatrix, int overdrawScreenPoints /*= 10*/)
{
	/* Get its absolute dimensions */
	Vector3i origin;
	Vector2i center(0, 0);
	int wsRadius = 22; // World space radius, 22 = magic minimum
	float radius;

	if (pIMD)
	{
		wsRadius = MAX(wsRadius, pIMD->radius);
	}

	origin = Vector3i(0, wsRadius, 0); // take the center of the object

	/* get the screen coordinates */
	const float cZ = pie_RotateProjectWithPerspective(&origin, perspectiveViewModelMatrix, &center) * 0.1f;

	// avoid division by zero
	if (cZ > 0)
	{
		radius = wsRadius / cZ * pie_GetResScalingFactor();
	}
	else
	{
		radius = 1; // 1 just in case some other code assumes radius != 0
	}

	const int screenMinX = -overdrawScreenPoints;
	const int screenMinY = -overdrawScreenPoints;

	return (center.x + radius >  screenMinX &&
			center.x - radius < (pie_GetVideoBufferWidth() + overdrawScreenPoints) &&
			center.y + radius >  screenMinY &&
			center.y - radius < (pie_GetVideoBufferHeight() + overdrawScreenPoints));
}

// Use overdrawScreenPoints as a workaround for casting shadows when the main unit is off-screen (but right at the edge)
bool clipDroidOnScreen(DROID *psDroid, const glm::mat4 &perspectiveViewModelMatrix, int overdrawScreenPoints /*= 25*/)
{
	/* Get its absolute dimensions */
	// NOTE: This only takes into account body, but is "good enough"
	const BODY_STATS *psBStats = psDroid->getBodyStats();
	const iIMDShape * pIMD = (psBStats != nullptr) ? psBStats->pIMD->displayModel() : nullptr;

	return clipShapeOnScreen(pIMD, perspectiveViewModelMatrix, overdrawScreenPoints);
}

bool clipStructureOnScreen(STRUCTURE *psStructure)
{
	StructureBounds b = getStructureBounds(psStructure);
	assert(b.size.x != 0 && b.size.y != 0);
	for (int breadth = 0; breadth < b.size.y + 2; ++breadth) // +2 to make room for shadows on the terrain
	{
		for (int width = 0; width < b.size.x + 2; ++width)
		{
			if (clipXY(world_coord(b.map.x + width), world_coord(b.map.y + breadth)))
			{
				return true;
			}
		}
	}

	return false;
}

/** Get the screen coordinates for the current transform matrix.
 * This function is used to determine the area the user can click for the
 * intelligence screen buttons. The radius parameter is always set to the same value.
 */
static void	calcFlagPosScreenCoords(SDWORD *pX, SDWORD *pY, SDWORD *pR, const glm::mat4 &perspectiveViewModelMatrix)
{
	/* Get it's absolute dimensions */
	Vector3i center3d(0, 0, 0);
	Vector2i center2d(0, 0);
	/* How big a box do we want - will ultimately be calculated using xmax, ymax, zmax etc */
	UDWORD	radius = 22;

	/* Pop matrices and get the screen coordinates for last point*/
	pie_RotateProjectWithPerspective(&center3d, perspectiveViewModelMatrix, &center2d);

	/*store the coords*/
	*pX = center2d.x;
	*pY = center2d.y;
	*pR = radius;
}

/// Decide whether to render a projectile, and make sure it will be drawn
static void display3DProjectiles(const glm::mat4 &viewMatrix, const glm::mat4 &perspectiveViewMatrix)
{
	WZ_PROFILE_SCOPE(display3DProjectiles);
	PROJECTILE *psObj = proj_GetFirst();
	while (psObj != nullptr)
	{
		// If source or destination is visible, and projectile has been spawned and has not impacted.
		if (graphicsTime >= psObj->prevSpacetime.time && graphicsTime <= psObj->time && gfxVisible(psObj))
		{
			/* Draw a bullet at psObj->pos.x for X coord
			   psObj->pos.y for Z coord
			   whatever for Y (height) coord - arcing ?
			*/
			/* these guys get drawn last */
			if (psObj->psWStats->weaponSubClass == WSC_ROCKET ||
			    psObj->psWStats->weaponSubClass == WSC_MISSILE ||
			    psObj->psWStats->weaponSubClass == WSC_COMMAND ||
			    psObj->psWStats->weaponSubClass == WSC_SLOWMISSILE ||
			    psObj->psWStats->weaponSubClass == WSC_SLOWROCKET ||
			    psObj->psWStats->weaponSubClass == WSC_ENERGY ||
			    psObj->psWStats->weaponSubClass == WSC_EMP)
			{
				bucketAddTypeToList(RENDER_PROJECTILE, psObj, perspectiveViewMatrix);
			}
			else
			{
				renderProjectile(psObj, viewMatrix, perspectiveViewMatrix);
			}
		}

		psObj = proj_GetNext();
	}
}	/* end of function display3DProjectiles */

/// Draw a projectile to the screen
void	renderProjectile(PROJECTILE *psCurr, const glm::mat4 &viewMatrix, const glm::mat4 &perspectiveViewMatrix)
{
	WEAPON_STATS	*psStats;
	Vector3i			dv;
	iIMDShape		*pIMD;
	Spacetime       st;

	psStats = psCurr->psWStats;
	/* Reject flame or command since they have interim drawn fx */
	if (psStats->weaponSubClass == WSC_FLAME ||
	    psStats->weaponSubClass == WSC_COMMAND || // || psStats->weaponSubClass == WSC_ENERGY)
	    psStats->weaponSubClass == WSC_ELECTRONIC ||
	    psStats->weaponSubClass == WSC_EMP ||
	    (bMultiPlayer && psStats->weaponSubClass == WSC_LAS_SAT))
	{
		/* We don't do projectiles from these guys, cos there's an effect instead */
		return;
	}

	st = interpolateObjectSpacetime(psCurr, graphicsTime);

	//the weapon stats holds the reference to which graphic to use
	/*Need to draw the graphic depending on what the projectile is doing - hitting target,
	missing target, in flight etc - JUST DO IN FLIGHT FOR NOW! */
	pIMD = (psStats->pInFlightGraphic) ? psStats->pInFlightGraphic->displayModel() : nullptr;

	if (!clipXYZ(st.pos.x, st.pos.y, st.pos.z, perspectiveViewMatrix))
	{
		return; // Projectile is not on the screen (Note: This uses the position point of the projectile, not a full shape clipping check, for speed.)
	}

	/* Get bullet's x coord */
	dv.x = st.pos.x;

	/* Get it's y coord (z coord in the 3d world */
	dv.z = -(st.pos.y);

	/* What's the present height of the bullet? */
	dv.y = st.pos.z;

	/* Set up the matrix */
	Vector3i camera_base = actualCameraPosition;

	/* Translate to the correct position */
	Vector3i camera_mod;
	camera_mod.x = st.pos.x - playerPos.p.x;
	camera_mod.z = -(st.pos.y - playerPos.p.z);
	camera_mod.y = st.pos.z;
	camera_base -= camera_mod;

	/* Rotate it to the direction it's facing */
	rotateSomething(camera_base.z, camera_base.x, -(-st.rot.direction));

	/* pitch it */
	rotateSomething(camera_base.y, camera_base.z, -st.rot.pitch);

	const glm::mat4 modelMatrix_base =
		glm::translate(glm::vec3(dv)) *
		glm::rotate(UNDEG(-st.rot.direction), glm::vec3(0.f, 1.f, 0.f)) *
		glm::rotate(UNDEG(st.rot.pitch), glm::vec3(1.f, 0.f, 0.f));

	for (; pIMD != nullptr; pIMD = pIMD->next.get())
	{
		bool rollToCamera = false;
		bool pitchToCamera = false;
		bool premultiplied = false;
		bool additive = psStats->weaponSubClass == WSC_ROCKET || psStats->weaponSubClass == WSC_MISSILE || psStats->weaponSubClass == WSC_SLOWROCKET || psStats->weaponSubClass == WSC_SLOWMISSILE;

		if (pIMD->flags & iV_IMD_ROLL_TO_CAMERA)
		{
			rollToCamera = true;
		}
		if (pIMD->flags & iV_IMD_PITCH_TO_CAMERA)
		{
			rollToCamera = true;
			pitchToCamera = true;
		}
		if (pIMD->flags & iV_IMD_NO_ADDITIVE)
		{
			additive = false;
		}
		if (pIMD->flags & iV_IMD_ADDITIVE)
		{
			additive = true;
		}
		if (pIMD->flags & iV_IMD_PREMULTIPLIED)
		{
			additive = false;
			premultiplied = true;
		}

		Vector3i camera = camera_base;
		glm::mat4 modelMatrix = modelMatrix_base;

		if (pitchToCamera || rollToCamera)
		{
			// Centre on projectile (relevant for twin projectiles).
			camera -= Vector3i(pIMD->connectors[0].x, pIMD->connectors[0].y, pIMD->connectors[0].z);
			modelMatrix *= glm::translate(glm::vec3(pIMD->connectors[0]));
		}

		if (pitchToCamera)
		{
			int x = iAtan2(camera.z, camera.y);
			rotateSomething(camera.y, camera.z, -x);
			modelMatrix *= glm::rotate(UNDEG(x), glm::vec3(1.f, 0.f, 0.f));
		}

		if (rollToCamera)
		{
			int z = -iAtan2(camera.x, camera.y);
			rotateSomething(camera.x, camera.y, -z);
			modelMatrix *= glm::rotate(UNDEG(z), glm::vec3(0.f, 0.f, 1.f));
		}

		if (pitchToCamera || rollToCamera)
		{
			camera -= Vector3i(-pIMD->connectors[0].x, -pIMD->connectors[0].y, -pIMD->connectors[0].z);
			// Undo centre on projectile (relevant for twin projectiles).
			modelMatrix *= glm::translate(glm::vec3(-pIMD->connectors[0].x, -pIMD->connectors[0].y, -pIMD->connectors[0].z));
		}

		if (premultiplied)
		{
			pie_Draw3DShape(pIMD, 0, 0, WZCOL_WHITE, pie_PREMULTIPLIED, 0, modelMatrix, viewMatrix, 0.f, true);
		}
		else if (additive)
		{
			pie_Draw3DShape(pIMD, 0, 0, WZCOL_WHITE, pie_ADDITIVE, 164, modelMatrix, viewMatrix, 0.f, true);
		}
		else
		{
			pie_Draw3DShape(pIMD, 0, 0, WZCOL_WHITE, 0, 0, modelMatrix, viewMatrix, 0.f, true);
		}
	}
}

/// Draw the buildings
static void displayStaticObjects(const glm::mat4 &viewMatrix, const glm::mat4 &perspectiveViewMatrix)
{
	WZ_PROFILE_SCOPE(displayStaticObjects);
	// to solve the flickering edges of baseplates
//	pie_SetDepthOffset(-1.0f);

	/* Go through all the players */
	for (unsigned aPlayer = 0; aPlayer < MAX_PLAYERS; ++aPlayer)
	{
		/* Now go all buildings for that player */
		for (BASE_OBJECT* obj : apsStructLists[aPlayer])
		{
			/* Worth rendering the structure? */
			if (obj->type != OBJ_STRUCTURE || (obj->died != 0 && obj->died < graphicsTime))
			{
				continue;
			}
			STRUCTURE *psStructure = castStructure(obj);

			if (!clipStructureOnScreen(psStructure))
			{
				continue;
			}

			renderStructure(psStructure, viewMatrix, perspectiveViewMatrix);
		}
	}

	// Walk through destroyed objects.

	/* Now go all buildings for that player */
	for (BASE_OBJECT* obj : psDestroyedObj)
	{
		/* Worth rendering the structure? */
		if (obj->type != OBJ_STRUCTURE || (obj->died != 0 && obj->died < graphicsTime))
		{
			continue;
		}
		STRUCTURE* psStructure = castStructure(obj);

		if (!clipStructureOnScreen(psStructure))
		{
			continue;
		}

		renderStructure(psStructure, viewMatrix, perspectiveViewMatrix);
	}

//	pie_SetDepthOffset(0.0f);
}

static bool tileHasIncompatibleStructure(MAPTILE const *tile, STRUCTURE_STATS const *stats, int moduleIndex)
{
	STRUCTURE *psStruct = castStructure(tile->psObject);
	if (psStruct == nullptr)
	{
		return false;
	}
	if (psStruct->status == SS_BEING_BUILT && nextModuleToBuild(psStruct, -1) >= moduleIndex)
	{
		return true;
	}
	if (isWall(psStruct->pStructureType->type) && isBuildableOnWalls(stats->type))
	{
		return false;
	}
	if (IsStatExpansionModule(stats))
	{
		return false;
	}
	return true;
}

static void drawLineBuild(uint8_t player, STRUCTURE_STATS const *psStats, Vector2i pos, Vector2i pos2, uint16_t direction, STRUCT_STATES state)
{
	auto lb = calcLineBuild(psStats, direction, pos, pos2);

	for (int i = 0; i < lb.count; ++i)
	{
		Vector2i cur = lb[i];
		if (tileHasIncompatibleStructure(worldTile(cur), psStats, 0))
		{
			continue;  // construction has started
		}

		StructureBounds b = getStructureBounds(psStats, cur, direction);
		int z = 0;
		for (int j = 0; j <= b.size.y; ++j)
			for (int k = 0; k <= b.size.x; ++k)
			{
				z = std::max(z, map_TileHeight(b.map.x + k, b.map.y + j));
			}
		Blueprint blueprint(psStats, Vector3i(cur, z), snapDirection(direction), 0, state, player); // snapDirection may be unnecessary here
		blueprints.push_back(blueprint);
	}
}

static void renderBuildOrder(uint8_t droidPlayer, DroidOrder const &order, STRUCT_STATES state)
{
	STRUCTURE_STATS const *stats;
	Vector2i pos = order.pos;
	if (order.type == DORDER_BUILDMODULE)
	{
		STRUCTURE const *structure = castStructure(order.psObj);
		if (structure == nullptr)
		{
			return;
		}
		stats = getModuleStat(structure);
		pos = structure->pos.xy();
	}
	else
	{
		stats = order.psStats;
	}

	if (stats == nullptr)
	{
		return;
	}

	//draw the current build site if its a line of structures
	if (order.type == DORDER_LINEBUILD)
	{
		drawLineBuild(droidPlayer, stats, pos, order.pos2, order.direction, state);
	}
	if ((order.type == DORDER_BUILD || order.type == DORDER_BUILDMODULE) && !tileHasIncompatibleStructure(mapTile(map_coord(pos)), stats, order.index))
	{
		StructureBounds b = getStructureBounds(stats, pos, order.direction);
		int z = 0;
		for (int j = 0; j <= b.size.y; ++j)
			for (int i = 0; i <= b.size.x; ++i)
			{
				z = std::max(z, map_TileHeight(b.map.x + i, b.map.y + j));
			}
		Blueprint blueprint(stats, Vector3i(pos, z), snapDirection(order.direction), order.index, state, droidPlayer);
		blueprints.push_back(blueprint);
	}
}

std::unique_ptr<Blueprint> playerBlueprint = std::make_unique<Blueprint>();
std::unique_ptr<ValueTracker> playerBlueprintX = std::make_unique<ValueTracker>();
std::unique_ptr<ValueTracker> playerBlueprintY = std::make_unique<ValueTracker>();
std::unique_ptr<ValueTracker> playerBlueprintZ = std::make_unique<ValueTracker>();
std::unique_ptr<ValueTracker> playerBlueprintDirection = std::make_unique<ValueTracker>();

void displayBlueprints(const glm::mat4 &viewMatrix, const glm::mat4 &perspectiveViewMatrix)
{
	blueprints.clear();  // Delete old blueprints and draw new ones.

	if ((buildState == BUILD3D_VALID || buildState == BUILD3D_POS) &&
	    sBuildDetails.x > 0 && sBuildDetails.x < (int)mapWidth &&
	    sBuildDetails.y > 0 && sBuildDetails.y < (int)mapHeight)
	{
		STRUCT_STATES state;
		if (buildState == BUILD3D_VALID)
		{
			state = SS_BLUEPRINT_VALID;
		}
		else
		{
			state = SS_BLUEPRINT_INVALID;
		}
		// we are placing a building or a delivery point
		if (STRUCTURE_STATS *stats = castStructureStats(sBuildDetails.psStats))
		{
			// it's a building
			uint16_t direction = getBuildingDirection();
			if (wallDrag.status == DRAG_PLACING || wallDrag.status == DRAG_DRAGGING)
			{
				drawLineBuild(selectedPlayer, stats, wallDrag.pos, wallDrag.pos2, direction, state);
			}
			else
			{
				unsigned width, height;
				if ((direction & 0x4000) == 0)
				{
					width  = sBuildDetails.width;
					height = sBuildDetails.height;
				}
				else
				{
					// Rotated 90°, swap width and height
					width  = sBuildDetails.height;
					height = sBuildDetails.width;
				}
				// a single building
				Vector2i pos(world_coord(sBuildDetails.x) + world_coord(width) / 2, world_coord(sBuildDetails.y) + world_coord(height) / 2);

				StructureBounds b = getStructureBounds(stats, pos, direction);
				int z = 0;
				for (int j = 0; j <= b.size.y; ++j)
					for (int i = 0; i <= b.size.x; ++i)
					{
						z = std::max(z, map_TileHeight(b.map.x + i, b.map.y + j));
					}

				if(!playerBlueprintX->isTracking()){
					playerBlueprintX->startTracking(pos.x)->setSpeed(BlueprintTrackAnimationSpeed);
					playerBlueprintY->startTracking(pos.y)->setSpeed(BlueprintTrackAnimationSpeed);
					playerBlueprintZ->startTracking(z)->setSpeed(BlueprintTrackAnimationSpeed);
					playerBlueprintDirection->startTracking(direction)->setSpeed(BlueprintTrackAnimationSpeed+30);
				}

				playerBlueprintX->setTarget(pos.x)->update();
				playerBlueprintY->setTarget(pos.y)->update();
				playerBlueprintZ->setTarget(z)->update();

				if(playerBlueprintDirection->reachedTarget()){
					playerBlueprintDirection->startTracking(playerBlueprintDirection->getTarget());
					playerBlueprintDirection->setTargetDelta((SWORD)(direction - playerBlueprintDirection->getTarget()));
				}

				playerBlueprintDirection->update();

				playerBlueprint->stats = stats;
				playerBlueprint->pos = { playerBlueprintX->getCurrent(), playerBlueprintY->getCurrent(), playerBlueprintZ->getCurrent() };
				playerBlueprint->dir = playerBlueprintDirection->getCurrent();
				playerBlueprint->index = 0;
				playerBlueprint->state = state;
				playerBlueprint->player = selectedPlayer;

				blueprints.push_back(*playerBlueprint);
			}
		}
	} else {
		playerBlueprintX->stopTracking();
		playerBlueprintY->stopTracking();
		playerBlueprintZ->stopTracking();
		playerBlueprintDirection->stopTracking();
	}

	// now we draw the blueprints for all ordered buildings
	for (unsigned player = 0; player < MAX_PLAYERS; ++player)
	{
		if (!hasSharedVision(selectedPlayer, player) && !NetPlay.players[selectedPlayer].isSpectator)
		{
			continue;
		}
		STRUCT_STATES state = player == selectedPlayer ? SS_BLUEPRINT_PLANNED : SS_BLUEPRINT_PLANNED_BY_ALLY;
		for (const DROID *psDroid : apsDroidLists[player])
		{
			if (psDroid->droidType == DROID_CONSTRUCT || psDroid->droidType == DROID_CYBORG_CONSTRUCT)
			{
				renderBuildOrder(psDroid->player, psDroid->order, state);
				//now look thru' the list of orders to see if more building sites
				for (int order = psDroid->listPendingBegin; order < (int)psDroid->asOrderList.size(); order++)
				{
					renderBuildOrder(psDroid->player, psDroid->asOrderList[order], state);
				}
			}
		}
	}

	// Erase duplicate blueprints.
	std::sort(blueprints.begin(), blueprints.end());
	blueprints.erase(std::unique(blueprints.begin(), blueprints.end()), blueprints.end());

	// Actually render everything.
	for (auto &blueprint : blueprints)
	{
		blueprint.renderBlueprint(viewMatrix, perspectiveViewMatrix);
	}

	renderDeliveryRepos(viewMatrix, perspectiveViewMatrix);
}

/// Draw Factory Delivery Points
static void displayDelivPoints(const glm::mat4& viewMatrix, const glm::mat4 &perspectiveViewMatrix)
{
	WZ_PROFILE_SCOPE(displayDelivPoints);
	if (selectedPlayer >= MAX_PLAYERS) { return; /* no-op */ }
	for (const auto& psDelivPoint : apsFlagPosLists[selectedPlayer])
	{
		if (clipXY(psDelivPoint->coords.x, psDelivPoint->coords.y))
		{
			renderDeliveryPoint(psDelivPoint, false, viewMatrix, perspectiveViewMatrix);
		}
	}
}

/// Draw the features
static void displayFeatures(const glm::mat4 &viewMatrix, const glm::mat4 &perspectiveViewMatrix)
{
	WZ_PROFILE_SCOPE(displayFeatures);
	// player can only be 0 for the features.

	/* Go through all the features */
	for (BASE_OBJECT* obj : apsFeatureLists[0])
	{
		if (obj->type == OBJ_FEATURE
			&& (obj->died == 0 || obj->died > graphicsTime)
			&& clipXY(obj->pos.x, obj->pos.y))
		{
			FEATURE* psFeature = castFeature(obj);
			renderFeature(psFeature, viewMatrix, perspectiveViewMatrix);
		}
	}

	// Walk through destroyed objects.

	/* Go through all the features */
	for (BASE_OBJECT* obj : psDestroyedObj)
	{
		if (obj->type == OBJ_FEATURE
			&& (obj->died == 0 || obj->died > graphicsTime)
			&& clipXY(obj->pos.x, obj->pos.y))
		{
			FEATURE* psFeature = castFeature(obj);
			renderFeature(psFeature, viewMatrix, perspectiveViewMatrix);
		}
	}
}

/// Draw the Proximity messages for the *SELECTED PLAYER ONLY*
static void displayProximityMsgs(const glm::mat4& viewMatrix, const glm::mat4 &perspectiveViewMatrix)
{
	if (selectedPlayer >= MAX_PLAYERS) { return; /* no-op */ }

	/* Go through all the proximity Displays*/
	for (PROXIMITY_DISPLAY* psProxDisp : apsProxDisp[selectedPlayer])
	{
		if (!(psProxDisp->psMessage->read))
		{
			unsigned x, y;
			if (psProxDisp->type == POS_PROXDATA)
			{
				VIEW_PROXIMITY *pViewProximity = (VIEW_PROXIMITY *)psProxDisp->psMessage->pViewData->pData;
				x = pViewProximity->x;
				y = pViewProximity->y;
			}
			else
			{
				if (!psProxDisp->psMessage->psObj)
				{
					continue;    // sanity check
				}
				x = psProxDisp->psMessage->psObj->pos.x;
				y = psProxDisp->psMessage->psObj->pos.y;
			}
			/* Is the Message worth rendering? */
			if (clipXY(x, y))
			{
				renderProximityMsg(psProxDisp, viewMatrix, perspectiveViewMatrix);
			}
		}
	}
}

/// Draw the droids
static void displayDynamicObjects(const glm::mat4 &viewMatrix, const glm::mat4 &perspectiveViewMatrix)
{
	WZ_PROFILE_SCOPE(displayDynamicObjects);
	/* Need to go through all the droid lists */
	for (unsigned player = 0; player < MAX_PLAYERS; ++player)
	{
		for (DROID* psDroid : apsDroidLists[player])
		{
			if (!psDroid || (psDroid->died != 0 && psDroid->died < graphicsTime)
			    || !quickClipXYToMaximumTilesFromCurrentPosition(psDroid->pos.x, psDroid->pos.y))
			{
				continue;
			}

			/* No point in adding it if you can't see it? */
			if (psDroid->visibleForLocalDisplay())
			{
				displayComponentObject(psDroid, viewMatrix, perspectiveViewMatrix);
			}
		}
	}

	// Walk through destroyed objects.
	for (const auto& obj : psDestroyedObj)
	{
		DROID* psDroid = castDroid(obj);
		if (!psDroid || (obj->died != 0 && obj->died < graphicsTime)
			|| !quickClipXYToMaximumTilesFromCurrentPosition(obj->pos.x, obj->pos.y))
		{
			continue;
		}

		/* No point in adding it if you can't see it? */
		if (psDroid->visibleForLocalDisplay())
		{
			displayComponentObject(psDroid, viewMatrix, perspectiveViewMatrix);
		}
	}
}

/// Sets the player's position and view angle - defaults player rotations as well
void setViewPos(UDWORD x, UDWORD y, WZ_DECL_UNUSED bool Pan)
{
	playerPos.p.x = world_coord(x);
	playerPos.p.z = world_coord(y);
	playerPos.r.z = 0;

	updatePlayerAverageCentreTerrainHeight();

	if(playerPos.p.y < averageCentreTerrainHeight)
	{
		playerPos.p.y = averageCentreTerrainHeight + CAMERA_PIVOT_HEIGHT - HEIGHT_TRACK_INCREMENTS;
	}

	if (getWarCamStatus())
	{
		camToggleStatus();
	}
}

/// Get the player position
Vector2i getPlayerPos()
{
	return playerPos.p.xz();
}

/// Set the player position
void setPlayerPos(SDWORD x, SDWORD y)
{
	ASSERT(x >= 0 && x < world_coord(mapWidth) && y >= 0 && y < world_coord(mapHeight), "Position off map");
	playerPos.p.x = x;
	playerPos.p.z = y;
	playerPos.r.z = 0;
}

/// Get the distance at which the player views the world
float getViewDistance()
{
	return distance;
}

/// Set the distance at which the player views the world
void setViewDistance(float dist)
{
	distance = dist;
	debug(LOG_WZ, _("Setting zoom to %.0f"), distance);
}

/// Draw a feature (tree/rock/etc.)
void	renderFeature(FEATURE *psFeature, const glm::mat4 &viewMatrix, const glm::mat4 &perspectiveViewMatrix)
{
	PIELIGHT brightness = pal_SetBrightness(200);
	bool bForceDraw = (getRevealStatus() && psFeature->psStats->visibleAtStart);
	int pieFlags = 0;

	if (!psFeature->visibleForLocalDisplay() && !bForceDraw)
	{
		return;
	}

	/* Mark it as having been drawn */
	psFeature->sDisplay.frameNumber = currentGameFrame;

	/* Daft hack to get around the oil derrick issue */
	if (!TileHasFeature(mapTile(map_coord(psFeature->pos.xy()))))
	{
		return;
	}

	Vector3i dv = Vector3i(
	         psFeature->pos.x,
	         psFeature->pos.z, // features sits at the height of the tile it's centre is on
	         -(psFeature->pos.y)
	     );

	Vector3i rotation;
	/* Get all the pitch,roll,yaw info */
	rotation.y = -psFeature->rot.direction;
	rotation.x = psFeature->rot.pitch;
	rotation.z = psFeature->rot.roll;

	glm::mat4 modelMatrix = glm::translate(glm::vec3(dv)) *
		glm::rotate(UNDEG(rotation.y), glm::vec3(0.f, 1.f, 0.f)) *
		glm::rotate(UNDEG(rotation.x), glm::vec3(1.f, 0.f, 0.f)) *
		glm::rotate(UNDEG(rotation.z), glm::vec3(0.f, 0.f, 1.f));

	if (psFeature->psStats->subType == FEAT_SKYSCRAPER)
	{
		modelMatrix *= objectShimmy((BASE_OBJECT *)psFeature);
	}

	if (!getRevealStatus())
	{
		brightness = pal_SetBrightness(avGetObjLightLevel((BASE_OBJECT *)psFeature, brightness.byte.r));
	}
	if (!hasSensorOnTile(mapTile(map_coord(psFeature->pos.x), map_coord(psFeature->pos.y)), selectedPlayer))
	{
		brightness.byte.r /= 2;
		brightness.byte.g /= 2;
		brightness.byte.b /= 2;
	}

	if (psFeature->psStats->subType == FEAT_BUILDING
	    || psFeature->psStats->subType == FEAT_SKYSCRAPER
	    || psFeature->psStats->subType == FEAT_GEN_ARTE
	    || psFeature->psStats->subType == FEAT_BOULDER
	    || psFeature->psStats->subType == FEAT_VEHICLE
	    || psFeature->psStats->subType == FEAT_TREE
	    || psFeature->psStats->subType == FEAT_OIL_DRUM)
	{
		/* these cast a shadow */
		pieFlags = pie_SHADOW;
	}

	iIMDBaseShape *imd = psFeature->sDisplay.imd;
	if (imd)
	{
		iIMDShape *strImd = imd->displayModel();

		float stretchDepth = 0.f;
		if (!(strImd->flags & iV_IMD_NOSTRETCH))
		{
			if (psFeature->psStats->subType == FEAT_TREE) // for now, only do this for trees
			{
				stretchDepth = psFeature->pos.z - psFeature->foundationDepth;
			}
		}

		/* Translate the feature  - N.B. We can also do rotations here should we require
		buildings to face different ways - Don't know if this is necessary - should be IMO */
		pie_Draw3DShape(strImd, 0, 0, brightness, pieFlags, 0, modelMatrix, viewMatrix, stretchDepth);
	}

	setScreenDispWithPerspective(&psFeature->sDisplay, perspectiveViewMatrix * modelMatrix);
}

void renderProximityMsg(PROXIMITY_DISPLAY *psProxDisp, const glm::mat4& viewMatrix, const glm::mat4 &perspectiveViewMatrix)
{
	UDWORD			msgX = 0, msgY = 0;
	Vector3i                dv(0, 0, 0);
	VIEW_PROXIMITY	*pViewProximity = nullptr;
	SDWORD			x, y, r;
	iIMDShape		*proxImd = nullptr;

	//store the frame number for when deciding what has been clicked on
	psProxDisp->frameNumber = currentGameFrame;

	/* Get it's x and y coordinates so we don't have to deref. struct later */
	if (psProxDisp->type == POS_PROXDATA)
	{
		pViewProximity = (VIEW_PROXIMITY *)psProxDisp->psMessage->pViewData->pData;
		if (pViewProximity)
		{
			msgX = pViewProximity->x;
			msgY = pViewProximity->y;
			/* message sits at the height specified at input*/
			dv.y = pViewProximity->z + 64;

			/* in case of a beacon message put above objects */
			if (psProxDisp->psMessage->pViewData->type == VIEW_BEACON)
			{
				if (TileIsOccupied(mapTile(msgX / TILE_UNITS, msgY / TILE_UNITS)))
				{
					dv.y = pViewProximity->z + 150;
				}
			}

		}
	}
	else if (psProxDisp->type == POS_PROXOBJ)
	{
		msgX = psProxDisp->psMessage->psObj->pos.x;
		msgY = psProxDisp->psMessage->psObj->pos.y;
		/* message sits at the height specified at input*/
		dv.y = psProxDisp->psMessage->psObj->pos.z + 64;
	}
	else
	{
		ASSERT(!"unknown proximity display message type", "Buggered proximity message type");
		return;
	}

	dv.x = msgX;
#if defined( _MSC_VER )
	#pragma warning( push )
	#pragma warning( disable : 4146 ) // warning C4146: unary minus operator applied to unsigned type, result still unsigned
#endif
	dv.z = -(static_cast<int>(msgY));
#if defined( _MSC_VER )
	#pragma warning( pop )
#endif

	/* Translate the message */
	glm::mat4 modelMatrix = glm::translate(glm::vec3(dv));

	//get the appropriate IMD
	if (pViewProximity)
	{
		switch (pViewProximity->proxType)
		{
		case PROX_ENEMY:
			proxImd = getDisplayImdFromIndex(MI_BLIP_ENEMY);
			break;
		case PROX_RESOURCE:
			proxImd = getDisplayImdFromIndex(MI_BLIP_RESOURCE);
			break;
		case PROX_ARTEFACT:
			proxImd = getDisplayImdFromIndex(MI_BLIP_ARTEFACT);
			break;
		default:
			ASSERT(!"unknown proximity display message type", "Buggered proximity message type");
			break;
		}
	}
	else
	{
		//object Proximity displays are for oil resources and artefacts
		ASSERT_OR_RETURN(, psProxDisp->psMessage->psObj->type == OBJ_FEATURE, "Invalid object type for proximity display");

		if (castFeature(psProxDisp->psMessage->psObj)->psStats->subType == FEAT_OIL_RESOURCE)
		{
			//resource
			proxImd = getDisplayImdFromIndex(MI_BLIP_RESOURCE);
		}
		else
		{
			//artefact
			proxImd = getDisplayImdFromIndex(MI_BLIP_ARTEFACT);
		}
	}

	modelMatrix *= glm::rotate(UNDEG(-playerPos.r.y), glm::vec3(0.f, 1.f, 0.f)) *
		glm::rotate(UNDEG(-playerPos.r.x), glm::vec3(1.f, 0.f, 0.f));

	if (proxImd)
	{
		pie_Draw3DShape(proxImd, getModularScaledGraphicsTime(proxImd->animInterval, proxImd->numFrames), 0, WZCOL_WHITE, pie_ADDITIVE, 192, modelMatrix, viewMatrix);
	}
	//get the screen coords for determining when clicked on
	calcFlagPosScreenCoords(&x, &y, &r, perspectiveViewMatrix * modelMatrix);
	psProxDisp->screenX = x;
	psProxDisp->screenY = y;
	psProxDisp->screenR = r;
}

static PIELIGHT getBlueprintColour(STRUCT_STATES state)
{
	switch (state)
	{
	case SS_BLUEPRINT_VALID:
		return WZCOL_LGREEN;
	case SS_BLUEPRINT_INVALID:
		return WZCOL_LRED;
	case SS_BLUEPRINT_PLANNED:
		return WZCOL_BLUEPRINT_PLANNED;
	case SS_BLUEPRINT_PLANNED_BY_ALLY:
		return WZCOL_BLUEPRINT_PLANNED_BY_ALLY;
	default:
		debug(LOG_ERROR, "this is not a blueprint");
		return WZCOL_WHITE;
	}
}

static void renderStructureTurrets(STRUCTURE *psStructure, iIMDShape *strImd, PIELIGHT buildingBrightness, int pieFlag, int pieFlagData, int ecmFlag,
	const glm::mat4 &modelMatrix, const glm::mat4 &viewMatrix)
{
	iIMDBaseShape *mountImd[MAX_WEAPONS] = { nullptr };
	iIMDBaseShape *weaponImd[MAX_WEAPONS] = { nullptr };
	iIMDBaseShape *flashImd[MAX_WEAPONS] = { nullptr };

	int colour = getPlayerColour(psStructure->player);

	//get an imd to draw on the connector priority is weapon, ECM, sensor
	//check for weapon
	for (int i = 0; i < MAX(1, psStructure->numWeaps); i++)
	{
		if (psStructure->asWeaps[i].nStat > 0)
		{
			const int nWeaponStat = psStructure->asWeaps[i].nStat;

			weaponImd[i] = asWeaponStats[nWeaponStat].pIMD;
			mountImd[i] = asWeaponStats[nWeaponStat].pMountGraphic;
			flashImd[i] = asWeaponStats[nWeaponStat].pMuzzleGraphic;
		}
	}

	// check for ECM
	if (weaponImd[0] == nullptr && psStructure->pStructureType->pECM != nullptr)
	{
		weaponImd[0] = psStructure->pStructureType->pECM->pIMD;
		mountImd[0] = psStructure->pStructureType->pECM->pMountGraphic;
		flashImd[0] = nullptr;
	}
	// check for sensor (or repair center)
	bool noRecoil = false;
	if (weaponImd[0] == nullptr && psStructure->pStructureType->pSensor != nullptr)
	{
		weaponImd[0] =  psStructure->pStructureType->pSensor->pIMD;
		/* No recoil for sensors */
		noRecoil = true;
		mountImd[0]  =  psStructure->pStructureType->pSensor->pMountGraphic;
		flashImd[0] = nullptr;
	}

	// flags for drawing weapons
	if (psStructure->isBlueprint())
	{
		pieFlag = pie_TRANSLUCENT;
		pieFlagData = BLUEPRINT_OPACITY;
	}
	else
	{
		pieFlag = pie_SHADOW | ecmFlag;
		pieFlagData = 0;
	}

	// draw Weapon / ECM / Sensor for structure
	for (int i = 0; i < psStructure->numWeaps || i == 0; i++)
	{
		Rotation rot = structureGetInterpolatedWeaponRotation(psStructure, i, graphicsTime);

		if (weaponImd[i] != nullptr)
		{
			glm::mat4 matrix = glm::translate(glm::vec3(strImd->connectors[i].xzy())) * glm::rotate(UNDEG(-rot.direction), glm::vec3(0.f, 1.f, 0.f));
			float heightAboveTerrain = strImd->connectors[i].z;
			int recoilValue = noRecoil ? 0 : getRecoil(psStructure->asWeaps[i]);
			if (mountImd[i] != nullptr)
			{
				iIMDShape *pMountDisplayIMD = mountImd[i]->displayModel();
				matrix *= glm::translate(glm::vec3(0.f, 0.f, recoilValue / 3.f));
				int animFrame = 0;
				if (pMountDisplayIMD->numFrames > 0)	// Calculate an animation frame
				{
					animFrame = getModularScaledGraphicsTime(pMountDisplayIMD->animInterval, pMountDisplayIMD->numFrames);
				}
				pie_Draw3DShape(pMountDisplayIMD, animFrame, colour, buildingBrightness, pieFlag, pieFlagData, modelMatrix * matrix, viewMatrix, -heightAboveTerrain);
				if (!pMountDisplayIMD->connectors.empty())
				{
					matrix *= glm::translate(glm::vec3(pMountDisplayIMD->connectors[0].xzy()));
				}
			}
			matrix *= glm::rotate(UNDEG(rot.pitch), glm::vec3(1.f, 0.f, 0.f));
			matrix *= glm::translate(glm::vec3(0, 0, recoilValue));

			iIMDShape *pWeaponDisplayIMD = weaponImd[i]->displayModel();
			pie_Draw3DShape(pWeaponDisplayIMD, 0, colour, buildingBrightness, pieFlag, pieFlagData, modelMatrix * matrix, viewMatrix, -heightAboveTerrain);
			if (psStructure->status == SS_BUILT && psStructure->visibleForLocalDisplay() > (UBYTE_MAX / 2))
			{
				if (psStructure->pStructureType->type == REF_REPAIR_FACILITY)
				{
					REPAIR_FACILITY *psRepairFac = &psStructure->pFunctionality->repairFacility;
					// draw repair flash if the Repair Facility has a target which it has started work on
					if (!pWeaponDisplayIMD->connectors.empty() && psRepairFac->psObj != nullptr
					    && psRepairFac->psObj->type == OBJ_DROID)
					{
						DROID *psDroid = (DROID *)psRepairFac->psObj;
						SDWORD xdiff, ydiff;
						xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psStructure->pos.x;
						ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psStructure->pos.y;
						if (xdiff * xdiff + ydiff * ydiff <= (TILE_UNITS * 5 / 2) * (TILE_UNITS * 5 / 2))
						{
							iIMDShape	*pRepImd = getDisplayImdFromIndex(MI_FLAME);

							matrix *= glm::translate(glm::vec3(pWeaponDisplayIMD->connectors[0].x, pWeaponDisplayIMD->connectors[0].z - 12, pWeaponDisplayIMD->connectors[0].y)) *
								glm::rotate(UNDEG(rot.direction), glm::vec3(0.f, 1.f, 0.f)) *
								glm::rotate(UNDEG(-playerPos.r.y), glm::vec3(0.f, 1.f, 0.f)) *
								glm::rotate(UNDEG(-playerPos.r.x), glm::vec3(1.f, 0.f, 0.f));
							pie_Draw3DShape(pRepImd, getModularScaledGraphicsTime(pRepImd->animInterval, pRepImd->numFrames), colour, buildingBrightness, pie_ADDITIVE, 192, modelMatrix * matrix, viewMatrix, -heightAboveTerrain);
						}
					}
				}
				else // we have a weapon so we draw a muzzle flash
				{
					iIMDShape *pFlashDisplayIMD = (flashImd[i]) ? flashImd[i]->displayModel() : nullptr;
					drawMuzzleFlash(psStructure->asWeaps[i], pWeaponDisplayIMD, pFlashDisplayIMD, buildingBrightness, pieFlag, pieFlagData, modelMatrix * matrix, viewMatrix, heightAboveTerrain, colour);
				}
			}
		}
		// no IMD, its a baba machine gun, bunker, etc.
		else if (psStructure->asWeaps[i].nStat > 0)
		{
			if (psStructure->status == SS_BUILT)
			{
				const int nWeaponStat = psStructure->asWeaps[i].nStat;

				// get an imd to draw on the connector priority is weapon, ECM, sensor
				// check for weapon
				flashImd[i] =  asWeaponStats[nWeaponStat].pMuzzleGraphic;
				// draw Weapon/ECM/Sensor for structure
				if (flashImd[i] != nullptr)
				{
					iIMDShape *pFlashDisplayIMD = flashImd[i]->displayModel();
					glm::mat4 matrix(1.f);
					// horrendous hack
					if (strImd->max.y > 80) // babatower
					{
						matrix *= glm::translate(glm::vec3(0.f, 80.f, 0.f)) * glm::rotate(UNDEG(-rot.direction), glm::vec3(0.f, 1.f, 0.f)) * glm::translate(glm::vec3(0.f, 0.f, -20.f));
					}
					else//baba bunker
					{
						matrix *= glm::translate(glm::vec3(0.f, 10.f, 0.f)) * glm::rotate(UNDEG(-rot.direction), glm::vec3(0.f, 1.f, 0.f)) * glm::translate(glm::vec3(0.f, 0.f, -40.f));
					}
					matrix *= glm::rotate(UNDEG(rot.pitch), glm::vec3(1.f, 0.f, 0.f));
					// draw the muzzle flash?
					if (psStructure->visibleForLocalDisplay() > UBYTE_MAX / 2)
					{
						// animate for the duration of the flash only
						// assume no clan colours for muzzle effects
						if (pFlashDisplayIMD->numFrames == 0 || pFlashDisplayIMD->animInterval <= 0)
						{
							// no anim so display one frame for a fixed time
							if (graphicsTime >= psStructure->asWeaps[i].lastFired && graphicsTime < psStructure->asWeaps[i].lastFired + BASE_MUZZLE_FLASH_DURATION)
							{
								pie_Draw3DShape(pFlashDisplayIMD, 0, colour, buildingBrightness, 0, 0, modelMatrix * matrix, viewMatrix); //muzzle flash
							}
						}
						else
						{
							const int frame = (graphicsTime - psStructure->asWeaps[i].lastFired) / pFlashDisplayIMD->animInterval;
							if (frame < pFlashDisplayIMD->numFrames && frame >= 0)
							{
								pie_Draw3DShape(pFlashDisplayIMD, 0, colour, buildingBrightness, 0, 0, modelMatrix * matrix, viewMatrix); //muzzle flash
							}
						}
					}
				}
			}
		}
		// if there is an unused connector, but not the first connector, add a light to it
		else if (psStructure->sDisplay.imd->displayModel()->connectors.size() > 1)
		{
			switch (psStructure->pStructureType->type)
			{
			case REF_FACTORY:
			case REF_FACTORY_MODULE:
			case REF_CYBORG_FACTORY:
			case REF_VTOL_FACTORY:
				// don't do this
				break;
			default:
				for (size_t cIdx = 1; cIdx < psStructure->sDisplay.imd->displayModel()->connectors.size(); cIdx++)
				{
					iIMDShape *lImd = getDisplayImdFromIndex(MI_LANDING);
					pie_Draw3DShape(lImd, getModularScaledGraphicsTime(lImd->animInterval, lImd->numFrames), colour, buildingBrightness, 0, 0,
									modelMatrix * glm::translate(glm::vec3(psStructure->sDisplay.imd->displayModel()->connectors[cIdx].xzy())), viewMatrix);
				}
				break;
			}
		}
	}
}

/// Draw the structures
void renderStructure(STRUCTURE *psStructure, const glm::mat4 &viewMatrix, const glm::mat4 &perspectiveViewMatrix)
{
	int colour, pieFlagData, ecmFlag = 0, pieFlag = 0;
	PIELIGHT buildingBrightness;
	const Vector3i dv = Vector3i(psStructure->pos.x, psStructure->pos.z, -(psStructure->pos.y));
	bool bHitByElectronic = false;
	bool defensive = false;
	const FACTION *faction = getPlayerFaction(psStructure->player);
	iIMDShape *strImd = getFactionDisplayIMD(faction, psStructure->sDisplay.imd->displayModel());
	MAPTILE *psTile = worldTile(psStructure->pos.x, psStructure->pos.y);

	glm::mat4 modelMatrix = glm::translate(glm::vec3(dv)) * glm::rotate(UNDEG(-psStructure->rot.direction), glm::vec3(0.f, 1.f, 0.f));

	if (psStructure->pStructureType->type == REF_WALL || psStructure->pStructureType->type == REF_WALLCORNER
	    || psStructure->pStructureType->type == REF_GATE)
	{
		renderWallSection(psStructure, viewMatrix, perspectiveViewMatrix);
		return;
	}
	// If the structure is not truly visible, but we know there is something there, we will instead draw a blip
	UBYTE visibilityAmount = psStructure->visibleForLocalDisplay();
	if (visibilityAmount < UBYTE_MAX && visibilityAmount > 0)
	{
		int frame = graphicsTime / BLIP_ANIM_DURATION + psStructure->id % 8192;  // de-sync the blip effect, but don't overflow the int
		pie_Draw3DShape(getFactionDisplayIMD(faction, getDisplayImdFromIndex(MI_BLIP)), frame, 0, WZCOL_WHITE, pie_ADDITIVE, visibilityAmount / 2,
			glm::translate(glm::vec3(dv)), viewMatrix);
		return;
	}
	else if (!visibilityAmount)
	{
		return;
	}
	else if (psStructure->pStructureType->type == REF_DEFENSE)
	{
		defensive = true;
	}

	if (psTile->jammerBits & alliancebits[psStructure->player])
	{
		ecmFlag = pie_ECM;
	}

	colour = getPlayerColour(psStructure->player);

	// -------------------------------------------------------------------------------

	/* Mark it as having been drawn */
	psStructure->sDisplay.frameNumber = currentGameFrame;

	if (!defensive
	    && psStructure->timeLastHit - graphicsTime < ELEC_DAMAGE_DURATION
	    && psStructure->lastHitWeapon == WSC_ELECTRONIC)
	{
		bHitByElectronic = true;
	}

	buildingBrightness = structureBrightness(psStructure);

	if (!defensive)
	{
		/* Draw the building's base first */
		if (psStructure->pStructureType->pBaseIMD != nullptr)
		{
			if (psStructure->isBlueprint())
			{
				pieFlagData = BLUEPRINT_OPACITY;
			}
			else
			{
				pieFlag = pie_FORCE_FOG | ecmFlag;
				pieFlagData = 255;
			}
			pie_Draw3DShape(getFactionDisplayIMD(faction, psStructure->pStructureType->pBaseIMD->displayModel()), 0, colour, buildingBrightness, pieFlag | pie_TRANSLUCENT | pie_FORCELIGHT, pieFlagData,
				modelMatrix, viewMatrix);
		}

		// override
		if (bHitByElectronic)
		{
			buildingBrightness = pal_SetBrightness(150);
		}
	}

	if (bHitByElectronic)
	{
		modelMatrix *= objectShimmy((BASE_OBJECT *)psStructure);
	}

	//first check if partially built - ANOTHER HACK!
	if (psStructure->status == SS_BEING_BUILT)
	{
		if (psStructure->prebuiltImd != nullptr)
		{
			// strImd is a module, so render the already-built part at full height.
			iIMDBaseShape *imd = psStructure->prebuiltImd;
			if (imd)
			{
				pie_Draw3DShape(getFactionDisplayIMD(faction, imd->displayModel()), 0, colour, buildingBrightness, pie_SHADOW, 0,
					modelMatrix, viewMatrix);
			}
		}
		if (strImd)
		{
			pie_Draw3DShape(strImd, 0, colour, buildingBrightness, pie_HEIGHT_SCALED | pie_SHADOW, static_cast<int>(structHeightScale(psStructure) * pie_RAISE_SCALE), modelMatrix, viewMatrix);
		}
		setScreenDispWithPerspective(&psStructure->sDisplay, perspectiveViewMatrix * modelMatrix);
		return;
	}

	if (psStructure->isBlueprint())
	{
		pieFlag = pie_TRANSLUCENT;
		pieFlagData = BLUEPRINT_OPACITY;
	}
	else
	{
		// structures can be rotated, so use a dynamic shadow for them
		pieFlag = pie_SHADOW | ecmFlag;
		pieFlagData = 0;
	}

	// check for animation model replacement - if none found, use animation in existing IMD
	if (strImd->objanimpie[psStructure->animationEvent])
	{
		strImd = strImd->objanimpie[psStructure->animationEvent]->displayModel();
	}

	while (strImd)
	{
		float stretch = 0.f;
		if (defensive && !psStructure->isBlueprint() && !(strImd->flags & iV_IMD_NOSTRETCH))
		{
			stretch = psStructure->pos.z - psStructure->foundationDepth;
		}
		drawShape(strImd, psStructure->timeAnimationStarted, colour, buildingBrightness, pieFlag, pieFlagData, modelMatrix, viewMatrix, stretch);
		if (strImd->connectors.size() > 0)
		{
			renderStructureTurrets(psStructure, strImd, buildingBrightness, pieFlag, pieFlagData, ecmFlag, modelMatrix, viewMatrix);
		}
		strImd = strImd->next.get();
	}
	setScreenDispWithPerspective(&psStructure->sDisplay, perspectiveViewMatrix * modelMatrix);
}

/// draw the delivery points
void renderDeliveryPoint(FLAG_POSITION *psPosition, bool blueprint, const glm::mat4& viewMatrix, const glm::mat4 &perspectiveViewMatrix)
{
	Vector3i	dv;
	SDWORD		x, y, r;
	int pieFlag, pieFlagData;
	PIELIGHT colour;

	//store the frame number for when deciding what has been clicked on
	psPosition->frameNumber = currentGameFrame;

	dv.x = psPosition->coords.x;
	dv.z = -(psPosition->coords.y);
	dv.y = psPosition->coords.z;

	//quick check for invalid data
	ASSERT_OR_RETURN(, psPosition && psPosition->factoryType < NUM_FLAG_TYPES && psPosition->factoryInc < MAX_FACTORY_FLAG_IMDS, "Invalid assembly point");

	const glm::mat4 modelMatrix = glm::translate(glm::vec3(dv)) * glm::scale(glm::vec3(.5f)) * glm::rotate(-UNDEG(playerPos.r.y), glm::vec3(0,1,0));

	pieFlag = pie_TRANSLUCENT;
	pieFlagData = BLUEPRINT_OPACITY;

	if (blueprint)
	{
		colour = deliveryReposValid() ? WZCOL_LGREEN : WZCOL_LRED;
	}
	else
	{
		pieFlag |= pie_FORCE_FOG;
		colour = WZCOL_WHITE;
		STRUCTURE *structure = findDeliveryFactory(psPosition);
		if (structure != nullptr && structure->selected)
		{
			colour = selectionBrightness();
		}
	}
	iIMDBaseShape *psIMD = pAssemblyPointIMDs[psPosition->factoryType][psPosition->factoryInc];
	if (psIMD != nullptr)
	{
		pie_Draw3DShape(psIMD->displayModel(), 0, 0, colour, pieFlag, pieFlagData, modelMatrix, viewMatrix);
	}

	//get the screen coords for the DP
	calcFlagPosScreenCoords(&x, &y, &r, perspectiveViewMatrix * modelMatrix);
	psPosition->screenX = x;
	psPosition->screenY = y;
	psPosition->screenR = r;
}

static bool renderWallSection(STRUCTURE *psStructure, const glm::mat4 &viewMatrix, const glm::mat4 &perspectiveViewMatrix)
{
	int ecmFlag = 0;
	PIELIGHT		brightness;
	Vector3i			dv;
	int				pieFlag, pieFlagData;
	MAPTILE			*psTile = worldTile(psStructure->pos.x, psStructure->pos.y);
	const FACTION *faction = getPlayerFaction(psStructure->player);

	if (!psStructure->visibleForLocalDisplay())
	{
		return false;
	}

	if (psTile->jammerBits & alliancebits[psStructure->player])
	{
		ecmFlag = pie_ECM;
	}

	psStructure->sDisplay.frameNumber = currentGameFrame;

	brightness = structureBrightness(psStructure);
	float stretch = psStructure->pos.z - psStructure->foundationDepth;

	/* Establish where it is in the world */
	dv.x = psStructure->pos.x;
	dv.z = -(psStructure->pos.y);
	dv.y = psStructure->pos.z;

	dv.y -= gateCurrentOpenHeight(psStructure, graphicsTime, 1);  // Make gate stick out by 1 unit, so that the tops of ┼ gates can safely have heights differing by 1 unit.

	const glm::mat4 modelMatrix = glm::translate(glm::vec3(dv)) * glm::rotate(UNDEG(-psStructure->rot.direction), glm::vec3(0.f, 1.f, 0.f));

	/* Actually render it */
	if (psStructure->status == SS_BEING_BUILT)
	{
		if (psStructure->sDisplay.imd)
		{
			pie_Draw3DShape(getFactionDisplayIMD(faction, psStructure->sDisplay.imd->displayModel()), 0, getPlayerColour(psStructure->player),
							brightness, pie_HEIGHT_SCALED | pie_SHADOW | ecmFlag, static_cast<int>(structHeightScale(psStructure) * pie_RAISE_SCALE), modelMatrix, viewMatrix, stretch);
		}
	}
	else
	{
		if (psStructure->isBlueprint())
		{
			pieFlag = pie_TRANSLUCENT;
			pieFlagData = BLUEPRINT_OPACITY;
		}
		else
		{
			// Use a dynamic shadow
			pieFlag = pie_SHADOW;
			pieFlagData = 0;
		}
		iIMDBaseShape *imd = psStructure->sDisplay.imd;
		if (imd)
		{
			pie_Draw3DShape(getFactionDisplayIMD(faction, imd->displayModel()), 0, getPlayerColour(psStructure->player), brightness, pieFlag | ecmFlag, pieFlagData, modelMatrix, viewMatrix, stretch);
		}
	}
	setScreenDispWithPerspective(&psStructure->sDisplay, perspectiveViewMatrix * modelMatrix);
	return true;
}

/// SHURCOOL: Draws the strobing 3D drag box that is used for multiple selection
static void	drawDragBox()
{
	if (dragBox3D.status != DRAG_DRAGGING || buildState != BUILD3D_NONE)
	{
		return;
	}

	int X1 = MIN(dragBox3D.x1, mouseX());
	int X2 = MAX(dragBox3D.x1, mouseX());
	int Y1 = MIN(dragBox3D.y1, mouseY());
	int Y2 = MAX(dragBox3D.y1, mouseY());

	// draw static box

	iV_Box(X1, Y1, X2, Y2, WZCOL_UNIT_SELECT_BORDER);
	pie_UniTransBoxFill(X1, Y1, X2, Y2, WZCOL_UNIT_SELECT_BOX);

	// draw pulse effect

	dragBox3D.pulse += (BOX_PULSE_SIZE - dragBox3D.pulse) * realTimeAdjustedIncrement(5);

	if(dragBox3D.pulse > BOX_PULSE_SIZE - 0.1f) {
		dragBox3D.pulse = 0;
	}

	PIELIGHT color = WZCOL_UNIT_SELECT_BOX;

	color.byte.a = static_cast<uint8_t>((float)color.byte.a * (1 - (dragBox3D.pulse / BOX_PULSE_SIZE))); // alpha relative to max pulse size

	pie_UniTransBoxFill(X2, Y1, X2 + dragBox3D.pulse, Y2 + dragBox3D.pulse, color); // east side + south-east corner
	pie_UniTransBoxFill(X1 - dragBox3D.pulse, Y2, X2, Y2 + dragBox3D.pulse, color); // south side + south-west corner
	pie_UniTransBoxFill(X1 - dragBox3D.pulse, Y1 - dragBox3D.pulse, X1, Y2, color); // west side + north-west corner
	pie_UniTransBoxFill(X1, Y1 - dragBox3D.pulse, X2 + dragBox3D.pulse, Y1, color); // north side + north-east corner
}


/// Display reload bars for structures and droids
static void queueWeaponReloadBarRects(BASE_OBJECT *psObj, WEAPON *psWeap, int weapon_slot, BatchedMultiRectRenderer& batchedMultiRectRenderer, size_t rectGroup)
{
	SDWORD			scrX, scrY, scrR, scale;
	STRUCTURE		*psStruct;
	float			mulH;	// display unit resistance instead of reload!
	DROID			*psDroid;
	int			armed, firingStage;

	if (ctrlShiftDown() && (psObj->type == OBJ_DROID))
	{
		psDroid = (DROID *)psObj;
		scrX = psObj->sDisplay.screenX;
		scrY = psObj->sDisplay.screenY;
		scrR = psObj->sDisplay.screenR;
		scrY += scrR + 2;

		if (weapon_slot != 0) // only rendering resistance in the first slot
		{
			return;
		}
		if (psDroid->resistance)
		{
			mulH = (float)PERCENT(psDroid->resistance, psDroid->droidResistance());
		}
		else
		{
			mulH = 100.f;
		}
		firingStage = static_cast<int>(mulH);
		firingStage = ((((2 * scrR) * 10000) / 100) * firingStage) / 10000;
		if (firingStage >= (UDWORD)(2 * scrR))
		{
			firingStage = (2 * scrR);
		}
		batchedMultiRectRenderer.addRect(PIERECT_DrawRequest(scrX - scrR - 1, 3 + scrY + 0 + (weapon_slot * 2), scrX - scrR + (2 * scrR) + 1,    3 + scrY + 3 + (weapon_slot * 2), WZCOL_RELOAD_BACKGROUND));
		batchedMultiRectRenderer.addRect(PIERECT_DrawRequest(scrX - scrR,   3 + scrY + 1 + (weapon_slot * 2), scrX - scrR + firingStage, 3 + scrY + 2 + (weapon_slot * 2), WZCOL_HEALTH_RESISTANCE));
		return;
	}

	armed = droidReloadBar(psObj, psWeap, weapon_slot);
	if (armed >= 0 && armed < 100) // no need to draw if full
	{
		scrX = psObj->sDisplay.screenX;
		scrY = psObj->sDisplay.screenY;
		scrR = psObj->sDisplay.screenR;
		switch (psObj->type)
		{
		case OBJ_DROID:
			scrY += scrR + 2;
			break;
		case OBJ_STRUCTURE:
			psStruct = (STRUCTURE *)psObj;
			scale = MAX(psStruct->pStructureType->baseWidth, psStruct->pStructureType->baseBreadth);
			scrY += scale * 10;
			scrR = scale * 20;
			break;
		default:
			break;
		}
		/* Scale it into an appropriate range */
		firingStage = ((((2 * scrR) * 10000) / 100) * armed) / 10000;
		if (firingStage >= 2 * scrR)
		{
			firingStage = (2 * scrR);
		}
		/* Power bars */
		batchedMultiRectRenderer.addRect(PIERECT_DrawRequest(scrX - scrR - 1, 3 + scrY + 0 + (weapon_slot * 2), scrX - scrR + (2 * scrR) + 1,  3 + scrY + 3 + (weapon_slot * 2), WZCOL_RELOAD_BACKGROUND));
		batchedMultiRectRenderer.addRect(PIERECT_DrawRequest(scrX - scrR,   3 + scrY + 1 + (weapon_slot * 2), scrX - scrR + firingStage, 3 + scrY + 2 + (weapon_slot * 2), WZCOL_RELOAD_BAR));
	}
}

/// draw target origin icon for the specified structure
static void drawStructureTargetOriginIcon(STRUCTURE *psStruct, int weapon_slot)
{
	SDWORD		scrX, scrY, scrR;
	UDWORD		scale;

	// Process main weapon only for now
	if (!tuiTargetOrigin || weapon_slot || !((psStruct->asWeaps[weapon_slot]).nStat))
	{
		return;
	}

	scale = MAX(psStruct->pStructureType->baseWidth, psStruct->pStructureType->baseBreadth);
	scrX = psStruct->sDisplay.screenX;
	scrY = psStruct->sDisplay.screenY + (scale * 10);
	scrR = scale * 20;

	/* Render target origin graphics */
	switch (psStruct->asWeaps[weapon_slot].origin)
	{
	case ORIGIN_VISUAL:
		iV_DrawImage(IntImages, IMAGE_ORIGIN_VISUAL, scrX + scrR + 5, scrY - 1);
		break;
	case ORIGIN_COMMANDER:
		iV_DrawImage(IntImages, IMAGE_ORIGIN_COMMANDER, scrX + scrR + 5, scrY - 1);
		break;
	case ORIGIN_SENSOR:
		iV_DrawImage(IntImages, IMAGE_ORIGIN_SENSOR_STANDARD, scrX + scrR + 5, scrY - 1);
		break;
	case ORIGIN_CB_SENSOR:
		iV_DrawImage(IntImages, IMAGE_ORIGIN_SENSOR_CB, scrX + scrR + 5, scrY - 1);
		break;
	case ORIGIN_AIRDEF_SENSOR:
		iV_DrawImage(IntImages, IMAGE_ORIGIN_SENSOR_AIRDEF, scrX + scrR + 5, scrY - 1);
		break;
	case ORIGIN_RADAR_DETECTOR:
		iV_DrawImage(IntImages, IMAGE_ORIGIN_RADAR_DETECTOR, scrX + scrR + 5, scrY - 1);
		break;
	case ORIGIN_UNKNOWN:
		// Do nothing
		break;
	default:
		debug(LOG_WARNING, "Unexpected target origin in structure(%d)!", psStruct->id);
	}
}

/// draw the health bar for the specified structure
static void queueStructureHealth(STRUCTURE *psStruct, BatchedMultiRectRenderer& batchedMultiRectRenderer, size_t rectGroup)
{
	int32_t		scrX, scrY, scrR;
	PIELIGHT	powerCol = WZCOL_BLACK, powerColShadow = WZCOL_BLACK;
	int32_t		health, width;

	int32_t scale = static_cast<int32_t>(MAX(psStruct->pStructureType->baseWidth, psStruct->pStructureType->baseBreadth));
	width = scale * 20;
	scrX = psStruct->sDisplay.screenX;
	scrY = static_cast<int32_t>(psStruct->sDisplay.screenY) + (scale * 10);
	scrR = width;
	//health = PERCENT(psStruct->body, psStruct->baseBodyPoints);
	if (ctrlShiftDown())
	{
		//show resistance values if CTRL/SHIFT depressed
		UDWORD  resistance = structureResistance(
		                         psStruct->pStructureType, psStruct->player);
		if (resistance)
		{
			health = static_cast<int32_t>(PERCENT(MAX(0, psStruct->resistance), resistance));
		}
		else
		{
			health = 100;
		}
	}
	else
	{
		//show body points
		health = static_cast<int32_t>((1. - getStructureDamage(psStruct) / 65536.f) * 100);

		// If structure is incomplete, make bar correspondingly thinner.
		int maxBody = psStruct->structureBody();
		int maxBodyBuilt = structureBodyBuilt(psStruct);
		width = (uint64_t)width * maxBodyBuilt / maxBody;
	}
	if (health > REPAIRLEV_HIGH)
	{
		powerCol = WZCOL_HEALTH_HIGH;
		powerColShadow = WZCOL_HEALTH_HIGH_SHADOW;
	}
	else if (health > REPAIRLEV_LOW)
	{
		powerCol = WZCOL_HEALTH_MEDIUM;
		powerColShadow = WZCOL_HEALTH_MEDIUM_SHADOW;
	}
	else
	{
		powerCol = WZCOL_HEALTH_LOW;
		powerColShadow = WZCOL_HEALTH_LOW_SHADOW;
	}
	health = (((width * 10000) / 100) * health) / 10000;
	health *= 2;
	batchedMultiRectRenderer.addRectF(PIERECT_DrawRequest_f(scrX - scrR - 1, scrY - 1, scrX - scrR + 2 * width + 1, scrY + 3, WZCOL_RELOAD_BACKGROUND), rectGroup);
	batchedMultiRectRenderer.addRectF(PIERECT_DrawRequest_f(scrX - scrR, scrY, scrX - scrR + health, scrY + 1, powerCol), rectGroup);
	batchedMultiRectRenderer.addRectF(PIERECT_DrawRequest_f(scrX - scrR, scrY + 1, scrX - scrR + health, scrY + 2, powerColShadow), rectGroup);
}

/// draw the construction bar for the specified structure
static void queueStructureBuildProgress(STRUCTURE *psStruct, BatchedMultiRectRenderer& batchedMultiRectRenderer, size_t rectGroup)
{
	int32_t scale = static_cast<int32_t>(MAX(psStruct->pStructureType->baseWidth, psStruct->pStructureType->baseBreadth));
	int32_t scrX = static_cast<int32_t>(psStruct->sDisplay.screenX);
	int32_t scrY = static_cast<int32_t>(psStruct->sDisplay.screenY) + (scale * 10);
	int32_t scrR = scale * 20;
	auto progress = scale * 40 * structureCompletionProgress(*psStruct);
	batchedMultiRectRenderer.addRectF(PIERECT_DrawRequest_f(scrX - scrR - 1, scrY - 1 + 5, scrX + scrR + 1, scrY + 3 + 5, WZCOL_RELOAD_BACKGROUND), rectGroup);
	batchedMultiRectRenderer.addRectF(PIERECT_DrawRequest_f(scrX - scrR, scrY + 5, scrX - scrR + progress, scrY + 1 + 5, WZCOL_HEALTH_MEDIUM_SHADOW), rectGroup);
	batchedMultiRectRenderer.addRectF(PIERECT_DrawRequest_f(scrX - scrR, scrY + 1 + 5, scrX - scrR + progress, scrY + 2 + 5, WZCOL_HEALTH_MEDIUM), rectGroup);
}

/// Draw the health of structures and show enemy structures being targetted
static void	drawStructureSelections()
{
	STRUCTURE	*psStruct;
	BASE_OBJECT	*psClickedOn;
	bool		bMouseOverStructure = false;
	bool		bMouseOverOwnStructure = false;

	psClickedOn = mouseTarget();
	if (psClickedOn != nullptr && psClickedOn->type == OBJ_STRUCTURE)
	{
		bMouseOverStructure = true;
		if (psClickedOn->player == selectedPlayer)
		{
			bMouseOverOwnStructure = true;
		}
	}
	pie_SetFogStatus(false);
	const bool isSpectator = bMultiPlayer && NetPlay.players[selectedPlayer].isSpectator;
	if (isSpectator)
	{
		if (barMode == BAR_DROIDS_AND_STRUCTURES)
		{
			for (int player = 0; player < MAX_PLAYERS; player++)
			{
				batchedObjectStatusRenderer.addStructureSelectionsToRender(player, psClickedOn, bMouseOverOwnStructure);
			}
		}
		return;
	}
	if (selectedPlayer >= MAX_PLAYERS) { return; /* no-op */ }

	batchedObjectStatusRenderer.addStructureSelectionsToRender(selectedPlayer, psClickedOn, bMouseOverOwnStructure);

	batchedObjectStatusRenderer.addStructureTargetingGfxToRender();

	if (bMouseOverStructure && !bMouseOverOwnStructure)
	{
		if (mouseDown(getRightClickOrders() ? MOUSE_LMB : MOUSE_RMB))
		{
			psStruct = (STRUCTURE *)psClickedOn;
			batchedObjectStatusRenderer.addEnemyStructureHealthToRender(psStruct);
		}
	}

}

static UDWORD	getTargettingGfx()
{
	const unsigned index = getModularScaledRealTime(1000, 10);
	switch (index)
	{
	case	0:
	case	1:
	case	2:
		return (IMAGE_TARGET1 + index);
		break;
	default:
		if (index & 0x01)
		{
			return (IMAGE_TARGET4);
		}
		else
		{
			return (IMAGE_TARGET5);
		}
		break;
	}
}

/// Is the droid, its commander or its sensor tower selected?
bool	eitherSelected(DROID *psDroid)
{
	if (psDroid->selected)
	{
		return true;
	}

	if (psDroid->psGroup)
	{
		if (psDroid->psGroup->psCommander)
		{
			if (psDroid->psGroup->psCommander->selected)
			{
				return true;
			}
		}
	}

	BASE_OBJECT *psObj = orderStateObj(psDroid, DORDER_FIRESUPPORT);
	if (psObj && psObj->selected)
	{
		return true;
	}

	return false;
}

static void queueDroidPowerBarsRects(DROID *psDroid, bool drawBox, BatchedMultiRectRenderer& batchedMultiRectRenderer, size_t rectGroup)
{
	UDWORD damage = PERCENT(psDroid->body, psDroid->originalBody);

	PIELIGHT powerCol = WZCOL_BLACK;
	PIELIGHT powerColShadow = WZCOL_BLACK;

	if (damage > REPAIRLEV_HIGH)
	{
		powerCol = WZCOL_HEALTH_HIGH;
		powerColShadow = WZCOL_HEALTH_HIGH_SHADOW;
	}
	else if (damage > REPAIRLEV_LOW)
	{
		powerCol = WZCOL_HEALTH_MEDIUM;
		powerColShadow = WZCOL_HEALTH_MEDIUM_SHADOW;
	}
	else
	{
		powerCol = WZCOL_HEALTH_LOW;
		powerColShadow = WZCOL_HEALTH_LOW_SHADOW;
	}

	damage = static_cast<UDWORD>((float)psDroid->body / (float)psDroid->originalBody * (float)psDroid->sDisplay.screenR);
	if (damage > psDroid->sDisplay.screenR)
	{
		damage = psDroid->sDisplay.screenR;
	}

	damage *= 2;

	if (drawBox)
	{
		batchedMultiRectRenderer.addRect(PIERECT_DrawRequest(psDroid->sDisplay.screenX - psDroid->sDisplay.screenR, psDroid->sDisplay.screenY + psDroid->sDisplay.screenR - 7, psDroid->sDisplay.screenX - psDroid->sDisplay.screenR + 1, psDroid->sDisplay.screenY + psDroid->sDisplay.screenR, WZCOL_WHITE), rectGroup);
		batchedMultiRectRenderer.addRect(PIERECT_DrawRequest(psDroid->sDisplay.screenX - psDroid->sDisplay.screenR, psDroid->sDisplay.screenY + psDroid->sDisplay.screenR, psDroid->sDisplay.screenX - psDroid->sDisplay.screenR + 7, psDroid->sDisplay.screenY + psDroid->sDisplay.screenR + 1, WZCOL_WHITE), rectGroup);
		batchedMultiRectRenderer.addRect(PIERECT_DrawRequest(psDroid->sDisplay.screenX + psDroid->sDisplay.screenR - 7, psDroid->sDisplay.screenY + psDroid->sDisplay.screenR, psDroid->sDisplay.screenX + psDroid->sDisplay.screenR, psDroid->sDisplay.screenY + psDroid->sDisplay.screenR + 1, WZCOL_WHITE), rectGroup);
		batchedMultiRectRenderer.addRect(PIERECT_DrawRequest(psDroid->sDisplay.screenX + psDroid->sDisplay.screenR, psDroid->sDisplay.screenY + psDroid->sDisplay.screenR - 7, psDroid->sDisplay.screenX + psDroid->sDisplay.screenR + 1, psDroid->sDisplay.screenY + psDroid->sDisplay.screenR + 1, WZCOL_WHITE), rectGroup);
	}

	/* Power bars */
	batchedMultiRectRenderer.addRect(PIERECT_DrawRequest(psDroid->sDisplay.screenX - psDroid->sDisplay.screenR - 1, psDroid->sDisplay.screenY + psDroid->sDisplay.screenR + 2, psDroid->sDisplay.screenX + psDroid->sDisplay.screenR + 1, psDroid->sDisplay.screenY + psDroid->sDisplay.screenR + 6, WZCOL_RELOAD_BACKGROUND), rectGroup);
	batchedMultiRectRenderer.addRect(PIERECT_DrawRequest(psDroid->sDisplay.screenX - psDroid->sDisplay.screenR, psDroid->sDisplay.screenY + psDroid->sDisplay.screenR + 3, psDroid->sDisplay.screenX - psDroid->sDisplay.screenR + damage, psDroid->sDisplay.screenY + psDroid->sDisplay.screenR + 4, powerCol), rectGroup);
	batchedMultiRectRenderer.addRect(PIERECT_DrawRequest(psDroid->sDisplay.screenX - psDroid->sDisplay.screenR, psDroid->sDisplay.screenY + psDroid->sDisplay.screenR + 4, psDroid->sDisplay.screenX - psDroid->sDisplay.screenR + damage, psDroid->sDisplay.screenY + psDroid->sDisplay.screenR + 5, powerColShadow), rectGroup);
}

static void queueDroidEnemyHealthBarsRects(DROID *psDroid, BatchedMultiRectRenderer& batchedMultiRectRenderer, size_t rectGroup)
{
	PIELIGHT powerCol = WZCOL_BLACK;
	PIELIGHT powerColShadow = WZCOL_BLACK;
	PIELIGHT boxCol;
	UDWORD damage;
	float mulH;

	//show resistance values if CTRL/SHIFT depressed
	if (ctrlShiftDown())
	{
		if (psDroid->resistance)
		{
			damage = PERCENT(psDroid->resistance, psDroid->droidResistance());
		}
		else
		{
			damage = 100;
		}
	}
	else
	{
		damage = PERCENT(psDroid->body, psDroid->originalBody);
	}

	if (damage > REPAIRLEV_HIGH)
	{
		powerCol = WZCOL_HEALTH_HIGH;
		powerColShadow = WZCOL_HEALTH_HIGH_SHADOW;
	}
	else if (damage > REPAIRLEV_LOW)
	{
		powerCol = WZCOL_HEALTH_MEDIUM;
		powerColShadow = WZCOL_HEALTH_MEDIUM_SHADOW;
	}
	else
	{
		powerCol = WZCOL_HEALTH_LOW;
		powerColShadow = WZCOL_HEALTH_LOW_SHADOW;
	}

	//show resistance values if CTRL/SHIFT depressed
	if (ctrlShiftDown())
	{
		if (psDroid->resistance)
		{
			mulH = (float)psDroid->resistance / (float)psDroid->droidResistance();
		}
		else
		{
			mulH = 100.f;
		}
	}
	else
	{
		mulH = (float)psDroid->body / (float)psDroid->originalBody;
	}
	damage = static_cast<UDWORD>(mulH * (float)psDroid->sDisplay.screenR);// (((psDroid->sDisplay.screenR*10000)/100)*damage)/10000;
	if (damage > psDroid->sDisplay.screenR)
	{
		damage = psDroid->sDisplay.screenR;
	}
	damage *= 2;
	auto scrX = psDroid->sDisplay.screenX;
	auto scrY = psDroid->sDisplay.screenY;
	auto scrR = psDroid->sDisplay.screenR;

	/* Three DFX clips properly right now - not sure if software does */
	if ((scrX + scrR) > 0
		&&	(scrX - scrR) < pie_GetVideoBufferWidth()
		&&	(scrY + scrR) > 0
		&&	(scrY - scrR) < pie_GetVideoBufferHeight())
	{
		boxCol = WZCOL_WHITE;

		/* Power bars */
		batchedMultiRectRenderer.addRect(PIERECT_DrawRequest(scrX - scrR - 1, scrY + scrR + 2, scrX + scrR + 1, scrY + scrR + 6, WZCOL_RELOAD_BACKGROUND), rectGroup);
		batchedMultiRectRenderer.addRect(PIERECT_DrawRequest(scrX - scrR, scrY + scrR + 3, scrX - scrR + damage, scrY + scrR + 4, powerCol), rectGroup);
		batchedMultiRectRenderer.addRect(PIERECT_DrawRequest(scrX - scrR, scrY + scrR + 4, scrX - scrR + damage, scrY + scrR + 5, powerColShadow), rectGroup);
	}
}

/// Draw the selection graphics for selected droids
static void	drawDroidSelections()
{
	BASE_OBJECT		*psClickedOn;
	bool			bMouseOverDroid = false;
	bool			bMouseOverOwnDroid = false;
	UDWORD			index;

	psClickedOn = mouseTarget();
	if (psClickedOn != nullptr && psClickedOn->type == OBJ_DROID)
	{
		bMouseOverDroid = true;
		if (psClickedOn->player == selectedPlayer && !psClickedOn->selected)
		{
			bMouseOverOwnDroid = true;
		}
	}
	const bool isSpectator = bMultiPlayer && NetPlay.players[selectedPlayer].isSpectator;

	if (isSpectator)
	{
		if (barMode == BAR_DROIDS || barMode == BAR_DROIDS_AND_STRUCTURES)
		{
			for (int player = 0; player < MAX_PLAYERS; player++)
			{
				batchedObjectStatusRenderer.addDroidSelectionsToRender(player, psClickedOn, bMouseOverDroid);
			}
		}

		return;
	}

	if (selectedPlayer >= MAX_PLAYERS) { return; /* no-op */ }
	pie_SetFogStatus(false);

	batchedObjectStatusRenderer.addDroidSelectionsToRender(selectedPlayer, psClickedOn, bMouseOverDroid);

	/* Are we over an enemy droid */
	if (bMouseOverDroid && !bMouseOverOwnDroid)
	{
		if (mouseDown(getRightClickOrders() ? MOUSE_LMB : MOUSE_RMB))
		{
			if (psClickedOn->player != selectedPlayer && psClickedOn->sDisplay.frameNumber == currentGameFrame)
			{
				DROID *psDroid = (DROID *)psClickedOn;
				batchedObjectStatusRenderer.addEnemyDroidHealthToRender(psDroid);
			}
		}
	}

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		/* Go thru' all the droids */
		for (const DROID *psDroid : apsDroidLists[i])
		{
			if (showORDERS)
			{
				drawDroidOrder(psDroid);
			}
			if (!psDroid->died && psDroid->sDisplay.frameNumber == currentGameFrame)
			{
				/* If it's selected */
				if (psDroid->flags.test(OBJECT_FLAG_TARGETED) && psDroid->visibleForLocalDisplay() == UBYTE_MAX)
				{
					index = IMAGE_BLUE1 + getModularScaledRealTime(1020, 5);
					iV_DrawImage(IntImages, index, psDroid->sDisplay.screenX, psDroid->sDisplay.screenY);
				}
			}
		}
	}

	for (const FEATURE *psFeature : apsFeatureLists[0])
	{
		if (!psFeature->died && psFeature->sDisplay.frameNumber == currentGameFrame)
		{
			if (psFeature->flags.test(OBJECT_FLAG_TARGETED))
			{
				iV_DrawImage(IntImages, getTargettingGfx(), psFeature->sDisplay.screenX, psFeature->sDisplay.screenY);
			}
		}
	}
}

static void drawDroidAndStructureSelections()
{
	drawDroidSelections();
	drawStructureSelections();

	batchedObjectStatusRenderer.drawAllSelections();
	batchedObjectStatusRenderer.clear();
}

/* ---------------------------------------------------------------------------- */
/// X/Y offset to display the group number at
#define GN_X_OFFSET	(8)
#define GN_Y_OFFSET	(3)
enum GROUPNUMBER_TYPE
{
	GN_NORMAL,
	GN_DAMAGED,
	GN_FACTORY
};
/// rendering of the object's group next to the object itself,
/// or the group that will be assigned to the object after production in the factory
static void	drawGroupNumber(BASE_OBJECT *psObject)
{
	UWORD id = UWORD_MAX;
	UBYTE groupNumber = UBYTE_MAX;
	int32_t x = 0, y = 0;
	GROUPNUMBER_TYPE groupNumberType = GN_NORMAL;

	if (auto *psDroid = castDroid(psObject))
	{
		int32_t xShift = psDroid->sDisplay.screenR + GN_X_OFFSET;
		int32_t yShift = psDroid->sDisplay.screenR;

		x = psDroid->sDisplay.screenX - xShift;
		y = psDroid->sDisplay.screenY + yShift;

		if (psDroid->repairGroup != UBYTE_MAX)
		{
			groupNumber = psDroid->repairGroup;
			groupNumberType = GN_DAMAGED;
		}
		else
		{
			groupNumber = psDroid->group;
		}
	}
	else if (auto *psStruct = castStructure(psObject))
	{
		// same as in queueStructureHealth
		int32_t scale = static_cast<int32_t>(MAX(psStruct->pStructureType->baseWidth, psStruct->pStructureType->baseBreadth));
		int32_t width = scale * 20;
		int32_t scrX = psStruct->sDisplay.screenX;
		int32_t scrY = static_cast<int32_t>(psStruct->sDisplay.screenY) + (scale * 10);
		int32_t scrR = width;

		x = scrX - scrR - GN_X_OFFSET;
		y = scrY - GN_Y_OFFSET;

		groupNumber = psStruct->productToGroup;
		groupNumberType = GN_FACTORY;
	}

	switch (groupNumber)
	{
	case 0:
		id = IMAGE_GN_0;
		break;
	case 1:
		id = IMAGE_GN_1;
		break;
	case 2:
		id = IMAGE_GN_2;
		break;
	case 3:
		id = IMAGE_GN_3;
		break;
	case 4:
		id = IMAGE_GN_4;
		break;
	case 5:
		id = IMAGE_GN_5;
		break;
	case 6:
		id = IMAGE_GN_6;
		break;
	case 7:
		id = IMAGE_GN_7;
		break;
	case 8:
		id = IMAGE_GN_8;
		break;
	case 9:
		id = IMAGE_GN_9;
		break;
	default:
		break;
	}

	if (id != UWORD_MAX)
	{
		switch (groupNumberType)
		{
		case GN_NORMAL:
			iV_DrawImage(IntImages, id, x, y);
			break;
		case GN_DAMAGED:
			iV_DrawImageTint(IntImages, id, x, y, pal_RGBA(255, 0, 0, 255) /* red */);
			break;
		case GN_FACTORY:
			iV_DrawImageTint(IntImages, id, x, y, pal_RGBA(255, 220, 115, 255) /* gold */);
			break;
		default:
			break;
		}
	}
}

/// X offset to display the group number at
#define CMND_STAR_X_OFFSET	(6)
#define CMND_GN_Y_OFFSET	(8)

static void drawDroidOrder(const DROID *psDroid)
{
	const int xShift = psDroid->sDisplay.screenR + GN_X_OFFSET;
	const int yShift = psDroid->sDisplay.screenR - CMND_GN_Y_OFFSET;
	const char *letter = getDroidOrderKey(psDroid->order.type);
	iV_SetTextColour(WZCOL_TEXT_BRIGHT);
	iV_DrawText(letter, psDroid->sDisplay.screenX - xShift - CMND_STAR_X_OFFSET,  psDroid->sDisplay.screenY + yShift, font_regular);
}

/// Draw the number of the commander the droid is assigned to
static void	drawDroidCmndNo(DROID *psDroid)
{
	SDWORD	xShift, yShift, index;
	UDWORD	id2;
	UWORD	id;
	bool	bDraw = true;

	id = UWORD_MAX;

	id2 = IMAGE_GN_STAR;
	index = SDWORD_MAX;
	if (psDroid->droidType == DROID_COMMAND)
	{
		index = cmdDroidGetIndex(psDroid);
	}
	else if (hasCommander(psDroid))
	{
		index = cmdDroidGetIndex(psDroid->psGroup->psCommander);
	}
	switch (index)
	{
	case 1:
		id = IMAGE_GN_1;
		break;
	case 2:
		id = IMAGE_GN_2;
		break;
	case 3:
		id = IMAGE_GN_3;
		break;
	case 4:
		id = IMAGE_GN_4;
		break;
	case 5:
		id = IMAGE_GN_5;
		break;
	case 6:
		id = IMAGE_GN_6;
		break;
	case 7:
		id = IMAGE_GN_7;
		break;
	case 8:
		id = IMAGE_GN_8;
		break;
	case 9:
		id = IMAGE_GN_9;
		break;
	default:
		bDraw = false;
		break;
	}

	if (bDraw)
	{
		xShift = psDroid->sDisplay.screenR + GN_X_OFFSET;
		yShift = psDroid->sDisplay.screenR - CMND_GN_Y_OFFSET;
		iV_DrawImage(IntImages, id2, psDroid->sDisplay.screenX - xShift - CMND_STAR_X_OFFSET, psDroid->sDisplay.screenY + yShift);
		iV_DrawImage(IntImages, id, psDroid->sDisplay.screenX - xShift, psDroid->sDisplay.screenY + yShift);
	}
}
/* ---------------------------------------------------------------------------- */


/**	Get the onscreen coordinates of a droid so we can draw a bounding box
 * This need to be severely speeded up and the accuracy increased to allow variable size bouding boxes
 * @todo Remove all magic numbers and hacks
 */
void calcScreenCoords(DROID *psDroid, const glm::mat4 &perspectiveViewMatrix)
{
	/* Get it's absolute dimensions */
	const BODY_STATS *psBStats = psDroid->getBodyStats();
	Vector3i origin;
	Vector2i center(0, 0);
	int wsRadius = 22; // World space radius, 22 = magic minimum
	float radius;

	// NOTE: This only takes into account body, but seems "good enough"
	if (psBStats && psBStats->pIMD)
	{
		wsRadius = MAX(wsRadius, psBStats->pIMD->radius);
	}

	origin = Vector3i(0, wsRadius, 0); // take the center of the object

	/* get the screen coordinates */
	const float cZ = pie_RotateProjectWithPerspective(&origin, perspectiveViewMatrix, &center) * 0.1f;

	// avoid division by zero
	if (cZ > 0)
	{
		radius = wsRadius / cZ * pie_GetResScalingFactor();
	}
	else
	{
		radius = 1; // 1 just in case some other code assumes radius != 0
	}

	/* Deselect all the droids if we've released the drag box */
	if (dragBox3D.status == DRAG_RELEASED)
	{
		if (inQuad(&center, &dragQuad) && psDroid->player == selectedPlayer)
		{
			//don't allow Transporter Droids to be selected here
			//unless we're in multiPlayer mode!!!!
			if (!psDroid->isTransporter() || bMultiPlayer)
			{
				dealWithDroidSelect(psDroid, true);
			}
		}
	}

	/* Store away the screen coordinates so we can select the droids without doing a transform */
	psDroid->sDisplay.screenX = center.x;
	psDroid->sDisplay.screenY = center.y;
	psDroid->sDisplay.screenR = static_cast<UDWORD>(radius);
}

void screenCoordToWorld(Vector2i screenCoord, Vector2i &worldCoord, SDWORD &tileX, SDWORD &tileY)
{
	int nearestZ = INT_MAX;
	Vector2i outMousePos(0, 0);
	// Intentionally not the same range as in drawTiles()
	for (int i = -visibleTiles.y / 2, idx = 0; i < visibleTiles.y / 2; i++, ++idx)
	{
		for (int j = -visibleTiles.x / 2, jdx = 0; j < visibleTiles.x / 2; j++, ++jdx)
		{
			const int tileZ = tileScreenInfo[idx][jdx].z;

			if (tileZ <= nearestZ)
			{
				QUAD quad;

				quad.coords[0].x = tileScreenInfo[idx + 0][jdx + 0].x;
				quad.coords[0].y = tileScreenInfo[idx + 0][jdx + 0].y;

				quad.coords[1].x = tileScreenInfo[idx + 0][jdx + 1].x;
				quad.coords[1].y = tileScreenInfo[idx + 0][jdx + 1].y;

				quad.coords[2].x = tileScreenInfo[idx + 1][jdx + 1].x;
				quad.coords[2].y = tileScreenInfo[idx + 1][jdx + 1].y;

				quad.coords[3].x = tileScreenInfo[idx + 1][jdx + 0].x;
				quad.coords[3].y = tileScreenInfo[idx + 1][jdx + 0].y;

				/* We've got a match for our mouse coords */
				if (inQuad(&screenCoord, &quad))
				{
					outMousePos.x = playerPos.p.x + world_coord(j);
					outMousePos.y = playerPos.p.z + world_coord(i);
					outMousePos += positionInQuad(screenCoord, quad);
					if (outMousePos.x < 0)
					{
						outMousePos.x = 0;
					}
					else if (outMousePos.x > world_coord(mapWidth - 1))
					{
						outMousePos.x = world_coord(mapWidth - 1);
					}
					if (outMousePos.y < 0)
					{
						outMousePos.y = 0;
					}
					else if (outMousePos.y > world_coord(mapHeight - 1))
					{
						outMousePos.y = world_coord(mapHeight - 1);
					}
					tileX = map_coord(outMousePos.x);
					tileY = map_coord(outMousePos.y);
					/* Store away z value */
					nearestZ = tileZ;
				}
			}
		}
	}
	worldCoord = outMousePos;
}

/**
 * Find the tile the mouse is currently over
 * \todo This is slow - speed it up
 */
static void locateMouse()
{
	const Vector2i pt(mouseX(), mouseY());
	screenCoordToWorld(pt, mousePos, mouseTileX, mouseTileY);
}

/// Render the sky and surroundings
static void renderSurroundings(const glm::mat4& projectionMatrix, const glm::mat4 &skyboxViewMatrix)
{
	WZ_PROFILE_SCOPE(renderSurroundings);
	// Render skybox relative to ground (i.e. undo player y translation)
	// then move it somewhat below ground level for the blending effect
	// rotate it

	if (!gamePaused())
	{
		wind = std::remainder(wind + graphicsTimeAdjustedIncrement(windSpeed), 360.0f);
	}

	// TODO: skybox needs to be just below lowest point on map (because we have a bottom cap now). Hardcoding for now.
	pie_DrawSkybox(skybox_scale, projectionMatrix, skyboxViewMatrix * glm::translate(glm::vec3(0.f, -500, 0.f)) * glm::rotate(RADIANS(wind), glm::vec3(0.f, 1.f, 0.f)));
}

static int calculateCameraHeight(int _mapHeight)
{
	return static_cast<int>(std::ceil(static_cast<float>(_mapHeight) / static_cast<float>(HEIGHT_TRACK_INCREMENTS))) * HEIGHT_TRACK_INCREMENTS + CAMERA_PIVOT_HEIGHT;
}

int calculateCameraHeightAt(int tileX, int tileY)
{
	return calculateCameraHeight(calcAverageTerrainHeight(tileX, tileY));
}

/// Smoothly adjust player height to match the desired height
static void trackHeight(int desiredHeight)
{
	static const uint32_t minTrackHeightInterval = GAME_TICKS_PER_SEC / 60;
	static uint32_t lastHeightAdjustmentRealTime = 0;
	static float heightSpeed = 0.0f;

	uint32_t deltaTrackHeightRealTime = realTime - lastHeightAdjustmentRealTime;
	if (deltaTrackHeightRealTime < minTrackHeightInterval)
	{
		// avoid processing this too rapidly, such as when vsync is disabled
		return;
	}
	lastHeightAdjustmentRealTime = realTime;

	if ((desiredHeight == playerPos.p.y) && ((heightSpeed > -5.f) && (heightSpeed < 5.f)))
	{
		heightSpeed = 0.0f;
		return;
	}

	float separation = desiredHeight - playerPos.p.y;	// How far are we from desired height?

	// d²/dt² player.p.y = -ACCEL_CONSTANT * (player.p.y - desiredHeight) - VELOCITY_CONSTANT * d/dt player.p.y
	solveDifferential2ndOrder(&separation, &heightSpeed, ACCEL_CONSTANT, VELOCITY_CONSTANT, (float) deltaTrackHeightRealTime / (float) GAME_TICKS_PER_SEC);

	/* Adjust the height accordingly */
	playerPos.p.y = desiredHeight - static_cast<int>(std::trunc(separation));
}

/// Select the next energy bar display mode
ENERGY_BAR toggleEnergyBars()
{
	if (++barMode == BAR_LAST)
	{
		barMode = BAR_SELECTED;
	}
	return (ENERGY_BAR)barMode;
}

/// Set everything up for when the player assigns the sensor target
void assignSensorTarget(BASE_OBJECT *psObj)
{
	bSensorTargetting = true;
	lastTargetAssignation = realTime;
	psSensorObj = psObj;
}

/// Set everything up for when the player selects the destination
void	assignDestTarget()
{
	bDestTargetting = true;
	lastDestAssignation = realTime;
	destTargetX = mouseX();
	destTargetY = mouseY();
	destTileX = mouseTileX;
	destTileY = mouseTileY;
}

/// Draw a graphical effect after selecting a sensor target
static void processSensorTarget()
{
	if (bSensorTargetting)
	{
		if ((realTime - lastTargetAssignation) < TARGET_TO_SENSOR_TIME)
		{
			if (!psSensorObj->died && psSensorObj->sDisplay.frameNumber == currentGameFrame)
			{
				const int x = /*mouseX();*/(SWORD)psSensorObj->sDisplay.screenX;
				const int y = (SWORD)psSensorObj->sDisplay.screenY;
				int index = IMAGE_BLUE1;
				if (!gamePaused())
				{
					index = IMAGE_BLUE1 + getModularScaledGraphicsTime(1020, 5);
				}
				iV_DrawImage(IntImages, index, x, y);
				const int offset = 12 + ((TARGET_TO_SENSOR_TIME) - (realTime - lastTargetAssignation)) / 2;
				const int x0 = x - offset;
				const int y0 = y - offset;
				const int x1 = x + offset;
				const int y1 = y + offset;
				const std::vector<glm::ivec4> lines = { glm::ivec4(x0, y0, x0 + 8, y0), glm::ivec4(x0, y0, x0, y0 + 8),
				                                        glm::ivec4(x1, y0, x1 - 8, y0), glm::ivec4(x1, y0, x1, y0 + 8),
				                                        glm::ivec4(x1, y1, x1 - 8, y1), glm::ivec4(x1, y1, x1, y1 - 8),
				                                        glm::ivec4(x0, y1, x0 + 8, y1), glm::ivec4(x0, y1, x0, y1 - 8),
				                                        glm::ivec4(x0, y0, x0 + 8, y0), glm::ivec4(x0, y0, x0, y0 + 8) };
				iV_Lines(lines, WZCOL_WHITE);
			}
			else
			{
				bSensorTargetting = false;
			}
		}
		else
		{
			bSensorTargetting = false;
		}
	}

}

/// Draw a graphical effect after selecting a destination
static void processDestinationTarget()
{
	if (bDestTargetting)
	{
		if ((realTime - lastDestAssignation) < DEST_TARGET_TIME)
		{
			const int x = destTargetX;
			const int y = destTargetY;
			const int offset = ((DEST_TARGET_TIME) - (realTime - lastDestAssignation)) / 2;
			const int x0 = x - offset;
			const int y0 = y - offset;
			const int x1 = x + offset;
			const int y1 = y + offset;

			pie_BoxFill(x0, y0, x0 + 2, y0 + 2, WZCOL_WHITE);
			pie_BoxFill(x1 - 2, y0 - 2, x1, y0, WZCOL_WHITE);
			pie_BoxFill(x1 - 2, y1 - 2, x1, y1, WZCOL_WHITE);
			pie_BoxFill(x0, y1, x0 + 2, y1 + 2, WZCOL_WHITE);
		}
		else
		{
			bDestTargetting = false;
		}
	}
}

/// Set what tile is being used to draw the bottom of a body of water
void	setUnderwaterTile(UDWORD num)
{
	underwaterTile = num;
}

/// Set what tile is being used to show rubble
void	setRubbleTile(UDWORD num)
{
	rubbleTile = num;
}
/// Get the tile that is currently being used to draw underwater ground
UDWORD	getWaterTileNum()
{
	return (underwaterTile);
}
/// Get the tile that is being used to show rubble
UDWORD	getRubbleTileNum()
{
	return (rubbleTile);
}

/// Draw the spinning particles for power stations and re-arm pads for the specified player
static void structureEffectsPlayer(UDWORD player)
{
	SDWORD	radius;
	SDWORD	xDif, yDif;
	Vector3i	pos;
	UDWORD	gameDiv;

	const int effectsPerSecond = 12;  // Effects per second. Will add effects up to once time per frame, so won't add as many effects if the framerate is low, but will be consistent, otherwise.
	unsigned effectTime = graphicsTime / (GAME_TICKS_PER_SEC / effectsPerSecond) * (GAME_TICKS_PER_SEC / effectsPerSecond);
	if (effectTime <= graphicsTime - deltaGraphicsTime)
	{
		return;  // Don't add effects this frame.
	}

	for (const STRUCTURE *psStructure : apsStructLists[player])
	{
		if (psStructure->status != SS_BUILT)
		{
			continue;
		}
		if (psStructure->pStructureType->type == REF_POWER_GEN && psStructure->visibleForLocalDisplay())
		{
			POWER_GEN *psPowerGen = &psStructure->pFunctionality->powerGenerator;
			unsigned numConnected = 0;
			for (int i = 0; i < NUM_POWER_MODULES; i++)
			{
				if (psPowerGen->apResExtractors[i])
				{
					numConnected++;
				}
			}
			/* No effect if nothing connected */
			if (!numConnected)
			{
				//keep looking for another!
				continue;
			}
			else switch (numConnected)
				{
				case 1:
				case 2:
					gameDiv = 1440;
					break;
				case 3:
				case 4:
				default:
					gameDiv = 1080;   // really fast!!!
					break;
				}

			/* New addition - it shows how many are connected... */
			for (int i = 0 ; i < numConnected; i++)
			{
				radius = 32 - (i * 2);	// around the spire
				xDif = iSinSR(effectTime, gameDiv, radius);
				yDif = iCosSR(effectTime, gameDiv, radius);

				pos.x = psStructure->pos.x + xDif;
				pos.z = psStructure->pos.y + yDif;
				pos.y = map_Height(pos.x, pos.z) + 64 + (i * 20);	// 64 up to get to base of spire
				effectGiveAuxVar(50);	// half normal plasma size...
				addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_LASER, false, nullptr, 0);

				pos.x = psStructure->pos.x - xDif;
				pos.z = psStructure->pos.y - yDif;
				effectGiveAuxVar(50);	// half normal plasma size...

				addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_LASER, false, nullptr, 0);
			}
		}
		/* Might be a re-arm pad! */
		else if (psStructure->pStructureType->type == REF_REARM_PAD
		         && psStructure->visibleForLocalDisplay())
		{
			REARM_PAD *psReArmPad = &psStructure->pFunctionality->rearmPad;
			BASE_OBJECT *psChosenObj = psReArmPad->psObj;
			if (psChosenObj != nullptr && (((DROID *)psChosenObj)->visibleForLocalDisplay()))
			{
				unsigned bFXSize = 0;
				DROID *psDroid = (DROID *) psChosenObj;
				if (!psDroid->died && psDroid->action == DACTION_WAITDURINGREARM)
				{
					bFXSize = 30;
				}
				/* Then it's repairing...? */
				iIMDShape *pDisplayModel = psStructure->sDisplay.imd->displayModel();
				radius = pDisplayModel->radius;
				xDif = iSinSR(effectTime, 720, radius);
				yDif = iCosSR(effectTime, 720, radius);
				pos.x = psStructure->pos.x + xDif;
				pos.z = psStructure->pos.y + yDif;
				pos.y = map_Height(pos.x, pos.z) + pDisplayModel->max.y;
				effectGiveAuxVar(30 + bFXSize);	// half normal plasma size...
				addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_LASER, false, nullptr, 0);
				pos.x = psStructure->pos.x - xDif;
				pos.z = psStructure->pos.y - yDif;	// buildings are level!
				effectGiveAuxVar(30 + bFXSize);	// half normal plasma size...
				addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_LASER, false, nullptr, 0);
			}
		}
	}
}

/// Draw the effects for all players and buildings
static void structureEffects()
{
	for (unsigned i = 0; i < MAX_PLAYERS; i++)
	{
		if (!apsStructLists[i].empty())
		{
			structureEffectsPlayer(i);
		}
	}
}

/// Show the sensor ranges of selected droids and buildings
static void	showDroidSensorRanges()
{
	static uint32_t lastRangeUpdateTime = 0;

	if (selectedPlayer >= MAX_PLAYERS) { return; /* no-op */ }

	if (rangeOnScreen
		&& (graphicsTime - lastRangeUpdateTime) >= 50)		// note, we still have to decide what to do with multiple units selected, since it will draw it for all of them! -Q 5-10-05
	{
		for (DROID* psDroid : apsDroidLists[selectedPlayer])
		{
			if (psDroid->selected)
			{
				showSensorRange2((BASE_OBJECT *)psDroid);
			}
		}

		for (STRUCTURE* psStruct : apsStructLists[selectedPlayer])
		{
			if (psStruct->selected)
			{
				showSensorRange2((BASE_OBJECT *)psStruct);
			}
		}

		lastRangeUpdateTime = graphicsTime;
	}//end if we want to display...
}

static void showEffectCircle(Position centre, int32_t radius, uint32_t auxVar, EFFECT_GROUP group, EFFECT_TYPE type)
{
	const int32_t circumference = radius * 2 * 355 / 113 / TILE_UNITS; // 2πr in tiles.
	for (int i = 0; i < circumference; ++i)
	{
		Vector3i pos;
		pos.x = centre.x - iSinSR(i, circumference, radius);
		pos.z = centre.y - iCosSR(i, circumference, radius);  // [sic] y -> z

		// Check if it's actually on map
		if (worldOnMap(pos.x, pos.z))
		{
			pos.y = map_Height(pos.x, pos.z) + 16;
			effectGiveAuxVar(auxVar);
			addEffect(&pos, group, type, false, nullptr, 0);
		}
	}
}

// Shows the weapon (long) range of the object in question.
// Note, it only does it for the first weapon slot!
static void showWeaponRange(BASE_OBJECT *psObj)
{
	WEAPON_STATS *psStats;

	if (psObj->type == OBJ_DROID)
	{
		DROID *psDroid = (DROID *)psObj;
		const int compIndex = psDroid->asWeaps[0].nStat;	// weapon_slot
		ASSERT_OR_RETURN(, compIndex < asWeaponStats.size(), "Invalid range referenced for numWeaponStats, %d > %zu", compIndex, asWeaponStats.size());
		psStats = &asWeaponStats[compIndex];
	}
	else
	{
		STRUCTURE *psStruct = (STRUCTURE *)psObj;
		if (psStruct->pStructureType->numWeaps == 0)
		{
			return;
		}
		psStats = psStruct->pStructureType->psWeapStat[0];
	}
	const unsigned weaponRange = proj_GetLongRange(*psStats, psObj->player);
	const unsigned minRange = proj_GetMinRange(*psStats, psObj->player);
	showEffectCircle(psObj->pos, weaponRange, 40, EFFECT_EXPLOSION, EXPLOSION_TYPE_SMALL);
	if (minRange > 0)
	{
		showEffectCircle(psObj->pos, minRange, 40, EFFECT_EXPLOSION, EXPLOSION_TYPE_TESLA);
	}
}

static void showSensorRange2(BASE_OBJECT *psObj)
{
	showEffectCircle(psObj->pos, objSensorRange(psObj), 80, EFFECT_EXPLOSION, EXPLOSION_TYPE_LASER);
	showWeaponRange(psObj);
}

/// Draw a circle on the map (to show the range of something)
static void drawRangeAtPos(SDWORD centerX, SDWORD centerY, SDWORD radius)
{
	Position pos(centerX, centerY, 0);  // .z ignored.
	showEffectCircle(pos, radius, 80, EFFECT_EXPLOSION, EXPLOSION_TYPE_SMALL);
}

/** Turn on drawing some effects at certain position to visualize the radius.
 * \note Pass a negative radius to turn this off
 */
void showRangeAtPos(SDWORD centerX, SDWORD centerY, SDWORD radius)
{
	rangeCenterX = centerX;
	rangeCenterY = centerY;
	rangeRadius = radius;

	bRangeDisplay = true;

	if (radius <= 0)
	{
		bRangeDisplay = false;
	}
}
UDWORD  getDroidRankGraphicFromLevel(unsigned int level)
{
	UDWORD gfxId = UDWORD_MAX;
	switch (level)
		{
		case 0:
			break;
		case 1:
			gfxId = IMAGE_LEV_0;
			break;
		case 2:
			gfxId = IMAGE_LEV_1;
			break;
		case 3:
			gfxId = IMAGE_LEV_2;
			break;
		case 4:
			gfxId = IMAGE_LEV_3;
			break;
		case 5:
			gfxId = IMAGE_LEV_4;
			break;
		case 6:
			gfxId = IMAGE_LEV_5;
			break;
		case 7:
			gfxId = IMAGE_LEV_6;
			break;
		case 8:
			gfxId = IMAGE_LEV_7;
			break;
		default:
			ASSERT(!"out of range droid rank", "Weird droid level in drawDroidRank");
			break;
		}

	return gfxId;
}
/// Get the graphic ID for a droid rank
UDWORD  getDroidRankGraphic(const DROID *psDroid)
{
	/* Establish the numerical value of the droid's rank */
	return getDroidRankGraphicFromLevel(getDroidLevel(psDroid));
}

/**	Will render a graphic depiction of the droid's present rank.
 * \note Assumes matrix context set and that z-buffer write is force enabled (Always).
 */
static void	drawDroidRank(DROID *psDroid)
{
	UDWORD	gfxId = getDroidRankGraphic(psDroid);

	/* Did we get one? - We should have... */
	if (gfxId != UDWORD_MAX)
	{
		/* Render the rank graphic at the correct location */ // remove hardcoded numbers?!
		iV_DrawImage(IntImages, (UWORD)gfxId,
		             psDroid->sDisplay.screenX + psDroid->sDisplay.screenR + 8,
		             psDroid->sDisplay.screenY + psDroid->sDisplay.screenR);
	}
}

/**	Will render a sensor graphic for a droid locked to a sensor droid/structure
 * \note Assumes matrix context set and that z-buffer write is force enabled (Always).
 */
static void	drawDroidSensorLock(DROID *psDroid)
{
	//if on fire support duty - must be locked to a Sensor Droid/Structure
	if (orderState(psDroid, DORDER_FIRESUPPORT))
	{
		/* Render the sensor graphic at the correct location - which is what?!*/
		iV_DrawImage(IntImages, IMAGE_GN_STAR, psDroid->sDisplay.screenX, psDroid->sDisplay.screenY);
	}
}

/// Draw the construction lines for all construction droids
static void doConstructionLines(const glm::mat4 &viewMatrix)
{
	WZ_PROFILE_SCOPE(doConstructionLines);
	for (unsigned i = 0; i < MAX_PLAYERS; i++)
	{
		for (DROID *psDroid : apsDroidLists[i])
		{
			if (clipXY(psDroid->pos.x, psDroid->pos.y)
			    && psDroid->visibleForLocalDisplay() == UBYTE_MAX
			    && psDroid->sMove.Status != MOVESHUFFLE)
			{
				if (psDroid->action == DACTION_BUILD)
				{
					if (psDroid->order.psObj)
					{
						if (psDroid->order.psObj->type == OBJ_STRUCTURE)
						{
							addConstructionLine(psDroid, (STRUCTURE *)psDroid->order.psObj, viewMatrix);
						}
					}
				}
				else if ((psDroid->action == DACTION_DEMOLISH) ||
				         (psDroid->action == DACTION_REPAIR) ||
				         (psDroid->action == DACTION_RESTORE))
				{
					if (psDroid->psActionTarget[0]
					    && psDroid->psActionTarget[0]->type == OBJ_STRUCTURE)
					{
						addConstructionLine(psDroid, (STRUCTURE *)psDroid->psActionTarget[0], viewMatrix);
					}
				}
			}
		}
	}
}

static uint32_t randHash(std::initializer_list<uint32_t> data) {
	uint32_t v = 0x12345678;
	auto shuffle = [&v](uint32_t d, uint32_t x){
		v += d;
		v *= x;
		v ^= v>>15;
		v *= 0x987decaf;
		v ^= v>>17;
	};
	for (int i : data) {
		shuffle(i, 0x7ea99999);
	}
	for (int i : data) {
		shuffle(i, 0xc0ffee77);
	}
	return v;
}

/// Draw the construction or demolish lines for one droid
static void addConstructionLine(DROID *psDroid, STRUCTURE *psStructure, const glm::mat4 &viewMatrix)
{
	if (!psStructure->sDisplay.imd)
	{
		return;
	}
	iIMDShape *pDisplayModel = psStructure->sDisplay.imd->displayModel();

	auto deltaPlayer = Vector3f(0,0,0);
	auto pt0 = Vector3f(psDroid->pos.x, psDroid->pos.z + 24, -psDroid->pos.y) + deltaPlayer;

	int constructPoints = constructorPoints(*psDroid->getConstructStats(), psDroid->player);
	int amount = 800 * constructPoints * (graphicsTime - psDroid->actionStarted) / GAME_TICKS_PER_SEC;

	Vector3i each;
	auto getPoint = [&](uint32_t c) {
		uint32_t t = (amount + c)/1000;
		float s = (amount + c)%1000*.001f;
		unsigned pointIndexA = randHash({psDroid->id, psStructure->id, psDroid->actionStarted, t, c}) % pDisplayModel->points.size();
		unsigned pointIndexB = randHash({psDroid->id, psStructure->id, psDroid->actionStarted, t + 1, c}) % pDisplayModel->points.size();
		auto &pointA = pDisplayModel->points[pointIndexA];
		auto &pointB = pDisplayModel->points[pointIndexB];
		auto point = mix(pointA, pointB, s);

		each = Vector3f(psStructure->pos.x, psStructure->pos.z, psStructure->pos.y)
			+ Vector3f(point.x, structHeightScale(psStructure) * point.y, -point.z);
		return Vector3f(each.x, each.y, -each.z) + deltaPlayer;
	};

	auto pt1 = getPoint(250);
	auto pt2 = getPoint(750);

	if (psStructure->currentBuildPts < 10) {
		auto pointC = Vector3f(psStructure->pos.x, psStructure->pos.z + 10, -psStructure->pos.y) + deltaPlayer;
		auto cross = Vector3f(psStructure->pos.y - psDroid->pos.y, 0, psStructure->pos.x - psDroid->pos.x);
		auto shift = 40.f*normalize(cross);
		pt1 = mix(pointC - shift, pt1, psStructure->currentBuildPts*.1f);
		pt2 = mix(pointC + shift, pt1, psStructure->currentBuildPts*.1f);
	}

	if (rand() % 250u < deltaGraphicsTime)
	{
		effectSetSize(30);
		addEffect(&each, EFFECT_EXPLOSION, EXPLOSION_TYPE_SPECIFIED, true, getDisplayImdFromIndex(MI_PLASMA), 0);
	}

	PIELIGHT colour = psDroid->action == DACTION_DEMOLISH? WZCOL_DEMOLISH_BEAM : WZCOL_CONSTRUCTOR_BEAM;
	pie_TransColouredTriangle({pt0, pt1, pt2}, colour, viewMatrix);
	pie_TransColouredTriangle({pt0, pt2, pt1}, colour, viewMatrix);
}
