/*
 * Data.c
 *
 * Data loading functions used by the framework resource module
 *
 */

#include <assert.h>


#include "lib/framework/frame.h"
//render library
#include "lib/ivis_common/piedef.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/bitimage.h"

#include "texture.h"
#include "warzoneconfig.h"
#include "lib/ivis_common/tex.h"
#include "lib/ivis_common/textdraw.h"

#include "lib/framework/frameresource.h"
#include "stats.h"
#include "structure.h"
#include "feature.h"
#include "research.h"
#include "data.h"
#include "text.h"
#include "droid.h"
#include "function.h"
#include "message.h"
#include "lib/script/script.h"
#include "scriptvals.h"
#include "display3d.h"
#include "game.h"
#include "objects.h"
#include "display.h"
#include "lib/sound/audio.h"
#include "lib/gamelib/anim.h"
#include "lib/gamelib/parser.h"
#include "levels.h"
#include "mechanics.h"
#include "display3d.h"
#include "display3ddef.h"
#include "init.h"

#include "multiplay.h"
#include "lib/netplay/netplay.h"

/**********************************************************
 *
 * Local Variables
 *
 *********************************************************/

BOOL	bTilesPCXLoaded = FALSE;

// whether a save game is currently being loaded
BOOL	saveFlag = FALSE;
extern STRING	aCurrResDir[255];		// Arse

UDWORD	cheatHash[CHEAT_MAXCHEAT];

void dataISpriteRelease(void *pData);

/**********************************************************
 *
 * Source
 *
 *********************************************************/
void calcCheatHash(UBYTE *pBuffer, UDWORD size, UDWORD cheat)
{
	if(!bMultiPlayer)
	{
		return;
	}

	// create the hash for that data block.
	cheatHash[cheat] =cheatHash[cheat] ^ NEThashBuffer(pBuffer,size);
	return;
}

void resetCheatHash()
{
	UDWORD i;
	for(i=0;i<CHEAT_MAXCHEAT;i++)
	{
		cheatHash[i] =0;
	}
}

/**********************************************************/


void dataSetSaveFlag(void)
{
	saveFlag = TRUE;
}
void dataClearSaveFlag(void)
{
	saveFlag = FALSE;
}


/* Load the body stats */
BOOL bufferSBODYLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	calcCheatHash(pBuffer,size,CHEAT_SBODY);
	if (!loadBodyStats((SBYTE*)pBuffer, size))
	{
		return FALSE;
	}

	if (!allocComponentList(COMP_BODY, numBodyStats))
	{
		return FALSE;
	}

	// set a dummy value so the release function gets called
	*ppData = (void *)1;
	return TRUE;
}

void dataReleaseStats(void *pData)
{
	freeComponentLists();
	statsShutDown();
}


/* Load the weapon stats */
BOOL bufferSWEAPONLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	calcCheatHash(pBuffer,size, CHEAT_SWEAPON);
	if (!loadWeaponStats((SBYTE*)pBuffer, size))
	{
		return FALSE;
	}

	if (!allocComponentList(COMP_WEAPON, numWeaponStats))
	{
		return FALSE;
	}

	//not interested in this value
	*ppData = NULL;
	return TRUE;
}

/* Load the constructor stats */
BOOL bufferSCONSTRLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	calcCheatHash(pBuffer,size,CHEAT_SCONSTR);
	if (!loadConstructStats((SBYTE*)pBuffer, size))
	{
		return FALSE;
	}

	if (!allocComponentList(COMP_CONSTRUCT, numConstructStats))
	{
		return FALSE;
	}

	//not interested in this value
	*ppData = NULL;
	return TRUE;
}

/* Load the ECM stats */
BOOL bufferSECMLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	calcCheatHash(pBuffer,size,CHEAT_SECM);

	if (!loadECMStats((SBYTE*)pBuffer, size))
	{
		return FALSE;
	}

	if (!allocComponentList(COMP_ECM, numECMStats))
	{
		return FALSE;
	}

	//not interested in this value
	*ppData = NULL;
	return TRUE;
}

/* Load the Propulsion stats */
BOOL bufferSPROPLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	calcCheatHash(pBuffer,size,CHEAT_SPROP);

	if (!loadPropulsionStats((SBYTE*)pBuffer, size))
	{
		return FALSE;
	}

	if (!allocComponentList(COMP_PROPULSION, numPropulsionStats))
	{
		return FALSE;
	}

	//not interested in this value
	*ppData = NULL;
	return TRUE;
}

