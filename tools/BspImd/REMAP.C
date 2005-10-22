
/*


  This is the routines for remaping the texture PCX from the one currently assiociated with the IMD to a new one



*/
/*

  imdRemap


  Job description:-

i)       Loads in an IMD file and its associated PCX texture file.
ii)		Loads in a second PCX file containing the new layout for the textures
iii)	Remaps all the textures coords from the imd to point to the same texture in the second PCX
iv)		Saves out a new IMD with the new texture coords associated with the second PCX.

  This allows us to move away from the one PCX per IMD, which will not be possible on the Playstation 
  due to the lack of video memory.


  Tim Cannell - Pumpkin Studios - Eidos - Sept 8th 1997


  v1.1
    - Polygon size check was 1pixel to high & wide ... this meant to many slow checks on textures
	- percentage accuracy now displayed ... and a warning if more than 20% of pixels are incorrect
	- A warning is displayed if any polygons are smaller than 100 pixels in area




*/


#include <stdio.h>

#include "frame.h"	
#include "tex.h"
#include "imd.h"


BOOL RemapIMD(iIMDShape *IMD, int NewTexpage);




__inline int CountDiffs(UBYTE *Mem1, UBYTE *Mem2, int Size)
{
	int Diffs=0;
	while (Size>0)
	{
		if ((*Mem1)!=(*Mem2)) Diffs++;
		Mem1++;
		Mem2++;
		Size--;
	}
	return Diffs;
}



// Do 'dat remap
BOOL RemapIMD(iIMDShape *IMD, int NewTexpage)
{
	SDWORD Polygon;

	iIMDPoly *CurPoly;	

	BYTE *SourceTexture;
	UDWORD SourceTextureWidth,SourceTextureHeight;

	BYTE *Texture1,*Texture2;


	BOOL FoundMatch;

	UBYTE *NewTexture;
	UDWORD NewTextureWidth;
	UDWORD NewTextureHeight;

	NewTexture= *(UBYTE **)(iV_TEXBMP(NewTexpage));	// get pointer to texture page bitmap
	NewTextureWidth=iV_TEXWIDTH(NewTexpage);
	NewTextureHeight=iV_TEXHEIGHT(NewTexpage);

	SourceTexture= *(UBYTE **)(iV_TEXBMP(IMD->texpage));	// get pointer to texture page bitmap
	SourceTextureWidth=iV_TEXWIDTH(IMD->texpage);
	SourceTextureHeight=iV_TEXHEIGHT(IMD->texpage);

	for (Polygon=0;Polygon<IMD->npolys;Polygon++)
	{
		SDWORD minX,minY,maxX,maxY;
		SDWORD Point;
		SDWORD PolyTexWidth,PolyTexHeight;
		SDWORD Ycoord,Xcoord;
		SDWORD TexY;

		CurPoly=& IMD->polys[Polygon];
	
		// First we get the rectangle which contains the texture coords on the source bitmap
		maxX=maxY=0;
		minX=minY=99999;
		for (Point=0;Point<CurPoly->npnts;Point++)
		{
			if (CurPoly->vrt[Point].u	> maxX) maxX=CurPoly->vrt[Point].u;
			if (CurPoly->vrt[Point].u	< minX) minX=CurPoly->vrt[Point].u;
			if (CurPoly->vrt[Point].v	> maxY) maxY=CurPoly->vrt[Point].v;
			if (CurPoly->vrt[Point].v	< minY) minY=CurPoly->vrt[Point].v;
		}





		// Now we hunt for a match in the NewTexture


		PolyTexWidth=(maxX-minX)+1;		// Min is starting pos, Max is stopping pos ... so we need to add 1
		PolyTexHeight=(maxY-minY);			// For some reason this is not needed on the y

		FoundMatch=FALSE;
		for (Ycoord=0;Ycoord<NewTextureHeight-PolyTexHeight;Ycoord++)
		{
//			printf("checking line %d\n",Ycoord);
			for (Xcoord=0;Xcoord<NewTextureWidth-PolyTexWidth;Xcoord++)
			{
				Texture1 = SourceTexture + (minY*SourceTextureWidth) + (minX);
				Texture2 = NewTexture+ (Ycoord*NewTextureWidth) + (Xcoord);

				for (TexY=0;TexY<PolyTexHeight;TexY++)
				{
					if (memcmp(Texture1,Texture2,PolyTexWidth)!=0 ) break;

//					printf("Matched line %d of %d\n",TexY,PolyTexHeight);
					Texture1 += SourceTextureWidth;
					Texture2 += NewTextureWidth;
				}

				// found a match !!!
				if (TexY==PolyTexHeight)
				{
					FoundMatch=TRUE;
					break;		// 
				}
			
			}
			if (FoundMatch==TRUE) break;
		}


		// completed the check
		// If we havn't found a perfect match then scan for the best match
		if (FoundMatch==FALSE)
		{
			int BestDifs,BestX,BestY,Difs;
			int DifPercent;

			printf("No perfect match found for (%d,%d)->(%d,%d) Polygon %d/%d  Scanning...\n",minX,minY,maxX,maxY,Polygon,IMD->npolys);
			BestDifs=99999;
			BestX=0;
			BestY=0;

			for (Ycoord=0;Ycoord<NewTextureHeight-PolyTexHeight;Ycoord++)
			{
	//			printf("checking line %d\n",Ycoord);
				for (Xcoord=0;Xcoord<NewTextureWidth-PolyTexWidth;Xcoord++)
				{


					Texture1 = SourceTexture + (minY*SourceTextureWidth) + (minX);
					Texture2 = NewTexture+ (Ycoord*NewTextureWidth) + (Xcoord);
					Difs=0;

					for (TexY=0;TexY<PolyTexHeight;TexY++)
					{

						Difs+=CountDiffs(Texture1,Texture2,PolyTexWidth);		// Count the number of differances in one line
						Texture1 += SourceTextureWidth;
						Texture2 += NewTextureWidth;
						if (Difs > BestDifs) break;	// already too crap ...
					}

					if (Difs<BestDifs)
					{
						BestDifs=Difs;
						BestX=Xcoord;
						BestY=Ycoord;
					}
				
				}
			}

			DifPercent=((BestDifs*100)/(PolyTexWidth*PolyTexHeight));
			printf("Best match is @(%d,%d) with %d differances - %d%%",BestX,BestY,BestDifs,DifPercent);
			Xcoord=BestX;
			Ycoord=BestY;

			if (DifPercent > 20)
			{
				printf(" ****** Not a good match!");
			}

			printf("\n\n");


		}

//		printf("Match found (%d,%d) -> (%d,%d)\n",minX,minY,Xcoord,Ycoord);
		if ((PolyTexWidth * PolyTexHeight) < 100)
		{
			printf(" ********* Small polygon!\n");

		}

		// now remap minX->TexX  & minY->TexY
		for (Point=0;Point<CurPoly->npnts;Point++)
		{
			CurPoly->vrt[Point].u	= CurPoly->vrt[Point].u - minX + Xcoord ;
			CurPoly->vrt[Point].v	= CurPoly->vrt[Point].v - minY + Ycoord ;
		}

		

	}


	// need to free old tex page ...
	IMD->texpage=NewTexpage;

	return TRUE;

}

