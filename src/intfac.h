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
 * @file intfac.h
 *
 * interface enums
 * List corresponding to the files in data/base/images/intfac.img
 */


#ifndef __INCLUDED_SRC_INTFAC_H__
#define __INCLUDED_SRC_INTFAC_H__

enum INTFAC_TYPE
{
	IMAGE_PBAR_EMPTY,
	IMAGE_PBAR_AVAIL,
	IMAGE_PBAR_USED,
	IMAGE_PBAR_REQUIRED,
	IMAGE_BUT0_UP,
	IMAGE_BUT0_DOWN,
	IMAGE_BUTB0_UP,
	IMAGE_BUTB0_DOWN,
	IMAGE_BUT_HILITE,
	IMAGE_BUTB_HILITE,
	IMAGE_TAB1,
	IMAGE_TAB2,
	IMAGE_TAB3,
	IMAGE_TAB4,
	IMAGE_TAB1DOWN,
	IMAGE_TAB2DOWN,
	IMAGE_TAB3DOWN,
	IMAGE_TAB4DOWN,
	IMAGE_TABSELECTED,
	IMAGE_TABHILIGHT,
	IMAGE_FRAME_C0,
	IMAGE_FRAME_C1,
	IMAGE_FRAME_C2,
	IMAGE_FRAME_C3,
	IMAGE_PBAR_TOP,
	IMAGE_PBAR_BOTTOM,
	IMAGE_INTELMAP_UP,
	IMAGE_INTELMAP_DOWN,
	IMAGE_COMMANDDROID_UP,
	IMAGE_COMMANDDROID_DOWN,
	IMAGE_DESIGN_UP,
	IMAGE_DESIGN_DOWN,
	IMAGE_BUILD_UP,
	IMAGE_BUILD_DOWN,
	IMAGE_RESEARCH_UP,
	IMAGE_RESEARCH_DOWN,
	IMAGE_MANUFACTURE_UP,
	IMAGE_MANUFACTURE_DOWN,
	IMAGE_RETICULE_HILIGHT,
	IMAGE_RETICULE_BUTDOWN,
	IMAGE_CANCEL_UP,
	IMAGE_CANCEL_DOWN,
	IMAGE_CANCEL_HILIGHT,
	IMAGE_CLOSE,
	IMAGE_CLOSEDOWN,
	IMAGE_CLOSEHILIGHT,
	IMAGE_SLIDER_BUT,
	IMAGE_FRAME_HT,
	IMAGE_FRAME_HB,
	IMAGE_SLIDER_BACK,
	IMAGE_FRAME_VL,
	IMAGE_FRAME_VR,
	IMAGE_DES_TURRET,
	IMAGE_DES_TURRETH,
	IMAGE_DES_BODY,
	IMAGE_DES_BODYH,
	IMAGE_DES_PROPULSION,
	IMAGE_DES_PROPULSIONH,
	IMAGE_DES_HILIGHT,
	IMAGE_FRAME_VL2,
	IMAGE_FRAME_VR2,
	IMAGE_FRAME_VLH,
	IMAGE_FRAME_VRH,
	IMAGE_DES_WEAPONS,
	IMAGE_DES_WEAPONSDOWN,
	IMAGE_DES_SYSTEMS,
	IMAGE_DES_SYSTEMSDOWN,
	IMAGE_DES_COMMAND,
	IMAGE_DES_COMMANDDOWN,
	IMAGE_FRAME_HT2,
	IMAGE_FRAME_HB2,
	IMAGE_FRAME_HTH,
	IMAGE_FRAME_HBH,
	IMAGE_DES_POWERCURR,
	IMAGE_DES_STATSBACK,
	IMAGE_DES_STATSCURR,
	IMAGE_DES_POWERBACK,
	IMAGE_DES_EDITBOXLEFT,
	IMAGE_DES_EDITBOXLEFTH,
	IMAGE_FRAME_VC0,
	IMAGE_FRAME_VC1,
	IMAGE_FRAME_VC2,
	IMAGE_FRAME_VC3,
	IMAGE_FRAME_HC0,
	IMAGE_FRAME_HC1,
	IMAGE_FRAME_HC2,
	IMAGE_FRAME_HC3,
	IMAGE_DES_BACK,
	IMAGE_DES_STATSCOMP,
	IMAGE_DES_EXTRAHI,
	IMAGE_DES_TABWEAPON,
	IMAGE_DES_TABWEAPONDOWN,
	IMAGE_DES_BUILDRATE,
	IMAGE_DES_RANGE,
	IMAGE_DES_ARMOUR_EXPLOSIVE,
	IMAGE_DES_CROSSCOUNTRY,
	IMAGE_DES_ROAD,
	IMAGE_DES_WEIGHT,
	IMAGE_DES_DAMAGE,
	IMAGE_DES_POWER,
	IMAGE_DES_HOVER,
	IMAGE_DES_FIRERATE,
	IMAGE_DES_BODYPOINTS,
	IMAGE_ROCKET,
	IMAGE_CANNON,
	IMAGE_HOVERCRAFT,
	IMAGE_RES_MINOR_PLASSTEEL,
	IMAGE_ECM,
	IMAGE_PLASCRETE,
	IMAGE_RES_MINOR_RADAR,
	IMAGE_RES_MAJOR_PLASCRETE,
	IMAGE_RES_MAJOR_ELECTRONIC,
	IMAGE_RES_MAJOR_HEAVYWEP,
	IMAGE_RES_MAJOR_HOVER,
	IMAGE_0,
	IMAGE_1,
	IMAGE_2,
	IMAGE_3,
	IMAGE_4,
	IMAGE_5,
	IMAGE_6,
	IMAGE_7,
	IMAGE_8,
	IMAGE_9,
	IMAGE_DES_BARBACK,
	IMAGE_DES_BARBLUE,
	IMAGE_DES_BARRED,
	IMAGE_DES_BARYELLOW,
	IMAGE_INTEL_RESEARCH,
	IMAGE_INTEL_RESEARCHDOWN,
	IMAGE_INTEL_MISSION,
	IMAGE_INTEL_MISSIONDOWN,
	IMAGE_INTEL_CAMPAIGN,
	IMAGE_INTEL_CAMPAIGNDOWN,
	IMAGE_ASTERISK,
	IMAGE_SIDETAB,
	IMAGE_SIDETABHI,
	IMAGE_SIDETABDOWN,
	IMAGE_SIDETABSEL,
	IMAGE_DES_STATBACKLEFT,
	IMAGE_DES_STATBACKRIGHT,
	IMAGE_DES_STATBACKMID,
	IMAGE_SLIDER_BIG,
	IMAGE_STAR,
	IMAGE_TRACKS,
	IMAGE_RES_MINOR_AUTOWEAPONS,
	IMAGE_RES_MAJOR_ROCKET,
	IMAGE_QUESTION_MARK,
	IMAGE_RES_DROIDTECH,
	IMAGE_RES_WEAPONTECH,
	IMAGE_RES_COMPUTERTECH,
	IMAGE_RES_POWERTECH,
	IMAGE_RES_SYSTEMTECH,
	IMAGE_RES_STRUCTURETECH,
	IMAGE_RES_QUESTIONMARK,
	IMAGE_ORD_DESTRUCT1UP,
	IMAGE_ORD_DESTRUCT1DOWN,
	IMAGE_ORD_DESTRUCT1GREY,
	IMAGE_ORD_RANGE1UP,
	IMAGE_ORD_RANGE1DOWN,
	IMAGE_ORD_RANGE1GREY,
	IMAGE_ORD_RANGE2UP,
	IMAGE_ORD_RANGE2DOWN,
	IMAGE_ORD_RANGE2GREY,
	IMAGE_ORD_RANGE3UP,
	IMAGE_ORD_RANGE3DOWN,
	IMAGE_ORD_RANGE3GREY,
	IMAGE_ORD_REPAIR2UP,
	IMAGE_ORD_REPAIR2DOWN,
	IMAGE_ORD_REPAIR2GREY,
	IMAGE_ORD_REPAIR1UP,
	IMAGE_ORD_REPAIR1DOWN,
	IMAGE_ORD_REPAIR1GREY,
	IMAGE_ORD_REPAIR3UP,
	IMAGE_ORD_REPAIR3DOWN,
	IMAGE_ORD_REPAIR3GREY,
	IMAGE_ORD_FATWILLUP,
	IMAGE_ORD_FATWILLDOWN,
	IMAGE_ORD_FATWILLGREY,
	IMAGE_ORD_RETFIREUP,
	IMAGE_ORD_RETFIREDOWN,
	IMAGE_ORD_RETFIREGREY,
	IMAGE_ORD_HOLDFIREUP,
	IMAGE_ORD_HOLDFIREDOWN,
	IMAGE_ORD_HOLDFIREGRY,
	IMAGE_ORD_HALTUP,
	IMAGE_ORD_HALTDOWN,
	IMAGE_ORD_HALTGREY,
	IMAGE_ORD_GOTOHQUP,
	IMAGE_ORD_GOTOHQDOWN,
	IMAGE_ORD_GOTOHQGREY,
	IMAGE_ORD_DESTRUCT2UP,
	IMAGE_ORD_DESTRUCT2DOWN,
	IMAGE_ORD_DESTRUCT2GREY,
	IMAGE_SLIDER_BIGBUT,
	IMAGE_SLIDER_INFINITY,
	IMAGE_RAD_ENMREAD,
	IMAGE_RAD_ENM1,
	IMAGE_RAD_ENM2,
	IMAGE_RAD_ENM3,
	IMAGE_RAD_RESREAD,
	IMAGE_RAD_RES1,
	IMAGE_RAD_RES2,
	IMAGE_RAD_RES3,
	IMAGE_RAD_ARTREAD,
	IMAGE_RAD_ART1,
	IMAGE_RAD_ART2,
	IMAGE_RAD_ART3,
	IMAGE_MULTI_NOAL,
	IMAGE_MULTI_OFFAL,
	IMAGE_MULTI_OFFAL_HI,
	IMAGE_MULTI_OFFALGREY,
	IMAGE_MULTI_AL,
	IMAGE_MULTI_NOAL_HI,
	IMAGE_MULTI_AL_HI,
	IMAGE_MULTI_VIS,
	IMAGE_MULTI_POW,
	IMAGE_MULTI_DRO,
	IMAGE_MULTI_TEK,
	IMAGE_MULTI_TEK_HI,
	IMAGE_MULTI_DRO_HI,
	IMAGE_MULTI_POW_HI,
	IMAGE_MULTI_VIS_HI,
	IMAGE_CMDDROID_EXP,
	IMAGE_NRUTER,
	IMAGE_MISSION_CLOCK,
	IMAGE_LAUNCHDOWN,
	IMAGE_TRANSETA_UP,
	IMAGE_ORD_FAC1UP,
	IMAGE_ORD_FAC1DOWN,
	IMAGE_ORD_FAC2UP,
	IMAGE_ORD_FAC2DOWN,
	IMAGE_ORD_FAC3UP,
	IMAGE_ORD_FAC3DOWN,
	IMAGE_ORD_FAC4UP,
	IMAGE_ORD_FAC4DOWN,
	IMAGE_ORD_FAC5UP,
	IMAGE_ORD_FAC5DOWN,
	IMAGE_ORD_FACHILITE,
	IMAGE_RETICULE_GREY,
	IMAGE_INFINITE_UP,
	IMAGE_INFINITE_DOWN,
	IMAGE_INFINITE_HI,
	IMAGE_DES_BIN,
	IMAGE_DES_BINH,
	IMAGE_AUDIO_LASTSAMPLE,
	IMAGE_AUDIO_LASTSAMPLEH,
	IMAGE_DES_POWERBAR_LEFT,
	IMAGE_DES_POWERBAR_RIGHT,
	IMAGE_DES_EDITBOXMID,
	IMAGE_DES_EDITBOXRIGHT,
	IMAGE_DES_EDITBOXMIDH,
	IMAGE_DES_EDITBOXRIGHTH,
	IMAGE_RES_CYBORGTECH,
	IMAGE_LOOP_UP,
	IMAGE_LOOP_DOWN,
	IMAGE_LOOP_HI,
	IMAGE_CDP_DOWN,
	IMAGE_CDP_UP,
	IMAGE_CDP_HI,
	IMAGE_FDP_DOWN,
	IMAGE_FDP_UP,
	IMAGE_FDP_HI,
	IMAGE_VDP_DOWN,
	IMAGE_VDP_UP,
	IMAGE_VDP_HI,
	IMAGE_OBSOLETE_HIDE_UP,
	IMAGE_OBSOLETE_HIDE_HI,
	IMAGE_OBSOLETE_SHOW_UP,
	IMAGE_OBSOLETE_SHOW_HI,
	IMAGE_GN_STAR,
	IMAGE_GN_15,
	IMAGE_GN_14,
	IMAGE_GN_13,
	IMAGE_GN_12,
	IMAGE_GN_11,
	IMAGE_GN_10,
	IMAGE_GN_9,
	IMAGE_GN_8,
	IMAGE_GN_7,
	IMAGE_GN_6,
	IMAGE_GN_5,
	IMAGE_GN_4,
	IMAGE_GN_3,
	IMAGE_GN_2,
	IMAGE_GN_1,
	IMAGE_GN_0,
	IMAGE_ORD_CIRCLEUP,
	IMAGE_ORD_CIRCLEDOWN,
	IMAGE_ORD_CIRCLEGREY,
	IMAGE_ORD_PURSUEUP,
	IMAGE_ORD_PURSUEDOWN,
	IMAGE_ORD_PURSUEGREY,
	IMAGE_ORD_GUARDUP,
	IMAGE_ORD_GUARDDOWN,
	IMAGE_ORD_GUARDGREY,
	IMAGE_ORD_RTRUP,
	IMAGE_ORD_RTRDOWN,
	IMAGE_ORD_RTRGREY,
	IMAGE_ORD_EMBARKUP,
	IMAGE_ORD_EMBARKDOWN,
	IMAGE_ORD_EMBARKGREY,
	IMAGE_MULTI_NOCHAN,
	IMAGE_MULTI_CHAN,
	IMAGE_CDCHANGE_OK,
	IMAGE_CDCHANGE_CANCEL,
	IMAGE_LEV_0,
	IMAGE_LEV_1,
	IMAGE_LEV_2,
	IMAGE_LEV_3,
	IMAGE_LEV_4,
	IMAGE_LEV_5,
	IMAGE_LEV_6,
	IMAGE_LEV_7,
	IMAGE_RES_DEFENCE,
	IMAGE_RES_GRPUPG,
	IMAGE_RES_GRPREP,
	IMAGE_RES_GRPROF,
	IMAGE_RES_GRPACC,
	IMAGE_RES_GRPDAM,
	IMAGE_MISSION_CLOCK_UP,
	IMAGE_TARGET1,
	IMAGE_TARGET2,
	IMAGE_TARGET3,
	IMAGE_TARGET4,
	IMAGE_TARGET5,
	IMAGE_TAB1_SM,
	IMAGE_TAB1SELECTED_SM,
	IMAGE_TAB1DOWN_SM,
	IMAGE_TABHILIGHT_SM,
	IMAGE_DES_ARMOUR_KINETIC,
	IMAGE_BLUE1,
	IMAGE_BLUE2,
	IMAGE_BLUE3,
	IMAGE_BLUE4,
	IMAGE_BLUE5,
	IMAGE_BLUE6,
	IMAGE_LAUNCHUP,
	IMAGE_TRANSETA_DOWN,
	IMAGE_ORD_FIREDES_UP,
	IMAGE_ORD_FIREDES_DOWN,
	IMAGE_ORD_ACCREP_UP,
	IMAGE_ORD_ACCREP_DOWN,
	IMAGE_LFTTAB,
	IMAGE_LFTTABD,
	IMAGE_LFTTABH,
	IMAGE_RGTTAB,
	IMAGE_RGTTABD,
	IMAGE_RGTTABH,
	IMAGE_NADDA,
	IMAGE_ORD_PATROLUP,
	IMAGE_ORD_PATROLDOWN,
	IMAGE_DISCONNECT_LO,
	IMAGE_DISCONNECT_HI,
	IMAGE_PLAYER_LEFT_LO,
	IMAGE_PLAYER_LEFT_HI,
	IMAGE_EDIT_OPTIONS_UP,
	IMAGE_EDIT_OPTIONS_DOWN,
	IMAGE_EDIT_TILES_UP,
	IMAGE_EDIT_TILES_DOWN,
	IMAGE_EDIT_DESIGN_UP,
	IMAGE_EDIT_DESIGN_DOWN,
	IMAGE_EDIT_BUILD_UP,
	IMAGE_EDIT_BUILD_DOWN,
	IMAGE_EDIT_RESEARCH_UP,
	IMAGE_EDIT_RESEARCH_DOWN,
	IMAGE_EDIT_MANUFACTURE_UP,
	IMAGE_EDIT_MANUFACTURE_DOWN,
	RADAR_NORTH,
	IMAGE_ORIGIN_VISUAL,
	IMAGE_ORIGIN_COMMANDER,
	IMAGE_ORIGIN_SENSOR_STANDARD,
	IMAGE_ORIGIN_SENSOR_CB,
	IMAGE_ORIGIN_SENSOR_AIRDEF,
	IMAGE_ORIGIN_RADAR_DETECTOR,
	IMAGE_WAITING_LO,
	IMAGE_WAITING_HI,
	IMAGE_DESYNC_LO,
	IMAGE_DESYNC_HI,
	IMAGE_CURSOR_EMBARK,
	IMAGE_CURSOR_DEST,
	IMAGE_CURSOR_DEFAULT,
	IMAGE_CURSOR_BUILD,
	IMAGE_CURSOR_SCOUT,
	IMAGE_CURSOR_DISEMBARK,
	IMAGE_CURSOR_ATTACK,
	IMAGE_CURSOR_GUARD,
	IMAGE_CURSOR_FIX,
	IMAGE_CURSOR_SELECT,
	IMAGE_CURSOR_REPAIR,
	IMAGE_CURSOR_PICKUP,
	IMAGE_CURSOR_NOTPOS,
	IMAGE_CURSOR_MOVE,
	IMAGE_CURSOR_LOCKON,
	IMAGE_CURSOR_ECM,
	IMAGE_CURSOR_ATTACH,
	IMAGE_CURSOR_BRIDGE,
	IMAGE_CURSOR_BOMB,
	IMAGE_SLIDER_AI,
	IMAGE_RAD_BURNRESREAD,
	IMAGE_RAD_BURNRES1,
	IMAGE_RAD_BURNRES2,
	IMAGE_RAD_BURNRES3,
	IMAGE_RAD_BURNRES4,
	IMAGE_RAD_BURNRES5,
	IMAGE_RAD_BURNRES6,
	IMAGE_ALLY_RESEARCH,
	IMAGE_ALLY_RESEARCH_TC,
	IMAGE_GENERIC_TANK,
	IMAGE_GENERIC_TANK_TC,
	IMAGE_DES_SAVE,
	IMAGE_DES_SAVEH,
	IMAGE_DES_DELETE,
	IMAGE_DES_DELETEH,
	IMAGE_ATTACK_UP,
	IMAGE_ATTACK_DOWN,
	IMAGE_HOLD_UP,
	IMAGE_HOLD_DOWN,
	IMAGE_PATROL_UP,
	IMAGE_PATROL_DOWN,
	IMAGE_STOP_UP,
	IMAGE_STOP_DOWN,
	IMAGE_GUARD_UP,
	IMAGE_GUARD_DOWN,
	// begin: not currently used?
	IMAGE_PAUSE_UP,
	IMAGE_PAUSE_DOWN,
	IMAGE_AUTOREPAIR_UP,
	IMAGE_AUTOREPAIR_DOWN,
	IMAGE_SELFDESTRUCT_UP,
	IMAGE_SELFDESTRUCT_DOWN,
	IMAGE_REPAIR_UP,
	IMAGE_REPAIR_DOWN,
	IMAGE_JAM_UP,
	IMAGE_JAM_DOWN,
	IMAGE_DISEMBARK_UP,
	IMAGE_DISEMBARK_DOWN,
	IMAGE_REARM_UP,
	IMAGE_REARM_DOWN,
	IMAGE_DEMOLISH_UP,
	IMAGE_DEMOLISH_DOWN,
	IMAGE_OBSERVE_UP,
	IMAGE_OBSERVE_DOWN,
	// end: not currently used?
	IMAGE_SPECSTATS_DOWN,
	IMAGE_SPECSTATS_UP,
	IMAGE_INTFAC_VOLUME_UP,
	IMAGE_INTFAC_VOLUME_MUTE
};

#endif //__INCLUDED_SRC_INTFAC_H__
