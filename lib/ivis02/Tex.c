

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "frame.h"
#include "ivisdef.h"
#include "pieState.h"
#ifdef WIN32
#include "dx6TexMan.h"
#endif
#include "tex.h"
#include "rendmode.h"
#include "pcx.h"
#include "piePalette.h"
#include "bug.h"
#include "ivispatch.h"
#ifdef INC_GLIDE
#include "3dfxText.h"
#endif
#ifdef WIN32
#include "d3drender.h"
#endif

#ifdef PSX
void UploadPCXClut(UBYTE *Pal,UWORD VRAMx, UWORD VRAMy);
#endif


//*************************************************************************

iTexPage _TEX_PAGE[iV_TEX_MAX];

//*************************************************************************

int _TEX_INDEX;

//*************************************************************************

static int _tex_get_top_bit(uint32 n)

{
	int i;
	uint32 mask = 0x80000000;


	for (i=31; (n & mask) == 0; mask >>=1, i--)
		;

	return i;
}

#ifdef PIEPSX
/*
	When a binary PIE is loaded the texture page field contains just a number that the PieBin utility assigned it
	generally this number is the numeric part of the texture page file name. For example if the texture page was
	"Page-11-Player Buildings.pcx" the texture page field will contain 11. 
	When the texture pages are loaded up (from the WRF) the .num field contains a matching number which matchs up
	with the binary pie texture page. This will allow the texture page id field in the PIE to be replaced with the
	valid ID from the texturepage structure.

*/
BOOL MapTexturePage(iIMDShape *Pie)
{
		// Scan through all the loaded texture pages and find a match for the num.
	UDWORD Page=0;
	
	while (Page<_TEX_INDEX)
	{
		if (_TEX_PAGE[Page].tex.num==Pie->tpage)
		{
//			DBPRINTF(("PIE texture num %d mapped to ",Pie->tpage));
			Pie->tpage=_TEX_PAGE[Page].tex.tpage;
//			DBPRINTF(("%d\n",Pie->tpage));
			return(TRUE);
		}
		Page++;
	}		
	// no match found
	DBPRINTF(("No match for PIE texture num %d\n",Pie->tpage));
	return(FALSE);
			
}
#endif


//*************************************************************************
//*** load texture file or return index if already loaded
//*
//* params	filename = texture file name
//*			type		= 0 -> pcx
//*
//* returns	-1			= error
//*			else		= texture slot
//*
//******

					
/*
	Alex's shiny new texture loader. Will first check to see if the filename
	you're trying to load already resides in either memory (software) or in
	3dfx texture memory (3dfx). If it does, it'll send back the page number. If
	not, then it'll load it in for you. Also, it'll try and get it from the resource
	handler. If the resource handler doesn't know about this file then, the old
	texture load function is called. Should still work on PSX as the resource stuff isn't in
	there yet, so it'll default through to the old version 
*/

int pie_AddBMPtoTexPages(iSprite* s, char* filename, int type, iBool bColourKeyed, iBool bResource)
{
	int				i3d;
	int				i;
#ifdef WIN32
	/* Get next available texture page */
	i = _TEX_INDEX;
	/* Have we used up too many? */
	if (_TEX_INDEX >= iV_TEX_MAX) 
	{
		iV_DEBUG1("tex[TexLoad] = too many texture pages '%s'\n",buffer);
		return -1;
	}

	/* Stick the name into the tex page structures */
	strcpy(_TEX_PAGE[i].name,filename);

	/* Store away all the info */
	/* DID come from a resource */
	_TEX_PAGE[i].bResource = bResource;
	// Default values
	_TEX_PAGE[i].tex.bmp = NULL;
	_TEX_PAGE[i].tex.width = 256;
	_TEX_PAGE[i].tex.height = 256;
	_TEX_PAGE[i].tex.xshift = 0;

	if (s!=NULL)
	{
		_TEX_PAGE[i].tex.bmp = s->bmp;
		_TEX_PAGE[i].tex.width = s->width;
		_TEX_PAGE[i].tex.height = s->height;
		_TEX_PAGE[i].tex.xshift = _tex_get_top_bit(s->width);
	}
	_TEX_PAGE[i].tex.bColourKeyed = bColourKeyed;
	_TEX_PAGE[i].type = type;

#ifndef PIEPSX
	if ( rendSurface.usr >= REND_D3D_RGB &&
		 rendSurface.usr <= REND_D3D_REF )
	{
		/* set pie texture pointer */
		if ( dtm_LoadTexSurface( &_TEX_PAGE[i].tex, i) == FALSE )
		{
			return -1;
		}
	}

	/* Now some extra stuff if we're running on a 3dfx? */
	if(rendSurface.usr == REND_GLIDE_3DFX)
	{
		if(iV_TexSizeIsLegal(s->width,s->height)) {
			/* Bang it down to the card if we're on a 3dfx */
			i3d = gl_downLoad8bitTexturePage(s->bmp,(UWORD)s->width,(UWORD)s->height);
			/* Update 'real' texpage number in texpage structure */
		} else {
// If it's an illegal texture page size then chuck a message and default to TPage 1.
			DBERROR(("Illegal size for texture. [%s , (%d,%d)]\n",
					filename,s->width,s->height));
			i3d = 1;
		}
		_TEX_PAGE[i].textPage3dfx = i3d;
	}
#endif
	/* Send back the texpage number so we can store it in the IMD */

	_TEX_INDEX++;

#ifndef PIEPSX
	if (pie_GetRenderEngine() == ENGINE_GLIDE)
	{
		return(i3d);
	}
	else
#endif
	{
		return (i);
	}
#else
	assert(2+2==5);
#endif
}


