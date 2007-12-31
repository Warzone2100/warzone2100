/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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

#include "ivisdef.h"
#include "imd.h"
#include "tex.h"
#include "ivispatch.h"

//*************************************************************************

//*************************************************************************

#define MAX_SHAPE_RADIUS	160

//*************************************************************************


//*************************************************************************

#ifdef SAVEIMD
// new code the write out the connectors !
void _imd_save_connectors(FILE *fp, iIMDShape *s)
{
	Vector3i *p;
	int i;

	if (s->nconnectors != 0) {
		fprintf(fp,"CONNECTORS %d\n",s->nconnectors);
		p = s->connectors;
		for (i=0; i<s->nconnectors; i++, p++) {
			fprintf(fp,"\t%d %d %d\n", p->x,p->y,p->z);
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
BOOL iV_IMDSave(char *filename, iIMDShape *s, BOOL PieIMD)
{
	FILE *fp;
	iIMDShape *sp;
	iIMDPoly *poly;
	int nlevel, i, j, d;

	if ((fp = fopen(filename,"w")) == NULL) {
		return FALSE;
	}

	if (PieIMD==TRUE) {
		fprintf(fp,"%s %d\n",PIE_NAME,PIE_VER);
	} else {
		fprintf(fp,"%s %d\n",IMD_NAME,IMD_VER);
	}
	fprintf(fp,"TYPE %x\n",s->flags);

	// if textured write tex page file info
	if (s->texpage != iV_TEX_INVALID)
	{
		fprintf(fp,"TEXTURE %d %s %d %d\n",iV_TEXTYPE(s->texpage),
				iV_TEXNAME(s->texpage),iV_TEXWIDTH(s->texpage),
				iV_TEXHEIGHT(s->texpage));
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
			fprintf(fp,"\t%d %d %d\n",sp->points[j].x,sp->points[j].y,
					sp->points[j].z);
		}

		// write shape polys
		{
			fprintf(fp,"POLYGONS %d\n",sp->npolys);
			for (poly = sp->polys, j=0; j<sp->npolys; j++, poly++) {
				fprintf(fp,"\t%8x %u",poly->flags,poly->npnts);
				for (d=0; d<poly->npnts; d++) {
					fprintf(fp," %d",poly->pindex[d]);
				}

				if (poly->flags & iV_IMD_TEXANIM) {

					if (poly->pTexAnim == NULL) {
						debug(LOG_3D, "No TexAnim pointer!");
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
						fprintf(fp," %d %d",poly->vrt[d].u,poly->vrt[d].v);
					}
				}
				fprintf(fp,"\n");
			}
		}
	}

	_imd_save_connectors(fp,s);	// Write out the connectors if they exist

	fclose(fp);

	return TRUE;
}
#endif

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
   unsigned int i;
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
		d = s->next;
		free(s);
		iV_IMDRelease(d);
	}
}
