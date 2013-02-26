/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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
#include "lib/framework/opengl.h"
#include "lib/framework/math_ext.h"
#include "lib/framework/stdio_ext.h"

/* Includes direct access to render library */
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piedef.h"
#include "lib/ivis_opengl/tex.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/framework/fixedpoint.h"
#include "lib/ivis_opengl/piefunc.h"

#include "lib/gamelib/gtime.h"
#include "lib/gamelib/animobj.h"
#include "lib/script/script.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "lib/netplay/netplay.h"

#include "loop.h"
#include "atmos.h"
#include "levels.h"
#include "map.h"
#include "move.h"
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
#include "intelmap.h"
#include "mapdisplay.h"
#include "message.h"
#include "component.h"
#include "warcam.h"
#include "scripttabs.h"
#include "scriptextern.h"
#include "scriptcb.h"
#include "keymap.h"
#include "drive.h"
#include "gateway.h"
#include "transporter.h"
#include "warzoneconfig.h"
#include "action.h"
#include "keybind.h"
#include "combat.h"
#include "order.h"
#include "scores.h"
#include "multiplay.h"
#include "advvis.h"
#include "texture.h"
#include "anim_id.h"
#include "cmddroid.h"
#include "terrain.h"

/********************  Prototypes  ********************/

static UDWORD	getTargettingGfx(void);
static void	drawDroidGroupNumber(DROID *psDroid);
static void	trackHeight(float desiredHeight);
static void	renderSurroundings(void);
static void	locateMouse(void);
static bool	renderWallSection(STRUCTURE *psStructure);
static void	drawDragBox(void);
static void	calcFlagPosScreenCoords(SDWORD *pX, SDWORD *pY, SDWORD *pR);
static void	displayTerrain(void);
static void	drawTiles(iView *player);
static void	display3DProjectiles(void);
static void	drawDroidSelections(void);
static void	drawStructureSelections(void);
static void	displayAnimation(ANIM_OBJECT * psAnimObj, bool bHoldOnFirstFrame);
static void displayBlueprints(void);
static void	processSensorTarget(void);
static void	processDestinationTarget(void);
static bool	eitherSelected(DROID *psDroid);
static void	structureEffects(void);
static void	showDroidSensorRanges(void);
static void	showSensorRange2(BASE_OBJECT *psObj);
static void	drawRangeAtPos(SDWORD centerX, SDWORD centerY, SDWORD radius);
static void	addConstructionLine(DROID *psDroid, STRUCTURE *psStructure);
static void	doConstructionLines(void);
static void	drawDroidCmndNo(DROID *psDroid);
static void	drawDroidRank(DROID *psDroid);
static void	drawDroidSensorLock(DROID *psDroid);
static void	calcAverageTerrainHeight(iView *player);
bool	doWeDrawProximitys(void);
static PIELIGHT getBlueprintColour(STRUCT_STATES state);

