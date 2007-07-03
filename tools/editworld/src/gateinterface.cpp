
#include "stdafx.h"
#include "btedit.h"
#include "grdland.h"
#include "mainfrm.h"
#include "bteditdoc.h"
#include "mapprefs.h"
#include "textureprefs.h"
#include "dibdraw.h"
#include "debugprint.h"
#include "wfview.h"
#include "textureview.h"
#include "bteditview.h"
#include "snapprefs.h"
#include "savesegmentdialog.h"
#include "limitsdialog.h"
#include "initiallimitsdlg.h"
#include "expandlimitsdlg.h"
#include "playermap.h"
#include "gateinterface.h"


extern "C" BOOL gwInitialise(void);
extern "C" void gwShutDown(void);
extern "C" SDWORD gwGetZone(SDWORD x, SDWORD y);

extern "C" UBYTE **apRLEZones;
// The zone equivalence tables - shows which land zones
// border on a water zone
extern "C" SDWORD gwNumZones;
extern "C" UBYTE *aNumEquiv;
extern "C" UBYTE **apEquivZones;



CHeightMap *g_MapData;


BOOL giWriteZones(FILE *Stream)
{
	ZoneMapHeader Header;

	Header.version = 2;
	Header.numStrips = g_MapData->GetMapHeight();
	Header.numZones = gwNumZones;

	fwrite(&Header,sizeof(Header),1,Stream);

	if(apRLEZones) {
		UWORD Size = 0;
		for(DWORD y=0; y < g_MapData->GetMapHeight(); y++) {
			UBYTE *pCode = apRLEZones[y];
			UWORD pos = 0;
			UWORD x = 0;

			while (x < g_MapData->GetMapWidth()) {
				x += pCode[pos];
				pos += 2;
			}

			Size += pos;

//			DebugPrint("Zone %d\n",pos);
			fwrite(&pos,sizeof(pos),1,Stream);		// Size of this strip.
			fwrite(apRLEZones[y],pos,1,Stream);		// The strip.
		}

		DebugPrint("Total zone map size %d\n",Size);

		Size = 0;
		for(int i=0; i < gwNumZones; i++) {
			Size += aNumEquiv[i] + sizeof(UBYTE);

			fwrite(&aNumEquiv[i],sizeof(UBYTE),1,Stream);
			fwrite(apEquivZones[i],aNumEquiv[i],1,Stream);

//			DebugPrint("%d : %d\n",i,aNumEquiv[i]);
		}

		DebugPrint("Total equivalence tables size %d\n",Size);

		return TRUE;
	}

	return FALSE;
}


void giSetMapData(CHeightMap *MapData)
{
	g_MapData = MapData;
}


BOOL giCreateZones(void)
{
	giClearGatewayFlags();

	// Check for and act on returning FALSE;
	return gwInitialise();
}


void giDeleteZones(void)
{
	gwShutDown();
}


void giUpdateMapZoneIDs(void)
{
	for(int z = 0; z< g_MapData->GetMapHeight(); z++) {
		for(int x = 0; x< g_MapData->GetMapWidth(); x++) {
			g_MapData->SetMapZoneID(x,z,gwGetZone(x,z));
		}
	}
}


void giClearGatewayFlags(void)
{
	for(int z = 0; z< g_MapData->GetMapHeight(); z++) {
		for(int x = 0; x< g_MapData->GetMapWidth(); x++) {
			g_MapData->SetTileGateway(x,z,FALSE);
		}
	}
}



// CPP to C interface functions, can be called from standard c modules.

extern "C"
{

int giGetMapWidth(void)
{
	return g_MapData->GetMapWidth();
}


int giGetMapHeight(void)
{
	return g_MapData->GetMapHeight();
}

void giSetGatewayFlag(int x,int y,BOOL IsGateway)
{
	g_MapData->SetTileGateway(x,y,IsGateway);
}

BOOL giIsClifface(int x,int y)
{
	return g_MapData->GetTileType(x,y) == TF_TYPECLIFFFACE;
}

BOOL giIsWater(int x,int y)
{
	return g_MapData->GetTileType(x,y) == TF_TYPEWATER;
}

BOOL giIsGateway(int x,int y)
{
	return g_MapData->GetTileGateway(x,y);
}

int giGetNumGateways(void)
{
	return g_MapData->GetNumGateways();
}

BOOL giGetGateway(int Index,int *x0,int *y0,int *x1,int *y1)
{
	if(!g_MapData->GetGateway(Index,x0,y0,x1,y1)) {
		return FALSE;
	}
	
	return TRUE;
}


}
