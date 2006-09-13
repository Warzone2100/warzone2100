
#include <assert.h>
#include "lib/framework/frame.h"
#include "effects.h"
#include "structure.h"
#include "messagedef.h"
#include "miscimd.h"

/* Our great big array of imds */
static MISC_IMD	miscImds[] =
{						// Previous imd pointer names
	{NULL,"fxsexp"},	// explosionSmallImd	MI_EXPLOSION_SMALL
	{NULL,"fxlexp"},	// explosionMediumImd	MI_EXPLOSION_MEDIUM
	{NULL,"fxdust"},	// constructionImd		MI_CONSTRUCTION
	{NULL,"fxsmoke"},	// smallSmokeImd		MI_SMALL_SMOKE
	{NULL,"parthead"},	// babaHeadImd			MI_BABA_HEAD
	{NULL,"partlegs"},	// babaLegsImd			MI_BABA_LEGS
	{NULL,"partarm"},	// babaArmImd			MI_BABA_ARM
	{NULL,"partbody"},	// babaBodyImd			MI_BABA_BODY
	{NULL,"cybitrkt"},	// cyborgHeadImd		MI_CYBORG_HEAD
	{NULL,"cybitlg1"},	// cyborgLegsImd		MI_CYBORG_LEGS
	{NULL,"cybitgun"},	// cyborgArmImd			MI_CYBORG_ARM
	{NULL,"cybitbod"},	// cyborgBodyImd		MI_CYBORG_BODY
	{NULL,"fxatexp"},	// waterImd				MI_WATER
	{NULL,"fxssteam"},	// droidDamageImd		MI_DROID_DAMAGE
	{NULL,"fxssteam"},	// smallSteamImd		MI_SMALL_STEAM
	{NULL,"fxplasma"},	// plasmaImd			MI_PLASMA
	{NULL,"fxblip"},	// blipImd				MI_BLIP
	{NULL,"cyshadow"},	// shadowImd			MI_SHADOW
	{NULL,"Mitrnshd"},	// transporterShadowImd	MI_TRANSPORTER_SHADOW
	{NULL,"fxblood"},	// bloodImd				MI_BLOOD
	{NULL,"fxssmoke"},	// trailImd				MI_TRAIL
	{NULL,"fxft"},		// flameImd				MI_FLAME
	{NULL,"fxpower"},	// teslaImd				MI_TESLA
	{NULL,"fxmflare"},	// mFlareImd			MI_MFLARE
	{NULL,"mirain"},	// rainImd				MI_RAIN
	{NULL,"misnow"},	// snowImd				MI_SNOW
	{NULL,"fxssplsh"},	// splashImd			MI_SPLASH

	{NULL,"fxexpdrt"},	// kickImd				MI_KICK
	{NULL,"fxlightr"},	// landingImd			MI_LANDING
	{NULL,"fxl3dshk"},	// shockImd				MI_SHOCK

	{NULL,"blipenm"},	// proximityImds[0]	   	MI_BLIP_ENEMY
	{NULL,"blipres"},	// proximityImds[1]	   	MI_BLIP_RESOURCE
	{NULL,"blipart"},	// proximityImds[2]	   	MI_BLIP_ARTEFACT

	{NULL,"miwrek1"},	// wreckageImds[0]		MI_WRECK0
	{NULL,"miwrek2"},	// wreckageImds[1]		MI_WRECK1
	{NULL,"miwrek3"},	// wreckageImds[2]		MI_WRECK2
	{NULL,"miwrek4"},	// wreckageImds[3]		MI_WRECK3
	{NULL,"miwrek5"},	// wreckageImds[4]		MI_WRECK4

	{NULL,"midebr1"},	// debrisImds[0]		MI_DEBRIS0
	{NULL,"midebr2"},	// debrisImds[1]		MI_DEBRIS1
	{NULL,"midebr3"},	// debrisImds[2]		MI_DEBRIS2
	{NULL,"midebr4"},	// debrisImds[3]		MI_DEBRIS3
	{NULL,"midebr5"},	// debrisImds[4]		MI_DEBRIS4

	{NULL,"fxflecht"},	// met hit - for repair centre MI_FIREWORK



	{NULL,"END_OF_IMD_LIST"}
};