static void NetworkDisplayPlainForm(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
static void NetworkDisplayImage(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
void NotifyUserOfError(char *msg);
extern bool writeGameInfo(const char *pFileName); // Used to help debug issues when we have fatal errors & crash handler testing.
/********************  Variables  ********************/
// Should be cleaned up properly and be put in structures.

// Initialised at start of drawTiles().
// In model coordinates where x is east, y is up and z is north, rather than world coordinates where x is east, y is south and z is up.
// To get the real camera position, still need to add Vector3i(player.p.x, 0, player.p.z).
static Vector3i actualCameraPosition;

bool	bRender3DOnly;
static bool	bRangeDisplay = false;
static SDWORD	rangeCenterX,rangeCenterY,rangeRadius;
static bool	bDrawProximitys = true;
bool	godMode;
bool	showGateways = false;
bool	showPath = false;

/// The name of the texture page used to draw the skybox
static char skyboxPageName[PATH_MAX] = "page-25";
static float wind = 0.0f;
static float windSpeed = 0.0f;
static float skybox_scale = 10000.0f;

/// When to display HP bars
UWORD barMode;

/// Have we made a selection by clicking the mouse? - used for dragging etc
bool	selectAttempt = false;

/// Vectors that hold the player and camera directions and positions
iView	player;

/// How far away are we from the terrain
static float distance;

/// Records the present X and Y values for the current mouse tile (in tiles)
SDWORD mouseTileX, mouseTileY;
Vector2i mousePos(0, 0);

/// Do we want the radar to be rendered
bool radarOnScreen = true;
bool	radarPermitted = true;

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
bool showSAMPLES = false;
/**  Show the current selected units order / action
 *  default OFF, turn ON via console command 'showorders'
 */
bool showORDERS = false;
/** Show the current level name on the screen, toggle via the 'showlevelname'
 *  console command
*/
bool showLevelName = true;
/** When we have a connection issue, we will flash a message on screen
*/
static bool errorWaiting = false;
static char errorMessage[512];
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

/** The time at which a sensor target was last asssigned
 * Used to draw a visual effect.
 */
static UDWORD	lastTargetAssignation = 0;
/** The time at which an order concerning a destination was last given
 * Used to draw a visual effect.
 */
static UDWORD	lastDestAssignation = 0;
static bool	bSensorTargetting = false;
static bool	bDestTargetting = false;
static BASE_OBJECT *psSensorObj = NULL;
static UDWORD	destTargetX,destTargetY;
static UDWORD	destTileX=0,destTileY=0;

struct Blueprint
{
	Blueprint(STRUCTURE_STATS const *stats, Vector2i pos, uint16_t dir, uint32_t index, STRUCT_STATES state)
		: stats(stats)
		, pos(pos)
		, dir(dir)
		, index(index)
		, state(state)
	{}
	int compare(Blueprint const &b) const
	{
		if (stats->ref != b.stats->ref) return stats->ref < b.stats->ref? -1 : 1;
		if (pos.x != b.pos.x) return pos.x < b.pos.x? -1 : 1;
		if (pos.y != b.pos.y) return pos.y < b.pos.y? -1 : 1;
		if (dir != b.dir) return dir < b.dir? -1 : 1;
		if (index != b.index) return index < b.index? -1 : 1;
		if (state != b.state) return state < b.state? -1 : 1;
		return 0;
	}
	bool operator <(Blueprint const &b) const { return compare(b) < 0; }
	bool operator ==(Blueprint const &b) const { return compare(b) == 0; }
	STRUCTURE *buildBlueprint() const  ///< Must delete after use.
	{
		return ::buildBlueprint(stats, pos, dir, index, state);
	}
	void renderBlueprint() const
	{
		if (clipXY(pos.x, pos.y))
		{
			STRUCTURE *psStruct = buildBlueprint();
			renderStructure(psStruct);
			delete psStruct;
		}
	}

	STRUCTURE_STATS const *stats;
	Vector2i pos;
	uint16_t dir;
	uint32_t index;
	STRUCT_STATES state;
};

static std::vector<Blueprint> blueprints;

#define	TARGET_TO_SENSOR_TIME	((4*(GAME_TICKS_PER_SEC))/5)
#define	DEST_TARGET_TIME	(GAME_TICKS_PER_SEC/4)

/// The distance the selection box will pulse
#define BOX_PULSE_SIZE  10

/// the opacity at which building blueprints will be drawn
static const int BLUEPRINT_OPACITY=120;

/********************  Functions  ********************/

void setSkyBox(const char *page, float mywind, float myscale)
{
	sstrcpy(skyboxPageName, page);
	windSpeed = mywind;
	wind = 0.0f;
	skybox_scale = myscale;
	pie_InitSkybox(iV_GetTexture(skyboxPageName));
}

static inline void rotateSomething(int &x, int &y, uint16_t angle)
{
	int64_t cra = iCos(angle), sra = iSin(angle);
	int newX = (x*cra - y*sra)>>16;
	int newY = (x*sra + y*cra)>>16;
	x = newX;
	y = newY;
}


void NotifyUserOfError(char *msg)
{
	errorWaiting = true;
	ssprintf(errorMessage, "%s", msg);
	lastErrorTime = realTime;
}

static Blueprint getTileBlueprint(int mapX, int mapY)
{
	Vector2i mouse(world_coord(mapX) + TILE_UNITS/2, world_coord(mapY) + TILE_UNITS/2);

	for (std::vector<Blueprint>::const_iterator blueprint = blueprints.begin(); blueprint != blueprints.end(); ++blueprint)
	{
		Vector2i size = getStructureStatsSize(blueprint->stats, blueprint->dir)*TILE_UNITS;
		if (abs(mouse.x - blueprint->pos.x) < size.x/2 && abs(mouse.y - blueprint->pos.y) < size.y/2)
		{
			return *blueprint;
		}
	}

	return Blueprint(NULL, Vector2i(), 0, 0, SS_BEING_BUILT);
}

STRUCTURE *getTileBlueprintStructure(int mapX, int mapY)
{
	static STRUCTURE *psStruct = NULL;

	Blueprint blueprint = getTileBlueprint(mapX, mapY);
	if (blueprint.state == SS_BLUEPRINT_PLANNED)
	{
		delete psStruct;  // Delete previously returned structure, if any.
		psStruct = blueprint.buildBlueprint();
		return psStruct;  // This blueprint was clicked on.
	}

	return NULL;
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

static PIELIGHT structureBrightness(STRUCTURE *psStructure)
{
	PIELIGHT buildingBrightness;

	if (structureIsBlueprint(psStructure))
	{
		buildingBrightness = getBlueprintColour(psStructure->status);
	}
	else
	{
		buildingBrightness = pal_SetBrightness(200 - 100/65536.f * getStructureDamage(psStructure));

		/* If it's selected, then it's brighter */
		if (psStructure->selected)
		{
			SDWORD brightVar;

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
			buildingBrightness = pal_SetBrightness(200 + brightVar);
		}
		if (!getRevealStatus())
		{
			buildingBrightness = pal_SetBrightness(avGetObjLightLevel((BASE_OBJECT*)psStructure, buildingBrightness.byte.r));
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

/// Display the multiplayer chat box
static void displayMultiChat(void)
{
	iV_SetFont(font_regular);

	UDWORD	pixelLength;
	UDWORD	pixelHeight;

	pixelLength = iV_GetTextWidth(sTextToSend);
	pixelHeight = iV_GetTextLineSize();

	if((realTime % 500) < 250)
	{
		// implement blinking cursor in multiplayer chat
		pie_BoxFill(RET_X + pixelLength + 3, 474 + E_H - (pixelHeight/4), RET_X + pixelLength + 10, 473 + E_H, WZCOL_CURSOR);
	}

	/* FIXME: GET RID OF THE MAGIC NUMBERS BELOW */
	iV_TransBoxFill(RET_X + 1, 474 + E_H - pixelHeight, RET_X + 1 + pixelLength + 2, 473 + E_H);

	iV_DrawText(sTextToSend, RET_X + 3, 469 + E_H);
}

/// Show all droid movement parts by displaying an explosion at every step
static void showDroidPaths(void)
{
	DROID *psDroid;

	if ((graphicsTime / 250 % 2) != 0)
	{
		return;
	}

	for (psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid=psDroid->psNext)
	{
		if (psDroid->selected && psDroid->sMove.Status != MOVEINACTIVE)
		{
			int	len = psDroid->sMove.numPoints;

			int i = std::max(psDroid->sMove.pathIndex - 1, 0);
			for (; i < len; i++)
			{
				Vector3i pos;

				ASSERT(worldOnMap(psDroid->sMove.asPath[i].x, psDroid->sMove.asPath[i].y), "Path off map!");
				pos.x = psDroid->sMove.asPath[i].x;
				pos.z = psDroid->sMove.asPath[i].y;
				pos.y = map_Height(pos.x, pos.z) + 16;

				effectGiveAuxVar(80);
				addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_LASER, false, NULL, 0);
			}
		}
	}
}
/// Renders the Network Issue form
static void NetworkDisplayPlainForm(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	W_TABFORM *Form = (W_TABFORM*)psWidget;
	UDWORD x0,y0,x1,y1;

	x0 = xOffset+Form->x();
	y0 = yOffset+Form->y();
	x1 = x0 + Form->width();
	y1 = y0 + Form->height();

	// Don't draw anything, a rectangle behind the icon just looks strange, if you notice it.
	//RenderWindowFrame(FRAME_NORMAL, x0, y0, x1 - x0, y1 - y0);
	(void)x0;
	(void)y0;
	(void)x1;
	(void)y1;
}

/// Displays an image for the Network Issue button
static void NetworkDisplayImage(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();
	UWORD ImageID;
	CONNECTION_STATUS status = (CONNECTION_STATUS)UNPACKDWORD_TRI_A(psWidget->UserData);

	ASSERT( psWidget->type == WIDG_BUTTON,"Not a button" );

	// cheap way to do a button flash
	if ( (realTime/250) % 2 == 0 )
	{
		ImageID = UNPACKDWORD_TRI_B(psWidget->UserData);
	}
	else
	{
		ImageID = UNPACKDWORD_TRI_C(psWidget->UserData);
	}

	if (NETcheckPlayerConnectionStatus(status, NET_ALL_PLAYERS))
	{
		unsigned width, height;
		unsigned n, c = 0;
		char players[MAX_PLAYERS + 1];
		PlayerMask playerMaskMapped = 0;
		for (n = 0; n < MAX_PLAYERS; ++n)
		{
			if (NETcheckPlayerConnectionStatus(status, n))
			{
				playerMaskMapped |= 1<<NetPlay.players[n].position;
			}
		}
		for (n = 0; n < MAX_PLAYERS; ++n)
		{
			if ((playerMaskMapped & 1<<n) != 0)
			{
				STATIC_ASSERT(MAX_PLAYERS <= 32);  // If increasing MAX_PLAYERS, check all the 1<<playerNumber shifts, since the 1 is usually a 32-bit type.
				players[c++] = "0123456789ABCDEFGHIJKLMNOPQRSTUV"[n];
			}
		}
		players[c] = '\0';

		iV_SetFont(font_regular);
		width = iV_GetTextWidth(players) + 10;
		height = iV_GetTextHeight(players) + 10;

		iV_SetTextColour(WZCOL_TEXT_BRIGHT);
		iV_DrawText(players, x - width, y + height);
	}

	iV_DrawImage(IntImages,ImageID,x,y);
}

static void setupConnectionStatusForm(void)
{
	static unsigned          prevStatusMask = 0;

	const int separation = 3;
	unsigned statusMask = 0;
	unsigned total = 0;
	unsigned i;

	for (i = 0; i < CONNECTIONSTATUS_NORMAL; ++i)
	{
		if (NETcheckPlayerConnectionStatus((CONNECTION_STATUS)i, NET_ALL_PLAYERS))
		{
			statusMask |= 1<<i;
			++total;
		}
	}

	if (prevStatusMask != 0 && statusMask != prevStatusMask)
	{
		// Remove the icons.
		for (i = 0; i < CONNECTIONSTATUS_NORMAL; ++i)
		{
			if ((statusMask & 1<<i) != 0)
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
		sFormInit.x = (int)(pie_GetVideoBufferWidth() - 52);
		sFormInit.y = 80;
		sFormInit.width = 36;
		sFormInit.height = (24 + separation)*total - separation;
		sFormInit.pDisplay = NetworkDisplayPlainForm;  // NetworkDisplayPlainForm used to do something, but it looks ugly.
		if (!widgAddForm(psWScreen, &sFormInit))
		{
			//return false;
		}

		/* Now add the buttons */
		for (i = 0; i < CONNECTIONSTATUS_NORMAL; ++i)
		{
			if ((statusMask & 1<<i) == 0)
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
			sButInit.y = (24 + separation)*n;
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
void draw3DScene( void )
{
	GL_DEBUG("Draw 3D scene - start");

	/* What frame number are we on? */
	currentGameFrame = frameGetFrameNumber();

	// Tell shader system what the time is
	pie_SetShaderTime(graphicsTime);

	/* Build the drag quad */
	if(dragBox3D.status == DRAG_RELEASED)
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

	// draw terrain
	displayTerrain();

	pie_BeginInterface();
	drawDroidSelections();

	drawStructureSelections();

	if (radarOnScreen && radarPermitted)
	{
		pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);
		pie_SetFogStatus(false);
		if (getWidgetsStatus())
		{
			drawRadar();
		}
		pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);
		pie_SetFogStatus(true);
	}

	if (!bRender3DOnly)
	{
		/* Ensure that any text messages are displayed at bottom of screen */
		pie_SetFogStatus(false);
		displayConsoleMessages();
	}

	pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_OFF);
	pie_SetFogStatus(false);
	iV_SetTextColour(WZCOL_TEXT_BRIGHT);

	/* Dont remove this folks!!!! */
	if(!bAllowOtherKeyPresses)
	{
		displayMultiChat();
	}
	if (errorWaiting)
	{
		if (lastErrorTime + (60 * GAME_TICKS_PER_SEC) < realTime)
		{
			char trimMsg[255];
			audio_PlayTrack(ID_SOUND_BUILD_FAIL);
			ssprintf(trimMsg, "Error! (Check your logs!): %.78s", errorMessage);
			addConsoleMessage(trimMsg, DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
			errorWaiting = false;
		}
	}
	if (showSAMPLES)		//Displays the number of sound samples we currently have
	{
		iV_SetFont(font_regular);
		unsigned int width, height;
		const char *Qbuf, *Lbuf, *Abuf;

		sasprintf((char**)&Qbuf,"Que: %04u",audio_GetSampleQueueCount());
		sasprintf((char**)&Lbuf,"Lst: %04u",audio_GetSampleListCount());
		sasprintf((char**)&Abuf,"Act: %04u",sound_GetActiveSamplesCount());
		width = iV_GetTextWidth(Qbuf) + 11;
		height = iV_GetTextHeight(Qbuf);

		iV_DrawText(Qbuf, pie_GetVideoBufferWidth() - width, height + 2);
		iV_DrawText(Lbuf, pie_GetVideoBufferWidth() - width, height + 48);
		iV_DrawText(Abuf, pie_GetVideoBufferWidth() - width, height + 59);
	}
	if (showFPS)
	{
		iV_SetFont(font_regular);
		unsigned int width, height;
		const char* fps;
		sasprintf((char**)&fps, "FPS: %d", frameRate());
		width = iV_GetTextWidth(fps) + 10;
		height = iV_GetTextHeight(fps);

		iV_DrawText(fps, pie_GetVideoBufferWidth() - width, pie_GetVideoBufferHeight() - height);
	}
	if (showORDERS)
	{
		iV_SetFont(font_regular);
		unsigned int height;
		height = iV_GetTextHeight(DROIDDOING);
		iV_DrawText(DROIDDOING, 0, pie_GetVideoBufferHeight()- height);
	}

	setupConnectionStatusForm();

	if (getWidgetsStatus() && !gamePaused())
	{
		char buildInfo[255];
		if (showLevelName)
		{
			iV_SetFont(font_small);
			iV_SetTextColour(WZCOL_TEXT_MEDIUM);
			iV_DrawText( getLevelName(), RET_X + 134, 410 + E_H );
		}
		getAsciiTime(buildInfo, graphicsTime);
		iV_DrawText( buildInfo, RET_X + 134, 422 + E_H );

		if (getDebugMappingStatus())
		{
			iV_DrawText( "DEBUG ", RET_X + 134, 436 + E_H );
		}
	}

	while(player.r.y>DEG(360))
	{
		player.r.y-=DEG(360);
	}

	/* If we don't have an active camera track, then track terrain height! */
	if(!getWarCamStatus())
	{
		/* Move the autonomous camera if necessary */
		calcAverageTerrainHeight(&player);
		trackHeight(averageCentreTerrainHeight+CAMERA_PIVOT_HEIGHT);
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
		char *crash = 0;
#ifdef DEBUG
		ASSERT(false, "Yes, this is a assert.  This should not happen on release builds! Use --noassert to bypass in debug builds.");
		debug(LOG_WARNING, " *** Warning!  You have compiled in debug mode! ***");
#endif
		writeGameInfo("WZdebuginfo.txt");		//also test writing out this file.
		debug(LOG_FATAL, "Forcing a segfault! (crash handler test)");
		// and here comes the crash
		*crash = 0x3;
		exit(-1);	// will never reach this, but just in case...
	}
	//visualize radius if needed
	if (bRangeDisplay)
	{
		drawRangeAtPos(rangeCenterX,rangeCenterY,rangeRadius);
	}

	if (showPath)
	{
		showDroidPaths();
	}

	GL_DEBUG("Draw 3D scene - end");
}


/// Draws the 3D textured terrain
static void displayTerrain(void)
{
	pie_PerspectiveBegin();

	/* Now, draw the terrain */
	drawTiles(&player);

	pie_PerspectiveEnd();

	/* Show the drag Box if necessary */
	drawDragBox();

	/* Have we released the drag box? */
	if(dragBox3D.status == DRAG_RELEASED)
	{
		dragBox3D.status = DRAG_INACTIVE;
	}
}

/***************************************************************************/
bool	doWeDrawProximitys( void )
{
	return(bDrawProximitys);
}
/***************************************************************************/
void	setProximityDraw(bool val)
{
	bDrawProximitys = val;
}
/***************************************************************************/
/// Calculate the average terrain height for the area directly below the player
static void calcAverageTerrainHeight(iView *player)
{
	int numTilesAveraged = 0, i, j;

	/*	We track the height here - so make sure we get the average heights
		of the tiles directly underneath us
	*/
	averageCentreTerrainHeight = 0;
	for (i = -4; i <= 4; i++)
	{
		for (j = -4; j <= 4; j++)
		{
			if (tileOnMap(playerXTile + j, playerZTile + i))
			{
				/* Get a pointer to the tile at this location */
				MAPTILE *psTile = mapTile(playerXTile + j, playerZTile + i);

				averageCentreTerrainHeight += psTile->height;
				numTilesAveraged++;
			}
		}
	}
	/* Work out the average height. We use this information to keep the player camera
	 * above the terrain. */
	if (numTilesAveraged) // might not be if off map
	{
		MAPTILE *psTile = mapTile(playerXTile, playerZTile);

		averageCentreTerrainHeight /= numTilesAveraged;
		if (averageCentreTerrainHeight < psTile->height)
		{
			averageCentreTerrainHeight = psTile->height;
		}
	}
	else
	{
		averageCentreTerrainHeight = ELEVATION_SCALE * TILE_UNITS;
	}
}

/// Draw the terrain and all droids, missiles and other objects on it
static void drawTiles(iView *player)
{
	int i, j;
	int idx, jdx;
	Vector3f theSun;

	GL_DEBUG("Draw 3D scene - drawTiles");

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
	/* Push identity matrix onto stack */
	pie_MatBegin();

	actualCameraPosition = Vector3i(0, 0, 0);

	/* Set the camera position */
	pie_TRANSLATE(0, 0, distance);

	actualCameraPosition.z -= distance;

	// Now, scale the world according to what resolution we're running in
	pie_MatScale(pie_GetResScalingFactor() / 100.f);

	actualCameraPosition.z /= pie_GetResScalingFactor() / 100.f;

	/* Rotate for the player */
	pie_MatRotZ(player->r.z);
	pie_MatRotX(player->r.x);
	pie_MatRotY(player->r.y);

	rotateSomething(actualCameraPosition.x, actualCameraPosition.y, -player->r.z);
	rotateSomething(actualCameraPosition.y, actualCameraPosition.z, -player->r.x);
	rotateSomething(actualCameraPosition.z, actualCameraPosition.x, -player->r.y);

	/* Translate */
	pie_TRANSLATE(0, -player->p.y, 0);

	actualCameraPosition.y -= -player->p.y;

	// Not sure if should do this here or whenever using, since this transform seems to be done all over the place.
	//actualCameraPosition -= Vector3i(-player->p.x, 0, player->p.z);

	// this also detemines the length of the shadows
	theSun = getTheSun();
	pie_BeginLighting(&theSun, getDrawShadows());

	// update the fog of war... FIXME: Remove this
	for (i = -visibleTiles.y/2, idx=0; i <= visibleTiles.y/2; i++,++idx)
	{
		/* Go through the x's */
		for (j = -visibleTiles.x/2, jdx=0; j <= visibleTiles.x/2; j++,++jdx)
		{
			if (tileOnMap(playerXTile + j, playerZTile + i))
			{
				MAPTILE *psTile = mapTile(playerXTile + j, playerZTile + i);
				setTileColour(playerXTile + j, playerZTile + i, pal_SetBrightness(psTile->level));
			}
		}
	}

	/* This is done here as effects can light the terrain - pause mode problems though */
	processEffects();
	atmosUpdateSystem();
	avUpdateTiles();

	// now we are about to draw the terrain
	GL_DEBUG("Draw 3D scene - terrain");
	pie_SetAlphaTest(false);
	pie_SetFogStatus(true);

	pie_MatBegin();
	// also, make sure we can use world coordinates directly
	pie_TRANSLATE(-player->p.x, 0, player->p.z);

	// and draw it
	drawTerrain();

	// and to the warzone modelview transform
	pie_MatEnd();

	// draw skybox
	renderSurroundings();

	// and prepare for rendering the models
	GL_DEBUG("Draw 3D scene - models");
	pie_SetRendMode(REND_OPAQUE);
	pie_SetAlphaTest(true);

	/* ---------------------------------------------------------------- */
	/* Now display all the static objects                               */
	/* ---------------------------------------------------------------- */
	displayStaticObjects(); // bucket render implemented
	displayFeatures(); // bucket render implemented
	displayDynamicObjects(); //bucket render implemented
	if(doWeDrawProximitys())
	{
		displayProximityMsgs(); // bucket render implemented
	}
	displayDelivPoints(); // bucket render implemented
	display3DProjectiles(); // bucket render implemented

	GL_DEBUG("Draw 3D scene - particles");
	atmosDrawParticles();

	// prepare for the water and the lightmap
	pie_SetAlphaTest(false);
	pie_SetFogStatus(true);

	pie_MatBegin();
	// also, make sure we can use world coordinates directly
	pie_TRANSLATE(-player->p.x, 0, player->p.z);

	drawWater();

	// and to the warzone modelview transform
	pie_MatEnd();

	GL_DEBUG("Draw 3D scene - bucket render");
	bucketRenderCurrentList();

	GL_DEBUG("Draw 3D scene - blue prints");
	displayBlueprints();

	pie_RemainingPasses(); // draws shadows and transparent shapes

	pie_EndLighting();

	if(!gamePaused())
	{
		doConstructionLines();
	}

	locateMouse();

	/* Clear the matrix stack */
	pie_MatEnd();

	GL_DEBUG("Draw 3D scene - end of tiles");
}

/// Initialise the fog, skybox and some other stuff
bool init3DView(void)
{
	/* Arbitrary choice - from direct read! */
	Vector3f theSun(225.0f, -600.0f, 450.0f);

	setTheSun(theSun);

	/* There are no drag boxes */
	dragBox3D.status = DRAG_INACTIVE;

	/* Get all the init stuff out of here? */
	initWarCam();

	/* Init the game messaging system */
	initConsoleMessages();

	atmosInitSystem();

	// Set the initial fog distance
	UpdateFogDistance(distance);

	bRender3DOnly = false;

	setSkyBox("page-25", 0.0f, 10000.0f);

	// distance is not saved, so initialise it now
	distance = START_DISTANCE; // distance

	disp3d_resetView();	// clear player view variables

	if (!initTerrain())
	{
		return false;
	}

	return true;
}


/// set the view position from save game
void disp3d_setView(iView *newView)
{
	memcpy(&player,newView,sizeof(iView));
}

/// reset the camera rotation (used for save games <= 10)
void disp3d_resetView()
{
	player.r.z = 0; // roll
	player.r.y = INITIAL_DESIRED_ROTATION; // rotation
	player.r.x = DEG(360 + INITIAL_STARTING_PITCH); // angle

	// and set the camera position
	player.p.y = START_HEIGHT; // height
}

/// get the view position for save game
void disp3d_getView(iView *newView)
{
	memcpy(newView,&player,sizeof(iView));
}

/// Are the current tile coordinates visible on screen?
bool clipXY(SDWORD x, SDWORD y)
{
	// +2 for edge of visibility fading (see terrain.cpp)
	if (std::abs(x-player.p.x) < world_coord(visibleTiles.x/2+2) &&
		std::abs(y-player.p.z) < world_coord(visibleTiles.y/2+2))
		return(true);
	else
		return(false);
}

/** Get the screen coordinates for the current transform matrix.
 * This function is used to determine the area the user can click for the
 * intelligence screen buttons. The radius parameter is always set to the same value.
 */
static void	calcFlagPosScreenCoords(SDWORD *pX, SDWORD *pY, SDWORD *pR)
{
	/* Get it's absolute dimensions */
	Vector3i center3d(0, 0, 0);
	Vector2i center2d(0, 0);
	/* How big a box do we want - will ultimately be calculated using xmax, ymax, zmax etc */
	UDWORD	radius = 22;

	/* Pop matrices and get the screen coordinates for last point*/
	pie_RotateProject( &center3d, &center2d );

	/*store the coords*/
	*pX = center2d.x;
	*pY = center2d.y;
	*pR = radius;
}

/// Decide whether to render a projectile, and make sure it will be drawn
static void display3DProjectiles( void )
{
	PROJECTILE		*psObj;

	psObj = proj_GetFirst();

	while ( psObj != NULL )
	{
		// If source or destination is visible, and projectile has been spawned and has not impacted.
		if (graphicsTime >= psObj->prevSpacetime.time && graphicsTime <= psObj->time && gfxVisible(psObj))
		{
			/* Draw a bullet at psObj->pos.x for X coord
			   psObj->pos.y for Z coord
			   whatever for Y (height) coord - arcing ?
			*/
			/* these guys get drawn last */
			if(psObj->psWStats->weaponSubClass == WSC_ROCKET ||
			   psObj->psWStats->weaponSubClass == WSC_MISSILE ||
			   psObj->psWStats->weaponSubClass == WSC_COMMAND ||
			   psObj->psWStats->weaponSubClass == WSC_SLOWMISSILE ||
			   psObj->psWStats->weaponSubClass == WSC_SLOWROCKET ||
			   psObj->psWStats->weaponSubClass == WSC_ENERGY ||
			   psObj->psWStats->weaponSubClass == WSC_EMP)
			{
				bucketAddTypeToList(RENDER_PROJECTILE, psObj);
			}
			else
			{
				renderProjectile(psObj);
			}
		}

		psObj = proj_GetNext();
	}
}	/* end of function display3DProjectiles */

/// Draw a projectile to the screen
void	renderProjectile(PROJECTILE *psCurr)
{
	WEAPON_STATS	*psStats;
	Vector3i			dv;
	iIMDShape		*pIMD;
	Spacetime       st;

	psStats = psCurr->psWStats;
	/* Reject flame or command since they have interim drawn fx */
	if(psStats->weaponSubClass == WSC_FLAME ||
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
	pIMD = psStats->pInFlightGraphic;

	if (clipXY(st.pos.x, st.pos.y))
		for (; pIMD != NULL; pIMD = pIMD->next)
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

		/* Get bullet's x coord */
		dv.x = st.pos.x - player.p.x;

		/* Get it's y coord (z coord in the 3d world */
		dv.z = -(st.pos.y - player.p.z);

		/* What's the present height of the bullet? */
		dv.y = st.pos.z;
		/* Set up the matrix */
		pie_MatBegin();
		Vector3i camera = actualCameraPosition;

		/* Translate to the correct position */
		pie_TRANSLATE(dv.x,dv.y,dv.z);
		camera -= Vector3i(dv.x, dv.y, dv.z);

		/* Rotate it to the direction it's facing */
		pie_MatRotY(-st.rot.direction);
		rotateSomething(camera.z, camera.x, -(-st.rot.direction));

		/* pitch it */
		pie_MatRotX(st.rot.pitch);
		rotateSomething(camera.y, camera.z, -st.rot.pitch);

		if (pitchToCamera || rollToCamera)
		{
			// Centre on projectile (relevant for twin projectiles).
			pie_TRANSLATE(pIMD->connectors[0].x, pIMD->connectors[0].y, pIMD->connectors[0].z);
			camera -= Vector3i(pIMD->connectors[0].x, pIMD->connectors[0].y, pIMD->connectors[0].z);
		}

		if (pitchToCamera)
		{
			int x = iAtan2(camera.z, camera.y);
			pie_MatRotX(x);
			rotateSomething(camera.y, camera.z, -x);
		}

		if (rollToCamera)
		{
			int z = -iAtan2(camera.x, camera.y);
			pie_MatRotZ(z);
			rotateSomething(camera.x, camera.y, -z);
		}

		if (pitchToCamera || rollToCamera)
		{
			// Undo centre on projectile (relevant for twin projectiles).
			pie_TRANSLATE(-pIMD->connectors[0].x, -pIMD->connectors[0].y, -pIMD->connectors[0].z);
			camera -= Vector3i(-pIMD->connectors[0].x, -pIMD->connectors[0].y, -pIMD->connectors[0].z);
		}

		if (premultiplied)
		{
			pie_Draw3DShape(pIMD, 0, 0, WZCOL_WHITE, pie_PREMULTIPLIED, 0);
		}
		else if (additive)
		{
			pie_Draw3DShape(pIMD, 0, 0, WZCOL_WHITE, pie_ADDITIVE, 164);
		}
		else
		{
			pie_Draw3DShape(pIMD, 0, 0, WZCOL_WHITE, 0, 0);
		}

		pie_MatEnd();
	}
	/* Flush matrices */
}

/// Draw an animated component
void	renderAnimComponent( const COMPONENT_OBJECT *psObj )
{
	BASE_OBJECT *psParentObj = (BASE_OBJECT*)psObj->psParent;
	Spacetime spacetime = interpolateObjectSpacetime(psParentObj, graphicsTime);
	const SDWORD posX = spacetime.pos.x + psObj->position.x,
	             posY = spacetime.pos.y + psObj->position.y;

	ASSERT( psParentObj != NULL, "renderAnimComponent: invalid parent object pointer" );

	/* only draw visible bits */
	if (psParentObj->type == OBJ_DROID && ((DROID*)psParentObj)->visible[selectedPlayer] != UBYTE_MAX)
	{
		return;
	}

	/* render */
	if( clipXY( posX, posY ) )
	{
		/* get parent object translation */
		const Vector3i dv(
			spacetime.pos.x - player.p.x,
			spacetime.pos.z,
			-(spacetime.pos.y - player.p.z)
		);
		int iPlayer, pieFlag = pie_STATIC_SHADOW;
		PIELIGHT brightness;
		MAPTILE *psTile = worldTile(psParentObj->pos.x, psParentObj->pos.y);

		if (psTile->jammerBits & alliancebits[psParentObj->player])
		{
			pieFlag |= pie_ECM;
		}

		psParentObj->sDisplay.frameNumber = currentGameFrame;

		/* Push the indentity matrix */
		pie_MatBegin();

		/* parent object translation */
		pie_TRANSLATE(dv.x, dv.y, dv.z);

		/* parent object rotations */
		pie_MatRotY(-spacetime.rot.direction);
		pie_MatRotX(spacetime.rot.pitch);

		/* Set frame numbers - look into this later?? FIXME!!!!!!!! */
		if( psParentObj->type == OBJ_DROID )
		{
			DROID *psDroid = (DROID*)psParentObj;
			if ( psDroid->droidType == DROID_PERSON )
			{
				iPlayer = psParentObj->player - 6;
				pie_MatScale(.75f);
			}
			else
			{
				iPlayer = getPlayerColour(psParentObj->player);
			}

			/* Get the onscreen coordinates so we can draw a bounding box */
			calcScreenCoords( psDroid );
		}
		else
		{
			iPlayer = getPlayerColour(psParentObj->player);
		}

		//brightness and fog calculation
		if (psParentObj->type == OBJ_STRUCTURE)
		{
			const Vector3i zero(0, 0, 0);
			Vector2i s(0, 0);
			STRUCTURE *psStructure = (STRUCTURE*)psParentObj;

			brightness = structureBrightness(psStructure);

			pie_RotateProject( &zero, &s );
			psStructure->sDisplay.screenX = s.x;
			psStructure->sDisplay.screenY = s.y;
		}
		else
		{
			brightness = pal_SetBrightness(UBYTE_MAX);
			if (!getRevealStatus())
			{
				brightness = pal_SetBrightness(avGetObjLightLevel((BASE_OBJECT*)psParentObj, brightness.byte.r));
			}
		}

		// Do translation and rotation after setting sDisplay.screen[XY], so that the health bars for animated objects (such as oil derricks and cyborgs) will show on the stationary part.
		// object (animation) translations - ivis z and y flipped
		pie_TRANSLATE(psObj->position.x, psObj->position.z, psObj->position.y);
		// object (animation) rotations
		pie_MatRotY(-psObj->orientation.z);
		pie_MatRotZ(-psObj->orientation.y);
		pie_MatRotX(-psObj->orientation.x);

		pie_Draw3DShape(psObj->psShape, 0, iPlayer, brightness, pieFlag, 0);

		/* clear stack */
		pie_MatEnd();
	}
}

/// Draw the buildings
void displayStaticObjects( void )
{
	ANIM_OBJECT	*psAnimObj;

	// to solve the flickering edges of baseplates
	pie_SetDepthOffset(-1.0f);

	/* Go through all the players */
	for (unsigned player = 0; player <= MAX_PLAYERS; ++player)
	{
		BASE_OBJECT *list = player < MAX_PLAYERS? apsStructLists[player] : psDestroyedObj;

		/* Now go all buildings for that player */
		for (; list != NULL; list = list->psNext)
			if (list->type == OBJ_STRUCTURE && (list->died == 0 || list->died > graphicsTime))
		{
			STRUCTURE *psStructure = castStructure(list);

			/* Worth rendering the structure? */
			if(clipXY(psStructure->pos.x,psStructure->pos.y))
			{
				if ( psStructure->pStructureType->type == REF_RESOURCE_EXTRACTOR &&
					psStructure->psCurAnim == NULL &&
					(psStructure->currentBuildPts > (SDWORD)psStructure->pStructureType->buildPoints) )
				{
					psStructure->psCurAnim = animObj_Add( psStructure, ID_ANIM_DERIK, 0, 0 );
				}

				if ( psStructure->psCurAnim == NULL ||
						psStructure->psCurAnim->bVisible == false ||
						(psAnimObj = animObj_Find( psStructure,
						psStructure->psCurAnim->uwID )) == NULL )
				{
					renderStructure(psStructure);
				}
				else
				{
					if ( psStructure->visible[selectedPlayer] )
					{
						//check not a resource extractors
						if (psStructure->pStructureType->type != REF_RESOURCE_EXTRACTOR)
						{
							displayAnimation( psAnimObj, false );
						}
						/*check the building is active*/
						else if (psStructure->pFunctionality->resourceExtractor.active)
						{
							displayAnimation( psAnimObj, false );
							if(selectedPlayer == psStructure->player)
							{
								audio_PlayObjStaticTrack(psStructure, ID_SOUND_OIL_PUMP_2);
							}
						}
						else
						{
							/* hold anim on first frame */
							displayAnimation( psAnimObj, true );
							audio_StopObjTrack(psStructure, ID_SOUND_OIL_PUMP_2);
						}

					}
				}
			}
		}
	}
	pie_SetDepthOffset(0.0f);
}

static bool tileHasIncompatibleStructure(MAPTILE const *tile, STRUCTURE_STATS const *stats, int moduleIndex)
{
	STRUCTURE *psStruct = castStructure(tile->psObject);
	if (psStruct == NULL)
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

static void drawLineBuild(STRUCTURE_STATS const *psStats, int left, int right, int up, int down, uint16_t direction, STRUCT_STATES state)
{
	if (left != right && up != down)
	{
		// not a line, so don't draw
		return;
	}

	for (int i = left; i <= right; ++i)
	{
		for (int j = up; j <= down; ++j)
		{
			if (tileHasIncompatibleStructure(mapTile(i,j), psStats, 0))
			{
				continue; // construction has started
			}
			Vector2i pos(world_coord(i) + world_coord(1)/2, world_coord(j) + world_coord(1)/2);
			Blueprint blueprint(psStats, pos, direction, 0, state);
			blueprints.push_back(blueprint);
		}
	}
}

static void renderBuildOrder(DroidOrder const &order, STRUCT_STATES state)
{
	STRUCTURE_STATS const *stats;
	Vector2i pos = order.pos;
	if (order.type == DORDER_BUILDMODULE)
	{
		STRUCTURE const *structure = castStructure(order.psObj);
		if (structure == NULL)
		{
			return;
		}
		stats = getModuleStat(structure);
		pos = removeZ(structure->pos);
	}
	else
	{
		stats = order.psStats;
	}

	if (stats == NULL)
	{
		return;
	}

	//draw the current build site if its a line of structures
	if (order.type == DORDER_LINEBUILD)
	{
		int left, right, up, down;
		// a wall (or something like that)

		left = MIN(map_coord(pos.x), map_coord(order.pos2.x));
		right = MAX(map_coord(pos.x), map_coord(order.pos2.x));
		up = MIN(map_coord(pos.y), map_coord(order.pos2.y));
		down = MAX(map_coord(pos.y), map_coord(order.pos2.y));

		drawLineBuild(stats, left, right, up, down, order.direction, state);
	}
	if ((order.type == DORDER_BUILD || order.type == DORDER_BUILDMODULE) && !tileHasIncompatibleStructure(mapTile(map_coord(pos)), stats, order.index))
	{
		Blueprint blueprint(stats, pos, order.direction, order.index, state);
		blueprints.push_back(blueprint);
	}
}

void displayBlueprints(void)
{
	blueprints.clear();  // Delete old blueprints and draw new ones.

	if ( (buildState == BUILD3D_VALID || buildState == BUILD3D_POS) &&
	     sBuildDetails.x > 0 && sBuildDetails.x < mapWidth &&
	     sBuildDetails.y > 0 && sBuildDetails.y < mapHeight)
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
		if (sBuildDetails.psStats->ref >= REF_STRUCTURE_START
		    && sBuildDetails.psStats->ref < (REF_STRUCTURE_START + REF_RANGE))
		{
			// it's a building
			if (wallDrag.status == DRAG_PLACING || wallDrag.status == DRAG_DRAGGING)
			{
				int left, right, up, down;
				// a wall (or something like that)

				left = MIN(wallDrag.x1, wallDrag.x2);
				right = MAX(wallDrag.x1, wallDrag.x2);
				up = MIN(wallDrag.y1, wallDrag.y2);
				down = MAX(wallDrag.y1, wallDrag.y2);

				drawLineBuild((STRUCTURE_STATS *)sBuildDetails.psStats, left, right, up, down, (player.r.y + 0x2000)&0xC000, state);
			}
			else
			{
				unsigned width, height;
				if (((player.r.y + 0x2000) & 0x4000) == 0)
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
				Vector2i pos(world_coord(sBuildDetails.x)+world_coord(width)/2, world_coord(sBuildDetails.y)+world_coord(height)/2);
				Blueprint blueprint((STRUCTURE_STATS *)sBuildDetails.psStats, pos, (player.r.y + 0x2000)&0xC000, 0, state);
				blueprints.push_back(blueprint);
			}
		}
	}

	// now we draw the blueprints for all ordered buildings
	for (int player = 0; player < MAX_PLAYERS; ++player)
	{
		if (!hasSharedVision(selectedPlayer, player))
		{
			continue;
		}
		STRUCT_STATES state = player == selectedPlayer? SS_BLUEPRINT_PLANNED : SS_BLUEPRINT_PLANNED_BY_ALLY;
		for (DROID *psDroid = apsDroidLists[player]; psDroid; psDroid = psDroid->psNext)
		{
			if (psDroid->droidType == DROID_CONSTRUCT || psDroid->droidType == DROID_CYBORG_CONSTRUCT)
			{
				renderBuildOrder(psDroid->order, state);
				//now look thru' the list of orders to see if more building sites
				for (int order = psDroid->listPendingBegin; order < (int)psDroid->asOrderList.size(); order++)
				{
					renderBuildOrder(psDroid->asOrderList[order], state);
				}
			}
		}
	}

	// Erase duplicate blueprints.
	std::sort(blueprints.begin(), blueprints.end());
	blueprints.erase(std::unique(blueprints.begin(), blueprints.end()), blueprints.end());

	// Actually render everything.
	for (std::vector<Blueprint>::iterator blueprint = blueprints.begin(); blueprint != blueprints.end(); ++blueprint)
	{
		blueprint->renderBlueprint();
	}

	renderDeliveryRepos();
}

/// Draw Factory Delivery Points
void displayDelivPoints(void)
{
	for (FLAG_POSITION *psDelivPoint = apsFlagPosLists[selectedPlayer]; psDelivPoint != NULL; psDelivPoint = psDelivPoint->psNext)
	{
		if (clipXY(psDelivPoint->coords.x, psDelivPoint->coords.y))
		{
			renderDeliveryPoint(psDelivPoint, false);
		}
	}
}

/// Draw the features
void displayFeatures( void )
{
	// player can only be 0 for the features.
	for (unsigned player = 0; player <= 1; ++player)
	{
		BASE_OBJECT *list = player < 1? apsFeatureLists[player] : psDestroyedObj;

		/* Go through all the features */
		for (; list != NULL; list = list->psNext)
			if (list->type == OBJ_FEATURE && (list->died == 0 || list->died > graphicsTime))
		{
			FEATURE *psFeature = castFeature(list);

			/* Is the feature worth rendering? */
			if(clipXY(psFeature->pos.x,psFeature->pos.y))
			{
				renderFeature(psFeature);
			}
		}
	}
}

/// Draw the Proximity messages for the *SELECTED PLAYER ONLY*
void displayProximityMsgs( void )
{
	PROXIMITY_DISPLAY	*psProxDisp;
	VIEW_PROXIMITY		*pViewProximity;
	UDWORD				x, y;

	/* Go through all the proximity Displays*/
	for (psProxDisp = apsProxDisp[selectedPlayer]; psProxDisp != NULL;
		psProxDisp = psProxDisp->psNext)
	{
		if(!(psProxDisp->psMessage->read))
		{
			if (psProxDisp->type == POS_PROXDATA)
			{
				pViewProximity = (VIEW_PROXIMITY*)((VIEWDATA *)psProxDisp->psMessage->
					pViewData)->pData;
				x = pViewProximity->x;
				y = pViewProximity->y;
			}
			else
			{
				if (!psProxDisp->psMessage->pViewData) continue; // sanity check
				x = ((BASE_OBJECT *)psProxDisp->psMessage->pViewData)->pos.x;
				y = ((BASE_OBJECT *)psProxDisp->psMessage->pViewData)->pos.y;
			}
			/* Is the Message worth rendering? */
			if(clipXY(x,y))
			{
				renderProximityMsg(psProxDisp);
			}
		}
	}
}

/// Display an animation
static void displayAnimation( ANIM_OBJECT * psAnimObj, bool bHoldOnFirstFrame )
{
	UWORD i, uwFrame;
	Vector3i vecPos, vecRot, vecScale;
	COMPONENT_OBJECT *psComp;

	for ( i = 0; i < psAnimObj->psAnim->uwObj; i++ )
	{
		if ( bHoldOnFirstFrame == true )
		{
			uwFrame = 0;
			vecPos.x = vecPos.y = vecPos.z = 0;
			vecRot.x = vecRot.y = vecRot.z = 0;
			vecScale.x = vecScale.y = vecScale.z = 0;
		}
		else
		{
			uwFrame = anim_GetFrame3D( psAnimObj->psAnim, i, graphicsTime, psAnimObj->udwStartTime, psAnimObj->udwStartDelay, &vecPos, &vecRot, &vecScale );
		}

		if ( uwFrame != ANIM_DELAYED )
		{
			if ( psAnimObj->psAnim->animType == ANIM_3D_TRANS )
			{
				psComp = &psAnimObj->apComponents[i];
			}
			else
			{
				psComp = &psAnimObj->apComponents[uwFrame];
			}

			psComp->position.x = vecPos.x;
			psComp->position.y = vecPos.y;
			psComp->position.z = vecPos.z;

			psComp->orientation.x = vecRot.x;
			psComp->orientation.y = vecRot.y;
			psComp->orientation.z = vecRot.z;

			bucketAddTypeToList( RENDER_ANIMATION, psComp );
		}
	}
}

/// Draw the droids
void displayDynamicObjects( void )
{
	ANIM_OBJECT	*psAnimObj;

	/* Need to go through all the droid lists */
	for (unsigned player = 0; player <= MAX_PLAYERS; ++player)
	{
		BASE_OBJECT *list = player < MAX_PLAYERS? apsDroidLists[player] : psDestroyedObj;

		for (; list != NULL; list = list->psNext)
			if (list->type == OBJ_DROID && (list->died == 0 || list->died > graphicsTime))
		{
			DROID *psDroid = castDroid(list);

			/* Find out whether the droid is worth rendering */
				if(clipXY(psDroid->pos.x,psDroid->pos.y))
				{
					/* No point in adding it if you can't see it? */
					if (psDroid->visible[selectedPlayer])
					{
						psDroid->sDisplay.frameNumber = currentGameFrame;

						// NOTE! : anything that has multiple (anim) frames *must* use the bucket to render
						// In this case, AFAICT only DROID_CYBORG_SUPER had the issue.  (Same issue as oil pump anim)
						if (psDroid->droidType != DROID_CYBORG_SUPER)
						{
							renderDroid(psDroid);
						}
						else
						{
							bucketAddTypeToList(RENDER_DROID, psDroid);
						}
						/* draw anim if visible */
						if ( psDroid->psCurAnim != NULL &&
							psDroid->psCurAnim->bVisible == true &&
							(psAnimObj = animObj_Find( psDroid,
							psDroid->psCurAnim->uwID )) != NULL )
						{
							displayAnimation( psAnimObj, false );
						}
					}
				} // end clipDroid
		} // end for
	} // end for clan
} // end Fn

/// Sets the player's position and view angle - defaults player rotations as well
void setViewPos( UDWORD x, UDWORD y, WZ_DECL_UNUSED bool Pan )
{
	player.p.x = world_coord(x);
	player.p.z = world_coord(y);
	player.r.z = 0;

	if(getWarCamStatus())
	{
		camToggleStatus();
	}

	scroll();
}

/// Get the player position
Vector2i getPlayerPos()
{
	return removeZ(swapYZ(player.p));
}

/// Set the player position
void setPlayerPos(SDWORD x, SDWORD y)
{
	ASSERT( (x >= 0) && (x < world_coord(mapWidth)) &&
			(y >= 0) && (y < world_coord(mapHeight)),
		"setPlayerPos: position off map" );

	player.p.x = x;
	player.p.z = y;
	player.r.z = 0;
}

/// Set the angle at which the player views the world
void	setViewAngle(SDWORD angle)
{
	player.r.x = DEG(360 + angle);
}

/// Get the distance at which the player views the world
float getViewDistance(void)
{
	return distance;
}

/// Set the distance at which the player views the world
void setViewDistance(float dist)
{
	distance = dist;
}

/// Draw a feature (tree/rock/etc.)
void	renderFeature(FEATURE *psFeature)
{
	SDWORD		rotation;
	PIELIGHT	brightness;
	Vector3i dv;
	bool bForceDraw = ( getRevealStatus() && psFeature->psStats->visibleAtStart);
	int shadowFlags = 0;

	if (!psFeature->visible[selectedPlayer] && !bForceDraw)
	{
		return;
	}

	/* Mark it as having been drawn */
	psFeature->sDisplay.frameNumber = currentGameFrame;

	/* Daft hack to get around the oild derrick issue */
	if (!TileHasFeature(mapTile(map_coord(removeZ(psFeature->pos)))))
	{
		return;
	}

	dv = Vector3i(
		psFeature->pos.x - player.p.x,
		psFeature->pos.z, // features sits at the height of the tile it's centre is on
		-(psFeature->pos.y - player.p.z)
	);

	/* Push the indentity matrix */
	pie_MatBegin();

	/* Translate the feature  - N.B. We can also do rotations here should we require
	buildings to face different ways - Don't know if this is necessary - should be IMO */
	pie_TRANSLATE(dv.x,dv.y,dv.z);

	rotation = psFeature->rot.direction;

	pie_MatRotY(-rotation);

	brightness = pal_SetBrightness(200); //? HUH?

	if (psFeature->psStats->subType == FEAT_SKYSCRAPER)
	{
		objectShimmy((BASE_OBJECT*)psFeature);
	}

	if (bForceDraw)
	{
		brightness = pal_SetBrightness(200);
	}
	else if (!getRevealStatus())
	{
		brightness = pal_SetBrightness(avGetObjLightLevel((BASE_OBJECT*)psFeature, brightness.byte.r));
	}
	if (!hasSensorOnTile(mapTile(map_coord(psFeature->pos.x), map_coord(psFeature->pos.y)), selectedPlayer))
	{
		brightness.byte.r /= 2;
		brightness.byte.g /= 2;
		brightness.byte.b /= 2;
	}

	if (psFeature->psStats->subType == FEAT_BUILDING
		|| psFeature->psStats->subType == FEAT_SKYSCRAPER
		|| psFeature->psStats->subType == FEAT_OIL_DRUM)
	{
		/* these cast a shadow */
		shadowFlags = pie_STATIC_SHADOW;
	}

	pie_Draw3DShape(psFeature->sDisplay.imd, 0, 0, brightness, shadowFlags, 0);

	Vector3i zero(0, 0, 0);
	Vector2i s(0, 0);
	pie_RotateProject(&zero, &s);
	psFeature->sDisplay.screenX = s.x;
	psFeature->sDisplay.screenY = s.y;

	pie_MatEnd();
}

/// 
void renderProximityMsg(PROXIMITY_DISPLAY *psProxDisp)
{
	UDWORD			msgX = 0, msgY = 0;
	Vector3i                dv(0, 0, 0);
	VIEW_PROXIMITY	*pViewProximity = NULL;
	SDWORD			x, y, r;
	iIMDShape		*proxImd = NULL;

	//store the frame number for when deciding what has been clicked on
	psProxDisp->frameNumber = currentGameFrame;

	/* Get it's x and y coordinates so we don't have to deref. struct later */
	if (psProxDisp->type == POS_PROXDATA)
	{
		pViewProximity = (VIEW_PROXIMITY*)((VIEWDATA *)psProxDisp->psMessage->
			pViewData)->pData;
		if (pViewProximity)
		{
			msgX = pViewProximity->x;
			msgY = pViewProximity->y;
			/* message sits at the height specified at input*/
			dv.y = pViewProximity->z + 64;

			/* in case of a beacon message put above objects */
			if(((VIEWDATA *)psProxDisp->psMessage->pViewData)->type == VIEW_BEACON)
			{
				if(TileIsOccupied(mapTile(msgX / TILE_UNITS,msgY / TILE_UNITS)))
					dv.y = pViewProximity->z + 150;
			}

		}
	}
	else if (psProxDisp->type == POS_PROXOBJ)
	{
		msgX = ((BASE_OBJECT *)psProxDisp->psMessage->pViewData)->pos.x;
		msgY = ((BASE_OBJECT *)psProxDisp->psMessage->pViewData)->pos.y;
		/* message sits at the height specified at input*/
		dv.y = ((BASE_OBJECT *)psProxDisp->psMessage->pViewData)->pos.z + 64;
	}
	else
	{
		ASSERT(!"unknown proximity display message type", "Buggered proximity message type");
	}

	dv.x = msgX - player.p.x;
	dv.z = -(msgY - player.p.z);

	/* Push the indentity matrix */
	pie_MatBegin();

	/* Translate the message */
	pie_TRANSLATE(dv.x,dv.y,dv.z);

	//get the appropriate IMD
	if (pViewProximity)
	{
		switch(pViewProximity->proxType)
		{
		case PROX_ENEMY:
			proxImd = getImdFromIndex(MI_BLIP_ENEMY);
			break;
		case PROX_RESOURCE:
			proxImd = getImdFromIndex(MI_BLIP_RESOURCE);
			break;
		case PROX_ARTEFACT:
			proxImd = getImdFromIndex(MI_BLIP_ARTEFACT);
			break;
		default:
			ASSERT(!"unknown proximity display message type", "Buggered proximity message type");
			break;
		}
	}
	else
	{
		//object Proximity displays are for oil resources and artefacts
		ASSERT(((BASE_OBJECT *)psProxDisp->psMessage->pViewData)->type == OBJ_FEATURE, "Invalid object type for proximity display");

		if (((FEATURE *)psProxDisp->psMessage->pViewData)->psStats->subType == FEAT_OIL_RESOURCE)
		{
			//resource
			proxImd = getImdFromIndex(MI_BLIP_RESOURCE);
		}
		else
		{
			//artefact
			proxImd = getImdFromIndex(MI_BLIP_ARTEFACT);
		}
	}

	pie_MatRotY(-player.r.y);
	pie_MatRotX(-player.r.x);

	pie_Draw3DShape(proxImd, getModularScaledGraphicsTime(proxImd->animInterval, proxImd->numFrames), 0, WZCOL_WHITE, pie_ADDITIVE, 192);

	//get the screen coords for determining when clicked on
	calcFlagPosScreenCoords(&x, &y, &r);
	psProxDisp->screenX = x;
	psProxDisp->screenY = y;
	psProxDisp->screenR = r;

	pie_MatEnd();
}

static PIELIGHT getBlueprintColour(STRUCT_STATES state)
{
	switch (state)
	{
		case SS_BLUEPRINT_VALID:
			return WZCOL_BLUEPRINT_VALID;
		case SS_BLUEPRINT_INVALID:
			return WZCOL_BLUEPRINT_INVALID;
		case SS_BLUEPRINT_PLANNED:
			return WZCOL_BLUEPRINT_PLANNED;
		case SS_BLUEPRINT_PLANNED_BY_ALLY:
			return WZCOL_BLUEPRINT_PLANNED_BY_ALLY;
		default:
			debug(LOG_ERROR, "this is not a blueprint");
			return WZCOL_WHITE;
	}
}


/// Draw the structures
void	renderStructure(STRUCTURE *psStructure)
{
	int			i, structX, structY, colour, rotation, frame, animFrame, pieFlag, pieFlagData, ecmFlag = 0;
	PIELIGHT		buildingBrightness;
	Vector3i		dv;
	bool			bHitByElectronic = false;
	bool			defensive = false;
	iIMDShape		*strImd = psStructure->sDisplay.imd;
	MAPTILE			*psTile = worldTile(psStructure->pos.x, psStructure->pos.y);

	if (psStructure->pStructureType->type == REF_WALL || psStructure->pStructureType->type == REF_WALLCORNER
	    || psStructure->pStructureType->type == REF_GATE)
	{
		renderWallSection(psStructure);
		return;
	}
	// If the structure is not truly visible, but we know there is something there, we will instead draw a blip
	if (psStructure->visible[selectedPlayer] < UBYTE_MAX && psStructure->visible[selectedPlayer] > 0)
	{
		dv.x = psStructure->pos.x - player.p.x;
		dv.z = -(psStructure->pos.y - player.p.z);
		dv.y = psStructure->pos.z;
		pie_MatBegin();
		pie_TRANSLATE(dv.x,dv.y,dv.z);
		int frame = graphicsTime / BLIP_ANIM_DURATION + psStructure->id % 8192;  // de-sync the blip effect, but don't overflow the int
		pie_Draw3DShape(getImdFromIndex(MI_BLIP), frame, 0, WZCOL_WHITE, pie_ADDITIVE, psStructure->visible[selectedPlayer] / 2);
		pie_MatEnd();
		return;
	}
	else if (!psStructure->visible[selectedPlayer])
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
	animFrame = 0;

	if (strImd->numFrames > 0)
	{
		// Calculate an animation frame
		animFrame = getModularScaledGraphicsTime(strImd->animInterval, strImd->numFrames);
	}

	// -------------------------------------------------------------------------------

	/* Mark it as having been drawn */
	psStructure->sDisplay.frameNumber = currentGameFrame;

	/* Get it's x and y coordinates so we don't have to deref. struct later */
	structX = psStructure->pos.x;
	structY = psStructure->pos.y;

	dv.x = structX - player.p.x;
	dv.z = -(structY - player.p.z);
	dv.y = psStructure->pos.z;
	/* Push the indentity matrix */
	pie_MatBegin();

	/* Translate the building  - N.B. We can also do rotations here should we require
	buildings to face different ways - Don't know if this is necessary - should be IMO */
	pie_TRANSLATE(dv.x,dv.y,dv.z);

	/* OK - here is where we establish which IMD to draw for the building - luckily static objects,
	* buildings in other words are NOT made up of components - much quicker! */

	rotation = psStructure->rot.direction;
	pie_MatRotY(-rotation);
	if (!defensive
	    && graphicsTime - psStructure->timeLastHit < ELEC_DAMAGE_DURATION
	    && psStructure->lastHitWeapon == WSC_ELECTRONIC )
	{
		bHitByElectronic = true;
	}

	buildingBrightness = structureBrightness(psStructure);

	if (!defensive)
	{
		/* Draw the building's base first */
		if (psStructure->pStructureType->pBaseIMD != NULL)
		{
			if (structureIsBlueprint(psStructure))
			{
				pieFlag = pie_TRANSLUCENT;
				pieFlagData = BLUEPRINT_OPACITY;
			}
			else
			{
				pieFlag = pie_TRANSLUCENT | pie_FORCE_FOG | ecmFlag;
				pieFlagData = 255;
			}
			pie_Draw3DShape(psStructure->pStructureType->pBaseIMD, 0, colour, buildingBrightness, pieFlag, pieFlagData);
		}

		// override
		if(bHitByElectronic)
		{
			buildingBrightness = pal_SetBrightness(150);
		}
	}

	if (bHitByElectronic)
	{
		objectShimmy((BASE_OBJECT *)psStructure);
	}

	//first check if partially built - ANOTHER HACK!
	if (psStructure->status == SS_BEING_BUILT)
	{
		if (psStructure->prebuiltImd != NULL)
		{
			// strImd is a module, so render the already-built part at full height.
			pie_Draw3DShape(psStructure->prebuiltImd, 0, colour, buildingBrightness, pie_SHADOW, 0);
		}
		pie_Draw3DShape(strImd, 0, colour, buildingBrightness, pie_HEIGHT_SCALED | pie_SHADOW, structHeightScale(psStructure) * pie_RAISE_SCALE);
	}
	else
	{
		if (structureIsBlueprint(psStructure))
		{
			pieFlag = pie_TRANSLUCENT;
			pieFlagData = BLUEPRINT_OPACITY;
		}
		else
		{
			pieFlag = pie_STATIC_SHADOW | ecmFlag;
			pieFlagData = 0;
		}
		if (defensive && !structureIsBlueprint(psStructure) && !(strImd->flags & iV_IMD_NOSTRETCH))
		{
			pie_SetShaderStretchDepth(psStructure->pos.z - psStructure->foundationDepth);
		}
		pie_Draw3DShape(strImd, animFrame, colour, buildingBrightness, pieFlag, pieFlagData);
		pie_SetShaderStretchDepth(0);

		// It might have weapons on it
		if (psStructure->sDisplay.imd->nconnectors > 0)
		{
			iIMDShape	*mountImd[STRUCT_MAXWEAPS];
			iIMDShape	*weaponImd[STRUCT_MAXWEAPS];
			iIMDShape	*flashImd[STRUCT_MAXWEAPS];

			for (i = 0; i < STRUCT_MAXWEAPS; i++)
			{
				weaponImd[i] = NULL;//weapon is gun ecm or sensor
				mountImd[i] = NULL;
				flashImd[i] = NULL;
			}
			//get an imd to draw on the connector priority is weapon, ECM, sensor
			//check for weapon
			for (i = 0; i < MAX(1, psStructure->numWeaps); i++)
			{
				if (psStructure->asWeaps[i].nStat > 0)
				{
					const int nWeaponStat = psStructure->asWeaps[i].nStat;

					weaponImd[i] =  asWeaponStats[nWeaponStat].pIMD;
					mountImd[i] =  asWeaponStats[nWeaponStat].pMountGraphic;
					flashImd[i] =  asWeaponStats[nWeaponStat].pMuzzleGraphic;
				}
			}

			// check for ECM
			if (weaponImd[0] == NULL && psStructure->pStructureType->pECM != NULL)
			{
				weaponImd[0] =  psStructure->pStructureType->pECM->pIMD;
				mountImd[0] =  psStructure->pStructureType->pECM->pMountGraphic;
				flashImd[0] = NULL;
			}
			// check for sensor (or repair center)
			bool noRecoil = false;
			if (weaponImd[0] == NULL && psStructure->pStructureType->pSensor != NULL)
			{
				weaponImd[0] =  psStructure->pStructureType->pSensor->pIMD;
				/* No recoil for sensors */
				noRecoil = true;
				mountImd[0]  =  psStructure->pStructureType->pSensor->pMountGraphic;
				flashImd[0] = NULL;
			}

			// flags for drawing weapons
			if (structureIsBlueprint(psStructure))
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
			for (i = 0; i < psStructure->numWeaps || i == 0; i++)
			{
				Rotation rot = structureGetInterpolatedWeaponRotation(psStructure, i, graphicsTime);

				if (weaponImd[i] != NULL)
				{
					pie_MatBegin();
					pie_TRANSLATE(strImd->connectors[i].x, strImd->connectors[i].z, strImd->connectors[i].y);
					pie_MatRotY(-rot.direction);
					int recoilValue = noRecoil? 0 : getRecoil(psStructure->asWeaps[i]);
					if (mountImd[i] != NULL)
					{
						pie_TRANSLATE(0, 0, recoilValue / 3);

						pie_Draw3DShape(mountImd[i], animFrame, colour, buildingBrightness, pieFlag, pieFlagData);
						if(mountImd[i]->nconnectors)
						{
							pie_TRANSLATE(mountImd[i]->connectors->x, mountImd[i]->connectors->z, mountImd[i]->connectors->y);
						}
					}
					pie_MatRotX(rot.pitch);
					pie_TRANSLATE(0, 0, recoilValue);

					pie_Draw3DShape(weaponImd[i], 0, colour, buildingBrightness, pieFlag, pieFlagData);
					if (psStructure->status == SS_BUILT && psStructure->visible[selectedPlayer] > (UBYTE_MAX / 2))
					{
						if (psStructure->pStructureType->type == REF_REPAIR_FACILITY)
						{
							REPAIR_FACILITY* psRepairFac = &psStructure->pFunctionality->repairFacility;
							// draw repair flash if the Repair Facility has a target which it has started work on
							if (weaponImd[i]->nconnectors && psRepairFac->psObj != NULL
								&& psRepairFac->psObj->type == OBJ_DROID)
							{
								DROID *psDroid = (DROID *)psRepairFac->psObj;
								SDWORD xdiff, ydiff;
								xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psStructure->pos.x;
								ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psStructure->pos.y;
								if (xdiff * xdiff + ydiff * ydiff <= (TILE_UNITS*5/2)*(TILE_UNITS*5/2))
								{
									iIMDShape	*pRepImd;

									pie_TRANSLATE(weaponImd[i]->connectors->x,weaponImd[i]->connectors->z-12,weaponImd[i]->connectors->y);
									pRepImd = getImdFromIndex(MI_FLAME);

									pie_MatRotY(rot.direction);

									pie_MatRotY(-player.r.y);
									pie_MatRotX(-player.r.x);
									pie_Draw3DShape(pRepImd, getModularScaledGraphicsTime(pRepImd->animInterval, pRepImd->numFrames), colour, buildingBrightness, pie_ADDITIVE, 192);

									pie_MatRotX(player.r.x);
									pie_MatRotY(player.r.y);
									pie_MatRotY(rot.direction);
								}
							}
						}
						// we have a weapon so we draw a muzzle flash
						if( weaponImd[i]->nconnectors && flashImd[i] && psStructure->pStructureType->type != REF_REPAIR_FACILITY)
						{
							unsigned int connector_num = 0;

							// which barrel is firing if model have multiple muzzle connectors?
							if (psStructure->asWeaps[i].shotsFired && (weaponImd[i]->nconnectors > 1))
							{
								// shoot first, draw later - substract one shot to get correct results
								connector_num = (psStructure->asWeaps[i].shotsFired - 1) % (weaponImd[i]->nconnectors);
							}

							/* Now we need to move to the end of the firing barrel */
							pie_TRANSLATE(weaponImd[i]->connectors[connector_num].x,
											weaponImd[i]->connectors[connector_num].z,
											weaponImd[i]->connectors[connector_num].y);

							// assume no clan colours for muzzle effects
							if (flashImd[i]->numFrames == 0 || flashImd[i]->animInterval <= 0)
							{
								// no anim so display one frame for a fixed time
								if (graphicsTime >= psStructure->asWeaps[i].lastFired && graphicsTime < psStructure->asWeaps[i].lastFired + BASE_MUZZLE_FLASH_DURATION)
								{
									pie_Draw3DShape(flashImd[i], 0, colour, buildingBrightness, pieFlag | pie_ADDITIVE, EFFECT_MUZZLE_ADDITIVE);
								}
							}
							else if (graphicsTime >= psStructure->asWeaps[i].lastFired)
							{
								// animated muzzle
								frame = (graphicsTime - psStructure->asWeaps[i].lastFired)/flashImd[i]->animInterval;
								if (frame < flashImd[i]->numFrames)
								{
									pie_Draw3DShape(flashImd[i], frame, colour, buildingBrightness, pieFlag | pie_ADDITIVE, EFFECT_MUZZLE_ADDITIVE);
								}
							}
						}
					}
					pie_MatEnd();
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
						if (flashImd[i] != NULL)
						{
							pie_MatBegin();
							// horrendous hack
							if (strImd->max.y > 80) // babatower
							{
								pie_TRANSLATE(0, 80, 0);
								pie_MatRotY(-rot.direction);
								pie_TRANSLATE(0, 0, -20);
							}
							else//baba bunker
							{
								pie_TRANSLATE(0, 10, 0);
								pie_MatRotY(-rot.direction);
								pie_TRANSLATE(0, 0, -40);
							}
							pie_MatRotX(rot.pitch);
							// draw the muzzle flash?
							if (psStructure->visible[selectedPlayer] > UBYTE_MAX / 2)
							{
								// animate for the duration of the flash only
								// assume no clan colours for muzzle effects
								if (flashImd[i]->numFrames == 0 || flashImd[i]->animInterval <= 0)
								{
									// no anim so display one frame for a fixed time
									if (graphicsTime >= psStructure->asWeaps[i].lastFired && graphicsTime < psStructure->asWeaps[i].lastFired + BASE_MUZZLE_FLASH_DURATION)
									{
										pie_Draw3DShape(flashImd[i], 0, colour, buildingBrightness, 0, 0); //muzzle flash
									}
								}
								else
								{
									frame = (graphicsTime - psStructure->asWeaps[i].lastFired) / flashImd[i]->animInterval;
									if (frame < flashImd[i]->numFrames && frame >= 0)
									{
										pie_Draw3DShape(flashImd[i], 0, colour, buildingBrightness, 0, 0); //muzzle flash
									}
								}
							}
							pie_MatEnd();
						}
					}
				}
				// if there is an unused connector, but not the first connector, add a light to it
				else if (psStructure->sDisplay.imd->nconnectors > 1)
				{
					for (i = 0; i < psStructure->sDisplay.imd->nconnectors; i++)
					{
						iIMDShape *lImd;

						pie_MatBegin();
						pie_TRANSLATE(psStructure->sDisplay.imd->connectors->x, psStructure->sDisplay.imd->connectors->z,
						             psStructure->sDisplay.imd->connectors->y);
						lImd = getImdFromIndex(MI_LANDING);
						pie_Draw3DShape(lImd, getModularScaledGraphicsTime(lImd->animInterval, lImd->numFrames), colour, buildingBrightness, 0, 0);
						pie_MatEnd();
					}
				}
			}
		}
	}

	{
		Vector3i zero(0, 0, 0);
		Vector2i s(0, 0);

		pie_RotateProject(&zero, &s);
		psStructure->sDisplay.screenX = s.x;
		psStructure->sDisplay.screenY = s.y;
	}

	pie_MatEnd();
}

/// draw the delivery points
void	renderDeliveryPoint(FLAG_POSITION *psPosition, bool blueprint)
{
	Vector3i	dv;
	SDWORD		x, y, r;
	int pieFlag, pieFlagData;
	PIELIGHT colour;

	//store the frame number for when deciding what has been clicked on
	psPosition->frameNumber = currentGameFrame;

	dv.x = psPosition->coords.x - player.p.x;
	dv.z = -(psPosition->coords.y - player.p.z);
	dv.y = psPosition->coords.z;

	/* Push the indentity matrix */
	ScopedPieMatrix matScope;

	pie_TRANSLATE(dv.x,dv.y,dv.z);

	//quick check for invalid data
	ASSERT_OR_RETURN(, psPosition->factoryType < NUM_FLAG_TYPES && psPosition->factoryInc < MAX_FACTORY_FLAG_IMDS, "Invalid assembly point");

	pie_MatScale(.5f); // they are all big now so make this one smaller too

	pieFlag = pie_TRANSLUCENT;
	pieFlagData = BLUEPRINT_OPACITY;

	if (blueprint)
	{
		colour = deliveryReposValid() ? WZCOL_BLUEPRINT_VALID : WZCOL_BLUEPRINT_INVALID;
	}
	else
	{
		pieFlag |= pie_FORCE_FOG;
		colour = WZCOL_WHITE;
	}
	pie_Draw3DShape(pAssemblyPointIMDs[psPosition->factoryType][psPosition->factoryInc], 0, 0, colour, pieFlag, pieFlagData);

	//get the screen coords for the DP
	calcFlagPosScreenCoords(&x, &y, &r);
	psPosition->screenX = x;
	psPosition->screenY = y;
	psPosition->screenR = r;
}

/// Draw a piece of wall
static bool	renderWallSection(STRUCTURE *psStructure)
{
	int			structX, structY, ecmFlag = 0;
	PIELIGHT		brightness;
	SDWORD			rotation;
	Vector3i			dv;
	int				pieFlag, pieFlagData;
	MAPTILE			*psTile = worldTile(psStructure->pos.x, psStructure->pos.y);

	if(psStructure->visible[selectedPlayer])
	{
		if (psTile->jammerBits & alliancebits[psStructure->player])
		{
			ecmFlag = pie_ECM;
		}

		psStructure->sDisplay.frameNumber = currentGameFrame;
		/* Get it's x and y coordinates so we don't have to deref. struct later */
		structX = psStructure->pos.x;
		structY = psStructure->pos.y;

		brightness = structureBrightness(psStructure);
		pie_SetShaderStretchDepth(psStructure->pos.z - psStructure->foundationDepth);

		/* Establish where it is in the world */
		dv.x = structX - player.p.x;
		dv.z = -(structY - player.p.z);
		dv.y = psStructure->pos.z;

		dv.y -= gateCurrentOpenHeight(psStructure, graphicsTime, 1);  // Make gate stick out by 1 unit, so that the tops of ┼ gates can safely have heights differing by 1 unit.

		/* Push the indentity matrix */
		pie_MatBegin();

		/* Translate */
		pie_TRANSLATE(dv.x,dv.y,dv.z);

		rotation = psStructure->rot.direction;
		pie_MatRotY(-rotation);

		/* Actually render it */
		if (psStructure->status == SS_BEING_BUILT)
		{
			pie_Draw3DShape(psStructure->sDisplay.imd, 0, getPlayerColour(psStructure->player),
					brightness, pie_HEIGHT_SCALED|pie_SHADOW|ecmFlag, structHeightScale(psStructure) * pie_RAISE_SCALE);
		}
		else
		{
			if (structureIsBlueprint(psStructure))
			{
				pieFlag = pie_TRANSLUCENT;
				pieFlagData = BLUEPRINT_OPACITY;
			}
			else
			{
				if (psStructure->pStructureType->type == REF_WALL || psStructure->pStructureType->type == REF_GATE)
				{
					// walls can be rotated, so use a dynamic shadow for them
					pieFlag = pie_SHADOW;
				}
				else
				{
					pieFlag = pie_STATIC_SHADOW;
				}
				pieFlagData = 0;
			}
			pie_Draw3DShape(psStructure->sDisplay.imd, 0, getPlayerColour(psStructure->player), brightness, pieFlag|ecmFlag, pieFlagData);
		}

		{
			Vector3i zero(0, 0, 0);
			Vector2i s(0, 0);

			pie_RotateProject( &zero, &s );
			psStructure->sDisplay.screenX = s.x;
			psStructure->sDisplay.screenY = s.y;
		}

		pie_SetShaderStretchDepth(0);
		pie_MatEnd();

		return(true);
	}
	return false;
}

/// Draws a shadow under a droid
void renderShadow( DROID *psDroid, iIMDShape *psShadowIMD )
{
	Vector3i dv;

	dv.x = psDroid->pos.x - player.p.x;
	if(psDroid->droidType == DROID_TRANSPORTER || psDroid->droidType == DROID_SUPERTRANSPORTER)
	{
		dv.x -= bobTransporterHeight()/2;
	}
	dv.z = -(psDroid->pos.y - player.p.z);
	dv.y = map_Height(psDroid->pos.x, psDroid->pos.y);

	/* Push the indentity matrix */
	pie_MatBegin();

	pie_TRANSLATE(dv.x,dv.y,dv.z);

	pie_MatRotY(-psDroid->rot.direction);
	pie_MatRotX(psDroid->rot.pitch);
	pie_MatRotZ(psDroid->rot.roll);

	pie_Draw3DShape(psShadowIMD, 0, 0, WZCOL_WHITE, pie_TRANSLUCENT, 128);

	pie_MatEnd();
}

/// Draw all pieces of a droid and register it as a target
void renderDroid( DROID *psDroid )
{
	displayComponentObject(psDroid);
}

/// Draws the strobing 3D drag box that is used for multiple selection
static void	drawDragBox( void )
{
	int minX, maxX;		// SHURCOOL: These 4 ints will hold the corners of the selection box
	int minY, maxY;

	if(dragBox3D.status == DRAG_DRAGGING && buildState == BUILD3D_NONE)
	{
		if(graphicsTime - dragBox3D.lastTime > BOX_PULSE_SPEED)
		{
			dragBox3D.pulse++;
			if (dragBox3D.pulse >= BOX_PULSE_SIZE)
			{
				dragBox3D.pulse = 0;
			}
			dragBox3D.lastTime = graphicsTime;
		}

		// SHURCOOL: Determine the 4 corners of the selection box, and use them for consistent selection box rendering
		minX = MIN(dragBox3D.x1, mouseX());
		maxX = MAX(dragBox3D.x1, mouseX());
		minY = MIN(dragBox3D.y1, mouseY());
		maxY = MAX(dragBox3D.y1, mouseY());

		// SHURCOOL: Reduce the box in size to produce a (consistent) pulsing inward effect
		minX += dragBox3D.pulse / 2;
		maxX -= dragBox3D.pulse / 2;
		minY += dragBox3D.pulse / 2;
		maxY -= dragBox3D.pulse / 2;

		pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_OFF);
		iV_Box(minX, minY, maxX, maxY, WZCOL_UNIT_SELECT_BORDER);
		pie_UniTransBoxFill(minX + 1, minY, maxX, maxY - 1, WZCOL_UNIT_SELECT_BOX);
		pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);
	}
}


/// Display reload bars for structures and droids
static void drawWeaponReloadBar(BASE_OBJECT *psObj, WEAPON *psWeap, int weapon_slot)
{
	SDWORD			scrX,scrY, scrR, scale;
	STRUCTURE		*psStruct;
	float			mulH;	// display unit resistance instead of reload!
	DROID			*psDroid;
	int			armed, firingStage;

	if (ctrlShiftDown() && (psObj->type == OBJ_DROID))
	{
		psDroid = (DROID*)psObj;
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
			mulH = (float)psDroid->resistance / (float)droidResistance(psDroid);
		}
		else
		{
			mulH = 100.f;
		}
		firingStage = mulH;
		firingStage = ((((2*scrR)*10000)/100)*firingStage)/10000;
		if(firingStage >= (UDWORD)(2*scrR))
		{
			firingStage = (2*scrR);
		}
		pie_BoxFill(scrX - scrR-1, 3+scrY + 0 + (weapon_slot * 2), scrX - scrR +(2*scrR)+1,    3+scrY+3 + (weapon_slot * 2), WZCOL_RELOAD_BACKGROUND);
		pie_BoxFill(scrX - scrR,   3+scrY + 1 + (weapon_slot * 2), scrX - scrR +firingStage, 3+scrY+2 + (weapon_slot * 2), WZCOL_HEALTH_RESISTANCE);
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
		pie_BoxFill(scrX - scrR-1, 3+scrY + 0 + (weapon_slot * 2), scrX - scrR +(2*scrR)+1,  3+scrY+3 + (weapon_slot * 2), WZCOL_RELOAD_BACKGROUND);
		pie_BoxFill(scrX - scrR,   3+scrY + 1 + (weapon_slot * 2), scrX - scrR +firingStage, 3+scrY+2 + (weapon_slot * 2), WZCOL_RELOAD_BAR);
	}
}

/// draw target origin icon for the specified structure
static void drawStructureTargetOriginIcon(STRUCTURE *psStruct, int weapon_slot)
{
	SDWORD		scrX,scrY,scrR;
	UDWORD		scale;

	// Process main weapon only for now
	if (!tuiTargetOrigin || weapon_slot || !((psStruct->asWeaps[weapon_slot]).nStat))
	{
		return;
	}

	scale = MAX(psStruct->pStructureType->baseWidth, psStruct->pStructureType->baseBreadth);
	scrX = psStruct->sDisplay.screenX;
	scrY = psStruct->sDisplay.screenY + (scale*10);
	scrR = scale*20;

	/* Render target origin graphics */
	switch(psStruct->targetOrigin[weapon_slot])
	{
	case ORIGIN_VISUAL:
		iV_DrawImage(IntImages, IMAGE_ORIGIN_VISUAL, scrX+scrR+5, scrY-1);
		break;
	case ORIGIN_COMMANDER:
		iV_DrawImage(IntImages, IMAGE_ORIGIN_COMMANDER, scrX+scrR+5, scrY-1);
		break;
	case ORIGIN_SENSOR:
		iV_DrawImage(IntImages, IMAGE_ORIGIN_SENSOR_STANDARD, scrX+scrR+5, scrY-1);
		break;
	case ORIGIN_CB_SENSOR:
		iV_DrawImage(IntImages, IMAGE_ORIGIN_SENSOR_CB, scrX+scrR+5, scrY-1);
		break;
	case ORIGIN_AIRDEF_SENSOR:
		iV_DrawImage(IntImages, IMAGE_ORIGIN_SENSOR_AIRDEF, scrX+scrR+5, scrY-1);
		break;
	case ORIGIN_RADAR_DETECTOR:
		iV_DrawImage(IntImages, IMAGE_ORIGIN_RADAR_DETECTOR, scrX+scrR+5, scrY-1);
		break;
	case ORIGIN_UNKNOWN:
		// Do nothing
		break;
	default:
		debug(LOG_WARNING,"Unexpected target origin in structure(%d)!", psStruct->id);
	}
}

/// draw the health bar for the specified structure
static void drawStructureHealth(STRUCTURE *psStruct)
{
	SDWORD		scrX,scrY,scrR;
	PIELIGHT	powerCol = WZCOL_BLACK, powerColShadow = WZCOL_BLACK;
	UDWORD		health,width;
	UDWORD		scale;

	scale = MAX(psStruct->pStructureType->baseWidth, psStruct->pStructureType->baseBreadth);
	width = scale*20;
	scrX = psStruct->sDisplay.screenX;
	scrY = psStruct->sDisplay.screenY + (scale*10);
	scrR = width;
	//health = PERCENT(psStruct->body, psStruct->baseBodyPoints);
	if (ctrlShiftDown())
	{
		//show resistance values if CTRL/SHIFT depressed
		UDWORD  resistance = structureResistance(
			psStruct->pStructureType, psStruct->player);
		if (resistance)
		{
			health = PERCENT(MAX(0,psStruct->resistance), resistance);
		}
		else
		{
			health = 100;
		}
	}
	else
	{
		//show body points
		health = (1. - getStructureDamage(psStruct)/65536.f) * 100;

		// If structure is incomplete, make bar correspondingly thinner.
		int maxBody = structureBody(psStruct);
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
	health = (((width*10000)/100)*health)/10000;
	health*=2;
	pie_BoxFill(scrX-scrR-1, scrY-1, scrX-scrR+2*width+1, scrY+3, WZCOL_RELOAD_BACKGROUND);
	pie_BoxFill(scrX-scrR, scrY, scrX-scrR+health, scrY+1, powerCol);
	pie_BoxFill(scrX-scrR, scrY+1, scrX-scrR+health, scrY+2, powerColShadow);
}

/// draw the construction bar for the specified structure
static void drawStructureBuildProgress(STRUCTURE *psStruct)
{
	SDWORD		scrX,scrY,scrR;
	UDWORD		health,width;
	UDWORD		scale;

	scale = MAX(psStruct->pStructureType->baseWidth, psStruct->pStructureType->baseBreadth);
	width = scale*20;
	scrX = psStruct->sDisplay.screenX;
	scrY = psStruct->sDisplay.screenY + (scale*10);
	scrR = width;
	health =  PERCENT(psStruct->currentBuildPts , psStruct->pStructureType->buildPoints);
	health = (((width*10000)/100)*health)/10000;
	health*=2;
	pie_BoxFill(scrX - scrR - 1, scrY - 1 + 5, scrX + scrR + 1, scrY + 3 + 5, WZCOL_RELOAD_BACKGROUND);
	pie_BoxFill(scrX - scrR, scrY + 5, scrX - scrR + health, scrY + 1+5, WZCOL_HEALTH_MEDIUM_SHADOW);
	pie_BoxFill(scrX - scrR, scrY + 1+5, scrX - scrR + health, scrY + 2+5, WZCOL_HEALTH_MEDIUM);
}

/// Draw the health of structures and show enemy structures being targetted
static void	drawStructureSelections( void )
{
	STRUCTURE	*psStruct;
	SDWORD		scrX,scrY;
	UDWORD		i;
	BASE_OBJECT	*psClickedOn;
	bool		bMouseOverStructure = false;
	bool		bMouseOverOwnStructure = false;

	psClickedOn = mouseTarget();
	if(psClickedOn!=NULL && psClickedOn->type == OBJ_STRUCTURE)
	{
		bMouseOverStructure = true;
		if(psClickedOn->player == selectedPlayer)
		{
			bMouseOverOwnStructure = true;
		}
	}
	pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);
	pie_SetFogStatus(false);

	/* Go thru' all the buildings */
	for(psStruct = apsStructLists[selectedPlayer]; psStruct; psStruct = psStruct->psNext)
	{
		if (clipXY(psStruct->pos.x,psStruct->pos.y) && psStruct->sDisplay.frameNumber == currentGameFrame)
		{
			/* If it's selected */
			if (psStruct->selected ||
			    (barMode == BAR_DROIDS_AND_STRUCTURES && psStruct->pStructureType->type != REF_WALL && psStruct->pStructureType->type != REF_WALLCORNER) ||
			    (bMouseOverOwnStructure && psStruct == (STRUCTURE *)psClickedOn)
			   )
			{
				drawStructureHealth(psStruct);
				
				for (i = 0; i < psStruct->numWeaps; i++)
				{
					drawWeaponReloadBar((BASE_OBJECT *)psStruct, &psStruct->asWeaps[i], i);
					drawStructureTargetOriginIcon(psStruct, i);
				}
			}

			if (psStruct->status == SS_BEING_BUILT)
			{
				drawStructureBuildProgress(psStruct);
			}
		}
	}

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (i == selectedPlayer)
		{
			continue;	// not interesting in our own buildings
		}
		for (psStruct = apsStructLists[i]; psStruct; psStruct = psStruct->psNext)
		{
			/* If it's targetted and on-screen */
			if (clipXY(psStruct->pos.x,psStruct->pos.y)
			    && psStruct->bTargetted
			    && psStruct->sDisplay.frameNumber == currentGameFrame)
			{
				scrX = psStruct->sDisplay.screenX;
				scrY = psStruct->sDisplay.screenY - (psStruct->sDisplay.imd->max.y / 4);
				iV_DrawImage(IntImages, getTargettingGfx(), scrX, scrY);
			}
		}
	}

	if(bMouseOverStructure && !bMouseOverOwnStructure)
	{
		if (mouseDown(getRightClickOrders()?MOUSE_LMB:MOUSE_RMB))
		{
			psStruct = (STRUCTURE*)psClickedOn;
			drawStructureHealth(psStruct);
			if(psStruct->status == SS_BEING_BUILT)
			{
				drawStructureBuildProgress(psStruct);
			}
		}
	}

	pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);
}

static UDWORD	getTargettingGfx( void )
{
UDWORD	index;

	index = getModularScaledRealTime(1000, 10);

	switch(index)
	{
	case	0:
	case	1:
	case	2:
		return(IMAGE_TARGET1+index);
		break;
	default:
		if(index & 0x01)
		{
			return(IMAGE_TARGET4);
		}
		else
		{
			return(IMAGE_TARGET5);
		}
		break;
	}
}

/// Is the droid, its commander or its sensor tower selected?
bool	eitherSelected(DROID *psDroid)
{
bool			retVal;
BASE_OBJECT		*psObj;

	retVal = false;
	if(psDroid->selected)
	{
		retVal = true;
	}

	if(psDroid->psGroup)
	{
		if(psDroid->psGroup->psCommander)
		{
			if(psDroid->psGroup->psCommander->selected)
			{
				retVal = true;
			}
		}
	}

	psObj = orderStateObj(psDroid, DORDER_FIRESUPPORT);
	if (psObj
	 && psObj->selected)
	{
		retVal = true;
	}

	return retVal;
}

/// Draw the selection graphics for selected droids
static void	drawDroidSelections( void )
{
	UDWORD			scrX,scrY,scrR;
	DROID			*psDroid;
	UDWORD			damage;
	PIELIGHT		powerCol = WZCOL_BLACK, powerColShadow = WZCOL_BLACK;
	PIELIGHT		boxCol;
	BASE_OBJECT		*psClickedOn;
	bool			bMouseOverDroid = false;
	bool			bMouseOverOwnDroid = false;
	UDWORD			i,index;
	FEATURE			*psFeature;
	float			mulH;

	psClickedOn = mouseTarget();
	if(psClickedOn!=NULL && psClickedOn->type == OBJ_DROID)
	{
		bMouseOverDroid = true;
		if(psClickedOn->player == selectedPlayer && !psClickedOn->selected)
		{
			bMouseOverOwnDroid = true;
		}
	}

	pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);
	pie_SetFogStatus(false);
	for(psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		if (psDroid->sDisplay.frameNumber != currentGameFrame || !clipXY(psDroid->pos.x, psDroid->pos.y))
		{
			continue;  // Not visible, anyway. Don't bother with health bars.
		}

		/* If it's selected and on screen or it's the one the mouse is over ||*/
		// ABSOLUTELY MAD LOGICAL EXPRESSION!!! :-)
		// Now slightly less mad and slightly less buggy.
		if (eitherSelected(psDroid) ||
		    (bMouseOverOwnDroid && psDroid == (DROID *) psClickedOn) ||
		    droidUnderRepair(psDroid) ||
		    barMode == BAR_DROIDS || barMode == BAR_DROIDS_AND_STRUCTURES
		   )
		{
			damage = PERCENT(psDroid->body, psDroid->originalBody);

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
			mulH = (float)psDroid->body / (float)psDroid->originalBody;
			damage = mulH * (float)psDroid->sDisplay.screenR;// (((psDroid->sDisplay.screenR*10000)/100)*damage)/10000;
			if (damage > psDroid->sDisplay.screenR)
				damage = psDroid->sDisplay.screenR;

			damage *=2;
			scrX = psDroid->sDisplay.screenX;
			scrY = psDroid->sDisplay.screenY;
			scrR = psDroid->sDisplay.screenR;

			/* Yeah, yeah yeah - hardcoded palette entries - need to change to #defined colour names */
			/* Three DFX clips properly right now - not sure if software does */
//			if((scrX+scrR)>0 && (scrY+scrR)>0 && (scrX-scrR)<DISP_WIDTH
//				&& (scrY-scrR)<DISP_HEIGHT)
			{
				if(!driveModeActive() || driveIsDriven(psDroid)) {
					boxCol = WZCOL_WHITE;
				} else {
					boxCol = WZCOL_GREEN;
				}

				if (psDroid->selected)
				{
					pie_BoxFill(scrX - scrR, scrY + scrR - 7, scrX - scrR+1, scrY + scrR, boxCol);
					pie_BoxFill(scrX - scrR, scrY + scrR, scrX - scrR + 7, scrY + scrR + 1, boxCol);
					pie_BoxFill(scrX + scrR - 7, scrY + scrR, scrX + scrR, scrY + scrR + 1, boxCol);
					pie_BoxFill(scrX + scrR, scrY + scrR - 7, scrX + scrR + 1, scrY + scrR + 1, boxCol);
				}

				/* Power bars */
				pie_BoxFill(scrX - scrR - 1, scrY + scrR+2, scrX + scrR + 1, scrY + scrR + 6, WZCOL_RELOAD_BACKGROUND);
				pie_BoxFill(scrX - scrR, scrY + scrR+3, scrX - scrR + damage, scrY + scrR + 4, powerCol);
				pie_BoxFill(scrX - scrR, scrY + scrR+4, scrX - scrR + damage, scrY + scrR + 5, powerColShadow);

				/* Write the droid rank out */
				if (	(scrX + scrR) > 0
					&&	(scrX - scrR) < pie_GetVideoBufferWidth()
					&&	(scrY + scrR) > 0
					&&	(scrY - scrR) < pie_GetVideoBufferHeight())
				{
					drawDroidRank(psDroid);
					drawDroidSensorLock(psDroid);
					drawDroidCmndNo(psDroid);
					drawDroidGroupNumber(psDroid);
				}
			}

			for (i = 0;i < psDroid->numWeaps;i++)
			{
				drawWeaponReloadBar((BASE_OBJECT *)psDroid, &psDroid->asWeaps[i], i);
			}
		}
	}

	/* Are we over an enemy droid */
	if(bMouseOverDroid && !bMouseOverOwnDroid)
	{
		if (mouseDown(getRightClickOrders()?MOUSE_LMB:MOUSE_RMB))
		{
			if(psClickedOn->player!=selectedPlayer && psClickedOn->sDisplay.frameNumber == currentGameFrame)
			{
				psDroid = (DROID*)psClickedOn;
				//show resistance values if CTRL/SHIFT depressed
				if (ctrlShiftDown())
				{
					if (psDroid->resistance)
					{
						damage = PERCENT(psDroid->resistance, droidResistance(psDroid));
					}
					else
					{
						damage = 100;
					}
				}
				else
				{
					damage = PERCENT(psDroid->body,psDroid->originalBody);
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
						mulH = (float)psDroid->resistance / (float)droidResistance(psDroid);
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
				damage = mulH * (float)psDroid->sDisplay.screenR;// (((psDroid->sDisplay.screenR*10000)/100)*damage)/10000;
				if (damage > psDroid->sDisplay.screenR)
					damage = psDroid->sDisplay.screenR;
				damage *=2;
				scrX = psDroid->sDisplay.screenX;
				scrY = psDroid->sDisplay.screenY;
				scrR = psDroid->sDisplay.screenR;

				/* Yeah, yeah yeah - hardcoded palette entries - need to change to #defined colour names */
				/* Three DFX clips properly right now - not sure if software does */
				if (	(scrX + scrR) > 0
					&&	(scrX - scrR) < pie_GetVideoBufferWidth()
					&&	(scrY + scrR) > 0
					&&	(scrY - scrR) < pie_GetVideoBufferHeight())
				{
					if(!driveModeActive() || driveIsDriven(psDroid)) {
						boxCol = WZCOL_WHITE;
					} else {
						boxCol = WZCOL_GREEN;
					}

					//we always want to show the enemy health/resistance as energyBar - AB 18/06/99
					//if(bEnergyBars)
					{
						/* Power bars */
						pie_BoxFill(scrX - scrR - 1, scrY + scrR + 2, scrX + scrR + 1, scrY + scrR + 6, WZCOL_RELOAD_BACKGROUND);
						pie_BoxFill(scrX - scrR, scrY + scrR+3, scrX - scrR + damage, scrY + scrR + 4, powerCol);
						pie_BoxFill(scrX - scrR, scrY + scrR+4, scrX - scrR + damage, scrY + scrR + 5, powerColShadow);
					}
				}
			}
		}
	}

	for(i=0; i<MAX_PLAYERS; i++)
	{
		/* Go thru' all the droidss */
		for(psDroid = apsDroidLists[i]; psDroid; psDroid = psDroid->psNext)
		{
			if(i!=selectedPlayer && !psDroid->died && psDroid->sDisplay.frameNumber == currentGameFrame)
			{
				/* If it's selected */
				if(psDroid->bTargetted && (psDroid->visible[selectedPlayer] == UBYTE_MAX))
				{
					scrX = psDroid->sDisplay.screenX;
					scrY = psDroid->sDisplay.screenY;
					index = IMAGE_BLUE1 + getModularScaledRealTime(1020, 5);
					iV_DrawImage(IntImages,index,scrX,scrY);
				}
			}
		}
	}

	for(psFeature = apsFeatureLists[0]; psFeature; psFeature = psFeature->psNext)
	{
		if(!psFeature->died && psFeature->sDisplay.frameNumber == currentGameFrame)
		{
			if(psFeature->bTargetted)
			{
				scrX = psFeature->sDisplay.screenX;
				scrY = psFeature->sDisplay.screenY;
				iV_DrawImage(IntImages,getTargettingGfx(),scrX,scrY);
			}
		}
	}

	pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);
}

