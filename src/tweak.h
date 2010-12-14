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

/*
char tweakNames[tweak_num][20] = { "dummy" , "power_fact", "power_exp", "power_bonus", "power_loss", "power_start",
	"res_fact", "res_exp", "res_power", 
	"droidbuild_fact", "droidpower_fact", "droidbody_fact",
	"structbuild_fact", "structarmour_fact", "structcost_fact", "structbody_fact",
	"armour_fact", "damage_fact", "speed_fact", "accel_fact" };
 */

// /!\ The line above is a copy of ilutweak.c line 7 keep those 2 in synch
// under it's a list of the same tweaks keep their numbers the same as in the array. It just there to keep the code human readable. But I suggest we also keep close track of where is the tweak and what it's suppose to do here...

#define tweak_dummy 0 //will contain the last unidentified param

//seen in power.c line 295 also power.h for asPower.lastCalc and asPower.addPower
#define tweak_power_fact 	1	//multiply power
#define tweak_power_exp 	2	//make power exponential !
#define tweak_power_bonus	3	//power given with no well at all
#define tweak_power_loss	4 	//% of your power lost every second (hint: negative act just like interest)
#define tweak_power_start	5 	//(not implemented yet) set how much power you start with

//seen in research.c line 610
#define tweak_res_fact		6 //multiply research time and cost
#define tweak_res_exp		7 //make research expodentialy hard
#define tweak_res_power 	8 //multiply power cost but not time

//droid.c
#define tweak_droidbuild_fact 9 //droid build time factor
#define tweak_droidpower_fact 10 //droid cost factor
#define tweak_droidbody_fact 11 //droid cost factor

//seen in structure.c line 583
#define tweak_structbuild_fact 12 //build time
#define tweak_structarmour_fact 13 //armour
#define tweak_structcost_fact 14 //power cost
#define tweak_structbody_fact 15 // HP

//combat.c 602 and 621
#define tweak_armour_fact 16  //Multiply the armour (0=off)
#define tweak_damage_fact 17  //Multiply the damage (after armour)
//move.c 1949 and 2095
#define tweak_speed_fact 18 //Multiply speed
#define tweak_accel_fact 19 //Multiply accel (should follow speed fact most of time but who knows ? ) Should I multiply by speed_fact to ? 
//NB:buggy... I think path finding don't take correctly into account acceleration (or I didn't found where). It result in truck running into wall. The faster the accel, the funnier it look...

//seen in structure.c line 3308
#define tweak_res_loss 20//(not implemented yet)
//seen in structure.c line 3401
#define tweak_prod_loss 21 //(not implemented yet)


/* ----------------------- delete this as soon as it is replaced ! :D ------------- */

//combat.c 602 and 621
#define game_armour_fact 1  //Multiply the armour
#define game_damage_fact 2  //Multiply the damage (after armour)
//move.c 1949 and 2095
#define game_speed_fact 2 //Multiply speed
#define game_accel_fact 2 //Multiply accel (should follow speed fact most of time but who knows ? ) Should I multiply by speed_fact to ? 
//NB:buggy... I think path finding don't take correctly into account accelaration (or I didn't found where). It result in truck running into wall. The faster the accel, the funnier it look...