/* Load the Sensor stats */
BOOL bufferSSENSORLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	calcCheatHash(pBuffer,size,CHEAT_SSENSOR);

	if (!loadSensorStats((SBYTE*)pBuffer, size))
	{
		return FALSE;
	}

	if (!allocComponentList(COMP_SENSOR, numSensorStats))
	{
		return FALSE;
	}

	//not interested in this value
	*ppData = NULL;
	return TRUE;
}

/* Load the Repair stats */
BOOL bufferSREPAIRLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	calcCheatHash(pBuffer,size,CHEAT_SREPAIR);

	if (!loadRepairStats((SBYTE*)pBuffer, size))
	{
		return FALSE;
	}

	if (!allocComponentList(COMP_REPAIRUNIT, numRepairStats))
	{
		return FALSE;
	}

	//not interested in this value
	*ppData = NULL;
	return TRUE;
}

/* Load the Brain stats */
BOOL bufferSBRAINLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	calcCheatHash(pBuffer,size,CHEAT_SBRAIN);

	if (!loadBrainStats((SBYTE*)pBuffer, size))
	{
		return FALSE;
	}

	if (!allocComponentList(COMP_BRAIN, numBrainStats))
	{
		return FALSE;
	}

	//not interested in this value
	*ppData = NULL;
	return TRUE;
}

/* Load the Program stats */
/*BOOL bufferSPROGRAMLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	if (!loadProgramStats((SBYTE*)pBuffer, size))
	{
		return FALSE;
	}

	if (!allocComponentList(COMP_PROGRAM, numProgramStats))
	{
		return FALSE;
	}

	//not interested in this value
	*ppData = NULL;
	return TRUE;
}*/

/* Load the PropulsionType stats */
BOOL bufferSPROPTYPESLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	calcCheatHash(pBuffer,size,CHEAT_SPROPTY);

	if (!loadPropulsionTypes((SBYTE*)pBuffer, size))
	{
		return FALSE;
	}


	//not interested in this value
	*ppData = NULL;
	return TRUE;
}

/* Load the propulsion type sound stats */
BOOL bufferSPROPSNDLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	if (!loadPropulsionSounds((SBYTE*)pBuffer, size))
	{
		return FALSE;
	}

	//not interested in this value
	*ppData = NULL;
	return TRUE;
}

/* Load the SSPECABIL stats */
BOOL bufferSSPECABILLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	if (!loadSpecialAbility((SBYTE*)pBuffer, size))
	{
		return FALSE;
	}


	//not interested in this value
	*ppData = NULL;
	return TRUE;
}

/* Load the STERRTABLE stats */
BOOL bufferSTERRTABLELoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	calcCheatHash(pBuffer,size,CHEAT_STERRT);

	if (!loadTerrainTable((SBYTE*)pBuffer, size))
	{
		return FALSE;
	}


	//not interested in this value
	*ppData = NULL;
	return TRUE;
}

/* Load the body/propulsion IMDs stats */
BOOL bufferSBPIMDLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	if (!loadBodyPropulsionIMDs((SBYTE*)pBuffer, size))
	{
		return FALSE;
	}


	//not interested in this value
	*ppData = NULL;
	return TRUE;
}

/* Load the weapon sound stats */
BOOL bufferSWEAPSNDLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	if (!loadWeaponSounds((SBYTE*)pBuffer, size))
	{
		return FALSE;
	}


	//not interested in this value
	*ppData = NULL;
	return TRUE;
}

/* Load the Weapon Effect modifier stats */
BOOL bufferSWEAPMODLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	calcCheatHash(pBuffer,size,CHEAT_SWEAPMOD);

	if (!loadWeaponModifiers((SBYTE*)pBuffer, size))
	{
		return FALSE;
	}


	//not interested in this value
	*ppData = NULL;
	return TRUE;
}


/* Load the Template stats */
BOOL bufferSTEMPLLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	calcCheatHash(pBuffer,size,CHEAT_STEMP);

	if (!loadDroidTemplates((SBYTE*)pBuffer, size))
	{
		return FALSE;
	}


	// set a dummy value so the release function gets called
	*ppData = (void *)1;
	return TRUE;
}

// release the templates
void dataSTEMPLRelease(void *pData)
{
	//free the storage allocated to the droid templates
	droidTemplateShutDown();
}

/* Load the Template weapons stats */
BOOL bufferSTEMPWEAPLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	calcCheatHash(pBuffer,size,CHEAT_STEMPWEAP);

	if (!loadDroidWeapons((SBYTE*)pBuffer, size))
	{
		return FALSE;
	}


	//not interested in this value
	*ppData = NULL;
	return TRUE;
}