/* ---------------------------------------------------------------------------- */
/// X offset to display the group number at
#define GN_X_OFFSET	(8)
/// Draw the number of the group the droid is in next to the droid
static void	drawDroidGroupNumber(DROID *psDroid)
{
	UWORD id = UWORD_MAX;

	switch(psDroid->group)
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

	if(id != UWORD_MAX)
	{
		int xShift = psDroid->sDisplay.screenR + GN_X_OFFSET;
		int yShift = psDroid->sDisplay.screenR;
		iV_DrawImage(IntImages,id,psDroid->sDisplay.screenX-xShift,psDroid->sDisplay.screenY+yShift);
	}
}

/// X offset to display the group number at
#define CMND_STAR_X_OFFSET	(6)
#define CMND_GN_Y_OFFSET	(8)
/// Draw the number of the commander the droid is assigned to
static void	drawDroidCmndNo(DROID *psDroid)
{
	SDWORD	xShift,yShift, index;
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
	switch(index)
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

	if(bDraw)
	{
		xShift = psDroid->sDisplay.screenR + GN_X_OFFSET;
		yShift = psDroid->sDisplay.screenR - CMND_GN_Y_OFFSET;
		iV_DrawImage(IntImages,id2,psDroid->sDisplay.screenX-xShift-CMND_STAR_X_OFFSET,psDroid->sDisplay.screenY+yShift);
		iV_DrawImage(IntImages,id,psDroid->sDisplay.screenX-xShift,psDroid->sDisplay.screenY+yShift);
	}
}
/* ---------------------------------------------------------------------------- */


