/*mu;lt
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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
 * @file tweak.h
 *
 * This file contain definition for tweak fonction.
 * Basicaly you define stuff here and in tweak.c
 * Then you get an easy access with tweakData[PLAYER_NO][ tweak_name ] (providing you included tweak.h in the file)
 * The fun thing is that the players then can change their value in data/base/stats/tweak.txt and therefore, It can be "modded" easily
 *
 */
#define tweak_num 20
double tweakData [MAX_PLAYERS][tweak_num]; //actual array containing the values


// /!\ These lines must be absolutly synched  with tweak.c line 7
// under it's a list of the tweaks keep their numbers the same as in the array. It just there to keep the code human readable. But I suggest we also keep close track of where is the tweak and what it's suppose to do here...

 typedef enum tweakNo { 
	tweak_dummy,		//0 will contain the last unidentified param
	//power.c line 295 also power.h for asPower.lastCalc and asPower.addPower
	tweak_power_fact, 	//1 multiply power
	tweak_power_exp, 	//2 make power exponential !
	tweak_power_bonus, 	//3 power given with no well at all
	tweak_power_loss, 	//4 % of your power lost every second (hint: negative act just like interest)
	tweak_power_start, 	//5 (not implemented yet) set how much power you start with
	//research.c line 610
	tweak_res_fact, 	//6 multiply research time and cost
	tweak_res_exp,		//7 make research exponentialy hard
	tweak_res_power, 	//8 multiply power cost but not time
	//droid.c
	tweak_droidbuild_fact, 	//9 droid build time factor
	tweak_droidpower_fact, 	//10 droid cost factor
	tweak_droidbody_fact, 	//11 droid body (HP) factor
	//seen in structure.c line 583
	tweak_structbuild_fact, //12 build time
	tweak_structarmour_fact,//13 armour
	tweak_structcost_fact, 	//14 power cost
	tweak_structbody_fact, 	//15 HP
	//combat.c 602 and 621
	tweak_armour_fact, 	//16 Multiply the armour (0=off)
	tweak_damage_fact, 	//17 Multiply the damage (after armour)
	//move.c 1949 and 2095
	tweak_speed_fact, 	//18 Multiply speed
	tweak_accel_fact 	//19 Multiply accel (buggy pathfinding > 200)
	
 } tweakNo;