int iV_TexLoadNew( char *path, char *filename, int type,
					iBool palkeep, iBool bColourKeyed )
{
	char			fname[MAX_FILE_PATH];
	int				i;
	iSprite			*s;
	TEXTUREPAGE *TextPage;		// pointer to the resource texture page structure   ... palette stuff is BACK !!!!!




// If we are in the BSP or PIEBIN tool, then just added it into the array and exit
#ifdef PIETOOL
	return pie_AddBMPtoTexPages(NULL, filename, type, bColourKeyed, TRUE);
#endif


#ifdef WIN32
	/* If it's not a resource - use old way!  - PSX does not need this check because it MUST have been loaded allready by the resource loader */
	if(!resPresent("TEXPAGE",filename))
	{
		DBERROR(("Texture not in resources; %s.\n",	filename));
		return(iV_TexLoad( path, filename, type,
					palkeep, bColourKeyed ));
	}
#endif


#ifdef PSX
	{
		UDWORD TexNum;
		BOOL TextureLoaded;

		TexNum=GetTextureNumber(filename);		// which texture number does this page use
		TextureLoaded=FindTextureNumber(TexNum,&i);
		if(TextureLoaded==FALSE)
		{
			DBERROR(("iV_TexLoadNew : Texture was not in resources; %s.\n",	filename));
			return (-1);
		}
//	DBPRINTF(("Matched [%s] (%d) @ %d\n",filename,TexNum,i));
		return(i);
	}
#endif


#ifdef WIN32
	/* Ensure upper case for tex file names */
	ASSERT ((strlen(filename)<MAX_FILE_PATH,"Texture file path too long"));

	/* Get a copy of the name */
	// if we convert it to upper case ... the resource loading will not work

	for (i = 0; i < (int)strlen(filename); i++)
	{
#ifdef PSX
		fname[i] = tolower(filename[i]);	// PSX MUST convert to lower case to match .wrf entry names (all lower) to .pie names (some upper, some lower)
#else
		fname[i] = filename[i];
#endif
	  
	}


	/* Terminate it */
	fname[i] = '\0';

	/* Back to beginning */
	i = 0;

	/* Have we already loaded this one then? */
	while (i<_TEX_INDEX) 
	{
#ifdef PSX
		if (strcmp(fname,_TEX_PAGE[i].name) == 0)
#else
		if (stricmp(fname,_TEX_PAGE[i].name) == 0)
#endif
		{
			/* Send back 3dfx texpage number if we're on 3dfx - they're NOT the same */
		 	if(rendSurface.usr == REND_GLIDE_3DFX)
			{
				return(_TEX_PAGE[i].textPage3dfx);
			}
			else
			{
				/* Otherwise send back the software one */
				return i;
			}
		}
		i++;
	}

#ifdef PIEPSX

	DBPRINTF(("Texture page [%s] was not loaded in resource .wrf file! - fatal error\n",filename));
	return(-1);
#else

	
	/* Get a pointer to the texpage in memory - we KNOW it's there from the check at start */
	TextPage = (TEXTUREPAGE *)resGetData("TEXPAGE",filename);
	s = TextPage->Texture;

	return pie_AddBMPtoTexPages(s, fname, type, bColourKeyed, TRUE);

#endif


#endif
}