/**	Get the onscreen coordinates of a droid so we can draw a bounding box
 * This need to be severely speeded up and the accuracy increased to allow variable size bouding boxes
 * @todo Remove all magic numbers and hacks
 */
void calcScreenCoords(DROID *psDroid)
{
	/* Get it's absolute dimensions */
	const BODY_STATS *psBStats = asBodyStats + psDroid->asBits[COMP_BODY].nStat;
	Vector3i origin;
	Vector2i center;
	int wsRadius = 22; // World space radius, 22 = magic minimum
	float radius;

	// NOTE: This only takes into account body, but seems "good enough"
	if (psBStats && psBStats->pIMD)
	{
		wsRadius = MAX(wsRadius, psBStats->pIMD->radius);
	}

	origin = Vector3i(0, wsRadius, 0); // take the center of the object

	/* get the screen coordinates */
	// FP12_SHIFT-STRETCHED_Z_SHIFT is the shift of the scaling on the depth returned
	const float cZ = pie_RotateProject(&origin, &center) / (float) (1 << (FP12_SHIFT-STRETCHED_Z_SHIFT));

	//Watermelon:added a crash protection hack...
	if (cZ >= 0)
	{
		// 330 is the near plane depth from pie_PerspectiveBegin
		// not sure where magic comes from, could be another 1<<FP12_SHIFT-STRETCHED_Z_SHIFT
		const int magic = 4;
		radius = (wsRadius * 330 * magic) / cZ;
	}
	else
	{
		radius = 1; // 1 just in case some other code assumes radius != 0
	}

	/* Deselect all the droids if we've released the drag box */
	if(dragBox3D.status == DRAG_RELEASED)
	{
		if(inQuad(&center, &dragQuad) && psDroid->player == selectedPlayer)
		{
			//don't allow Transporter Droids to be selected here
			//unless we're in multiPlayer mode!!!!
			if ((psDroid->droidType != DROID_TRANSPORTER && psDroid->droidType != DROID_SUPERTRANSPORTER) || bMultiPlayer)
			{
				dealWithDroidSelect(psDroid, true);
			}
		}
	}

	/* Store away the screen coordinates so we can select the droids without doing a trasform */
	psDroid->sDisplay.screenX = center.x;
	psDroid->sDisplay.screenY = center.y;
	psDroid->sDisplay.screenR = radius;
}