/* Load the Template programs stats */
/*BOOL bufferSTEMPPROGLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	if (!loadDroidPrograms((SBYTE*)pBuffer, size))
	{
		return FALSE;
	}


	//not interested in this value
	*ppData = NULL;
	return TRUE;
}*/

/* Load the Structure stats */
BOOL bufferSSTRUCTLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	calcCheatHash(pBuffer,size,CHEAT_SSTRUCT);

	if (!loadStructureStats((SBYTE*)pBuffer, size))
	{
		return FALSE;
	}

	if (!allocStructLists())
	{
		return FALSE;
	}

	// set a dummy value so the release function gets called
	*ppData = (void *)1;
	return TRUE;
}

// release the structure stats
void dataSSTRUCTRelease(void *pData)
{
	freeStructureLists();
	structureStatsShutDown();
}

/* Load the Structure Weapons stats */
BOOL bufferSSTRWEAPLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	calcCheatHash(pBuffer,size,CHEAT_SSTRWEAP);

	if (!loadStructureWeapons((SBYTE*)pBuffer, size))
	{
		return FALSE;
	}


	//not interested in this value
	*ppData = NULL;
	return TRUE;
}

/* Load the Structure Functions stats */
BOOL bufferSSTRFUNCLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	calcCheatHash(pBuffer,size,CHEAT_STRFUNC);

	if (!loadStructureFunctions((SBYTE*)pBuffer, size))
	{
		return FALSE;
	}


	//not interested in this value
	*ppData = NULL;
	return TRUE;
}

/* Load the Structure strength modifier stats */
BOOL bufferSSTRMODLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	calcCheatHash(pBuffer,size,CHEAT_SSTRMOD);

	if (!loadStructureStrengthModifiers((SBYTE*)pBuffer, size))
	{
		return FALSE;
	}

	//not interested in this value
	*ppData = NULL;
	return TRUE;
}

/* Load the Feature stats */
BOOL bufferSFEATLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{

	calcCheatHash(pBuffer,size,CHEAT_SFEAT);

	if (!loadFeatureStats((SBYTE*)pBuffer, size))
	{
		return FALSE;
	}


	// set a dummy value so the release function gets called
	*ppData = (void *)1;
	return TRUE;
}

// free the feature stats
void dataSFEATRelease(void *pData)
{
	featureStatsShutDown();
}

/* Load the Functions stats */
BOOL bufferSFUNCLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	calcCheatHash(pBuffer,size,CHEAT_SFUNC);

	if (!loadFunctionStats((SBYTE*)pBuffer, size))
	{
		return FALSE;
	}

    //adjust max values of stats used in the design screen due to any possible upgrades
    adjustMaxDesignStats();

	// set a dummy value so the release function gets called
	*ppData = (void *)1;
	return TRUE;
}

// release the function stats
void dataSFUNCRelease(void *pData)
{
	FunctionShutDown();
}

// release the research stats
void dataRESCHRelease(void *pData)
{
	//free the storage allocated to the stats
	ResearchShutDown();
}

/* Load the Research stats */
BOOL bufferRESCHLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	calcCheatHash(pBuffer,size,CHEAT_RESCH);

    //check to see if already loaded
    if (numResearch > 0)
    {
        //release previous data before loading in the new
        dataRESCHRelease(NULL);
    }

	if (!loadResearch((SBYTE*)pBuffer, size))
	{
		return FALSE;
	}


	/* set a dummy value so the release function gets called - the Release
    function is now called when load up the next set
	// *ppData = (void *)1;
    pass back NULL so that can load the same name file for the next campaign*/
	*ppData = NULL;
	return TRUE;
}

/* Load the research pre-requisites */
BOOL bufferRPREREQLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	calcCheatHash(pBuffer,size,CHEAT_RPREREQ);

	if (!loadResearchPR((SBYTE*)pBuffer, size))
	{
		return FALSE;
	}

	//not interested in this value
	*ppData = NULL;
	return TRUE;
}

/* Load the research components required */
/*BOOL bufferRCOMPREQLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	//DON'T DO ANYTHING WITH IT SINCE SHOULDN'T BE LOADING ANYMORE - AB 20/04/98
	if (!loadResearchArtefacts((SBYTE*)pBuffer, size, REQ_LIST))
	{
		return FALSE;
	}


	//not interested in this value
	*ppData = NULL;
	return TRUE;
}*/

