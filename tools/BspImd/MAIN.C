/*


  BSPIMD - Loads IMD, Generates BSP tree, saves IMD with BSP infomation


  - Tim Cannell - Pumpkin Studios - EIDOS - 1997 & 1998


  ivis02.lib nees to be compiled with :-
  - for PSX Version			
  #define WIN32
  #define PIEPSX
  #define PIETOOL

  - for PC Version
  #define WIN32
  #define PIETOOL








  so does this program

  v1.2 - check for duplicate vertices in a polygon and skips them if they happen

  v2.0	- Now includes the IMD remapping tool ... all in one .... just need to do the wildcard support
  v2.1	- Well what do you know ... wildcard support
  v2.2	- Option to perform remap without BSP (-nobsp)
  V2.3	- Many internal changes to allow mesa to work (almost) correctly ... internal double precesion values
  V2.3a - Minor memory allocation changes to allow "release" build to work correctly
  V2.3b - Bug fix in remap code to fix correct sizes for textures ...
  v3.0  - Rewritten to use current version of IVIS 
  v3.1	- Amended to save out PIE's rather than IMD's ... and some texture page changes
  v3.2	- Update to current version of Ivis.   Fix the "bspimd imdname -nobsp" option (reads then writes imd)
  v3.3  - Now handles IMD_TEXANIM polygon types
  v3.4  - Creates .bak file
  v3.5  - Correctly handles connectors
  v3.6  - Re-Handlied IMD_TEXANIM polygon types ... weird
  v4.0	- Recompile with new ivis - May98
  v4.1	- Added -v (verbose) option
  v4.2  - More verbose prints on ouput data, fixed bad size of copy in LinePlaneInter
  v4.3  - Added -w (warnings) option
*/

#define BSPIMD
#define PIETOOL

#include <stdio.h>

#include "frame.h"	
#include "ivi.h"	// for the typedefs
#include "imd.h"
#include "bspimd.h"




PSBSPPTRLIST LoadedPolyList;	// linked list of polygons in shape - for BSP generation 
void GenerateIMDBSP(iIMDShape *imd);

BOOL ProcessIMD(char *Filename, BOOL PerformBSP, BOOL PlaystationFlag, BOOL RemapTextures, int NewTexpage);


void Usage(void)
{
		printf("Usage: BSPIMD input_imd [-dnewtexturepcx] [-psx | -nobsp] [-v] [-w]\n");
		printf("input_imd can use wild cards\n");

		printf("\nBSPIMD V4.3 - %s %s\n",__DATE__,__TIME__);

		printf("    Pumpkin Studios 1998 - Tim Cannell\n");

}

BOOL Verbose=FALSE;
BOOL Warnings=FALSE;		// Display warnings (for non-coplanar quads)

void main(int argc, char *argv[] )
{

	WIN32_FIND_DATA FindData;
	HANDLE hSearch;
	BOOL res;
	int FileCount=0;
	int BadFiles=0;

	BOOL PlaystationFlag,RemapTextures,PerformBSP;
	int NewTexpage;
	int CurrentArg;


	PlaystationFlag=FALSE;
	RemapTextures=FALSE;
	PerformBSP=TRUE;
	Verbose=FALSE;


	// Handle the options

	CurrentArg=2;	// Start at arg 2, arg 0 is the command name, arg 1 is imd name

	while (CurrentArg < argc)
	{
		if (argv[CurrentArg][0]!='-')
		{
			printf("Bad usage\n");
			Usage();
			exit(-1);
		}
		switch (tolower(argv[CurrentArg][1]))
		{
			case 'p':
				PlaystationFlag=TRUE;	// Not used any more
				break;

			case 'n':
				PerformBSP=FALSE;
				break;

			case 'd':		// set texture page data directory - not needed
/*
				if (argv[CurrentArg][2]!=0)
				{
					strcpy(TexturePagePath,&argv[CurrentArg][2]);
				}
				else
				{
					CurrentArg++;
					strcpy(TexturePagePath,argv[CurrentArg]);
				}
*/
				break;
			case 'v':
				Verbose=TRUE;
				printf("Entering Verbose mode\n");
				break;


			case 'w':
				Warnings=TRUE;
				printf("Entering Warnings mode\n");
				break;

			default:
				printf("Bad option\n");
				Usage();
				exit(-1);
		}
		CurrentArg++;
	}


	if (RemapTextures==TRUE)
	{
		if (NewTexpage==-1)
		{
			printf("Unable to load new texture page\n");
			exit(-1);
		}
	}

	hSearch=FindFirstFile(argv[1],&FindData);

	if (hSearch==INVALID_HANDLE_VALUE)
	{
		printf("No matching files\n");
		Usage();
		exit(-1);
	}

	res=TRUE;

	while(res==TRUE)
	{
		FileCount++;


		if (ProcessIMD(FindData.cFileName,PerformBSP,PlaystationFlag,RemapTextures,NewTexpage)==FALSE)
		{
			BadFiles++;
		}
//		printf("filename=%s\n",FindData.cFileName);
		res=FindNextFile(hSearch,&FindData);
	}

	FindClose(hSearch);
	printf("%d files processed\n",FileCount);
	if (BadFiles>0)
	{
		printf("%d files were unsuccessfully processed\n",BadFiles);
	}

}