static Vector3i getRotateProject(int x, int y)
{
	Vector2i screen(0, 0);
	Position pos;
	pos.x = world_coord(x);
	pos.z = -world_coord(y);
	pos.y = 0;
	if (tileOnMap(playerXTile + x, playerZTile + y))
	{
		pos.y = map_TileHeight(playerXTile + x, playerZTile + y);
	}
	Vector3i ret;
	ret.z = pie_RotateProject(&pos, &screen);
	ret.x = screen.x;
	ret.y = screen.y;
	return ret;
}

/**
 * Find the tile the mouse is currently over
 * \todo This is slow - speed it up
 */
static void locateMouse(void)
{
	const Vector2i pt(mouseX(), mouseY());
	int i, j;
	unsigned idx, jdx;
	int nearestZ = INT_MAX;

	// Intentionally not the same range as in drawTiles()
	for (i = -visibleTiles.y/2, idx=0; i < visibleTiles.y/2; i++,++idx)
	{
		for (j = -visibleTiles.x/2, jdx=0; j < visibleTiles.x/2; j++,++jdx)
		{
			Vector3i pos = getRotateProject(j, i);

			if (pos.z <= nearestZ)
			{
				QUAD quad;

				quad.coords[0] = pos;
				quad.coords[1] = getRotateProject(j, i+1);
				quad.coords[2] = getRotateProject(j+1, i+1);
				quad.coords[3] = getRotateProject(j+1, i);

				/* We've got a match for our mouse coords */
				if (inQuad(&pt, &quad))
				{
					mousePos.x = player.p.x + world_coord(j);
					mousePos.y = player.p.z + world_coord(i);
					mousePos += positionInQuad(pt, quad);

					if (mousePos.x < 0)
						mousePos.x = 0;
					else if (mousePos.x > world_coord(mapWidth-1))
						mousePos.x = world_coord(mapWidth-1);
					if (mousePos.y < 0)
						mousePos.y = 0;
					else if (mousePos.y > world_coord(mapHeight-1))
						mousePos.y = world_coord(mapHeight-1);

					mouseTileX = map_coord(mousePos.x);
					mouseTileY = map_coord(mousePos.y);

					/* Store away z value */
					nearestZ = pos.z;
				}
			}
		}
	}
}