/* Load the research components made redundant */
BOOL bufferRCOMPREDLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	calcCheatHash(pBuffer,size,CHEAT_RCOMPRED);

	if (!loadResearchArtefacts((SBYTE*)pBuffer, size, RED_LIST))
	{
		return FALSE;
	}


	//not interested in this value
	*ppData = NULL;
	return TRUE;
}

/* Load the research component results */
BOOL bufferRCOMPRESLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{

	calcCheatHash(pBuffer,size,CHEAT_RCOMPRES);

	if (!loadResearchArtefacts((SBYTE*)pBuffer, size, RES_LIST))
	{
		return FALSE;
	}


	//not interested in this value
	*ppData = NULL;
	return TRUE;
}

/* Load the research structures required */
BOOL bufferRSTRREQLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{

	calcCheatHash(pBuffer,size,CHEAT_RSTRREQ);

	if (!loadResearchStructures((SBYTE*)pBuffer, size, REQ_LIST))
	{
		return FALSE;
	}


	//not interested in this value
	*ppData = NULL;
	return TRUE;
}

/* Load the research structures made redundant */
BOOL bufferRSTRREDLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{

	calcCheatHash(pBuffer,size,CHEAT_RSTRRED);

	if (!loadResearchStructures((SBYTE*)pBuffer, size, RED_LIST))
	{
		return FALSE;
	}

	//not interested in this value
	*ppData = NULL;
	return TRUE;
}

/* Load the research structure results */
BOOL bufferRSTRRESLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{

	calcCheatHash(pBuffer,size,CHEAT_RSTRRES);

	if (!loadResearchStructures((SBYTE*)pBuffer, size, RES_LIST))
	{
		return FALSE;
	}


	//not interested in this value
	*ppData = NULL;
	return TRUE;
}

/* Load the research functions */
BOOL bufferRFUNCLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{

	calcCheatHash(pBuffer,size,CHEAT_RFUNC);

	if (!loadResearchFunctions((SBYTE*)pBuffer, size))
	{
		return FALSE;
	}


	//not interested in this value
	*ppData = NULL;
	return TRUE;
}

/* Load the message viewdata */
BOOL bufferSMSGLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	VIEWDATA	*pViewData;

	pViewData = loadViewData((SBYTE*)pBuffer, size);
	if (!pViewData)
	{
		return FALSE;
	}


	// set the pointer so the release function gets called with it
	*ppData = (void *)pViewData;
	return TRUE;
}


// release the message viewdata
void dataSMSGRelease(void *pData)
{
	viewDataShutDown((VIEWDATA *)pData);
}

/* Load an imd */
BOOL dataIMDBufferLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	iIMDShape	*psIMD;
	char *pBufferPosition;

	// FIXME: evil cast
	pBufferPosition = (char *)pBuffer;

	psIMD = iV_ProcessIMD(&pBufferPosition, pBufferPosition + size, "", "", FALSE);
	if (psIMD == NULL) {
		DBERROR(("IMD load failed - %s", GetLastResourceFilename()));
		return FALSE;
	}

	*ppData = psIMD;
	return TRUE;
}


/* Release an imd */
/*void dataIMDRelease(void *pData)
{
	iV_IMDRelease(pData);
}*/


BOOL dataIMGPAGELoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
//	iPalette sPal;
	iSprite *psSprite = (iSprite*) MALLOC(sizeof(iSprite));
	if (!psSprite)	{
		return FALSE;
	}

	if(!pie_PNGLoadMem((SBYTE *)pBuffer,psSprite,NULL))
	{
		debug( LOG_ERROR, "IMGPAGE load failed" );
		return FALSE;
	}

	*ppData = psSprite;

	return TRUE;
}


void dataIMGPAGERelease(void *pData)
{
	iSprite *psSprite = (iSprite*) pData;
	dataISpriteRelease(psSprite);
}