static UDWORD NodeCount,PolyCount;

void InfoOnTriangleList(PSBSPPTRLIST TriList)
{
	iIMDPoly *Triangle;



	if (TriList==NULL)
	{
		printf("None - null list\n");
		return;
	}
	if (TriList->iNumNodes==0)
	{
		printf("None - no nodes\n");
		return;
	}
	
	Triangle=list_GetFirst(TriList);	

	while (Triangle!=NULL)
	{
		printf("Poly %d) @ $%p\n",PolyCount,Triangle);
		PolyCount++;


		Triangle=list_GetNext(TriList,Triangle);
	}
}


void InfoOnBSPNode( PSBSPTREENODE psNode)
{
	if ( psNode == NULL )
	{
		return;
	}

	InfoOnBSPNode( psNode->link[LEFT]);

	

	printf("BSP Node %d @ $%p\n",NodeCount,psNode);

	NodeCount++;

	printf(" Polys on same side :");
	InfoOnTriangleList( psNode->psTriSameDir );
	printf(" Polys on oppo side :");
	InfoOnTriangleList( psNode->psTriOppoDir );
	InfoOnBSPNode( psNode->link[RIGHT]);
}


// rename()
// copy()

BOOL ProcessIMD(char *Filename, BOOL PerformBSP, BOOL PlaystationFlag, BOOL RemapTextures, int NewTexpage)
{
	iIMDShape *LoadedIMD;

	BOOL res;

	char BackupFilename[256];

	printf("Loading IMD File - %s\n",Filename);


	strcpy(BackupFilename,Filename);
	strcat(BackupFilename,".bak");

	LoadedIMD = iV_IMDLoad(Filename,FALSE);

	if (LoadedIMD==NULL)
	{
		printf("IMD Load failed...\n");
		return FALSE;
	}


	if (PlaystationFlag==TRUE)
	{

		int i; 
		iIMDPoly *CurPoly;
		printf("Converting to PlayStation\n");
		for (i=0;i<LoadedIMD->npolys;i++)
		{
			CurPoly=&LoadedIMD->polys[i];

			CurPoly->flags&=(~iV_IMD_TEX);	// remove old imd texture stuff
			CurPoly->flags|=(iV_IMD_PSXTEX);  // add playstation type
		}

	}

	// now do the remap if needed ... we need some way for turning this off...

	if (RemapTextures==TRUE)
	{
		printf("Remapping to new texture page\n");
		res=RemapIMD(LoadedIMD, NewTexpage);

		if (res==FALSE)
		{
			printf("Error whilst remapping IMD\n");
			return FALSE;
		}

	}

	if (PerformBSP==TRUE)
	{
		printf("Generating BSP\n");
		GenerateIMDBSP(LoadedIMD);

	
		if (Verbose)
		{
			printf("Polygon BSP completed - summary:-\nNumber Of Polygons=%d\nNumber Of Points=%d\nNumber Of Connectors=%d\n\n",
				LoadedIMD->npolys,LoadedIMD->npoints,LoadedIMD->nconnectors);

				NodeCount=0;
				PolyCount=0;
				InfoOnBSPNode( LoadedIMD->BSPNode);


		}
	
	}

	printf("Creating backup\n");
	remove(BackupFilename);
	res=rename(Filename,BackupFilename);

	if (res!=0)
	{
		printf("Unable to create backup file - %s. Saving aborted\n",BackupFilename);
	}
	else
	{
		printf("Saving IMD - %s\n",Filename);
		iV_IMDSave(Filename, LoadedIMD,TRUE);		// the bool true at the end indicates saving a PIE rather than an IMD
	}
	// Free the memory
	iV_IMDRelease(LoadedIMD);

	return TRUE;
}


int CompareFunc( void *n1, void *n2 );