/// Render the sky and surroundings
static void renderSurroundings(void)
{
	// Push current matrix onto stack
	pie_MatBegin();

	// Render skybox relative to ground (i.e. undo player y translation)
	// then move it somewhat below ground level for the blending effect
	pie_TRANSLATE(0, player.p.y-skybox_scale/8, 0);

	// rotate it
	pie_MatRotY(DEG(wind));

	// Set the texture page
	pie_SetTexturePage(iV_GetTexture(skyboxPageName));

	if(!gamePaused())
	{
		wind = wrapf(wind + graphicsTimeAdjustedIncrement(windSpeed), 360.0f);
	}
	pie_DrawSkybox(skybox_scale);

	// Load Saved State
	pie_MatEnd();
}

/// Smoothly adjust player height to match the desired height
static void trackHeight( float desiredHeight )
{
	static float heightSpeed = 0.0f;
	float separation = desiredHeight - player.p.y;	// How far are we from desired height?

	// d²/dt² player.p.y = -ACCEL_CONSTANT * (player.p.y - desiredHeight) - VELOCITY_CONSTANT * d/dt player.p.y
	solveDifferential2ndOrder(&separation, &heightSpeed, ACCEL_CONSTANT, VELOCITY_CONSTANT, realTimeAdjustedIncrement(1));

	/* Adjust the height accordingly */
	player.p.y = desiredHeight - separation;
}