// Tertiles loader. This version for software renderer.
BOOL dataTERTILESLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	if(pie_Hardware()) {
		*ppData = NULL;
		return TRUE;
	}

	// tile loader.
	if (bTilesPCXLoaded)
	{
		debug( LOG_TEXTURE, "Reloading terrain tiles\n" );
		if(!pie_PNGLoadMem((SBYTE *)pBuffer,&tilesPCX,NULL))
		{
			debug( LOG_ERROR, "TERTILES reload failed" );
			return FALSE;
		}
	}
	else
	{
		debug( LOG_TEXTURE, "Loading terrain tiles\n" );
		if(!pie_PNGLoadMem((SBYTE *)pBuffer,&tilesPCX,NULL))
		{
			debug( LOG_ERROR, "TERTILES load failed" );
			return FALSE;
		}
	}

		/* Squirt the tiles into a nice long thin bitmap */
		if (bTilesPCXLoaded)
		{
			if(!remakeTileTextures())
			{
	 			debug( LOG_ERROR, "Problem converting the terrain graphics file" );
				return(FALSE);
			}
		}
		else
		{
			if(!makeTileTextures())
			{
	 			debug( LOG_ERROR, "Problem converting the terrain graphics file" );
				return(FALSE);
			}
		}

	if (bTilesPCXLoaded)
	{
		*ppData = NULL;
	}
	else
	{
		bTilesPCXLoaded = TRUE;
		*ppData = &tilesPCX;
	}
	debug( LOG_TEXTURE, "Tiles loaded\n" );
	return TRUE;
}

void dataTERTILESRelease(void *pData)
{
	iSprite *psSprite = (iSprite*) pData;

	freeTileTextures();
	if( psSprite->bmp )
	{
		free(psSprite->bmp);
		psSprite->bmp = NULL;
	}
	bTilesPCXLoaded = FALSE;
}

// Tertiles loader. This version for hardware renderer.
BOOL dataHWTERTILESLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	if(!pie_Hardware()) {
		*ppData = NULL;
		return TRUE;
	}

	// tile loader.
	if (bTilesPCXLoaded)
	{
		debug( LOG_TEXTURE, "Reloading terrain tiles\n" );
		if(!pie_PNGLoadMem((SBYTE *)pBuffer,&tilesPCX,NULL))
		{
			debug( LOG_ERROR, "HWTERTILES reload failed" );
			return FALSE;
		}
	}
	else
	{
		debug( LOG_TEXTURE, "Loading terrain tiles\n" );
		if(!pie_PNGLoadMem((SBYTE *)pBuffer,&tilesPCX,NULL))
		{
			debug( LOG_ERROR, "HWTERTILES load failed" );
			return FALSE;
		}
	}

	if (pie_Hardware())
	{
		getTileRadarColours();
		// make several 256 * 256 pages
		if (bTilesPCXLoaded)
		{
			remakeTileTexturePages(tilesPCX.width,tilesPCX.height,TILE_WIDTH, TILE_HEIGHT, tilesPCX.bmp);
		}
		else
		{
			makeTileTexturePages(tilesPCX.width,tilesPCX.height,TILE_WIDTH, TILE_HEIGHT, tilesPCX.bmp);
		}
	}

	if (bTilesPCXLoaded)
	{
		*ppData = NULL;
	}
	else
	{
		bTilesPCXLoaded = TRUE;
		*ppData = &tilesPCX;
	}
	debug( LOG_TEXTURE, "HW Tiles loaded\n" );
	return TRUE;
}

void dataHWTERTILESRelease(void *pData)
{
	iSprite *psSprite = (iSprite*) pData;

	freeTileTextures();
	if( psSprite->bmp )
	{
		free(psSprite->bmp);
		psSprite->bmp = NULL;
	}
	// We are not allowed to free psSprite also, this would give an error on Windows: HEAP[Warzone.exe]: Invalid Address specified to RtlFreeHeap( xxx, xxx )
	bTilesPCXLoaded = FALSE;
	pie_TexShutDown();
}


BOOL dataIMGLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	IMAGEFILE *ImageFile;

	ImageFile = iV_LoadImageFile(pBuffer,size);
	if(ImageFile == NULL) {
		return FALSE;
	}

	*ppData = ImageFile;

	return TRUE;
}


void dataIMGRelease(void *pData)
{
	iV_FreeImageFile((IMAGEFILE*)pData);
}


#define TEXTUREWIDTH (256)
#define TEXTUREHEIGHT (256)

/* Load a texturepage into memory */
BOOL bufferTexPageLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	TEXTUREPAGE *NewTexturePage;
	iPalette	*psPal;
	iSprite		*psSprite;
	STRING		texfile[255];
	SDWORD		i, id;
