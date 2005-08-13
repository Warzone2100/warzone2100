

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "frame.h"
#include "ivisdef.h"
#include "piestate.h"
#include "tex.h"
#include "rendmode.h"
#include "pcx.h"
#include "piepalette.h"
#include "bug.h"
#include "ivispatch.h"
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
	int				i;
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

	/* Send back the texpage number so we can store it in the IMD */

	_TEX_INDEX++;

	{
		return (i);
	}

}

void pie_ChangeTexPage(int tex_index, iSprite* s, int type, iBool bColourKeyed, iBool bResource) {
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



	/* If it's not a resource - use old way!  - PSX does not need this check because it MUST have been loaded allready by the resource loader */
	if(!resPresent("TEXPAGE",filename))
	{
		DBERROR(("Texture not in resources[1]; %s.\n",	filename));
		return(iV_TexLoad( path, filename, type,
					palkeep, bColourKeyed ));
	}

	/* Ensure upper case for tex file names */
	ASSERT ((strlen(filename)<MAX_FILE_PATH,"Texture file path too long"));

	/* Get a copy of the name */
	// if we convert it to upper case ... the resource loading will not work

	for (i = 0; i < (int)strlen(filename); i++)
	{

		fname[i] = filename[i];

	  
	}


	/* Terminate it */
	fname[i] = '\0';

	/* Back to beginning */
	i = 0;

	/* Have we already loaded this one then? */
	while (i<_TEX_INDEX) 
	{

		if (stricmp(fname,_TEX_PAGE[i].name) == 0)

		{

			{
				/* Otherwise send back the software one */
				return i;
			}
		}
		i++;
	}



	
	/* Get a pointer to the texpage in memory - we KNOW it's there from the check at start */
	TextPage = (TEXTUREPAGE *)resGetData("TEXPAGE",filename);
	s = TextPage->Texture;

	return pie_AddBMPtoTexPages(s, fname, type, bColourKeyed, TRUE);



}

int pie_ReloadTexPage(STRING *filename, SBYTE *pBuffer)
{

	STRING fname[MAX_FILE_PATH];
	int				i;
	iSprite			s;

	/* Ensure upper case for tex file names */
	ASSERT ((strlen(filename)<MAX_FILE_PATH,"Texture file path too long"));

	/* Get a copy of the name */
	// if we convert it to upper case ... the resource loading will not work
	for (i = 0; i < (int)strlen(filename); i++) {
		fname[i] = filename[i];
	}
	/* Terminate it */
	fname[i] = '\0';

	/* Back to beginning */
	i = 0;
	/* Have we already loaded this one then? */

	while (stricmp(fname,_TEX_PAGE[i].name) != 0) {
		i++;
		if ( i>= _TEX_INDEX) {
			DBERROR(("Texture not in resources %s[2]\n",filename));
			return -1;
		}
	}

	//got the old texture page so load bmp straight in
	s.width = _TEX_PAGE[i].tex.width;
	s.height = _TEX_PAGE[i].tex.height;
	s.bmp = _TEX_PAGE[i].tex.bmp;

	// FIXME: evil cast
	pie_PCXLoadMemToBuffer((int8 *)pBuffer,&s,NULL); 

	return i;
}

int iV_TexLoad( char *path, char *filename, int type,
					iBool palkeep, iBool bColourKeyed )
{

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

	return -1;
}


// Routine to generate TEXpages 
//
// Called from the resource loading stuff
//
//  currently only for PSX but would make sense for the PC (?)
//

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

	DBPRINTF(("pie_TexShutDown successful - freed %d texture pages\n",j));

}

void pie_TexInit(void)
{

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

