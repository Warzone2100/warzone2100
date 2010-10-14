/*
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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "imd.h"
#include "texture.h"

#include "resmaster.h" // needed for texture width/height

//*************************************************************************

//*************************************************************************

#define MAX_SHAPE_RADIUS	160

//*************************************************************************


//*************************************************************************

// new code the write out the connectors !
void _imd_save_connectors(FILE *fp, iIMDShape *s)
{
	int i;

	if (s->nconnectors != 0) {
		fprintf(fp,"CONNECTORS %d\n",s->nconnectors);
		Vector3f *p = s->connectors;
		for (i=0; i<s->nconnectors; i++, p++) {
			fprintf(fp,"\t%f %f %f\n", p->x,p->y,p->z);
		}
	}
}

// new code the write out the connectors ! Old version (Integer)
void _imd_save_connectorsOld(FILE *fp, iIMDShape *s)
{
	int i;

	if (s->nconnectors != 0) {
		fprintf(fp,"CONNECTORS %d\n",s->nconnectors);
		Vector3f *p = s->connectors;
		for (i=0; i<s->nconnectors; i++, p++) {
			fprintf(fp,"\t%d %d %d\n", (Sint32)p->x,(Sint32)p->y,(Sint32)p->z);
		}
	}
}

//*************************************************************************
//*** save IMD file
//*
//* pre		shape successfully loaded
//*
//* params	filename = name of file to save to including .IMD extention
//*			s 			= pointer to IMD shape
//*
//* returns TRUE -> ok, FLASE -> error
//*
//******
bool iV_IMDSave(const char *filename, iIMDShape *s, bool PieIMD)
{
	FILE *fp;
	iIMDShape *sp;
	iIMDPoly *poly;
	int nlevel, i, j, d;
	const Uint32 dummyFlags = 0xDEADBEEF;

	if ((fp = fopen(filename,"w")) == NULL) {
		return false;
	}

	if (PieIMD == true) {
		fprintf(fp,"%s %d\n",PIE_NAME,PIE_VER);
	} else {
		fprintf(fp,"%s %d\n",IMD_NAME,IMD_VER);
	}
	fprintf(fp,"TYPE %x\n", dummyFlags);

	// if textured write tex page file info
	if (s->texpage != iV_TEX_INVALID)
	{
		//512W 512H for now...
		fprintf(fp,"TEXTURE %s %s %d %d\n", (const char*)"0",
				iV_TEXNAME(s->texpage), _TEX_PAGE[s->texpage].w,
				_TEX_PAGE[s->texpage].h);
	}

	// find number of levels in shape
	for (nlevel=0, sp = s; sp != NULL; sp = sp->next, nlevel++)
		;

	fprintf(fp,"LEVELS %d\n",nlevel);

	for (sp = s, i=0; i<nlevel; sp = sp->next, i++) {
		fprintf(fp,"LEVEL %d\n",(i+1));
		fprintf(fp,"POINTS %d\n",sp->npoints);

		// write shape points
		for (j = 0; j < sp->npoints; j++) {
			fprintf(fp,"\t%f %f %f\n",sp->points[j].x,sp->points[j].y,
					sp->points[j].z);
		}

		// write shape polys
		{
			fprintf(fp,"POLYGONS %d\n",sp->npolys);
			for (poly = sp->polys, j=0; j<sp->npolys; j++, poly++) {
				fprintf(fp,"\t%8x %d",poly->flags,poly->npnts);
				for (d=0; d<poly->npnts; d++) {
					fprintf(fp," %d",poly->pindex[d]);
				}

				if (poly->flags & iV_IMD_TEXANIM) {

					if (poly->pTexAnim == NULL) {
						fprintf( stderr, "No TexAnim pointer!\n" );
					} else {
						fprintf(fp," %d %d %d %d",
							poly->pTexAnim->nFrames,
							poly->pTexAnim->playbackRate,
							poly->pTexAnim->textureWidth,
							poly->pTexAnim->textureHeight);
					}
				}

				// if textured write texture uv's
				if (poly->flags & iV_IMD_TEX)
				{
					for (d=0; d<poly->npnts; d++) {
						fprintf(fp," %f %f",poly->texCoord[d].x,poly->texCoord[d].y);
					}
				}
				fprintf(fp,"\n");
			}
		}
	}

	_imd_save_connectors(fp,s);	// Write out the connectors if they exist

	fclose(fp);

	return true;
}

//*************************************************************************
//*** save IMD file (Old Integer version) WARNING:float mantissa will be discarded
//*
//* pre		shape successfully loaded
//*
//* params	filename = name of file to save to including .IMD extention
//*			s 			= pointer to IMD shape
//*
//* returns TRUE -> ok, FLASE -> error
//*
//******
bool iV_IMDSaveOld(const char *filename, iIMDShape *s, bool PieIMD)
{
	FILE *fp;
	iIMDShape *sp;
	iIMDPoly *poly;
	int nlevel, i, j, d;
	const Uint32 dummyFlags = 0xDEADBEEF;

	if ((fp = fopen(filename,"w")) == NULL) {
		return false;
	}

	if (PieIMD == true) {
		fprintf(fp,"%s %d\n",PIE_NAME,PIE_VER);
	} else {
		fprintf(fp,"%s %d\n",IMD_NAME,IMD_VER);
	}
	fprintf(fp,"TYPE %x\n", dummyFlags);

	// if textured write tex page file info
	if (s->texpage != iV_TEX_INVALID)
	{
		//512W 512H for now...
		fprintf(fp,"TEXTURE %s %s %d %d\n", (const char*)"0",
				iV_TEXNAME(s->texpage), _TEX_PAGE[s->texpage].w,
				_TEX_PAGE[s->texpage].h);
	}

	// find number of levels in shape
	for (nlevel=0, sp = s; sp != NULL; sp = sp->next, nlevel++)
		;

	fprintf(fp,"LEVELS %d\n",nlevel);

	for (sp = s, i=0; i<nlevel; sp = sp->next, i++) {
		fprintf(fp,"LEVEL %d\n",(i+1));
		fprintf(fp,"POINTS %d\n",sp->npoints);

		// write shape points
		for (j = 0; j < sp->npoints; j++) {
			fprintf(fp,"\t%d %d %d\n",(Sint32)sp->points[j].x,(Sint32)sp->points[j].y,
					(Sint32)sp->points[j].z);
		}

		// write shape polys
		{
			fprintf(fp,"POLYGONS %d\n",sp->npolys);
			for (poly = sp->polys, j=0; j<sp->npolys; j++, poly++) {
				fprintf(fp,"\t%8x %d",poly->flags,poly->npnts);
				for (d=0; d<poly->npnts; d++) {
					fprintf(fp," %d",poly->pindex[d]);
				}

				if (poly->flags & iV_IMD_TEXANIM) {

					if (poly->pTexAnim == NULL) {
						fprintf( stderr, "No TexAnim pointer!\n" );
					} else {
						fprintf(fp," %d %d %d %d",
							poly->pTexAnim->nFrames,
							poly->pTexAnim->playbackRate,
							poly->pTexAnim->textureWidth,
							poly->pTexAnim->textureHeight);
					}
				}

				// if textured write texture uv's
				if (poly->flags & iV_IMD_TEX)
				{
					for (d=0; d<poly->npnts; d++) {
						fprintf(fp," %d %d",(Sint32)poly->texCoord[d].x,(Sint32)poly->texCoord[d].y);
					}
				}
				fprintf(fp,"\n");
			}
		}
	}

	_imd_save_connectorsOld(fp,s);	// Write out the connectors if they exist

	fclose(fp);

	return true;
}


//*************************************************************************
//*** free IMD shape memory
//*
//* pre		shape successfully allocated
//*
//* params	shape = pointer to IMD shape
//*
//******
void iV_IMDRelease(iIMDShape *s)
{
   int i;
   iIMDShape *d;

   if (s) {
		if (s->points) {
			free(s->points);
		}
		if (s->connectors) {
			free(s->connectors);
		}
		if (s->polys) {
			for (i = 0; i < s->npolys; i++) {
				if (s->polys[i].pindex) {
					free(s->polys[i].pindex);
				}
				if (s->polys[i].pTexAnim) {
					free(s->polys[i].pTexAnim);
				}
				if (s->polys[i].texCoord) {
					free(s->polys[i].texCoord);
				}
			}
			free(s->polys);
		}
		if (s->shadowEdgeList)
		{
			free(s->shadowEdgeList);
			s->shadowEdgeList = NULL;
		}
		fprintf(stderr, "imd[IMDRelease] = release successful\n");
		d = s->next;
		free(s);
		iV_IMDRelease(d);
	}
}