int pie_ReloadTexPage(char *filename,UBYTE *pBuffer)
{
#ifndef PSX
	char			fname[MAX_FILE_PATH];
	int				i;
	iSprite			s;

	/* Ensure upper case for tex file names */
	ASSERT ((strlen(filename)<MAX_FILE_PATH,"Texture file path too long"));

	/* Get a copy of the name */
	// if we convert it to upper case ... the resource loading will not work
	for (i = 0; i < (int)strlen(filename); i++)
	{
#ifdef PSX
		fname[i] = tolower(filename[i]);	// PSX MUST convert to lower case to match .wrf entry names (all lower) to .pie names (some upper, some lower)
#else
		fname[i] = filename[i];
#endif
	}
	/* Terminate it */
	fname[i] = '\0';

	/* Back to beginning */
	i = 0;
	/* Have we already loaded this one then? */
#ifdef PSX
	while (strcmp(fname,_TEX_PAGE[i].name) != 0)
	{
		i++;
		if (i>=_TEX_INDEX) 
		{
				DBPRINTF(("pie_TexReload [%s] was not found in texpage list\n",filename));
				return -1;
		}
	}
#else
	while (stricmp(fname,_TEX_PAGE[i].name) != 0)
	{
		i++;
		if (i>=_TEX_INDEX) 
		{
				DBERROR(("Texture not in resources\n",filename));
				return -1;
		}
	}
#endif
	//got the old texture page so load bmp straight in
	s.width = _TEX_PAGE[i].tex.width;
	s.height = _TEX_PAGE[i].tex.height;
	s.bmp = _TEX_PAGE[i].tex.bmp;

	pie_PCXLoadMemToBuffer(pBuffer,&s,NULL); 

#ifndef PSX
 	if(pie_GetRenderEngine() == ENGINE_GLIDE)
	{
		gl_Reload8bitTexturePage(s.bmp,(UWORD)s.width,(UWORD)s.height,_TEX_PAGE[i].textPage3dfx);
	}
	else if(pie_GetRenderEngine() == ENGINE_D3D)
	{
		dtm_LoadTexSurface(&_TEX_PAGE[i].tex, i);
	}

#endif

	return i;
#endif
}

int iV_TexLoad( char *path, char *filename, int type,
					iBool palkeep, iBool bColourKeyed )
{
#ifndef PSX
	int				i;
	char			buffer[MAX_FILE_PATH], fname[MAX_FILE_PATH];
	iSprite			s;
//	iPalette		pal, *pPal;

	// ensure upper case for tex file names
	ASSERT ((strlen(filename)<MAX_FILE_PATH,"Texture file path too long"));

	for (i = 0; i < (int)strlen(filename); i++)
		fname[i] = filename[i];

	fname[i] = '\0';
	i = 0;

	while (i<_TEX_INDEX) {
		if (stricmp(fname,_TEX_PAGE[i].name) == 0) {
			return i;
		}
		i++;
	}

	strcpy(buffer,path);
	strcat(buffer,fname);

	i = _TEX_INDEX;

	switch (type) {
		case 0: // pcx

			// load texture

//			if (!iV_PCXLoad(buffer,&s,&pal[0])) 
			if (!iV_PCXLoad(buffer,&s,NULL)) 
			{
				iV_DEBUG1("WARNING: tex[TexLoad] = failed to load pcx file '%s' \n",buffer);
// the bspimd tool just needs to return a warning if the texture is not found
#ifdef PIETOOL
				_TEX_PAGE[i].tex.bmp = NULL;
				_TEX_PAGE[i].tex.width = 0;
				_TEX_PAGE[i].tex.height = 0;
				_TEX_PAGE[i].tex.xshift = 0;

				_TEX_PAGE[i].type = type;
				return i;
#else
// otherwise return an error					
				return -1;
#endif

			}
			return pie_AddBMPtoTexPages(&s, fname, type, bColourKeyed, FALSE);
			break;
		default:
			iV_DEBUG1("tex[TexLoad] = unrecognised texture page type %d\n",type);
			return -1;
	}
#endif
	return -1;
}


