/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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

#ifndef __INCLUDED_SRC_DESIGN_H__
#define __INCLUDED_SRC_DESIGN_H__

#include "lib/widget/widgbase.h"
#include "src/droiddef.h"

/* Design screen ID's */
#define IDDES_FORM			5000		// The base form for the design screen
#define IDDES_STATSFORM		5001		// The design screen stats form
#define IDDES_SYSTEMFORM	5006		// The clickable form for the weapon/ecm/sensor
#define IDDES_BODYFORM		5007		// The clickable form for the body
#define IDDES_PROPFORM		5008		// The clickable form for the propulsion
#define IDDES_3DVIEW		5009		// The 3D view of the droid
#define IDDES_BIN			5011		// The bin button
#define IDDES_NAMEBOX		5013		// The Name box
#define IDDES_POWERFORM		5019		// The form for the power and points bars
#define IDDES_TEMPLBASE		5020		// The base form for the Template (left) form
#define IDDES_RIGHTBASE		5021		// The base form for the right form!
#define IDDES_POWERBAR		5023		// The power bar for the template

#define IDDES_WEAPONS		5024		// The weapon button for the Component form (right)
#define IDDES_SYSTEMS		5025		// The systems (sensor/ecm) button for the Component form
#define IDDES_COMMAND		5026		// The command button for the Component form

#define	IDDES_PARTFORM		5027		// Part buttons form
#define IDDES_WEAPONS_A		5028		// The weapon TURRET_A button for the Component form (right)
#define IDDES_WEAPONS_B		5029		// The weapon TURRET_B button for the Component form (right)

#define IDDES_STOREBUTTON	5905		// Stored template button

/* Design screen bar graph IDs */
#define IDDES_BODYARMOUR_K	5100		// The body armour bar graph for kinetic weapons
#define IDDES_BODYPOWER		5101		// The body power plant graph
#define IDDES_BODYWEIGHT	5102		// The body weight
#define IDDES_PROPROAD		5105		// The propulsion road speed
#define IDDES_PROPCOUNTRY	5106		// The propulsion cross country speed
#define IDDES_PROPWATER		5107		// The propulsion water speed
#define IDDES_PROPAIR		5108		// The propulsion air speed
#define IDDES_PROPWEIGHT	5109		// The propulsion weight
#define IDDES_SENSORRANGE	5110		// The sensor range
#define IDDES_SENSORWEIGHT	5112		// The sensor weight
#define IDDES_ECMPOWER		5115		// The ecm power
#define IDDES_ECMWEIGHT		5116		// The ecm weight
#define IDDES_WEAPRANGE		5120		// The weapon range
#define IDDES_WEAPDAMAGE	5121		// The weapon damage
#define IDDES_WEAPROF		5122		// The weapon rate of fire
#define IDDES_WEAPWEIGHT	5123		// The weapon weight
#define IDDES_CONSTPOINTS	5125		// The construction build points
#define IDDES_CONSTWEIGHT	5126		// The construction weight
//extras added AB 3/9/97
#define IDDES_BODYPOINTS	5127		// The body points bar graph
#define IDDES_BODYARMOUR_H	5128		// The body armour bar graph for heat weapons
#define IDDES_REPAIRPOINTS  5129		// The Repair points
#define IDDES_REPAIRWEIGHT	5130		// The repair weight

/* Design screen bar graph labels */
#define IDDES_BODYARMOURKLAB	5200		// The body armour (kinetic) bar graph label
#define IDDES_BODYPOWERLAB		5201		// The body power plant graph label
#define IDDES_BODYWEIGHTLAB		5202		// The body weight graph label
#define IDDES_PROPROADLAB		5205		// The propulsion road speed label
#define IDDES_PROPCOUNTRYLAB	5206		// The propulsion cross country speed label
#define IDDES_PROPWATERLAB		5207		// The propulsion water speed label
#define IDDES_PROPAIRLAB		5208		// The propulsion air speed label
#define IDDES_PROPWEIGHTLAB		5209		// The propulsion weight label
#define IDDES_SENSORRANGELAB	5210		// The sensor range label
#define IDDES_SENSORWEIGHTLAB	5212		// The sensor weight label
#define IDDES_ECMPOWERLAB		5215		// The ecm power label
#define IDDES_ECMWEIGHTLAB		5216		// The ecm weight label
#define IDDES_WEAPRANGELAB		5220		// The weapon range label
#define IDDES_WEAPDAMAGELAB		5221		// The weapon damage label
#define IDDES_WEAPROFLAB		5222		// The weapon rate of fire label
#define IDDES_WEAPWEIGHTLAB		5223		// The weapon weight label
#define IDDES_CONSTPOINTSLAB	5225		// The construction build points label
#define IDDES_CONSTWEIGHTLAB	5226		// The construction weight label
//extras added AB 3/9/97
//#define IDDES_BODYPOINTSLAB		5227		// The body points label
#define IDDES_BODYARMOURHLAB	5228		// The body armour (heat) bar graph label

#define IDDES_TEMPPOWERLAB		5229		// The template's Power req label
#define IDDES_TEMPBODYLAB		5230		// The template's Body Points label

#define IDDES_REPAIRPTLAB		5231		// The Repair Points label
#define IDDES_REPAIRWGTLAB		5232		// The Repair Weigth label

/* Design screen buttons */
#define IDDES_TEMPLSTART		5300		// The first design template button
#define IDDES_TEMPLEND			5339		// The last design template button
#define IDDES_BARSTART			5400
#define IDDES_BAREND			5499
#define IDDES_COMPSTART			5500000		// The first component button
#define IDDES_COMPEND			5565535		// The last component button
#define IDDES_EXTRASYSSTART		5700		// The first extra system button
#define IDDES_EXTRASYSEND		5899		// The last extra system button

#define	IDDES_SYSTEMBUTTON		5900		// System button
#define	IDDES_BODYBUTTON		5901		// Body button
#define	IDDES_PROPBUTTON		5902		// Propulsion button
#define IDDES_WPABUTTON			5903		// WeaponA button
#define IDDES_WPBBUTTON			5904		// WeaponB button

bool intAddDesign(bool bShowCentreScreen);
void intRemoveDesign();
void intProcessDesign(UDWORD id);
void intRunDesign();

const char *GetDefaultTemplateName(DROID_TEMPLATE *psTemplate);

bool intValidTemplate(DROID_TEMPLATE *psTempl, const char *newName, bool complain, int player);

#endif // __INCLUDED_SRC_DESIGN_H__