// -------------------------------------------------------------------------------
// Load up all the imds into an array
BOOL	multiLoadMiscImds( void )
{
UDWORD	i=0;
BOOL	bMoreToProcess=TRUE;
char	name[15];	// hopefully!

	/* Go thru' the list */
	while(bMoreToProcess)
	{
		sprintf(name,miscImds[i].pName);
		strcat(name,".pie");
		/* see if the resource loader can find it */
		miscImds[i].pImd = resGetData("IMD",name);
		/* If it didn't get it then... */
		if(!miscImds[i].pImd)
		{
			/* Say which one and return FALSE */
			debug( LOG_ERROR, "Can't find misselaneous PIE file : %s", miscImds[i].pName );
			ASSERT( FALSE,"NULL PIE" );
			return(FALSE);
		}
		/*	If the next one's the end one, then get out now.
			This is cos strcmp will return 0 only at end of list
		*/
		bMoreToProcess = strcmp(miscImds[++i].pName,"END_OF_IMD_LIST");
	}
	return TRUE;
}
// -------------------------------------------------------------------------------
// Returns a pointer to the imd from a #define number passed in - see above
iIMDShape	*getImdFromIndex(UDWORD	index)
{
	ASSERT( index<MI_TOO_MANY,"Out of range index in getImdFromIndex" );

	return(miscImds[index].pImd);
}
// -------------------------------------------------------------------------------

iIMDShape	*getRandomWreckageImd( void )
{
	iIMDShape *WreckageIMD;

	// Get a random wreckage
	WreckageIMD=(getImdFromIndex(MI_WRECK0 + rand()%((MI_WRECK4-MI_WRECK0)+1)));

	// if it doesn't exsist (cam2) then get the first one
	if (WreckageIMD==NULL)
	{
		WreckageIMD=(getImdFromIndex(MI_WRECK0));
	}
	// check to make sure one exists  (will fail on renderfeature if null)
	assert(WreckageIMD);

	return(WreckageIMD);
}

// -------------------------------------------------------------------------------
iIMDShape	*getRandomDebrisImd( void )
{
	iIMDShape *DebrisIMD;

	DebrisIMD = getImdFromIndex(MI_DEBRIS0 + rand()%((MI_DEBRIS4-MI_DEBRIS0)+1));

	ASSERT( DebrisIMD != NULL,"getRandomDebrisImd : NULL PIE" );

	return DebrisIMD;
}
// -------------------------------------------------------------------------------

//iIMDShape	*pAssemblyPointIMDs[NUM_FACTORY_TYPES][MAX_FACTORY];
iIMDShape	*pAssemblyPointIMDs[NUM_FLAG_TYPES][MAX_FACTORY];

BOOL	initMiscImds( void )
{
	STRING		facName[] = "MINUM0.pie";
	STRING		cybName[] = "MICNUM0.pie";
	STRING		vtolName[] = "MIVNUM0.pie";
	STRING		pieNum[2];
	UDWORD		i;

	/* Do the new loading system */
	multiLoadMiscImds();

	/* Now load the multi array stuff */
	for (i=0; i < MAX_FACTORY; i++)
	{
		sprintf(pieNum, "%d", i+1);
		facName[5] = *pieNum;
		pAssemblyPointIMDs[FACTORY_FLAG][i] = resGetData("IMD", facName);
		if (!pAssemblyPointIMDs[FACTORY_FLAG][i])
		{
			debug( LOG_ERROR, "Can't find assembly point graphic for factory" );
			abort();
			return(FALSE);
		}
//#ifndef PSX
		cybName[6] = *pieNum;
		pAssemblyPointIMDs[CYBORG_FLAG][i] = resGetData("IMD", cybName);
		if (!pAssemblyPointIMDs[CYBORG_FLAG][i])
		{
			debug( LOG_ERROR, "Can't find assembly point graphic for cyborg factory" );
			abort();
			return(FALSE);
		}
		vtolName[6] = *pieNum;
		pAssemblyPointIMDs[VTOL_FLAG][i] = resGetData("IMD", vtolName);
		if (!pAssemblyPointIMDs[VTOL_FLAG][i])
		{
			debug( LOG_ERROR, "Can't find assembly point graphic for vtol factory" );
			abort();
			return(FALSE);
		}
		pAssemblyPointIMDs[REPAIR_FLAG][i] = resGetData("IMD", "mirnum1.pie");
		if (!pAssemblyPointIMDs[REPAIR_FLAG][i])
		{
			debug( LOG_ERROR, "Can't find assembly point graphic for repair facility" );
			abort();
			return(FALSE);
		}
//#else
//		pAssemblyPointIMDs[CYBORG_FLAG][i] = pAssemblyPointIMDs[FACTORY_FLAG][i];
//		pAssemblyPointIMDs[VTOL_FLAG][i] = pAssemblyPointIMDs[FACTORY_FLAG][i];
//#endif
	}

	return(TRUE);
}