// Generate a BSP tree for the IMD 
void GenerateIMDBSP(iIMDShape *imd)
{
	UWORD Polygon;
	int NewPolyCount,Poly;
	iIMDPoly *NewPolys;
	iIMDPoly *OldPoly;
	iIMDPoly *NewPoly;


	iIMDPoly * PolyList[iV_IMD_MAX_POLYS];	// Array of pointers to polys


	PSBSPTREENODE psRootNode;
	PSBINTREE psTree;

	// This set which IMD we are currently working with, this is so we can
	// keep a list of vertices, and also so we can adjust the IMD when splitting polygons
	//  - really this should be pased into PartitionTriListWithPlane, however we need to 
	//   keep parameters down to a minimum due to stack usage
	SetCurrentIMD(imd);


	CreateHiPreVectices(imd);	// Create a high-precesion vertex list


	// we need to generate the linked list of triangles in the shape

	// We will need to re-create the list of polygons from the linked list after we have finshed BSPing
	// This is because we are deleting & creating polygons
	list_Create(&(LoadedPolyList));
	
	for (Polygon=0;Polygon<imd->npolys;Polygon++)
	{
//		dumppoly(&(imd->polys[Polygon]));

		iIMDPoly *CurPoly;


		CurPoly = &(imd->polys[Polygon]);
		
		if (CurPoly->npnts == 3)
		{
			// Check for duplicate points ... this happens for some reason ...
			if ((CurPoly->pindex[0]==CurPoly->pindex[1]) ||
				 (CurPoly->pindex[0]==CurPoly->pindex[2]) ||
				 (CurPoly->pindex[1]==CurPoly->pindex[2]) )
			{
				printf("Duplicate points in polygon %d - skipping it\n",Polygon);
				continue;
			}
			list_Add(LoadedPolyList,CurPoly);
		}
		else if (CurPoly->npnts == 4)
		{
			if ((CurPoly->pindex[0]==CurPoly->pindex[1]) ||
			 (CurPoly->pindex[0]==CurPoly->pindex[2]) ||
			 (CurPoly->pindex[0]==CurPoly->pindex[3]) ||
 			 (CurPoly->pindex[1]==CurPoly->pindex[2]) ||
			 (CurPoly->pindex[1]==CurPoly->pindex[3]) ||
			 (CurPoly->pindex[2]==CurPoly->pindex[3]) )
			{
				printf("Duplicate points in polygon %d - skipping it\n",Polygon);
				continue;
			}

			list_Add(LoadedPolyList,CurPoly);
		}
		else
		{
			printf("Bad number of points in a polygon (%d) ... skipping it\n",Polygon);
		}
		
	}

	/* init root node */
	psRootNode = InitNode();

	psRootNode->psPlane->a=0;	// set plane in root node to zero, - important for first point test
	psRootNode->psPlane->b=0;
	psRootNode->psPlane->c=0;
	psRootNode->psPlane->d=0;

	/* create tree */
	psTree = BinTreeCreate( (PSBNODE) psRootNode,
						CompareFunc, 1, sizeof(BSPTREENODE) );

	/* init right link */
	psRootNode->link[RIGHT] = InitNode();


	PartitionPolyListWithPlane( psRootNode->link[RIGHT],
								LoadedPolyList );

	LoadedPolyList=NULL;	// This has been free'd already

	// update counter
	imd->BSPNode=psRootNode;	// set node 


	// - now we build a list of pointers to all the polygons


	CreateLoPreVectices(imd);	// Convert back the high-precesion vertex list


//	printf("ok\n");
}



// New routines to tie in with ivis

// Textures & cluts are now uploaded to the playstation Videomem when the imd is loaded
BOOL IVIStoVRAMtranslate(UDWORD u, UDWORD v, UWORD *Tpage,UWORD *Clut, UBYTE *TextureU, UBYTE *TextureV, int32 TexPage, UDWORD PolyFlag)
{
		*TextureU=(UBYTE)u;
		*TextureV=(UBYTE)v;
		*Tpage=0;
		*Clut=0;
	return TRUE;
}

iV_RenderAssign()
{
}

_mode_psx()
{
}

iV_ClearFonts()
{
}

_close_D3D()
{
}

_close_sr()
{
}

_close_4101()
{
}

_renderBegin_D3D()
{
}

_renderEnd_D3D()
{
}

_bank_off_sr()
{
}

_bank_off_4101()
{
}

_bank_on_sr()
{
}

_bank_on_4101()
{
}

gl_Reload8bitTexturePage(){}

reset3dfx(){}

free3dfxTexMemory(){}

gl_SetFoxStatus(){}

gl_SetGammaValue() {}

gl_VideoClose() {}

gl_DownloadDisplayBuffer() {}

gl_ScreenFlip() {}

gl_SetFogColour() {}

gl_SetFogStatus() {}