/// Select the next energy bar display mode
ENERGY_BAR toggleEnergyBars(void)
{
	if (++barMode == BAR_LAST)
	{
		barMode = BAR_SELECTED;
	}
	return (ENERGY_BAR)barMode;
}

/// Set everything up for when the player assigns the sensor target
void assignSensorTarget( BASE_OBJECT *psObj )
{
	bSensorTargetting = true;
	lastTargetAssignation = realTime;
	psSensorObj = psObj;
}

/// Set everything up for when the player selects the destination
void	assignDestTarget( void )
{
	bDestTargetting = true;
	lastDestAssignation = realTime;
	destTargetX = mouseX();
	destTargetY = mouseY();
	destTileX = mouseTileX;
	destTileY = mouseTileY;
}

/// Draw a graphical effect after selecting a sensor target
static void	processSensorTarget( void )
{
	SWORD x,y;
	SWORD offset;
	SWORD x0,y0,x1,y1;
	UDWORD	index;


	if(bSensorTargetting)
	{
		if( (realTime - lastTargetAssignation) < TARGET_TO_SENSOR_TIME)
		{
			if(!psSensorObj->died && psSensorObj->sDisplay.frameNumber == currentGameFrame)
			{
				x = /*mouseX();*/(SWORD)psSensorObj->sDisplay.screenX;
				y = (SWORD)psSensorObj->sDisplay.screenY;
				if(!gamePaused())
				{
					index = IMAGE_BLUE1+getModularScaledGraphicsTime(1020, 5);
				}
				else
				{
					index = IMAGE_BLUE1;
				}
				iV_DrawImage(IntImages,index,x,y);

				offset = (SWORD)(12+ ((TARGET_TO_SENSOR_TIME)-(realTime-
					lastTargetAssignation))/2);

				x0 = (SWORD)(x-offset);
				y0 = (SWORD)(y-offset);
				x1 = (SWORD)(x+offset);
				y1 = (SWORD)(y+offset);

				iV_Line(x0, y0, x0 + 8, y0, WZCOL_WHITE);
				iV_Line(x0, y0, x0, y0 + 8, WZCOL_WHITE);

				iV_Line(x1, y0, x1 - 8, y0, WZCOL_WHITE);
				iV_Line(x1, y0, x1, y0 + 8, WZCOL_WHITE);

				iV_Line(x1, y1,x1 - 8, y1, WZCOL_WHITE);
				iV_Line(x1, y1,x1, y1 - 8, WZCOL_WHITE);

				iV_Line(x0, y1, x0 + 8, y1, WZCOL_WHITE);
				iV_Line(x0, y1, x0, y1 - 8, WZCOL_WHITE);
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
static void	processDestinationTarget( void )
{
	SWORD x,y;
	SWORD offset;
	SWORD x0,y0,x1,y1;


	if(bDestTargetting)
	{
		if( (realTime - lastDestAssignation) < DEST_TARGET_TIME)
		{
				x = (SWORD)destTargetX;
				y = (SWORD)destTargetY;

				offset = (SWORD)(((DEST_TARGET_TIME)-(realTime-lastDestAssignation))/2);

				x0 = (SWORD)(x-offset);
				y0 = (SWORD)(y-offset);
				x1 = (SWORD)(x+offset);
				y1 = (SWORD)(y+offset);

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
UDWORD	getWaterTileNum( void )
{
	return(underwaterTile);
}
/// Get the tile that is being used to show rubble
UDWORD	getRubbleTileNum( void )
{
	return(rubbleTile);
}

/// Draw the spinning particles for power stations and re-arm pads for the specified player
static void structureEffectsPlayer( UDWORD player )
{
	SDWORD	radius;
	STRUCTURE	*psStructure;
	SDWORD	xDif,yDif;
	Vector3i	pos;
	UDWORD	numConnected;
	DROID	*psDroid;
	UDWORD	gameDiv;
	UDWORD	i;
	BASE_OBJECT			*psChosenObj = NULL;
	UWORD	bFXSize;

	const int effectsPerSecond = 12;  // Effects per second. Will add effects up to once time per frame, so won't add as many effects if the framerate is low, but will be consistent, otherwise.
	unsigned effectTime = graphicsTime / (GAME_TICKS_PER_SEC / effectsPerSecond)*(GAME_TICKS_PER_SEC / effectsPerSecond);
	if (effectTime <= graphicsTime - deltaGraphicsTime)
	{
		return;  // Don't add effects this frame.
	}

	for(psStructure = apsStructLists[player]; psStructure; psStructure = psStructure->psNext)
	{
		if(psStructure->status == SS_BUILT)
		{
			if(psStructure->pStructureType->type == REF_POWER_GEN && psStructure->visible[selectedPlayer])
			{
				POWER_GEN* psPowerGen = &psStructure->pFunctionality->powerGenerator;
				numConnected = 0;
				for (i = 0; i < NUM_POWER_MODULES; i++)
				{
					if (psPowerGen->apResExtractors[i])
					{
						numConnected++;
					}
				}
				/* No effect if nothing connected */
				if(!numConnected)
				{
					//keep looking for another!
					continue;
				}

				else switch(numConnected)
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
				for(i=0 ;i<numConnected; i++)
				{
					radius = 32 - (i*2);	// around the spire
					xDif = iSinSR(effectTime, gameDiv, radius);
					yDif = iCosSR(effectTime, gameDiv, radius);

					pos.x = psStructure->pos.x + xDif;
					pos.z = psStructure->pos.y + yDif;
					pos.y = map_Height(pos.x,pos.z) + 64 + (i*20);	// 64 up to get to base of spire
					effectGiveAuxVar(50);	// half normal plasma size...
					addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_LASER,false,NULL,0);

					pos.x = psStructure->pos.x - xDif;
					pos.z = psStructure->pos.y - yDif;
//					pos.y = map_Height(pos.x,pos.z) + 64 + (i*20);	// 64 up to get to base of spire
					effectGiveAuxVar(50);	// half normal plasma size...

					addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_LASER,false,NULL,0);
				}
			}
			/* Might be a re-arm pad! */
			else if(psStructure->pStructureType->type == REF_REARM_PAD
				&& psStructure->visible[selectedPlayer] )
			{
				REARM_PAD* psReArmPad = &psStructure->pFunctionality->rearmPad;
				psChosenObj = psReArmPad->psObj;
				if(psChosenObj!=NULL)
				{
					if((((DROID*)psChosenObj)->visible[selectedPlayer]))
					{
						bFXSize = 0;
						psDroid = (DROID*) psChosenObj;
						if(!psDroid->died && psDroid->action == DACTION_WAITDURINGREARM )
						{
							bFXSize = 30;

						}
						/* Then it's repairing...? */
						radius = psStructure->sDisplay.imd->radius;
						xDif = iSinSR(effectTime, 720, radius);
						yDif = iCosSR(effectTime, 720, radius);
						pos.x = psStructure->pos.x + xDif;
						pos.z = psStructure->pos.y + yDif;
						pos.y = map_Height(pos.x,pos.z) + psStructure->sDisplay.imd->max.y;
						effectGiveAuxVar(30+bFXSize);	// half normal plasma size...
						addEffect(&pos,EFFECT_EXPLOSION, EXPLOSION_TYPE_LASER,false,NULL,0);
						pos.x = psStructure->pos.x - xDif;
						pos.z = psStructure->pos.y - yDif;	// buildings are level!
		//				pos.y = map_Height(pos.x,pos.z) + psStructure->sDisplay->pos.max.y;
						effectGiveAuxVar(30+bFXSize);	// half normal plasma size...
						addEffect(&pos,EFFECT_EXPLOSION, EXPLOSION_TYPE_LASER,false,NULL,0);
					}
				}
			}
		}
	}
}

/// Draw the effects for all players and buildings
static void structureEffects()
{
	UDWORD	i;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (apsStructLists[i])
		{
			structureEffectsPlayer(i);
		}
	}
}

/// Show the sensor ranges of selected droids and buildings
static void	showDroidSensorRanges(void)
{
DROID		*psDroid;
STRUCTURE	*psStruct;

	if(rangeOnScreen)		// note, we still have to decide what to do with multiple units selected, since it will draw it for all of them! -Q 5-10-05
	{
	for(psDroid= apsDroidLists[selectedPlayer]; psDroid; psDroid=psDroid->psNext)
	{
		if(psDroid->selected)
		{
			showSensorRange2((BASE_OBJECT*)psDroid);
		}
	}

	for(psStruct = apsStructLists[selectedPlayer]; psStruct; psStruct = psStruct->psNext)
	{
		if(psStruct->selected)
		{
			showSensorRange2((BASE_OBJECT*)psStruct);
		}
	}
	}//end if we want to display...
}

static void showEffectCircle(Position centre, int32_t radius, uint32_t auxVar, EFFECT_GROUP group, EFFECT_TYPE type)
{
	int32_t circumference = radius * 2 * 355/113 / TILE_UNITS;  // 2πr in tiles.
	int32_t i;
	for (i = 0; i < circumference; ++i)
	{
		Vector3i pos;
		pos.x = centre.x - iSinSR(i, circumference, radius);
		pos.z = centre.y - iCosSR(i, circumference, radius);  // [sic] y -> z

		// Check if it's actually on map
		if (worldOnMap(pos.x, pos.z))
		{
			pos.y = map_Height(pos.x, pos.z) + 16;
			effectGiveAuxVar(auxVar);
			addEffect(&pos, group, type, false, NULL, 0);
		}
	}
}

// Shows the weapon (long) range of the object in question.
// Note, it only does it for the first weapon slot!
static void showWeaponRange(BASE_OBJECT *psObj)
{
	uint32_t weaponRange;
	uint32_t minRange;
	int compIndex;

	if (psObj->type == OBJ_DROID)
	{
		WEAPON_STATS *psStats = NULL;
		DROID *psDroid = (DROID*)psObj;
		compIndex = psDroid->asWeaps[0].nStat;	//weapon_slot
		ASSERT_OR_RETURN( , compIndex < numWeaponStats, "Invalid range referenced for numWeaponStats, %d > %d", compIndex, numWeaponStats);
		psStats = asWeaponStats + compIndex;
		weaponRange = psStats->longRange;
		minRange = psStats->minRange;
	}
	else
	{
		STRUCTURE *psStruct = (STRUCTURE*)psObj;
		if(psStruct->pStructureType->numWeaps == 0) return;
		weaponRange = psStruct->pStructureType->psWeapStat[0]->longRange;
		minRange = psStruct->pStructureType->psWeapStat[0]->minRange;
	}

	showEffectCircle(psObj->pos, weaponRange, 40, EFFECT_EXPLOSION, EXPLOSION_TYPE_SMALL);
	if (minRange > 0)
	{
		showEffectCircle(psObj->pos, minRange, 40, EFFECT_EXPLOSION, EXPLOSION_TYPE_TESLA);
	}
}

static void showSensorRange2(BASE_OBJECT *psObj)
{
	showEffectCircle(psObj->pos, psObj->sensorRange, 80, EFFECT_EXPLOSION, EXPLOSION_TYPE_LASER);
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

	if(radius <= 0)
		bRangeDisplay = false;
}

/// Get the graphic ID for a droid rank
UDWORD  getDroidRankGraphic(DROID *psDroid)
{
	UDWORD gfxId;
	/* Not found yet */
	gfxId = UDWORD_MAX;

	/* Establish the numerical value of the droid's rank */
	switch(getDroidLevel(psDroid))
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

/**	Will render a graphic depiction of the droid's present rank.
 * \note Assumes matrix context set and that z-buffer write is force enabled (Always).
 */
static void	drawDroidRank(DROID *psDroid)
{
	UDWORD	gfxId = getDroidRankGraphic(psDroid);

	/* Did we get one? - We should have... */
	if(gfxId!=UDWORD_MAX)
	{
		/* Render the rank graphic at the correct location */ // remove hardcoded numbers?!
		iV_DrawImage(IntImages,(UWORD)gfxId,
					 psDroid->sDisplay.screenX+psDroid->sDisplay.screenR + 8,
					 psDroid->sDisplay.screenY+psDroid->sDisplay.screenR);
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
		iV_DrawImage(IntImages,IMAGE_GN_STAR, psDroid->sDisplay.screenX, psDroid->sDisplay.screenY);
	}
}

/// Draw the construction lines for all construction droids
static	void	doConstructionLines( void )
{
	DROID	*psDroid;
	UDWORD	i;

	for(i=0; i<MAX_PLAYERS; i++)
	{
		for(psDroid= apsDroidLists[i]; psDroid; psDroid = psDroid->psNext)
		{
			if(clipXY(psDroid->pos.x,psDroid->pos.y))
			{

				if( (psDroid->visible[selectedPlayer]==UBYTE_MAX) &&
					(psDroid->sMove.Status != MOVESHUFFLE) )
				{
					if(psDroid->action == DACTION_BUILD)
					{
						if (psDroid->order.psObj)
						{
							if (psDroid->order.psObj->type == OBJ_STRUCTURE)
							{
								addConstructionLine(psDroid, (STRUCTURE*)psDroid->order.psObj);
							}
						}
					}

					else if ((psDroid->action == DACTION_DEMOLISH) ||
							(psDroid->action == DACTION_REPAIR) ||
							(psDroid->action == DACTION_RESTORE))
					{
						if(psDroid->psActionTarget[0])
						{
							if(psDroid->psActionTarget[0]->type == OBJ_STRUCTURE)
							{
								addConstructionLine(psDroid, (STRUCTURE*)psDroid->psActionTarget[0]);
							}
						}
					}
				}

			}
		}
	}
}

/// Draw the construction or demolish lines for one droid
static void addConstructionLine(DROID *psDroid, STRUCTURE *psStructure)
{
	Vector3f *point;
	Vector3f pts[3];
	int pointIndex;
	Vector3i each;
	PIELIGHT colour;

	pts[0] = Vector3f(psDroid->pos.x - player.p.x, psDroid->pos.z + 24, -(psDroid->pos.y - player.p.z));

	pointIndex = rand() % (psStructure->sDisplay.imd->npoints - 1);
	point = &(psStructure->sDisplay.imd->points[pointIndex]);

	each.x = psStructure->pos.x + point->x;
	each.y = psStructure->pos.z + (structHeightScale(psStructure) * point->y);
	each.z = psStructure->pos.y - point->z;

	if (rand() % 250 < deltaGraphicsTime)
	{
		effectSetSize(30);
		addEffect(&each,EFFECT_EXPLOSION,EXPLOSION_TYPE_SPECIFIED,true,getImdFromIndex(MI_PLASMA),0);
	}

	pts[1] = Vector3f(each.x - player.p.x, each.y, -(each.z - player.p.z));

	pointIndex = rand() % (psStructure->sDisplay.imd->npoints - 1);
	point = &(psStructure->sDisplay.imd->points[pointIndex]);

	each.x = psStructure->pos.x + point->x;
	each.y = psStructure->pos.z + (structHeightScale(psStructure) * point->y);
	each.z = psStructure->pos.y - point->z;

	pts[2] = Vector3f(each.x - player.p.x, each.y, -(each.z - player.p.z));

	colour = (psDroid->action == DACTION_DEMOLISH) ? WZCOL_DEMOLISH_BEAM : WZCOL_CONSTRUCTOR_BEAM;
	pie_TransColouredTriangle(pts, colour);
}