//	BOOL		bFound = FALSE;

	// generate a texture page name in "page-xx" format
	strncpy(texfile, GetLastResourceFilename(), 254);
	texfile[254]=0;
	resToLower(texfile);

	debug(LOG_TEXTURE, "bufferTexPageLoad: %s texturepage ...", texfile);

	if (war_GetAdditive())//(war_GetTranslucent())
	{
		//hardware
		if (strstr(texfile,"soft") != NULL)//and this is a software textpage
		{
			//so dont load it
			*ppData = NULL;
			return TRUE;
		}
	}
	else
	{
		//software or old d3d card
		if (strstr(texfile,"hard") != NULL)//and this is a hardware textpage
		{
			//so dont load it
			*ppData = NULL;
			return TRUE;
		}
	}


	if (strncmp(texfile, "page-", 5) == 0)
	{
		for(i=5; i<(SDWORD)strlen(texfile); i++)
		{
			if (!isdigit(texfile[i]))
			{
				break;
			}
		}
		texfile[i] = 0;
	}
	SetLastResourceFilename(texfile);

	debug(LOG_TEXTURE, "bufferTexPageLoad: %s texturepage added (length=%d)",
	      texfile, strlen(texfile));

	// see if this texture page has already been loaded
	if (resPresent("TEXPAGE", texfile))
	{
		// replace the old texture page with the new one
		debug(LOG_TEXTURE, "bufferTexPageLoad: replacing old");
		id = pie_ReloadTexPage(texfile,(UBYTE *)pBuffer);
		ASSERT((id >=0,"pie_ReloadTexPage failed"));
		*ppData = NULL;
	}
	else
	{
		NewTexturePage = MALLOC(sizeof(TEXTUREPAGE));
		if (!NewTexturePage) return FALSE;

		NewTexturePage->Texture=NULL;
		NewTexturePage->Palette=NULL;

		psPal = MALLOC(sizeof(iPalette));
		if (!psPal) return FALSE;

		psSprite = (iSprite*)MALLOC(sizeof(iSprite));
		if (!psSprite)
		{
			return FALSE;
		}

		if (!pie_PNGLoadMem((SBYTE *)pBuffer, psSprite, NULL))
		{
			return FALSE;
		}


		NewTexturePage->Texture=psSprite;
		NewTexturePage->Palette=psPal;

//Hack mar8 to load	textures in order
/*	for(i=0;i<_TEX_INDEX;i++)
	{
		if (stricmp(texfile,_TEX_PAGE[i].name) != 0)
		{
			bFound = TRUE;
			break;
		}
	}
	if (!bFound)
*/
	{
		pie_AddBMPtoTexPages(psSprite, texfile, 1, FALSE, TRUE);
	}
//Hack end

		*ppData = NewTexturePage;
	}

	return TRUE;

}

BOOL bufferTexPageLoadSoftOnly(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	if (!war_GetTranslucent())
	{
		return bufferTexPageLoad(pBuffer, size, ppData);
	}
	*ppData = NULL;
	return TRUE;
}

BOOL bufferTexPageLoadHardOnly(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	if (war_GetTranslucent())
	{
		return bufferTexPageLoad(pBuffer, size, ppData);
	}
	*ppData = NULL;
	return TRUE;
}


/* Release an iSprite */
void dataISpriteRelease(void *pData)
{
	iSprite *psSprite = (iSprite*) pData;

	if( psSprite )
	{
		if( psSprite->bmp )
		{
			free(psSprite->bmp);
			psSprite->bmp = NULL;
		}
		FREE(psSprite);
		psSprite = NULL;
	}
}


/* Release a texPage */
void dataTexPageRelease(void *pData)
{
	TEXTUREPAGE *Tpage = (TEXTUREPAGE *) pData;

	// We need to handle null texpage data
	if (Tpage == NULL) return;

	if (Tpage->Texture != NULL)
	{
		dataISpriteRelease(Tpage->Texture);
	}
	if (Tpage->Palette != NULL)
		FREE(Tpage->Palette);

	FREE(Tpage);
}


/* Load an audio file */
BOOL dataAudioLoad( UBYTE *pBuffer, UDWORD size, void **ppData )
{
	TRACK	*psTrack;

	if ( audio_Disabled() == TRUE )
	{
		*ppData = NULL;
		return TRUE;
	}
	else if ( (psTrack = audio_LoadTrackFromBuffer( pBuffer, size )) == NULL )
	{
		return FALSE;
	}

	/* save track data */
	*ppData = psTrack;

	return TRUE;
}

void dataAudioRelease( void *pData )
{
	if (audio_Disabled() == FALSE)
	{
		TRACK	*psTrack = (TRACK *) pData;

		ASSERT( (PTRVALID(psTrack, sizeof(TRACK)),
				"dataAudioRelease: invalid track pointer") );

		audio_ReleaseTrack( psTrack );
		FREE( psTrack );
	}
}


