/*
 * PowerCrypt.c
 *
 * Set up a seperate encrypted copy of each players power.
 *
 */

#include <string.h>

#include "lib/framework/frame.h"
#include "powercrypt.h"
#include "deliverance.h"
#include "lib/gamelib/gtime.h"
#include "objects.h"
#include "multiplay.h"
#include "lib/netplay/netplay.h"

// how long to pause between sending cheat messages
#define PWRC_MESSAGE_PAUSE		10000


// store copies of the power value encrypted to foil cheaters
typedef struct _pwrc_store
{
	UDWORD		key;	// xor key
	UWORD		pad1;	// get of a DWORD boundary
	UWORD		xorHigh;
	UWORD		xorLow;
	UBYTE		pad2;		// get even more off a DWORD boundary
	UBYTE		aShuffle[4];
	BOOL		valid;
	UDWORD		lastSent;
} PWRC_STORE;


PWRC_STORE		asPCrypt[MAX_PLAYERS];

BOOL			invalidPower[MAX_PLAYERS];

// set the current power value
void pwrcSetPlayerCryptPower(UDWORD player, UDWORD power)
{
	UBYTE	*pPower;

	ASSERT( player < MAX_PLAYERS,
		"pwrcSetPlayerCryptPower: invalid player number" );

	asPCrypt[player].pad1 = (UWORD)rand();
	asPCrypt[player].key = (UDWORD)rand();
	asPCrypt[player].xorHigh = (UWORD)((power >> 16) ^ (asPCrypt[player].key >> 16));
	asPCrypt[player].xorLow = (UWORD)(power ^ asPCrypt[player].key);
	asPCrypt[player].pad2 = (UBYTE)rand();

	pPower = (UBYTE *)&power;

	asPCrypt[player].aShuffle[0] = pPower[3];
	asPCrypt[player].aShuffle[1] = pPower[0];
	asPCrypt[player].aShuffle[2] = pPower[2];
	asPCrypt[player].aShuffle[3] = pPower[1];

	// the encrypted power is definately valid
	asPCrypt[player].valid = TRUE;
	asPCrypt[player].lastSent = 0;
}


// get the current power value
static UDWORD pwrcGetPlayerCryptPower(UDWORD player)
{
	ASSERT( player < MAX_PLAYERS,
		"pwrcGetPlayerCryptPower: invalid player number" );

	return 0;
}


// check the current power value
BOOL pwrcCheckPlayerCryptPower(UDWORD player, UDWORD power)
{
	UBYTE	aPower[4];
	BOOL	match;
	UWORD	res;

	ASSERT( player < MAX_PLAYERS,
		"pwrcCheckPlayerCryptPower: invalid player number" );

	if (!bMultiPlayer || !NetPlay.bComms || (player != selectedPlayer))
	{
		return TRUE;
	}

	match = TRUE;

	res = (UWORD)((power >> 16) ^ (asPCrypt[player].key >> 16));
	if (res != asPCrypt[player].xorHigh)
	{
		match = FALSE;
	}

	res = (UWORD)(power ^ asPCrypt[player].key);
	if (asPCrypt[player].xorLow != res)
	{
		match = FALSE;
	}

	memcpy(aPower, &power, 4);
	match = match && (asPCrypt[player].aShuffle[0] == aPower[3]);
	match = match && (asPCrypt[player].aShuffle[1] == aPower[0]);
	match = match && (asPCrypt[player].aShuffle[2] == aPower[2]);
	match = match && (asPCrypt[player].aShuffle[3] == aPower[1]);

	// if the power didn't match set the flag so that messages can be sent
	if (!match)
	{
		asPCrypt[player].valid = FALSE;
	}

	return match;
}


// check the valid flags and scream if the power isn't valid
void pwrcUpdate(void)
{
	SDWORD		i;
	char		aBuff[1024];

	if (!bMultiPlayer || !NetPlay.bComms)
	{
		return;
	}

	for(i=0; i<MAX_PLAYERS; i++)
	{
		if ((i == (SDWORD) selectedPlayer) &&
			!asPCrypt[i].valid && (asPCrypt[i].lastSent + PWRC_MESSAGE_PAUSE < gameTime))
		{
			sprintf(aBuff, "WARNING %s IS CHEATING (Power Changed)", getPlayerName(i));
			sendTextMessage(aBuff, TRUE);

			asPCrypt[i].lastSent = gameTime;
		}
	}
}




