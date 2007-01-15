/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
/***************************************************************************/
/*
 * Audio_id.c
 *
 * Matches wavs to audio id enums
 */
/***************************************************************************/

#include "lib/framework/frame.h"
#include "audio_id.h"

/***************************************************************************/

typedef struct AUDIO_ID
{
	SDWORD	iID;
	const char	*pWavStr;
}
AUDIO_ID;

/***************************************************************************/

static AUDIO_ID asAudioID[] =
{

/* Beeps */

{ID_SOUND_WINDOWCLOSE,					"Beep1.ogg"},
{ID_SOUND_WINDOWOPEN,					"Beep2.ogg"},
{ID_SOUND_SELECT,						"Beep4.ogg"},
{ID_SOUND_BUTTON_CLICK_5,				"Beep5.ogg"},
{FE_AUDIO_MESSAGEEND,					"Beep6.ogg"},
{ID_SOUND_ZOOM_ON_RADAR,					"Beep7.ogg"},
{ID_SOUND_BUILD_FAIL,					"Beep8.ogg"},
{ID_SOUND_MESSAGEEND,					"Beep9.ogg"},
{ID_SOUND_GAME_SHUTDOWN,					"GmeShtDn.ogg"},

/* Design Sequence */

{ID_SOUND_TURRET_SELECTED,				"PCV331.ogg"},
{ID_SOUND_BODY_SELECTED,					"PCV332.ogg"},
{ID_SOUND_PROPULSION_SELECTED,			"PCV333.ogg"},
{ID_SOUND_DESIGN_COMPLETED,				"PCV334.ogg"},

/* Structures */

{ID_SOUND_CONSTRUCTION_STARTED,			"PCV335.ogg"},
{ID_SOUND_STRUCTURE_COMPLETED,			"PCV336.ogg"},
{ID_SOUND_STRUCTURE_UNDER_ATTACK,		"PCV337.ogg"},
{ID_SOUND_STRUCTURE_REPAIR_IN_PROGRESS,	"PCV339.ogg"},
{ID_SOUND_STRUCTURE_DEMOLISHED,			"PCV340.ogg"},

/* Power */

{ID_SOUND_POWER_GENERATOR_UNDER_ATTACK,	"PCV341.ogg"},
{ID_SOUND_POWER_GENERATOR_DESTROYED,		"PCV342.ogg"},
{ID_SOUND_POWER_LOW,						"PCV343.ogg"},
{ID_SOUND_RESOURCE_HERE,					"PCV344.ogg"},
{ID_SOUND_DERRICK_UNDER_ATTACK,			"PCV345.ogg"},
{ID_SOUND_DERRICK_DESTROYED,				"PCV346.ogg"},
{ID_SOUND_RESOURCE_DEPLETED,				"PCV347.ogg"},
{ID_SOUND_POWER_TRANSFER_IN_PROGRESS,	"PCV348.ogg"},
{ID_SOUND_POWER_GENERATOR_REQUIRED,		"PCV349.ogg"},

/* Research */

{ID_SOUND_RESEARCH_FACILITY_REQUIRED,	"PCV350.ogg"},
{ID_SOUND_ARTIFACT,						"PCV351.ogg"},
{ID_SOUND_ARTIFACT_RECOVERED,			"PCV352.ogg"},
{ID_SOUND_NEW_RESEARCH_PROJ_AVAILABLE,	"PCV353.ogg"},
{ID_SOUND_NEW_STRUCTURE_AVAILABLE,		"PCV354.ogg"},
{ID_SOUND_NEW_COMPONENT_AVAILABLE,		"PCV355.ogg"},
{ID_SOUND_NEW_CYBORG_AVAILABLE,			"PCV356.ogg"},
{ID_SOUND_RESEARCH_COMPLETED,			"PCV357.ogg"},
{ID_SOUND_MAJOR_RESEARCH,				"PCV358.ogg"},
{ID_SOUND_STRUCTURE_RESEARCH_COMPLETED,	"PCV359.ogg"},
{ID_SOUND_POWER_RESEARCH_COMPLETED,		"PCV360.ogg"},
{ID_SOUND_COMPUTER_RESEARCH_COMPLETED,	"PCV361.ogg"},
{ID_SOUND_VEHICLE_RESEARCH_COMPLETED,	"PCV362.ogg"},
{ID_SOUND_SYSTEMS_RESEARCH_COMPLETED,	"PCV363.ogg"},
{ID_SOUND_WEAPON_RESEARCH_COMPLETED,		"PCV364.ogg"},
{ID_SOUND_CYBORG_RESEARCH_COMPLETED,		"PCV365.ogg"},

/* Production */

{ID_SOUND_PRODUCTION_STARTED,			"PCV366.ogg"},
{ID_SOUND_DROID_COMPLETED,				"PCV367.ogg"},
{ID_SOUND_PRODUCTION_PAUSED,				"PCV368.ogg"},
{ID_SOUND_PRODUCTION_CANCELLED,			"PCV369.ogg"},
{ID_SOUND_DELIVERY_POINT_ASSIGNED,		"PCV370.ogg"},
{ID_SOUND_DELIVERY_POINT_ASSIGNED_TO,	"PCV371.ogg"},

/* Repair */

{ID_SOUND_UNIT_REPAIRED,					"PCV372.ogg"},

/* Detection */

{ID_SOUND_SCAVENGERS_DETECTED,			"PCV373.ogg"},
{ID_SOUND_SCAVENGER_BASE_DETECTED,		"PCV374.ogg"},
{ID_SOUND_SCAVENGER_OUTPOST_DETECTED,	"PCV375.ogg"},
{ID_SOUND_POWER_RESOURCE,				"PCV376.ogg"},
{ID_SOUND_ARTEFACT_DISC,					"PCV377.ogg"},
{ID_SOUND_ENEMY_UNIT_DETECTED,			"PCV378.ogg"},
{ID_SOUND_ENEMY_BASE_DETECTED,			"PCV379.ogg"},
{ID_SOUND_ALLY_DETECTED,					"PCV380.ogg"},
{ID_SOUND_ENEMY_TRANSPORT_DETECTED,		"PCV381.ogg"},
{ID_SOUND_ENEMY_LZ_DETECTED,				"PCV382.ogg"},
{ID_SOUND_FRIENDLY_LZ_DETECTED,			"PCV383.ogg"},
{ID_SOUND_NEXUS_TOWER_DETECTED,			"PCV384.ogg"},
{ID_SOUND_NEXUS_TURRET_DETECTED,			"PCV385.ogg"},
{ID_SOUND_NEXUS_UNIT_DETECTED,			"PCV386.ogg"},
{ID_SOUND_ENEMY_BATTERY_DETECTED,		"PCV387.ogg"},
{ID_SOUND_ENEMY_VTOLS_DETECTED,			"PCV388.ogg"},

/* Status */

{ID_SOUND_SCAVENGER_BASE,				"PCV389.ogg"},
{ID_SOUND_SCAVENGER_OUTPOST,				"PCV390.ogg"},
{ID_SOUND_SCAVENGER_OUTPOST_ERADICATED,	"PCV391.ogg"},
{ID_SOUND_SCAVENGER_BASE_ERADICATED,		"PCV392.ogg"},
{ID_SOUND_ENEMY_BASE,					"PCV393.ogg"},
{ID_SOUND_ENEMY_BASE_ERADICATED,			"PCV394.ogg"},
{ID_SOUND_INCOMING_ENEMY_TRANSPORT,		"PCV395.ogg"},
{ID_SOUND_ENEMY_LZ,						"PCV396.ogg"},
{ID_SOUND_LZ1,							"PCV397.ogg"},
{ID_SOUND_LZ2,							"PCV398.ogg"},

/* Combat */

{ID_SOUND_UNIT_UNDER_ATTACK,				"PCV399.ogg"},
{ID_SOUND_UNIT_DESTROYED,				"PCV400.ogg"},
{ID_SOUND_UNIT_RETREATING,				"PCV401.ogg"},
{ID_SOUND_UNIT_RETURNING_FOR_REPAIR,		"PCV402.ogg"},

/* Artillery Batteries */
{ID_SOUND_ASSIGNED_TO_SENSOR,			"PCV403.ogg"},
{ID_SOUND_SENSOR_LOCKED_ON,				"PCV404.ogg"},
{ID_SOUND_ASSIGNED_TO_COUNTER_RADAR,		"PCV405.ogg"},
{ID_SOUND_ENEMY_BATTERY_LOCATED,			"PCV406.ogg"},
{ID_SOUND_BATTERY_FIRING_COUNTER_ATTACK,	"PCV407.ogg"},

/* Vtols */

{ID_SOUND_INTERCEPTORS_LAUNCHED,			"PCV408.ogg"},
{ID_SOUND_REARMING,						"PCV409.ogg"},
{ID_SOUND_VTOLS_ENGAGING,				"PCV410.ogg"},
{ID_SOUND_ASSIGNED,						"PCV411.ogg"},
{ID_SOUND_INTERCEPTORS_ASSIGNED,			"PCV412.ogg"},

/* Command Console */
{ID_SOUND_COMMAND_CONSOLE_ACTIVATED,		"PCV413.ogg"},
{ID_SOUND_SHORT_RANGE,					"PCV414.ogg"},
{ID_SOUND_LONG_RANGE,					"PCV415.ogg"},
{ID_SOUND_OPTIMUM_RANGE,					"PCV416.ogg"},
{ID_SOUND_RETREAT_AT_MEDIUM_DAMAGE,		"PCV417.ogg"},
{ID_SOUND_RETREAT_AT_HEAVY_DAMAGE,		"PCV418.ogg"},
{ID_SOUND_NO_RETREAT,					"PCV419.ogg"},
{ID_SOUND_FIRE_AT_WILL,					"PCV420.ogg"},
{ID_SOUND_RETURN_FIRE,					"PCV421.ogg"},
{ID_SOUND_CEASEFIRE,						"PCV422.ogg"},
{ID_SOUND_HOLD_POSITION,					"PCV423.ogg"},
{ID_SOUND_GUARD,							"PCV424.ogg"},
{ID_SOUND_PURSUE,						"PCV425.ogg"},
{ID_SOUND_PATROL,						"PCV426.ogg"},
{ID_SOUND_RETURN_TO_LZ,					"PCV427.ogg"},
{ID_SOUND_RECYCLING,						"PCV428.ogg"},
{ID_SOUND_SCATTER,						"PCV429.ogg"},

/* Tutorial Stuff */

{ID_SOUND_NOT_POSSIBLE_TRY_AGAIN,		"PCV430.ogg"},
{ID_SOUND_NO,							"PCV431.ogg"},
{ID_SOUND_THAT_IS_INCORRECT,				"PCV432.ogg"},
{ID_SOUND_WELL_DONE,						"PCV433.ogg"},
{ID_SOUND_EXCELLENT,						"PCV434.ogg"},

/* Group and Commander Assignment */

{ID_SOUND_ASSIGNED_TO_COMMANDER,			"PCV435.ogg"},
{ID_SOUND_GROUP_REPORTING,				"PCV436.ogg"},
{ID_SOUND_COMMANDER_REPORTING,			"PCV437.ogg"},

/* Routing */

{ID_SOUND_ROUTE_OBSTRUCTED,				"PCV438.ogg"},
{ID_SOUND_NO_ROUTE_AVAILABLE,			"PCV439.ogg"},

/* Transports and LZS */
{ID_SOUND_REINFORCEMENTS_AVAILABLE,		"PCV440.ogg"},
{ID_SOUND_REINFORCEMENTS_IN_TRANSIT,		"PCV441.ogg"},
{ID_SOUND_TRANSPORT_LANDING,				"PCV442.ogg"},
{ID_SOUND_TRANSPORT_UNDER_ATTACK,		"PCV443.ogg"},
{ID_SOUND_TRANSPORT_REPAIRING,			"PCV444.ogg"},
{ID_SOUND_LZ_COMPROMISED,				"PCV445.ogg"},
{ID_SOUND_LZ_CLEAR,						"LZ-Clear.ogg"},
{ID_SOUND_TRANSPORT_RETURNING_TO_BASE, 	"PCV446.ogg"},
{ID_SOUND_TRANSPORT_UNABLE_TO_LAND, 		"PCV447.ogg"},

/* Mission Messages */

{ID_SOUND_MISSION_OBJECTIVE,				"PCV448.ogg"},
{ID_SOUND_MISSION_UPDATE,				"PCV449.ogg"},
{ID_SOUND_WARZONE_PAUSED,				"PCV450.ogg"},
{ID_SOUND_WARZONE_ACTIVE,				"PCV451.ogg"},
{ID_SOUND_MISSION_RESULTS,				"PCV452.ogg"},
{ID_SOUND_RESEARCH_STOLEN,				"PCV453.ogg"},
{ID_SOUND_TECHNOLOGY_TAKEN,				"PCV454.ogg"},
{ID_SOUND_INCOMING_TRANSMISSION,			"PCV455.ogg"},
{ID_SOUND_INCOMING_INTELLIGENCE_REPORT,	"PCV456.ogg"},
{ID_SOUND_MISSION_FAILED,				"PCV458.ogg"},
{ID_SOUND_MISSION_SUCCESSFUL,			"PCV459.ogg"},
{ID_SOUND_OBJECTIVE_ACCOMPLISHED,		"PCV460.ogg"},
{ID_SOUND_OBJECTIVE_FAILED,				"PCV461.ogg"},
{ID_SOUND_MISSION_TIMER_ACTIVATED,		"PCV462.ogg"},
{ID_SOUND_10_MINUTES_REMAINING,			"PCV463.ogg"},
{ID_SOUND_5_MINUTES_REMAINING,			"PCV464.ogg"},
{ID_SOUND_3_MINUTES_REMAINING,			"PCV465.ogg"},
{ID_SOUND_2_MINUTES_REMAINING,			"PCV466.ogg"},
{ID_SOUND_1_MINUTE_REMAINING,			"PCV467.ogg"},
{ID_SOUND_UNIT_CAPTURED,					"PCV468.ogg"},
{ID_SOUND_SYSTEM_FAILURE_IMMINENT,		"PCV469.ogg"},
{ID_SOUND_YOU_ARE_DEFEATED,				"PCV470.ogg"},
{ID_SOUND_MISSILE_CODES_DECIPHERED,		"PCV471.ogg"},
{ID_SOUND_1ST_MISSILE_CODES_DECIPHERED,	"PCV472.ogg"},
{ID_SOUND_2ND_MISSILE_CODES_DECIPHERED,	"PCV473.ogg"},
{ID_SOUND_3RD_MISSILE_CODES_DECIPHERED,	"PCV474.ogg"},
{ID_SOUND_MISSILE_CODES_CRACKED,			"PCV475.ogg"},
{ID_SOUND_ENTERING_WARZONE,				"PCV476.ogg"},
{ID_ALLIANCE_ACC,						"PCV477.ogg"},
{ID_ALLIANCE_BRO,						"PCV478.ogg"},
{ID_ALLIANCE_OFF,						"PCV479.ogg"},
{ID_CLAN_ENTER,							"PCV480.ogg"},
{ID_CLAN_EXIT,							"PCV481.ogg"},
{ID_GIFT,								"PCV482.ogg"},
{ID_POWER_TRANSMIT,						"PCV483.ogg"},
{ID_SENSOR_DOWNLOAD,						"PCV484.ogg"},
{ID_TECHNOLOGY_TRANSFER,					"PCV485.ogg"},
{ID_UNITS_TRANSFER,						"PCV486.ogg"},

/* Group and Commander Voices - Male */
{ID_SOUND_GROUP,							"Group.ogg"},
{ID_SOUND_GROUP_0,						"0.ogg"},
{ID_SOUND_GROUP_1,						"1.ogg"},
{ID_SOUND_GROUP_2,						"2.ogg"},
{ID_SOUND_GROUP_3,						"3.ogg"},
{ID_SOUND_GROUP_4,						"4.ogg"},
{ID_SOUND_GROUP_5,						"5.ogg"},
{ID_SOUND_GROUP_6,						"6.ogg"},
{ID_SOUND_GROUP_7,						"7.ogg"},
{ID_SOUND_GROUP_8,						"8.ogg"},
{ID_SOUND_GROUP_9,						"9.ogg"},
{ID_SOUND_REPORTING,						"Reprting.ogg"},
{ID_SOUND_COMMANDER,						"Commnder.ogg"},
{ID_SOUND_COM_SCAVS_DETECTED,			"COM021.ogg"},
{ID_SOUND_COM_SCAV_BASE_DETECTED,		"COM022.ogg"},
{ID_SOUND_COM_SCAV_OUTPOST_DETECTED,		"COM023.ogg"},
{ID_SOUND_COM_RESOURCE_DETECTED,			"COM024.ogg"},
{ID_SOUND_COM_ARTEFACT_DETECTED,			"COM025.ogg"},
{ID_SOUND_COM_ENEMY_DETECTED,			"COM026.ogg"},
{ID_SOUND_COM_ENEMY_BASE_DETECTED,		"COM027.ogg"},
{ID_SOUND_COM_ALLY_DETECTED,				"COM028.ogg"},
{ID_SOUND_COM_ENEMY_TRANSPORT_DETECTED,	"COM029.ogg"},
{ID_SOUND_COM_ENEMY_LZ_DETECTED,			"COM030.ogg"},
{ID_SOUND_COM_FRIENDLY_LZ_DETECTED,		"COM031.ogg"},
{ID_SOUND_COM_NEXUS_TOWER_DETECTED,		"COM032.ogg"},
{ID_SOUND_COM_NEXUS_TURRET_DETECTED,		"COM033.ogg"},
{ID_SOUND_COM_NEXUS_DETECTED,			"COM034.ogg"},
{ID_SOUND_COM_ENEMY_BATTERY_DETECTED,	"COM035.ogg"},
{ID_SOUND_COM_ENEMY_VTOLS_DETECTED,		"COM036.ogg"},
{ID_SOUND_COM_ROUTE_OBSTRUCTED,			"COM037.ogg"},
{ID_SOUND_COM_NO_ROUTE_AVAILABLE,		"COM038.ogg"},
{ID_SOUND_COM_UNABLE_TO_COMPLY,			"COM039.ogg"},
{ID_SOUND_COM_RETURNING_FOR_REPAIR,		"COM040.ogg"},
{ID_SOUND_COM_HEADING_FOR_RALLY_POINT,	"COM041.ogg"},

/* Radio Clicks */
{ID_SOUND_RADIOCLICK_1,					"RadClik1.ogg"},
{ID_SOUND_RADIOCLICK_2,					"RadClik2.ogg"},
{ID_SOUND_RADIOCLICK_3,					"RadClik3.ogg"},
{ID_SOUND_RADIOCLICK_4,					"RadClik4.ogg"},
{ID_SOUND_RADIOCLICK_5,					"RadClik5.ogg"},
{ID_SOUND_RADIOCLICK_6,					"RadClik6.ogg"},

/* Transport Pilots */
{ID_SOUND_APPROACHING_LZ,				"T-AproLZ.ogg"},
{ID_SOUND_ALRIGHT_BOYS,					"T-ARBoys.ogg"},
{ID_SOUND_GREEN_LIGHT_IN_5,				"T-GrnLi5.ogg"},
{ID_SOUND_GREEN_LIGHT_IN_4,				"T-GrnLi4.ogg"},
{ID_SOUND_GREEN_LIGHT_IN_3,				"T-GrnLi3.ogg"},
{ID_SOUND_GREEN_LIGHT_IN_2,				"T-GrnLi2.ogg"},
{ID_SOUND_GO_GO_GO,						"T-GoGoGo,wav"},
{ID_SOUND_PREPARE_FOR_DUST_OFF,			"T-DustOf.ogg"},

/* VTol Pilots */

	/* Ver-1 */
{ID_SOUND_ENEMY_LOCATED1,				"V-Eloc1.ogg"},
{ID_SOUND_ON_OUR_WAY1,					"V-OnWay1.ogg"},
{ID_SOUND_RETURNING_TO_BASE1,			"V-RetBa1.ogg"},
{ID_SOUND_LOCKED_ON1,					"V-LocOn1.ogg"},
{ID_SOUND_COMMENCING_ATTACK_RUN1,		"V-AtkRn1.ogg"},
{ID_SOUND_ABORTING_ATTACK_RUN1,			"V-AbtRn1.ogg"},

	/* Ver-2 */
{ID_SOUND_ENEMY_LOCATED2,				"V-Eloc2.ogg"},
{ID_SOUND_ON_OUR_WAY2,					"V-OnWay2.ogg"},
{ID_SOUND_RETURNING_TO_BASE2,			"V-RetBa2.ogg"},
{ID_SOUND_LOCKED_ON2,					"V-LocOn2.ogg"},
{ID_SOUND_COMMENCING_ATTACK_RUN2,		"V-AtkRn2.ogg"},
{ID_SOUND_ABORTING_ATTACK_RUN2,			"V-AbtRn2.ogg"},

	/* Ver-3 */
{ID_SOUND_ENEMY_LOCATED3,				"V-Eloc3.ogg"},
{ID_SOUND_ON_OUR_WAY3,					"V-OnWay3.ogg"},
{ID_SOUND_RETURNING_TO_BASE3,			"V-RetBa3.ogg"},
{ID_SOUND_LOCKED_ON3,					"V-LocOn3.ogg"},
{ID_SOUND_COMMENCING_ATTACK_RUN3,		"V-AtkRn3.ogg"},
{ID_SOUND_ABORTING_ATTACK_RUN3,			"V-AbtRn3.ogg"},


/* The Collective */
{ID_SOUND_COLL_CLEANSE_AND_DESTROY,		"COl011a.ogg"},
{ID_SOUND_COLL_DESTROYING_BIOLOGICALS,	"COl012a.ogg"},
{ID_SOUND_COLL_ATTACK,					"COl013a.ogg"},
{ID_SOUND_COLL_FIRE,						"COl014a.ogg"},
{ID_SOUND_COLL_ENEMY_DETECTED,			"COl015a.ogg"},
{ID_SOUND_COLL_ENGAGING,					"COl016a.ogg"},
{ID_SOUND_COLL_STARTING_ATTACK_RUN,		"COl017a.ogg"},
{ID_SOUND_COLL_DIE,						"COl018a.ogg"},
{ID_SOUND_COLL_INTERCEPT_AND_DESTROY,	"COl019a.ogg"},
{ID_SOUND_COLL_ENEMY_DESTROYED,			"COl020a.ogg"},

/* SFX */
	/* Weapon Sounds */
{ID_SOUND_ROCKET,						"rocket.ogg"},
{ID_SOUND_ROTARY_LASER,					"RotLsr.ogg"},
{ID_SOUND_GAUSSGUN,						"GaussGun.ogg"},
{ID_SOUND_LARGE_CANNON,					"LrgCan.ogg"},
{ID_SOUND_SMALL_CANNON,					"SmlCan.ogg"},
{ID_SOUND_MEDIUM_CANNON,					"MedCan.ogg"},
{ID_SOUND_FLAME_THROWER,					"FlmThrow.ogg"},
{ID_SOUND_PULSE_LASER,					"PlsLsr.ogg"},
{ID_SOUND_BEAM_LASER,					"BemLsr.ogg"},
{ID_SOUND_MORTAR,						"Mortar.ogg"},
{ID_SOUND_HOWITZ_FLIGHT,					"HwtzFlgt.ogg"},
{ID_SOUND_BABA_MG_1,						"MgBar1.ogg"},
{ID_SOUND_BABA_MG_2,						"MgBar2.ogg"},
{ID_SOUND_BABA_MG_3,						"MgBar3.ogg"},
{ID_SOUND_BABA_MG_HEAVY,					"MgHeavy.ogg"},
{ID_SOUND_BABA_MG_TOWER,					"MgTower.ogg"},
{ID_SOUND_SPLASH,						"Splash.ogg"},
{ID_SOUND_ASSAULT_MG,					"AsltMG.ogg"},
{ID_SOUND_RAPID_CANNON,					"RapdCan.ogg"},
{ID_SOUND_HIVEL_CANNON,					"HiVelCan.ogg"},
{ID_SOUND_NEXUS_TOWER,					"NxsTower.ogg"},

	/* Construction sounds */
{ID_SOUND_WELD_1,						"Weld-1.ogg"},
{ID_SOUND_WELD_2,						"Weld-2.ogg"},
{ID_SOUND_CONSTRUCTION_START,			"BldStart.ogg"},
{ID_SOUND_CONSTRUCTION_LOOP,				"BldLoop.ogg"},
{ID_SOUND_CONSTRUCTION_1,				"Build1.ogg"},
{ID_SOUND_CONSTRUCTION_2,				"Build2.ogg"},
{ID_SOUND_CONSTRUCTION_3,				"Build3.ogg"},
{ID_SOUND_CONSTRUCTION_4,				"Build4.ogg"},

	/* Explosions */

{ID_SOUND_EXPLOSION_SMALL,				"SmlExpl.ogg"},
{ID_SOUND_EXPLOSION_LASER,				"LsrExpl.ogg"},
{ID_SOUND_EXPLOSION,						"LrgExpl.ogg"},
{ID_SOUND_EXPLOSION_ANTITANK,			"ATnkExpl.ogg"},
{ID_SOUND_RICOCHET_1,					"Richet1.ogg"},
{ID_SOUND_RICOCHET_2,					"Richet2.ogg"},
{ID_SOUND_RICOCHET_3,					"Richet3.ogg"},
{ID_SOUND_BARB_SQUISH,					"Squish.ogg"},
{ID_SOUND_BUILDING_FALL,					"BldFall.ogg"},
{ID_SOUND_NEXUS_EXPLOSION,				"NxsExpld.ogg"},

	/* Droid Engine Noises */
{ID_SOUND_CONSTRUCTOR_MOVEOFF,			"Con-Move Off.ogg"},
{ID_SOUND_CONSTRUCTOR_MOVE,				"Con-Move.ogg"},
{ID_SOUND_CONSTRUCTOR_SHUTDOWN,			"Con-Shut down.ogg"},

	/* Transports */
{ID_SOUND_BLIMP_FLIGHT,					"TFlight.ogg"},
{ID_SOUND_BLIMP_IDLE,					"THover.ogg"},
{ID_SOUND_BLIMP_LAND,					"TLand.ogg"},
{ID_SOUND_BLIMP_TAKE_OFF,				"TStart.ogg"},

	/* Vtols */
{ID_SOUND_VTOL_LAND,						"VtolLand.ogg"},
{ID_SOUND_VTOL_OFF,						"VtolOff.ogg"},
{ID_SOUND_VTOL_MOVE,						"VtolMove.ogg"},

	/* Treads */
{ID_SOUND_TREAD,							"Tread.ogg"},

	/* Hovers */
{ID_SOUND_HOVER_MOVE,					"HovMove.ogg"},
{ID_SOUND_HOVER_START,					"HovStart.ogg"},
{ID_SOUND_HOVER_STOP,					"HovStop.ogg"},

	/* Cyborgs */
{ID_SOUND_CYBORG_MOVE,					"Cyber-Move.ogg"},

	/* Buildings */

{ID_SOUND_OIL_PUMP_2,					"OilPump.ogg"},
{ID_SOUND_POWER_HUM,						"PowerHum.ogg"},
{ID_SOUND_POWER_SPARK,					"PowerSpk.ogg"},
{ID_SOUND_STEAM,							"Steam.ogg"},
{ID_SOUND_ECM_TOWER,						"ECMTower.ogg"},
{ID_SOUND_FIRE_ROAR,						"FreRoar.ogg"},

	/* Misc */

{ID_SOUND_HELP,							"help.ogg"},
{ID_SOUND_BARB_SCREAM,					"Scream.ogg"},
{ID_SOUND_BARB_SCREAM2,					"Scream2.ogg"},
{ID_SOUND_BARB_SCREAM3,					"Scream3.ogg"},
{ID_SOUND_OF_SILENCE,					"Silence.ogg"},
{ID_SOUND_NOFAULTS,						"Scream4.ogg"},

	/* Extra */
{ID_SOUND_LANDING_ZONE,					"LndgZne.ogg"},
{ID_SOUND_SATELLITE_UPLINK,				"Pcv652.ogg"},
{ID_SOUND_NASDA_CENTRAL,					"Pcv653.ogg"},
{ID_SOUND_NUCLEAR_REACTOR,				"Pcv654.ogg"},
{ID_SOUND_SAM_SITE,						"Pcv655.ogg"},
{ID_SOUND_MISSILE_SILO,					"Pcv656.ogg"},
{ID_SOUND_MISSILE_NME_DETECTED,			"NmeDeted.ogg"},
{ID_SOUND_STRUCTURE_CAPTURED,			"Pcv611.ogg"},
{ID_SOUND_CIVILIAN_RESCUED,				"Pcv612.ogg"},
{ID_SOUND_CIVILIANS_RESCUED,				"Pcv613.ogg"},
{ID_SOUND_UNITS_RESCUED,					"Pcv615.ogg"},
{ID_SOUND_GROUP_RESCUED,					"Pcv616.ogg"},
{ID_SOUND_GROUP_CAPTURED,				"Pcv618.ogg"},
{ID_SOUND_OBJECTIVE_CAPTURED,			"Pcv621.ogg"},
{ID_SOUND_OBJECTIVE_DESTROYED,			"Pcv622.ogg"},
{ID_SOUND_STRUCTURE_INFECTED,			"Pcv623.ogg"},
{ID_SOUND_GROUP_INFECTED,				"Pcv625.ogg"},
{ID_SOUND_OUT_OF_TIME,					"Pcv629.ogg"},
{ID_SOUND_ENEMY_ESCAPED,					"Pcv631.ogg"},
{ID_SOUND_ENEMY_ESCAPING,				"Pcv632.ogg"},
{ID_SOUND_ENEMY_TRANSPORT_LANDING,		"Pcv633.ogg"},
{ID_SOUND_TEAM_ALPHA_ERADICATED,			"Pcv635.ogg"},
{ID_SOUND_TEAM_BETA_ERADICATED,			"Pcv636.ogg"},
{ID_SOUND_TEAM_GAMMA_ERADICATED,			"Pcv637.ogg"},
{ID_SOUND_TEAM_ALPHA_RESCUED,			"Pcv638.ogg"},
{ID_SOUND_TEAM_BETA_RESCUED,				"Pcv639.ogg"},
{ID_SOUND_TEAM_GAMMA_RESCUED,			"Pcv640.ogg"},
{ID_SOUND_LASER_SATELLITE_FIRING,		"Pcv650.ogg"},
{ID_SOUND_INCOMING_LASER_SAT_STRIKE,		"Pcv657.ogg"},

	/* Nexus */
{ID_SOUND_NEXUS_DEFENCES_ABSORBED,		"DefAbsrd.ogg"},
{ID_SOUND_NEXUS_DEFENCES_NEUTRALISED,	"DefNut.ogg"},
{ID_SOUND_NEXUS_LAUGH1,					"Laugh1.ogg"},
{ID_SOUND_NEXUS_LAUGH2,					"Laugh2.ogg"},
{ID_SOUND_NEXUS_LAUGH3,					"Laugh3.ogg"},
{ID_SOUND_NEXUS_PRODUCTION_COMPLETED,	"PordComp.ogg"},
{ID_SOUND_NEXUS_RESEARCH_ABSORBED,		"ResAbsrd.ogg"},
{ID_SOUND_NEXUS_STRUCTURE_ABSORBED,		"StrutAbs.ogg"},
{ID_SOUND_NEXUS_STRUCTURE_NEUTRALISED,	"StrutNut.ogg"},
{ID_SOUND_NEXUS_SYNAPTIC_LINK,			"SynpLnk.ogg"},
{ID_SOUND_NEXUS_UNIT_ABSORBED,			"UntAbsrd.ogg"},
{ID_SOUND_NEXUS_UNIT_NEUTRALISED,		"UntNut.ogg"},

/* extra multiplayer sounds */
{ID_SOUND_CYBORG_GROUND,					"CybGrnd.ogg"},
{ID_SOUND_CYBORG_HEAVY,					"HvCybMov.ogg"},
{ID_SOUND_EMP,							"EMP.ogg"},
{ID_SOUND_LASER_HEAVY,					"HevLsr.ogg"},
{ID_SOUND_PLASMA_FLAMER,					"PlasFlm.ogg"},
{ID_SOUND_UPLINK,						"UpLink.ogg"},
{ID_SOUND_LAS_SAT_COUNTDOWN,             "LasStrk.ogg"},
};

/***************************************************************************/

BOOL
audioID_GetIDFromStr( const char *pWavStr, SDWORD *piID )
{
	SDWORD		i;

	for ( i=0; i<ID_MAX_SOUND; i++ )
	{
		if ( strcasecmp( pWavStr, asAudioID[i].pWavStr ) == 0 )
		{
			ASSERT( i == asAudioID[i].iID,
				"audioID_GetIDFromStr: %s stored IDs don't match", pWavStr );

			*piID = asAudioID[i].iID;
			return TRUE;
		}
	}

	*piID = NO_SOUND;
	return FALSE;
}

/***************************************************************************/