/* Load an audio file */
BOOL dataAudioCfgLoad( UBYTE *pBuffer, UDWORD size, void **ppData )
{
	*ppData = NULL;

	if ( audio_Disabled() == FALSE &&
		 ParseResourceFile( pBuffer, size ) == FALSE )
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}


/* Load an anim file */
BOOL dataAnimLoad( UBYTE *pBuffer, UDWORD size, void **ppData )
{
	BASEANIM	*psAnim;

	if ( (psAnim = anim_LoadFromBuffer( pBuffer, size )) == NULL )
	{
		return FALSE;
	}

	/* copy anim for return */
	*ppData = psAnim;



	return TRUE;
}


/* Load an audio config file */
BOOL dataAnimCfgLoad( UBYTE *pBuffer, UDWORD size, void **ppData )
{
	*ppData = NULL;
//	printf("\n\n\n\n\n\n\n=======dataAnimCfgLoad=========\n\n %s \n\n\n\n\n\n",pBuffer);
	if ( ParseResourceFile( pBuffer, size ) == FALSE )
	{
		return FALSE;
	}



	return TRUE;
}


void dataAnimRelease( void *pData )
{
	anim_ReleaseAnim(pData);
}

/* Load a string resource file */
BOOL dataStrResLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	// recreate the string resource if it was freed by a WRF release
	if (psStringRes == NULL)
	{
		if (!stringsInitialise())
		{
			return FALSE;
		}
	}

	if (!strresLoad(psStringRes, pBuffer, size))
	{
		return FALSE;
	}

	*ppData = psStringRes;
	return TRUE;
}

void dataStrResRelease(void *pData)
{
	if (psStringRes != NULL)
	{
		strresDestroy(psStringRes);
		psStringRes = NULL;
	}
}


/* Load a script file */
// All scripts, binary or otherwise are now passed through this routine
BOOL dataScriptLoad(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	SCRIPT_CODE		*psProg=NULL;
	BLOCK_HEAP		*psHeap;
	BOOL			printHack = FALSE;

	calcCheatHash(pBuffer,size,CHEAT_SCRIPT);

#ifndef NOSCRIPT
	DBPRINTF(("COMPILING SCRIPT ...%s\n",GetLastResourceFilename()));
	// make sure the memory system uses normal malloc for a compile
	psHeap = memGetBlockHeap();
	memSetBlockHeap(NULL);

	if (!scriptCompile(pBuffer, size, &psProg, SCRIPTTYPE))		// see script.h
	{
		DBERROR(("Script %s did not compile", GetLastResourceFilename()));
		return FALSE;
	}
	memSetBlockHeap(psHeap);

	if (printHack)
	{
		cpPrintProgram(psProg);
	}

	*ppData = psProg;
#endif
	return TRUE;
}

// Load a script variable values file
BOOL dataScriptLoadVals(UBYTE *pBuffer, UDWORD size, void **ppData)
{
	*ppData = NULL;

	calcCheatHash(pBuffer,size,CHEAT_SCRIPTVAL);

	// don't load anything if a saved game is being loaded
	if (saveFlag)
	{
		return TRUE;
	}

	DBPRINTF(("Loading script data %s\n",GetLastResourceFilename()));

	if (!scrvLoad(pBuffer, size))
	{
		return FALSE;
	}

	*ppData = NULL;
	return TRUE;
}

BOOL dataSaveGameLoad(STRING *pFile, void **ppData)
{
	if (!stageTwoInitialise())
	{
		return FALSE;
	}

	if (!loadGameInit(pFile))
	{
		return FALSE;
	}
	if (!loadGame(pFile, !KEEPOBJECTS, FREEMEM,TRUE))
	{
		return FALSE;
	}

	if (!newMapInitialise())
	{
		return FALSE;
	}

	//not interested in this value
	*ppData = NULL;
	return TRUE;
}





// New reduced resource type ... specially for PSX
// These are statically defined in data.c
// this is also defined in frameresource.c - needs moving to a .h file
typedef struct
{
	STRING *aType;				// points to the string defining the type (e.g. SCRIPT) - NULL indicates end of list
	RES_BUFFERLOAD buffLoad;		// routine to process the data for this type
	RES_FREE release;			// routine to release the data (NULL indicates none)
	void *ResourceData;			// Linked list of data - set to null initially
	UDWORD HashedType;			// hashed version of aType
} RES_TYPE_MIN;




