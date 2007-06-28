

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef _MSC_VER			//we need windows.h for below inculde.  --Qamly
#include <windows.h>
#endif
#include <GL/gl.h>

#include "frame.h"
#include "ivisdef.h"
#include "piestate.h"
#include "tex.h"
#include "rendmode.h"
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


/**************************************************************************
	Add an image buffer given in s as a new texture page in the texture
	table.  We check first if the given image has already been loaded,
	as a sanity check (should never happen).  The texture numbers are
	stored in a special texture table, not in the resource system, for
	some unknown reason.

	Returns the texture number of the image.
**************************************************************************/
int pie_AddBMPtoTexPages(iSprite* s, STRING* filename, int type, iBool bColourKeyed, 
                         iBool bResource) {
	int	i = 0;

	debug(LOG_TEXTURE, "pie_AddBMPtoTexPages: %s type=%d col=%d res=%d", filename, type, 
	      bColourKeyed, bResource);
	assert(s != NULL);

	/* Have we already loaded this one? (Should generally not happen here.) */
	while (i < _TEX_INDEX) {
		if (stricmp(filename, _TEX_PAGE[i].name) == 0) {
			// this happens with terrain for some reason, which is necessary
			debug(LOG_TEXTURE, "pie_AddBMPtoTexPages: %s loaded again", filename);
		}
		i++;
	}

	/* Get next available texture page */
	i = _TEX_INDEX;

	/* Have we used up too many? */
	if (_TEX_INDEX >= iV_TEX_MAX) {
		debug(LOG_ERROR, "pie_AddBMPtoTexPages: too many texture pages");
		assert(FALSE);
		return -1;
	}

	/* Stick the name into the tex page structures */
	strcpy(_TEX_PAGE[i].name, filename);

	/* Store away all the info */
	/* DID come from a resource */
	_TEX_PAGE[i].bResource = bResource;
	// Default values
	_TEX_PAGE[i].tex.bmp = NULL;
	_TEX_PAGE[i].tex.width = 256;
	_TEX_PAGE[i].tex.height = 256;
	_TEX_PAGE[i].tex.xshift = 0;
	_TEX_PAGE[i].tex.bmp = s->bmp;
	_TEX_PAGE[i].tex.width = s->width;
	_TEX_PAGE[i].tex.height = s->height;
	_TEX_PAGE[i].tex.xshift = _tex_get_top_bit(s->width);
	_TEX_PAGE[i].tex.bColourKeyed = bColourKeyed;
	_TEX_PAGE[i].type = type;

	glGenTextures(1, &_TEX_PAGE[i].textPage3dfx);

	pie_SetTexturePage(i);

	if (   (s->width & (s->width-1)) == 0
	    && (s->height & (s->height-1)) == 0) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, s->width, s->height, 0,
			     GL_RGBA, GL_UNSIGNED_BYTE, s->bmp);
	}

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	/* Send back the texpage number so we can store it in the IMD */

	_TEX_INDEX++;

	return i;
}

void pie_ChangeTexPage(int tex_index, iSprite* s, int type, iBool bColourKeyed, iBool bResource)
{
	assert(s != NULL);

	/* DID come from a resource */
	_TEX_PAGE[tex_index].bResource = bResource;
	// Default values
	_TEX_PAGE[tex_index].tex.bmp = NULL;
	_TEX_PAGE[tex_index].tex.width = 256;
	_TEX_PAGE[tex_index].tex.height = 256;
	_TEX_PAGE[tex_index].tex.xshift = 0;
	_TEX_PAGE[tex_index].tex.bmp = s->bmp;
	_TEX_PAGE[tex_index].tex.width = s->width;
	_TEX_PAGE[tex_index].tex.height = s->height;
	_TEX_PAGE[tex_index].tex.xshift = _tex_get_top_bit(s->width);
	_TEX_PAGE[tex_index].tex.bColourKeyed = bColourKeyed;
	_TEX_PAGE[tex_index].type = type;

	pie_SetTexturePage(tex_index);
}

/**************************************************************************
	Return the texture number for a given texture resource.  We keep
	textures in a separate data structure _TEX_PAGE apart from the
	normal resource system.
**************************************************************************/
int iV_GetTexture(char *filename)
{
	int i  = 0;

	/* Have we already loaded this one then? (Yes. Always.) */
	while (i < _TEX_INDEX) {
		if (stricmp(filename, _TEX_PAGE[i].name) == 0) {
			return i;
		}
		i++;
	}

	/* This should never happen - by now all textures should have been loaded. */
	debug(LOG_ERROR, "iV_TexLoad: texture %s not loaded!", filename);
	assert(FALSE);
	return -1;
}

int pie_ReloadTexPage(STRING *filename, SBYTE *pBuffer)
{
	int i = 0;
	iSprite			s;

	/* Have we already loaded this one then? */
	while (stricmp(filename,_TEX_PAGE[i].name) != 0) {
		i++;
		if (i >= _TEX_INDEX) {
			debug(LOG_TEXTURE, "Texture not in resources");
			assert(FALSE);
			return -1;
		}
	}
	//got the old texture page so load bmp straight in
	s.width = _TEX_PAGE[i].tex.width;
	s.height = _TEX_PAGE[i].tex.height;
	s.bmp = _TEX_PAGE[i].tex.bmp;

  // FIXME: evil cast
	pie_PNGLoadMemToBuffer((int8 *)pBuffer, &s, NULL); 

	return i;
}

/*
	Alex - fixed this so it doesn't try to free up the memory if it got the page from resource
	handler - this is because the resource handler will deal with freeing it, and in all probability
	will have already done so by the time this is called, thus avoiding an 'already freed' moan.
*/
void pie_TexShutDown(void) {
	int i,j;

	i = 0;
	j = 0;

	while (i < _TEX_INDEX) {
		/*	Only free up the ones that were NOT allocated through resource handler cos they'll already
			be free */
		if(_TEX_PAGE[i].bResource == FALSE)
		{
			if(_TEX_PAGE[i].tex.bmp) {
				j++;
				iV_HeapFree(_TEX_PAGE[i].tex.bmp,_TEX_PAGE[i].tex.width * _TEX_PAGE[i].tex.height);
			}
		}
		i++;
	}

	DBPRINTF(("pie_TexShutDown successful - freed %d texture pages\n",j));
}

void pie_TexInit(void) {
	int i;

	i = 0;

	while (i < _TEX_INDEX) {
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
	if ((Width > 256) || (Height > 256)) {
		return FALSE;
	}

	if (!iV_IsPower2(Width)) {
		return FALSE;
	}

//  For now don't limit height to 2^n.
	if (!iV_IsPower2(Height)) {
		return FALSE;
	}

	return TRUE;
}


// Return TRUE if the given value is 2^n.
//
BOOL iV_IsPower2(UDWORD Value) {
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

