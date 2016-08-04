/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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

struct AUDIO_ID_MAP
{
	INGAME_AUDIO    ID;
	const char     *fileName;
};

/***************************************************************************/

static AUDIO_ID_MAP asAudioID[] =
{

	/* Beeps */

	{ID_SOUND_WINDOWCLOSE,                         "beep1.ogg"},
	{ID_SOUND_WINDOWOPEN,                          "beep2.ogg"},
	{ID_SOUND_BUTTON_CLICK_3,                      "beep3.ogg"},
	{ID_SOUND_SELECT,                              "beep4.ogg"},
	{ID_SOUND_BUTTON_CLICK_5,                      "beep5.ogg"},
	{FE_AUDIO_MESSAGEEND,                          "beep6.ogg"},
	{ID_SOUND_ZOOM_ON_RADAR,                       "beep7.ogg"},
	{ID_SOUND_BUILD_FAIL,                          "beep8.ogg"},
	{ID_SOUND_MESSAGEEND,                          "beep9.ogg"},
	{ID_SOUND_GAME_SHUTDOWN,                       "gmeshtdn.ogg"},

	/* Design Sequence */

	{ID_SOUND_TURRET_SELECTED,                     "pcv331.ogg"},
	{ID_SOUND_BODY_SELECTED,                       "pcv332.ogg"},
	{ID_SOUND_PROPULSION_SELECTED,                 "pcv333.ogg"},
	{ID_SOUND_DESIGN_COMPLETED,                    "pcv334.ogg"},

	/* Structures */

	{ID_SOUND_CONSTRUCTION_STARTED,                "pcv335.ogg"},
	{ID_SOUND_STRUCTURE_COMPLETED,                 "pcv336.ogg"},
	{ID_SOUND_STRUCTURE_UNDER_ATTACK,              "pcv337.ogg"},
	{ID_SOUND_STRUCTURE_REPAIR_IN_PROGRESS,        "pcv339.ogg"},
	{ID_SOUND_STRUCTURE_DEMOLISHED,                "pcv340.ogg"},

	/* Power */

	{ID_SOUND_POWER_GENERATOR_UNDER_ATTACK,        "pcv341.ogg"},
	{ID_SOUND_POWER_GENERATOR_DESTROYED,           "pcv342.ogg"},
	{ID_SOUND_POWER_LOW,                           "pcv343.ogg"},
	{ID_SOUND_RESOURCE_HERE,                       "pcv344.ogg"},
	{ID_SOUND_DERRICK_UNDER_ATTACK,                "pcv345.ogg"},
	{ID_SOUND_DERRICK_DESTROYED,                   "pcv346.ogg"},
	{ID_SOUND_RESOURCE_DEPLETED,                   "pcv347.ogg"},
	{ID_SOUND_POWER_TRANSFER_IN_PROGRESS,          "pcv348.ogg"},
	{ID_SOUND_POWER_GENERATOR_REQUIRED,            "pcv349.ogg"},

	/* Research */

	{ID_SOUND_RESEARCH_FACILITY_REQUIRED,          "pcv350.ogg"},
	{ID_SOUND_ARTIFACT,                            "pcv351.ogg"},
	{ID_SOUND_ARTIFACT_RECOVERED,                  "pcv352.ogg"},
	{ID_SOUND_NEW_RESEARCH_PROJ_AVAILABLE,         "pcv353.ogg"},
	{ID_SOUND_NEW_STRUCTURE_AVAILABLE,             "pcv354.ogg"},
	{ID_SOUND_NEW_COMPONENT_AVAILABLE,             "pcv355.ogg"},
	{ID_SOUND_NEW_CYBORG_AVAILABLE,                "pcv356.ogg"},
	{ID_SOUND_RESEARCH_COMPLETED,                  "pcv357.ogg"},
	{ID_SOUND_MAJOR_RESEARCH,                      "pcv358.ogg"},
	{ID_SOUND_STRUCTURE_RESEARCH_COMPLETED,        "pcv359.ogg"},
	{ID_SOUND_POWER_RESEARCH_COMPLETED,            "pcv360.ogg"},
	{ID_SOUND_COMPUTER_RESEARCH_COMPLETED,         "pcv361.ogg"},
	{ID_SOUND_VEHICLE_RESEARCH_COMPLETED,          "pcv362.ogg"},
	{ID_SOUND_SYSTEMS_RESEARCH_COMPLETED,          "pcv363.ogg"},
	{ID_SOUND_WEAPON_RESEARCH_COMPLETED,           "pcv364.ogg"},
	{ID_SOUND_CYBORG_RESEARCH_COMPLETED,           "pcv365.ogg"},

	/* Production */

	{ID_SOUND_PRODUCTION_STARTED,                  "pcv366.ogg"},
	{ID_SOUND_DROID_COMPLETED,                     "pcv367.ogg"},
	{ID_SOUND_PRODUCTION_PAUSED,                   "pcv368.ogg"},
	{ID_SOUND_PRODUCTION_CANCELLED,                "pcv369.ogg"},
	{ID_SOUND_DELIVERY_POINT_ASSIGNED,             "pcv370.ogg"},
	{ID_SOUND_DELIVERY_POINT_ASSIGNED_TO,          "pcv371.ogg"},

	/* Repair */

	{ID_SOUND_UNIT_REPAIRED,                       "pcv372.ogg"},

	/* Detection */

	{ID_SOUND_SCAVENGERS_DETECTED,                 "pcv373.ogg"},
	{ID_SOUND_SCAVENGER_BASE_DETECTED,             "pcv374.ogg"},
	{ID_SOUND_SCAVENGER_OUTPOST_DETECTED,          "pcv375.ogg"},
	{ID_SOUND_POWER_RESOURCE,                      "pcv376.ogg"},
	{ID_SOUND_ARTEFACT_DISC,                       "pcv377.ogg"},
	{ID_SOUND_ENEMY_UNIT_DETECTED,                 "pcv378.ogg"},
	{ID_SOUND_ENEMY_BASE_DETECTED,                 "pcv379.ogg"},
	{ID_SOUND_ALLY_DETECTED,                       "pcv380.ogg"},
	{ID_SOUND_ENEMY_TRANSPORT_DETECTED,            "pcv381.ogg"},
	{ID_SOUND_ENEMY_LZ_DETECTED,                   "pcv382.ogg"},
	{ID_SOUND_FRIENDLY_LZ_DETECTED,                "pcv383.ogg"},
	{ID_SOUND_NEXUS_TOWER_DETECTED,                "pcv384.ogg"},
	{ID_SOUND_NEXUS_TURRET_DETECTED,               "pcv385.ogg"},
	{ID_SOUND_NEXUS_UNIT_DETECTED,                 "pcv386.ogg"},
	{ID_SOUND_ENEMY_BATTERY_DETECTED,              "pcv387.ogg"},
	{ID_SOUND_ENEMY_VTOLS_DETECTED,                "pcv388.ogg"},

	/* Status */

	{ID_SOUND_SCAVENGER_BASE,                      "pcv389.ogg"},
	{ID_SOUND_SCAVENGER_OUTPOST,                   "pcv390.ogg"},
	{ID_SOUND_SCAVENGER_OUTPOST_ERADICATED,        "pcv391.ogg"},
	{ID_SOUND_SCAVENGER_BASE_ERADICATED,           "pcv392.ogg"},
	{ID_SOUND_ENEMY_BASE,                          "pcv393.ogg"},
	{ID_SOUND_ENEMY_BASE_ERADICATED,               "pcv394.ogg"},
	{ID_SOUND_INCOMING_ENEMY_TRANSPORT,            "pcv395.ogg"},
	{ID_SOUND_ENEMY_LZ,                            "pcv396.ogg"},
	{ID_SOUND_LZ1,                                 "pcv397.ogg"},
	{ID_SOUND_LZ2,                                 "pcv398.ogg"},

	/* Combat */

	{ID_SOUND_UNIT_UNDER_ATTACK,                   "pcv399.ogg"},
	{ID_SOUND_UNIT_DESTROYED,                      "pcv400.ogg"},
	{ID_SOUND_UNIT_RETREATING,                     "pcv401.ogg"},
	{ID_SOUND_UNIT_RETURNING_FOR_REPAIR,           "pcv402.ogg"},

	/* Artillery Batteries */
	{ID_SOUND_ASSIGNED_TO_SENSOR,                  "pcv403.ogg"},
	{ID_SOUND_SENSOR_LOCKED_ON,                    "pcv404.ogg"},
	{ID_SOUND_ASSIGNED_TO_COUNTER_RADAR,           "pcv405.ogg"},
	{ID_SOUND_ENEMY_BATTERY_LOCATED,               "pcv406.ogg"},
	{ID_SOUND_BATTERY_FIRING_COUNTER_ATTACK,       "pcv407.ogg"},

	/* Vtols */

	{ID_SOUND_INTERCEPTORS_LAUNCHED,               "pcv408.ogg"},
	{ID_SOUND_REARMING,                            "pcv409.ogg"},
	{ID_SOUND_VTOLS_ENGAGING,                      "pcv410.ogg"},
	{ID_SOUND_ASSIGNED,                            "pcv411.ogg"},
	{ID_SOUND_INTERCEPTORS_ASSIGNED,               "pcv412.ogg"},

	/* Command Console */
	{ID_SOUND_COMMAND_CONSOLE_ACTIVATED,           "pcv413.ogg"},
	{ID_SOUND_SHORT_RANGE,                         "pcv414.ogg"},
	{ID_SOUND_LONG_RANGE,                          "pcv415.ogg"},
	{ID_SOUND_OPTIMUM_RANGE,                       "pcv416.ogg"},
	{ID_SOUND_RETREAT_AT_MEDIUM_DAMAGE,            "pcv417.ogg"},
	{ID_SOUND_RETREAT_AT_HEAVY_DAMAGE,             "pcv418.ogg"},
	{ID_SOUND_NO_RETREAT,                          "pcv419.ogg"},
	{ID_SOUND_FIRE_AT_WILL,                        "pcv420.ogg"},
	{ID_SOUND_RETURN_FIRE,                         "pcv421.ogg"},
	{ID_SOUND_CEASEFIRE,                           "pcv422.ogg"},
	{ID_SOUND_HOLD_POSITION,                       "pcv423.ogg"},
	{ID_SOUND_GUARD,                               "pcv424.ogg"},
	{ID_SOUND_PURSUE,                              "pcv425.ogg"},
	{ID_SOUND_PATROL,                              "pcv426.ogg"},
	{ID_SOUND_RETURN_TO_LZ,                        "pcv427.ogg"},
	{ID_SOUND_RECYCLING,                           "pcv428.ogg"},
	{ID_SOUND_SCATTER,                             "pcv429.ogg"},

	/* Tutorial Stuff */

	{ID_SOUND_NOT_POSSIBLE_TRY_AGAIN,              "pcv430.ogg"},
	{ID_SOUND_NO,                                  "pcv431.ogg"},
	{ID_SOUND_THAT_IS_INCORRECT,                   "pcv432.ogg"},
	{ID_SOUND_WELL_DONE,                           "pcv433.ogg"},
	{ID_SOUND_EXCELLENT,                           "pcv434.ogg"},

	/* Group and Commander Assignment */

	{ID_SOUND_ASSIGNED_TO_COMMANDER,               "pcv435.ogg"},
	{ID_SOUND_GROUP_REPORTING,                     "pcv436.ogg"},
	{ID_SOUND_COMMANDER_REPORTING,                 "pcv437.ogg"},

	/* Routing */

	{ID_SOUND_ROUTE_OBSTRUCTED,                    "pcv438.ogg"},
	{ID_SOUND_NO_ROUTE_AVAILABLE,                  "pcv439.ogg"},

	/* Transports and LZS */
	{ID_SOUND_REINFORCEMENTS_AVAILABLE,            "pcv440.ogg"},
	{ID_SOUND_REINFORCEMENTS_IN_TRANSIT,           "pcv441.ogg"},
	{ID_SOUND_TRANSPORT_LANDING,                   "pcv442.ogg"},
	{ID_SOUND_TRANSPORT_UNDER_ATTACK,              "pcv443.ogg"},
	{ID_SOUND_TRANSPORT_REPAIRING,                 "pcv444.ogg"},
	{ID_SOUND_LZ_COMPROMISED,                      "pcv445.ogg"},
	{ID_SOUND_LZ_CLEAR,                            "lz-clear.ogg"},
	{ID_SOUND_TRANSPORT_RETURNING_TO_BASE,         "pcv446.ogg"},
	{ID_SOUND_TRANSPORT_UNABLE_TO_LAND,            "pcv447.ogg"},

	/* Mission Messages */

	{ID_SOUND_MISSION_OBJECTIVE,                   "pcv448.ogg"},
	{ID_SOUND_MISSION_UPDATE,                      "pcv449.ogg"},
	{ID_SOUND_WARZONE_PAUSED,                      "pcv450.ogg"},
	{ID_SOUND_WARZONE_ACTIVE,                      "pcv451.ogg"},
	{ID_SOUND_MISSION_RESULTS,                     "pcv452.ogg"},
	{ID_SOUND_RESEARCH_STOLEN,                     "pcv453.ogg"},
	{ID_SOUND_TECHNOLOGY_TAKEN,                    "pcv454.ogg"},
	{ID_SOUND_INCOMING_TRANSMISSION,               "pcv455.ogg"},
	{ID_SOUND_INCOMING_INTELLIGENCE_REPORT,        "pcv456.ogg"},
	{ID_SOUND_MISSION_FAILED,                      "pcv458.ogg"},
	{ID_SOUND_MISSION_SUCCESSFUL,                  "pcv459.ogg"},
	{ID_SOUND_OBJECTIVE_ACCOMPLISHED,              "pcv460.ogg"},
	{ID_SOUND_OBJECTIVE_FAILED,                    "pcv461.ogg"},
	{ID_SOUND_MISSION_TIMER_ACTIVATED,             "pcv462.ogg"},
	{ID_SOUND_10_MINUTES_REMAINING,                "pcv463.ogg"},
	{ID_SOUND_5_MINUTES_REMAINING,                 "pcv464.ogg"},
	{ID_SOUND_3_MINUTES_REMAINING,                 "pcv465.ogg"},
	{ID_SOUND_2_MINUTES_REMAINING,                 "pcv466.ogg"},
	{ID_SOUND_1_MINUTE_REMAINING,                  "pcv467.ogg"},
	{ID_SOUND_UNIT_CAPTURED,                       "pcv468.ogg"},
	{ID_SOUND_SYSTEM_FAILURE_IMMINENT,             "pcv469.ogg"},
	{ID_SOUND_YOU_ARE_DEFEATED,                    "pcv470.ogg"},
	{ID_SOUND_MISSILE_CODES_DECIPHERED,            "pcv471.ogg"},
	{ID_SOUND_1ST_MISSILE_CODES_DECIPHERED,        "pcv472.ogg"},
	{ID_SOUND_2ND_MISSILE_CODES_DECIPHERED,        "pcv473.ogg"},
	{ID_SOUND_3RD_MISSILE_CODES_DECIPHERED,        "pcv474.ogg"},
	{ID_SOUND_MISSILE_CODES_CRACKED,               "pcv475.ogg"},
	{ID_SOUND_ENTERING_WARZONE,                    "pcv476.ogg"},
	{ID_ALLIANCE_ACC,                              "pcv477.ogg"},
	{ID_ALLIANCE_BRO,                              "pcv478.ogg"},
	{ID_ALLIANCE_OFF,                              "pcv479.ogg"},
	{ID_CLAN_ENTER,                                "pcv480.ogg"},
	{ID_CLAN_EXIT,                                 "pcv481.ogg"},
	{ID_GIFT,                                      "pcv482.ogg"},
	{ID_POWER_TRANSMIT,                            "power-transferred.ogg"},
	{ID_SENSOR_DOWNLOAD,                           "pcv484.ogg"},
	{ID_TECHNOLOGY_TRANSFER,                       "pcv485.ogg"},
	{ID_UNITS_TRANSFER,                            "pcv486.ogg"},

	/* Group and Commander Voices - Male */
	{ID_SOUND_GROUP,                               "group.ogg"},
	{ID_SOUND_GROUP_0,                             "0.ogg"},
	{ID_SOUND_GROUP_1,                             "1.ogg"},
	{ID_SOUND_GROUP_2,                             "2.ogg"},
	{ID_SOUND_GROUP_3,                             "3.ogg"},
	{ID_SOUND_GROUP_4,                             "4.ogg"},
	{ID_SOUND_GROUP_5,                             "5.ogg"},
	{ID_SOUND_GROUP_6,                             "6.ogg"},
	{ID_SOUND_GROUP_7,                             "7.ogg"},
	{ID_SOUND_GROUP_8,                             "8.ogg"},
	{ID_SOUND_GROUP_9,                             "9.ogg"},
	{ID_SOUND_REPORTING,                           "reprting.ogg"},
	{ID_SOUND_COMMANDER,                           "commnder.ogg"},
	{ID_SOUND_COM_SCAVS_DETECTED,                  "com021.ogg"},
	{ID_SOUND_COM_SCAV_BASE_DETECTED,              "com022.ogg"},
	{ID_SOUND_COM_SCAV_OUTPOST_DETECTED,           "com023.ogg"},
	{ID_SOUND_COM_RESOURCE_DETECTED,               "com024.ogg"},
	{ID_SOUND_COM_ARTEFACT_DETECTED,               "com025.ogg"},
	{ID_SOUND_COM_ENEMY_DETECTED,                  "com026.ogg"},
	{ID_SOUND_COM_ENEMY_BASE_DETECTED,             "com027.ogg"},
	{ID_SOUND_COM_ALLY_DETECTED,                   "com028.ogg"},
	{ID_SOUND_COM_ENEMY_TRANSPORT_DETECTED,        "com029.ogg"},
	{ID_SOUND_COM_ENEMY_LZ_DETECTED,               "com030.ogg"},
	{ID_SOUND_COM_FRIENDLY_LZ_DETECTED,            "com031.ogg"},
	{ID_SOUND_COM_NEXUS_TOWER_DETECTED,            "com032.ogg"},
	{ID_SOUND_COM_NEXUS_TURRET_DETECTED,           "com033.ogg"},
	{ID_SOUND_COM_NEXUS_DETECTED,                  "com034.ogg"},
	{ID_SOUND_COM_ENEMY_BATTERY_DETECTED,          "com035.ogg"},
	{ID_SOUND_COM_ENEMY_VTOLS_DETECTED,            "com036.ogg"},
	{ID_SOUND_COM_ROUTE_OBSTRUCTED,                "com037.ogg"},
	{ID_SOUND_COM_NO_ROUTE_AVAILABLE,              "com038.ogg"},
	{ID_SOUND_COM_UNABLE_TO_COMPLY,                "com039.ogg"},
	{ID_SOUND_COM_RETURNING_FOR_REPAIR,            "com040.ogg"},
	{ID_SOUND_COM_HEADING_FOR_RALLY_POINT,         "com041.ogg"},

	/* Radio Clicks */
	{ID_SOUND_RADIOCLICK_1,                        "radclik1.ogg"},
	{ID_SOUND_RADIOCLICK_2,                        "radclik2.ogg"},
	{ID_SOUND_RADIOCLICK_3,                        "radclik3.ogg"},
	{ID_SOUND_RADIOCLICK_4,                        "radclik4.ogg"},
	{ID_SOUND_RADIOCLICK_5,                        "radclik5.ogg"},
	{ID_SOUND_RADIOCLICK_6,                        "radclik6.ogg"},

	/* Transport Pilots */
	{ID_SOUND_APPROACHING_LZ,                      "t-aprolz.ogg"},
	{ID_SOUND_ALRIGHT_BOYS,                        "t-arboys.ogg"},
	{ID_SOUND_GREEN_LIGHT_IN_5,                    "t-grnli5.ogg"},
	{ID_SOUND_GREEN_LIGHT_IN_4,                    "t-grnli4.ogg"},
	{ID_SOUND_GREEN_LIGHT_IN_3,                    "t-grnli3.ogg"},
	{ID_SOUND_GREEN_LIGHT_IN_2,                    "t-grnli2.ogg"},
	{ID_SOUND_GO_GO_GO,                            "t-gogogo.ogg"},
	{ID_SOUND_PREPARE_FOR_DUST_OFF,                "t-dustof.ogg"},

	/* VTol Pilots */

	/* Ver-1 */
	{ID_SOUND_ENEMY_LOCATED1,                      "v-eloc1.ogg"},
	{ID_SOUND_ON_OUR_WAY1,                         "v-onway1.ogg"},
	{ID_SOUND_RETURNING_TO_BASE1,                  "v-retba1.ogg"},
	{ID_SOUND_LOCKED_ON1,                          "v-locon1.ogg"},
	{ID_SOUND_COMMENCING_ATTACK_RUN1,              "v-atkrn1.ogg"},
	{ID_SOUND_ABORTING_ATTACK_RUN1,                "v-abtrn1.ogg"},

	/* Ver-2 */
	{ID_SOUND_ENEMY_LOCATED2,                      "v-eloc2.ogg"},
	{ID_SOUND_ON_OUR_WAY2,                         "v-onway2.ogg"},
	{ID_SOUND_RETURNING_TO_BASE2,                  "v-retba2.ogg"},
	{ID_SOUND_LOCKED_ON2,                          "v-locon2.ogg"},
	{ID_SOUND_COMMENCING_ATTACK_RUN2,              "v-atkrn2.ogg"},
	{ID_SOUND_ABORTING_ATTACK_RUN2,                "v-abtrn2.ogg"},

	/* Ver-3 */
	{ID_SOUND_ENEMY_LOCATED3,                      "v-eloc3.ogg"},
	{ID_SOUND_ON_OUR_WAY3,                         "v-onway3.ogg"},
	{ID_SOUND_RETURNING_TO_BASE3,                  "v-retba3.ogg"},
	{ID_SOUND_LOCKED_ON3,                          "v-locon3.ogg"},
	{ID_SOUND_COMMENCING_ATTACK_RUN3,              "v-atkrn3.ogg"},
	{ID_SOUND_ABORTING_ATTACK_RUN3,                "v-abtrn3.ogg"},


	/* The Collective */
	{ID_SOUND_COLL_CLEANSE_AND_DESTROY,            "col011a.ogg"},
	{ID_SOUND_COLL_DESTROYING_BIOLOGICALS,         "col012a.ogg"},
	{ID_SOUND_COLL_ATTACK,                         "col013a.ogg"},
	{ID_SOUND_COLL_FIRE,                           "col014a.ogg"},
	{ID_SOUND_COLL_ENEMY_DETECTED,                 "col015a.ogg"},
	{ID_SOUND_COLL_ENGAGING,                       "col016a.ogg"},
	{ID_SOUND_COLL_STARTING_ATTACK_RUN,            "col017a.ogg"},
	{ID_SOUND_COLL_DIE,                            "col018a.ogg"},
	{ID_SOUND_COLL_INTERCEPT_AND_DESTROY,          "col019a.ogg"},
	{ID_SOUND_COLL_ENEMY_DESTROYED,                "col020a.ogg"},

	/* SFX */
	/* Weapon Sounds */
	{ID_SOUND_ROCKET,                              "rocket.ogg"},
	{ID_SOUND_ROTARY_LASER,                        "rotlsr.ogg"},
	{ID_SOUND_GAUSSGUN,                            "gaussgun.ogg"},
	{ID_SOUND_LARGE_CANNON,                        "lrgcan.ogg"},
	{ID_SOUND_SMALL_CANNON,                        "smlcan.ogg"},
	{ID_SOUND_MEDIUM_CANNON,                       "medcan.ogg"},
	{ID_SOUND_FLAME_THROWER,                       "flmthrow.ogg"},
	{ID_SOUND_PULSE_LASER,                         "plslsr.ogg"},
	{ID_SOUND_BEAM_LASER,                          "bemlsr.ogg"},
	{ID_SOUND_MORTAR,                              "mortar.ogg"},
	{ID_SOUND_HOWITZ_FLIGHT,                       "hwtzflgt.ogg"},
	{ID_SOUND_BABA_MG_1,                           "mgbar1.ogg"},
	{ID_SOUND_BABA_MG_2,                           "mgbar2.ogg"},
	{ID_SOUND_BABA_MG_3,                           "mgbar3.ogg"},
	{ID_SOUND_BABA_MG_HEAVY,                       "mgheavy.ogg"},
	{ID_SOUND_BABA_MG_TOWER,                       "mgtower.ogg"},
	{ID_SOUND_SPLASH,                              "splash.ogg"},
	{ID_SOUND_ASSAULT_MG,                          "asltmg.ogg"},
	{ID_SOUND_RAPID_CANNON,                        "rapdcan.ogg"},
	{ID_SOUND_HIVEL_CANNON,                        "hivelcan.ogg"},
	{ID_SOUND_NEXUS_TOWER,                         "nxstower.ogg"},

	/* Construction sounds */
	{ID_SOUND_WELD_1,                              "weld-1.ogg"},
	{ID_SOUND_WELD_2,                              "weld-2.ogg"},
	{ID_SOUND_CONSTRUCTION_START,                  "bldstart.ogg"},
	{ID_SOUND_CONSTRUCTION_LOOP,                   "bldloop.ogg"},
	{ID_SOUND_CONSTRUCTION_1,                      "build1.ogg"},
	{ID_SOUND_CONSTRUCTION_2,                      "build2.ogg"},
	{ID_SOUND_CONSTRUCTION_3,                      "build3.ogg"},
	{ID_SOUND_CONSTRUCTION_4,                      "build4.ogg"},

	/* Explosions */

	{ID_SOUND_EXPLOSION_SMALL,                     "smlexpl.ogg"},
	{ID_SOUND_EXPLOSION_LASER,                     "lsrexpl.ogg"},
	{ID_SOUND_EXPLOSION,                           "lrgexpl.ogg"},
	{ID_SOUND_EXPLOSION_ANTITANK,                  "atnkexpl.ogg"},
	{ID_SOUND_RICOCHET_1,                          "richet1.ogg"},
	{ID_SOUND_RICOCHET_2,                          "richet2.ogg"},
	{ID_SOUND_RICOCHET_3,                          "richet3.ogg"},
	{ID_SOUND_BARB_SQUISH,                         "squish.ogg"},
	{ID_SOUND_BUILDING_FALL,                       "bldfall.ogg"},
	{ID_SOUND_NEXUS_EXPLOSION,                     "nxsexpld.ogg"},

	/* Droid Engine Noises */
	{ID_SOUND_CONSTRUCTOR_MOVEOFF,                 "con-move-off.ogg"},
	{ID_SOUND_CONSTRUCTOR_MOVE,                    "con-move.ogg"},
	{ID_SOUND_CONSTRUCTOR_SHUTDOWN,                "con-shut-down.ogg"},

	/* Transports */
	{ID_SOUND_BLIMP_FLIGHT,                        "tflight.ogg"},
	{ID_SOUND_BLIMP_IDLE,                          "thover.ogg"},
	{ID_SOUND_BLIMP_LAND,                          "tland.ogg"},
	{ID_SOUND_BLIMP_TAKE_OFF,                      "tstart.ogg"},

	/* Vtols */
	{ID_SOUND_VTOL_LAND,                           "vtolland.ogg"},
	{ID_SOUND_VTOL_OFF,                            "vtoloff.ogg"},
	{ID_SOUND_VTOL_MOVE,                           "vtol-move.ogg"},

	/* Treads */
	{ID_SOUND_TREAD,                               "tread.ogg"},

	/* Hovers */
	{ID_SOUND_HOVER_MOVE,                          "hovmove.ogg"},
	{ID_SOUND_HOVER_START,                         "hovstart.ogg"},
	{ID_SOUND_HOVER_STOP,                          "hovstop.ogg"},

	/* Cyborgs */
	{ID_SOUND_CYBORG_MOVE,                         "cyber-move.ogg"},

	/* Buildings */

	{ID_SOUND_OIL_PUMP_2,                          "oilpump.ogg"},
	{ID_SOUND_POWER_HUM,                           "powerhum.ogg"},
	{ID_SOUND_POWER_SPARK,                         "powerspk.ogg"},
	{ID_SOUND_STEAM,                               "steam.ogg"},
	{ID_SOUND_ECM_TOWER,                           "ecmtower.ogg"},
	{ID_SOUND_FIRE_ROAR,                           "freroar.ogg"},

	/* Misc */
	{ID_SOUND_HELP,									"help.ogg"},
	{ID_SOUND_BARB_SCREAM,                         "scream.ogg"},
	{ID_SOUND_BARB_SCREAM2,							"scream2.ogg"},
	{ID_SOUND_BARB_SCREAM3,							"scream3.ogg"},
	{ID_SOUND_OF_SILENCE,                          "silence.ogg"},

	/* Extra */
	{ID_SOUND_LANDING_ZONE,                        "lndgzne.ogg"},
	{ID_SOUND_SATELLITE_UPLINK,                    "pcv652.ogg"},
	{ID_SOUND_NASDA_CENTRAL,                       "pcv653.ogg"},
	{ID_SOUND_NUCLEAR_REACTOR,                     "pcv654.ogg"},
	{ID_SOUND_SAM_SITE,                            "pcv655.ogg"},
	{ID_SOUND_MISSILE_SILO,                        "pcv656.ogg"},
	{ID_SOUND_MISSILE_NME_DETECTED,                "nmedeted.ogg"},
	{ID_SOUND_STRUCTURE_CAPTURED,                  "pcv611.ogg"},
	{ID_SOUND_CIVILIAN_RESCUED,                    "pcv612.ogg"},
	{ID_SOUND_CIVILIANS_RESCUED,                   "pcv613.ogg"},
	{ID_SOUND_UNITS_RESCUED,                       "pcv615.ogg"},
	{ID_SOUND_GROUP_RESCUED,                       "pcv616.ogg"},
	{ID_SOUND_GROUP_CAPTURED,                      "pcv618.ogg"},
	{ID_SOUND_OBJECTIVE_CAPTURED,                  "pcv621.ogg"},
	{ID_SOUND_OBJECTIVE_DESTROYED,                 "pcv622.ogg"},
	{ID_SOUND_STRUCTURE_INFECTED,                  "pcv623.ogg"},
	{ID_SOUND_GROUP_INFECTED,                      "pcv625.ogg"},
	{ID_SOUND_OUT_OF_TIME,                         "pcv629.ogg"},
	{ID_SOUND_ENEMY_ESCAPED,                       "pcv631.ogg"},
	{ID_SOUND_ENEMY_ESCAPING,                      "pcv632.ogg"},
	{ID_SOUND_ENEMY_TRANSPORT_LANDING,             "pcv633.ogg"},
	{ID_SOUND_TEAM_ALPHA_ERADICATED,               "pcv635.ogg"},
	{ID_SOUND_TEAM_BETA_ERADICATED,                "pcv636.ogg"},
	{ID_SOUND_TEAM_GAMMA_ERADICATED,               "pcv637.ogg"},
	{ID_SOUND_TEAM_ALPHA_RESCUED,                  "pcv638.ogg"},
	{ID_SOUND_TEAM_BETA_RESCUED,                   "pcv639.ogg"},
	{ID_SOUND_TEAM_GAMMA_RESCUED,                  "pcv640.ogg"},
	{ID_SOUND_LASER_SATELLITE_FIRING,              "pcv650.ogg"},
	{ID_SOUND_INCOMING_LASER_SAT_STRIKE,           "pcv657.ogg"},

	/* Nexus */
	{ID_SOUND_NEXUS_DEFENCES_ABSORBED,             "defabsrd.ogg"},
	{ID_SOUND_NEXUS_DEFENCES_NEUTRALISED,          "defnut.ogg"},
	{ID_SOUND_NEXUS_LAUGH1,                        "laugh1.ogg"},
	{ID_SOUND_NEXUS_LAUGH2,                        "laugh2.ogg"},
	{ID_SOUND_NEXUS_LAUGH3,                        "laugh3.ogg"},
	{ID_SOUND_NEXUS_PRODUCTION_COMPLETED,          "pordcomp.ogg"},
	{ID_SOUND_NEXUS_RESEARCH_ABSORBED,             "resabsrd.ogg"},
	{ID_SOUND_NEXUS_STRUCTURE_ABSORBED,            "strutabs.ogg"},
	{ID_SOUND_NEXUS_STRUCTURE_NEUTRALISED,         "strutnut.ogg"},
	{ID_SOUND_NEXUS_SYNAPTIC_LINK,                 "synplnk.ogg"},
	{ID_SOUND_NEXUS_UNIT_ABSORBED,                 "untabsrd.ogg"},
	{ID_SOUND_NEXUS_UNIT_NEUTRALISED,              "untnut.ogg"},

	/* extra multiplayer sounds */
	{ID_SOUND_CYBORG_GROUND,                       "cybgrnd.ogg"},
	{ID_SOUND_CYBORG_HEAVY,                        "hvcybmov.ogg"},
	{ID_SOUND_EMP,                                 "emp.ogg"},
	{ID_SOUND_LASER_HEAVY,                         "hevlsr.ogg"},
	{ID_SOUND_PLASMA_FLAMER,                       "plasflm.ogg"},
	{ID_SOUND_UPLINK,                              "uplink.ogg"},
	{ID_SOUND_LAS_SAT_COUNTDOWN,                   "lasstrk.ogg"},
	{ID_SOUND_BEACON,								"beacon.ogg"},
};

/***************************************************************************/

INGAME_AUDIO audio_GetIDFromStr(const char *fileName)
{
	unsigned int i;

	for (i = 0; i != ID_SOUND_NEXT; ++i)
	{
		if (strcmp(fileName, asAudioID[i].fileName) == 0)
		{
			ASSERT(i == asAudioID[i].ID,
			       "audioID_GetIDFromStr: %s stored IDs don't match", fileName);

			return asAudioID[i].ID;
		}
	}

	return NO_SOUND;
}

/***************************************************************************/