static RES_TYPE_MIN ResourceTypes[]=
{
	{"SWEAPON", bufferSWEAPONLoad, NULL},
	{"SBODY", bufferSBODYLoad, dataReleaseStats},
	{"SBRAIN", bufferSBRAINLoad, NULL},
	{"SPROP", bufferSPROPLoad, NULL},
	{"SSENSOR", bufferSSENSORLoad, NULL},
	{"SECM", bufferSECMLoad, NULL},
	{"SREPAIR", bufferSREPAIRLoad, NULL},
	//{"SPROGRAM", bufferSPROGRAMLoad, NULL},
	{"SCONSTR", bufferSCONSTRLoad, NULL},
	{"SPROPTYPES", bufferSPROPTYPESLoad, NULL},
	{"SPROPSND", bufferSPROPSNDLoad, NULL},
	{"STERRTABLE", bufferSTERRTABLELoad, NULL},
	{"SSPECABIL", bufferSSPECABILLoad, NULL},
	{"SBPIMD", bufferSBPIMDLoad, NULL},
	{"SWEAPSND", bufferSWEAPSNDLoad, NULL},
	{"SWEAPMOD", bufferSWEAPMODLoad, NULL},
	{"STEMPL", bufferSTEMPLLoad, dataSTEMPLRelease},	//template and associated files
	{"STEMPWEAP", bufferSTEMPWEAPLoad, NULL},
	//{"STEMPPROG", bufferSTEMPPROGLoad, NULL},
	{"SSTRUCT", bufferSSTRUCTLoad, dataSSTRUCTRelease},		//structure stats and associated files
	{"SSTRFUNC", bufferSSTRFUNCLoad, NULL},
	{"SSTRWEAP", bufferSSTRWEAPLoad, NULL},
	{"SSTRMOD", bufferSSTRMODLoad, NULL},
	{"SFEAT", bufferSFEATLoad, dataSFEATRelease},	//feature stats file
	{"SFUNC", bufferSFUNCLoad, dataSFUNCRelease},	//function stats file
	{"RESCH", bufferRESCHLoad, dataRESCHRelease},	//research stats files
	{"RPREREQ", bufferRPREREQLoad, NULL},
	{"RCOMPRED", bufferRCOMPREDLoad, NULL},
	{"RCOMPRES", bufferRCOMPRESLoad, NULL},
	{"RSTRREQ", bufferRSTRREQLoad, NULL},
	{"RSTRRED", bufferRSTRREDLoad, NULL},
	{"RSTRRES", bufferRSTRRESLoad, NULL},
	{"RFUNC", bufferRFUNCLoad, NULL},
	{"SMSG", bufferSMSGLoad, dataSMSGRelease},
	{"SCRIPT", dataScriptLoad, (RES_FREE)scriptFreeCode},
	{"SCRIPTVAL", dataScriptLoadVals, NULL},
	{"STR_RES", dataStrResLoad, dataStrResRelease},
	{"IMGPAGE", dataIMGPAGELoad, dataIMGPAGERelease},
	{"TERTILES", dataTERTILESLoad, dataTERTILESRelease},	// freed by 3d shutdow},// Tertiles Files. This version used when running with software renderer.
	{"HWTERTILES", dataHWTERTILESLoad, dataHWTERTILESRelease},	// freed by 3d shutdow},// Tertiles Files. This version used when running with hardware renderer.
	{"AUDIOCFG", dataAudioCfgLoad, NULL},
	{"WAV", dataAudioLoad, dataAudioRelease},
	{"ANI", dataAnimLoad, dataAnimRelease},
	{"ANIMCFG", dataAnimCfgLoad, NULL},
	{"IMG", dataIMGLoad, dataIMGRelease},
	{"TEXPAGE", bufferTexPageLoad, dataTexPageRelease},
	{"IMD", dataIMDBufferLoad, (RES_FREE)iV_IMDRelease},


	{NULL,NULL,NULL}		// indicates end of list
};


/* Pass all the data loading functions to the framework library */
BOOL dataInitLoadFuncs(void)
{
	RES_TYPE_MIN *CurrentType;
//	UDWORD	i;

	// init the cheat system;
	resetCheatHash();

	CurrentType=ResourceTypes;	// point to the first entry

	// While there are still some entries in the list
	while( CurrentType->aType != NULL )
	{
//		printf(" ==>%s\n",CurrentType->aType);	//TESTING -Q
		if(!resAddBufferLoad(CurrentType->aType,CurrentType->buffLoad,CurrentType->release))
		{
			return FALSE;	// error whilst adding a buffer load
		}
		CurrentType++;
	}

	// Now add the only file load left!
	if (!resAddFileLoad("SAVEGAME", dataSaveGameLoad, NULL))
	{
		return FALSE;
	}

	return TRUE;
}