// Routine to generate TEXpages 
//
// Called from the resource loading stuff
//
//  currently only for PSX but would make sense for the PC (?)
//
BOOL GenerateTEXPAGE(char *Filename, RECT *VramArea, UDWORD Mode, UWORD Clut)
{
#ifdef PSX
		int i;
		UWORD TPage;

		i=_TEX_INDEX;

		/* Have we used up too many? */
		if (++_TEX_INDEX > iV_TEX_MAX) 
		{
			iV_DEBUG0("Resource TexPageLoad ... too many texture pages\n");
			return FALSE;
		}

		/* Store away all the info */
	 	TPage = GetTPage(Mode,0,VramArea->x,VramArea->y);
		memcpy(&_TEX_PAGE[i].tex.VRAMpos,VramArea,sizeof(RECT));
		_TEX_PAGE[i].tex.num	= GetTextureNumber(Filename);		// base on the name e.g.     Page-16-  will return 16
		_TEX_PAGE[i].tex.tpage = TPage;
		_TEX_PAGE[i].tex.clut = Clut;		// Not really used ...
#endif
		return(TRUE);
}		

#ifdef PSX
// Scan through the loaded tex pages to see if this one is loaded. Returns TRUE and TexPage points to the loaded entry
BOOL FindTextureNumber(UDWORD TexNum,int* TexPage)
{
	int i;

	i = 0;

	while (i < _TEX_INDEX) 
	{
		if(_TEX_PAGE[i].tex.num==TexNum)
		{
			*TexPage=i;
//			*TexPage=&_TEX_PAGE[i];
			return TRUE;
		}
		i++;
	}
	return FALSE;	
}

#endif

SBYTE GetTextureNumber(char *Name)
{
	char Letter,NextLetter;
	// find first digit
	SBYTE Num=-1;

	do
	{
		Letter=*Name++;
		if ((Letter>='0') && (Letter<='9'))
		{
			NextLetter=*Name++;
			if ((NextLetter>='0') && (NextLetter<='9'))
			{
				Num=((Letter-'0')*10)+(NextLetter-'0');	// 2 digit number
			}
			else		// single digit number
			{
				Num=Letter-'0';
			}
			break;
		}


	} while (Letter!=0);

	return(Num);
	
}


/*
	Alex - fixed this so it doesn't try to free up the memory if it got the page from resource
	handler - this is because the resource handler will deal with freeing it, and in all probability
	will have already done so by the time this is called, thus avoiding an 'already freed' moan.
*/
void pie_TexShutDown(void)

{
#ifdef WIN32
	int i,j;

	i = 0;
	j = 0;

	while (i < _TEX_INDEX) 
	{
		/*	Only free up the ones that were NOT allocated through resource handler cos they'll already
			be free */
		if(_TEX_PAGE[i].bResource == FALSE)
		{
			if(_TEX_PAGE[i].tex.bmp)
			{
				j++;
				iV_HeapFree(_TEX_PAGE[i].tex.bmp,_TEX_PAGE[i].tex.width * _TEX_PAGE[i].tex.height);
			}
		}
		i++;
	}

	if (pie_GetRenderEngine() == ENGINE_GLIDE)
	{
		free3dfxTexMemory();
	}


	DBPRINTF(("pie_TexShutDown successful - freed %d texture pages\n",j));
#endif
}

void pie_TexInit(void)
{
#ifdef WIN32
	int i;

	i = 0;

	while (i < _TEX_INDEX) 
	{
		_TEX_PAGE[i].tex.bmp = NULL;
		_TEX_PAGE[i].tex.width = 0;
		_TEX_PAGE[i].tex.height = 0;
		_TEX_PAGE[i].tex.xshift = 0;
		i++;
	}
#endif
}

// Check that a texture is  <= 256x256 and 2^n x 2^n in size.
//
BOOL iV_TexSizeIsLegal(UDWORD Width,UDWORD Height)
{
	if((Width > 256) || (Height > 256)) {
		return FALSE;
	}

	if(!iV_IsPower2(Width)) {
		return FALSE;
	}

//  For now don't limit height to 2^n.
	if(!iV_IsPower2(Height)) {
		return FALSE;
	}

	return TRUE;
}


// Return TRUE if the given value is 2^n.
//
BOOL iV_IsPower2(UDWORD Value)
{
	int Bits = 0;

	while(Value) {
		if(Value & 1) {
			Bits++;
		}
		Value = Value >> 1;
	}

	if(Bits != 1) {
		return FALSE;
	}

	return TRUE;
}

