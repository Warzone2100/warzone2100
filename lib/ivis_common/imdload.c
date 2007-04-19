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
/***************************************************************************/
/*
 * imdload.c
 *
 * updated to load version 4 files
 *
 * changes at version 4;
 *		pcx name as string
 *		pcx filepath
 *		cut down vertex list
 *
 */
/***************************************************************************/

#include "lib/framework/frame.h"

#include "ivisdef.h"	// for imd structures
#include "imd.h"	// for imd structures
#include "rendmode.h"
#include "ivispatch.h"
#include "tex.h"		// texture page loading
#include "bspfunc.h"	// for imd functions


// Static variables
static Uint32 	_IMD_FLAGS;
static char		_IMD_NAME[MAX_FILE_PATH];
static Sint32 	_IMD_VER;
static VERTEXID 	vertexTable[iV_IMD_MAX_POINTS];

// kludge
extern void pie_SurfaceNormal(Vector3i *p1, Vector3i *p2, Vector3i *p3, Vector3i *v);

// local prototypes
static iIMDShape *_imd_load_level(char **FileData, char *FileDataEnd, int nlevels,
                                  int texpage);

static BOOL AtEndOfFile(char *CurPos, char *EndOfFile)
{
	while ((*CurPos==0x09)||(*CurPos==0x0a)||(*CurPos==0x0d)||(*CurPos==0x20)||(*CurPos==0x00))
	{
		CurPos++;
		if (CurPos>=EndOfFile) return TRUE;
	}

	if (CurPos >= EndOfFile) {
	 return TRUE;
	} else {
		return FALSE;
	}
}

static UDWORD IMDcount = 0;
static UDWORD IMDPolycount = 0;
static UDWORD IMDVertexcount = 0;
static UDWORD IMDPoints = 0;
static UDWORD IMDTexAnims = 0;
static UDWORD IMDConnectors = 0;

static char texfile[MAX_PATH]; // Last loaded texture page filename

// ppFileData is incremented to the end of the file on exit!
iIMDShape *iV_ProcessIMD( char **ppFileData, char *FileDataEnd )
{
	char		*pFileData = *ppFileData;
	int 		cnt;
	char		buffer[MAX_FILE_PATH], texType[MAX_FILE_PATH], ch; //, *str;
	int			i, nlevels, ptype, pwidth, pheight, texpage;
	iIMDShape	*s, *psShape;
	BOOL		bTextured = FALSE;
#ifdef BSPIMD
	UDWORD		level;
#endif

	IMDcount++;

	if (sscanf(pFileData, "%s %d%n", buffer, &_IMD_VER, &cnt) != 2)  {
		debug(LOG_ERROR, "iV_ProcessIMD file corrupt -A (%s)", buffer);
		assert(FALSE);
		return NULL;
	}
	pFileData += cnt;

	if ((strcmp(IMD_NAME,buffer) != 0) && (strcmp(PIE_NAME, buffer) !=0 )) {
		debug(LOG_ERROR, "iV_ProcessIMD: not an IMD file (%s %d)", buffer, _IMD_VER);
		return NULL;
	}

	//Now supporting version 4 files
	if ((_IMD_VER < 1) || (_IMD_VER > 4)) {
		debug(LOG_ERROR, "iV_ProcessIMD: file version not supported (%s)", buffer);
		return NULL;
	}

	if (sscanf(pFileData, "%s %x%n", buffer, &_IMD_FLAGS, &cnt) != 2) {
		debug(LOG_ERROR, "iV_ProcessIMD: file corrupt -B (%s)", buffer);
		return NULL;
	}
	pFileData += cnt;

	texpage = -1;

	// get texture page if specified
	if (_IMD_FLAGS & iV_IMD_XTEX){
		if (_IMD_VER == 1) {
			if (sscanf(pFileData, "%s %d %s %d %d%n", buffer, &ptype, texfile, &pwidth,
			           &pheight, &cnt) != 5) {
				debug(LOG_ERROR, "iV_ProcessIMD: file corrupt -C (%s)", buffer);
				return NULL;
			}
			pFileData += cnt;
			if (strcmp(buffer,"TEXTURE") != 0) {
				debug(LOG_ERROR, "iV_ProcessIMD: expecting 'TEXTURE' directive (%s)", buffer);
				return NULL;
			}
			bTextured = TRUE;
		}
		else //version 2 copes with long file names
		{
			if (sscanf(pFileData, "%s %d%n", buffer, &ptype, &cnt) != 2) {
				debug(LOG_ERROR, "iV_ProcessIMD: file corrupt -D (%s)", buffer);
				return NULL;
			}
			pFileData += cnt;

			if (strcmp(buffer, "TEXTURE") == 0) {
				ch = *pFileData++;

				for( i = 0; (i < MAX_PATH-5) && ((ch = *pFileData++) != EOF) && (ch != '.'); i++ ) // Run up to the dot or till the buffer is filled. Leave room for the extension.
				{
 					texfile[i] = (char)ch;
				}
				texfile[i] = '\0';

				if (sscanf(pFileData, "%s%n", texType, &cnt) != 1) {
					debug(LOG_ERROR, "iV_ProcessIMD: file corrupt -E (%s)", buffer);
					return NULL;
				}
				pFileData += cnt;

				if (strcmp(texType, "png") != 0) {
					debug(LOG_ERROR, "iV_ProcessIMD: file corrupt -F (%s)", buffer);
					return NULL;
				}
				strcat(texfile, ".png");

				if (sscanf(pFileData, "%d %d%n", &pwidth, &pheight, &cnt) != 2) {
					debug(LOG_ERROR, "iV_ProcessIMD: file corrupt -G (%s)", buffer);
					return NULL;
				}
				pFileData += cnt;
				bTextured = TRUE;
			} else if (strcmp(buffer, "NOTEXTURE") == 0) {
				if (sscanf(pFileData, "%s %d %d%n", texfile, &pwidth, &pheight, &cnt) != 3) {
					debug(LOG_ERROR, "iV_ProcessIMD: file corrupt -H (%s)", buffer);
					return NULL;
				}
				pFileData += cnt;
			} else {
				debug(LOG_ERROR, "iV_ProcessIMD(2): expecting 'TEXTURE' directive (%s)", buffer);
				return NULL;
			}
		}
		if (bTextured) {
			pie_Pagename(texfile);
		}
	}

	if (sscanf(pFileData, "%s %d%n", buffer, &nlevels, &cnt) !=2) {
		debug(LOG_ERROR, "iV_ProcessIMD: file corrupt -I (%s)", buffer);
		return NULL;
	}
	pFileData += cnt;

	if (strcmp(buffer,"LEVELS") != 0) {
		debug(LOG_ERROR, "iV_ProcessIMD: expecting 'LEVELS' directive (%s)", buffer);
		return NULL;
	}

#ifdef BSPIMD
// if we might have BSP then we need to preread the LEVEL directive
		if (sscanf(pFileData,"%s %d%n",buffer,&level,&cnt) != 2) {
			iV_Error(0xff,"(_load_level) file corrupt -J");
			return NULL;
		}
		pFileData += cnt;

		if (strcmp(buffer,"LEVEL") != 0) {
			debug(LOG_ERROR, "iV_ProcessIMD(2): expecting 'LEVELS' directive (%s)", buffer);
			return NULL;
		}
#endif

	s = _imd_load_level(&pFileData,FileDataEnd,nlevels,texpage);

	// load texture page if specified
	if ( (s != NULL) && (_IMD_FLAGS & iV_IMD_XTEX))
	{
		if(bTextured) {
			texpage = iV_GetTexture(texfile);
			if (texpage < 0) {
				debug(LOG_ERROR, "iV_ProcessIMD: could not load tex page %s", texfile);
				return NULL;
			}
		} else {
			texpage = -1;
		}
		/* assign tex page to levels */
		psShape = s;
		while (psShape != NULL) {
			psShape->texpage = texpage;
			psShape = psShape->next;
		}
	}

	if (s == NULL) {
		debug(LOG_ERROR, "iV_ProcessIMD: unsuccessful (%s)", buffer);
	}
	*ppFileData = pFileData;
	return (s);
}

//*************************************************************************
//*** load shape level polygons
//*
//* pre		fp open
//*			s allocated, s->npolys set
//*
//* params	fp = currently open shape file pointer
//*			s	= pointer to shape level
//*
//* on exit	s->polys allocated (iFSDPoly * s->npolys
//*			s->pindex allocated for each poly
//* returns	FALSE on error (memory allocation failure/bad file format)
//*
//******
static BOOL _imd_load_polys( char **ppFileData, iIMDShape *s )
{
	char *pFileData = *ppFileData;
	int cnt;
	int i, j; //, anim;
	Vector3i p0, p1, p2, *points;
	iIMDPoly *poly;
	int	nFrames,pbRate,tWidth,tHeight;

	//assumes points already set
	points = s->points;

	IMDPolycount+= s->npolys;

	s->numFrames = 0;
	s->animInterval = 0;

	s->polys = (iIMDPoly *) malloc(sizeof(iIMDPoly) * s->npolys);

	if (s->polys) {
		poly = s->polys;

		for (i = 0; i < s->npolys; i++, poly++) {
			UDWORD flags,npnts;

			if (sscanf(pFileData, "%x %d%n", &flags, &npnts, &cnt) != 2) {
				iV_Error(0xff,"(_load_polys) [poly %d] error loading flags and npoints",i);
			}
			pFileData += cnt;

			poly->flags=flags;

			if (flags & PIE_NO_CULL) {
				s->flags |= iV_IMD_NOCULLSOME;
			}

			poly->npnts=npnts;

			IMDVertexcount+= poly->npnts;

			poly->pindex = (VERTEXID *) malloc(sizeof(VERTEXID) * poly->npnts);

			if ((poly->vrt = (iVertex *)	malloc(sizeof(iVertex) * poly->npnts)) == NULL) {
				iV_Error(0xff,"(_load_polys) [poly %d] memory alloc fail (vertex struct)",i);
				return FALSE;
			}

			if (poly->pindex) {
				for (j=0; j<poly->npnts; j++) {
					int NewID;

					if (sscanf(pFileData, "%d%n", &NewID,&cnt) != 1) {
						debug( LOG_NEVER, "failed poly %d. point %d [%s]\n", i, j, _IMD_NAME );
						iV_Error(0xff,"(_load_polys) [poly %d] error reading poly indices",i);
						return FALSE;
					}
					pFileData += cnt;
					poly->pindex[j]=vertexTable[NewID];
				}
			} else {
				iV_Error(0xff,"(_load_polys) [poly %d] memory alloc fail (poly indices)",i);
				return FALSE;
			}

			// calc poly normal
			if (poly->npnts > 2) {
				p0.x = points[poly->pindex[0]].x;
				p0.y = points[poly->pindex[0]].y;
				p0.z = points[poly->pindex[0]].z;

				p1.x = points[poly->pindex[1]].x;
				p1.y = points[poly->pindex[1]].y;
				p1.z = points[poly->pindex[1]].z;

				p2.x = points[poly->pindex[poly->npnts-1]].x;
				p2.y = points[poly->pindex[poly->npnts-1]].y;
				p2.z = points[poly->pindex[poly->npnts-1]].z;

				pie_SurfaceNormal(&p0,&p1,&p2,&poly->normal);
				//iV_DEBUG3("normal %d %d %d\n",poly->normal.x,poly->normal.y,poly->normal.z);
			} else {
				poly->normal.x = poly->normal.y = poly->normal.z = 0;
			}

			if (poly->flags & iV_IMD_TEXANIM) {
				IMDTexAnims++;

				if ((poly->pTexAnim = (iTexAnim *)malloc(sizeof(iTexAnim))) == NULL) {
					iV_Error(0xff,"(_load_polys) [poly %d] memory alloc fail (iTexAnim struct)",i);
					return FALSE;
				}

				// even the psx needs to skip the data
				if (sscanf(pFileData,"%d %d %d %d%n",
						&nFrames,
						&pbRate,
						&tWidth,
						&tHeight,&cnt)	!= 4)
				{
					iV_Error(0xff,"(_load_polys) [poly %d] error reading texanim data",i);
					return FALSE;
				}
				pFileData += cnt;

				ASSERT( tWidth>0, "_imd_load_polys: texture width = %i", tWidth );
				ASSERT( tHeight>0, "_imd_load_polys: texture height = %i", tHeight );

				poly->pTexAnim->nFrames = nFrames;

				/* Assumes same number of frames per poly */

				s->numFrames = nFrames;
				poly->pTexAnim->playbackRate =pbRate;

				/* Uses Max metric playback rate */
				s->animInterval = pbRate;
				poly->pTexAnim->textureWidth =tWidth;
				poly->pTexAnim->textureHeight =tHeight;
			} else {
				poly->pTexAnim = NULL;
			}
		// PC texture coord routine
			if (poly->vrt && (poly->flags & (iV_IMD_TEX|iV_IMD_PSXTEX))) {
				for (j=0; j<poly->npnts; j++) {
					Sint32 VertexU, VertexV;
					if (sscanf(pFileData, "%d %d%n", &VertexU, &VertexV, &cnt) != 2) {
						iV_Error(0xff,"(_load_polys) [poly %d] error reading tex outline",i);
						return FALSE;
					}
					pFileData += cnt;

					poly->vrt[j].u=VertexU;
					poly->vrt[j].v=VertexV;
					poly->vrt[j].g=255;
				}
			}

#ifdef BSPIMD
			poly->BSP_NextPoly=BSPPOLYID_TERMINATE;	// make it end end of the BSP chain by default
#endif

		}
	} else {
		return FALSE;
	}

	*ppFileData = pFileData;
	return TRUE;
}


#ifdef BSPIMD

// The order for the BSP section of the IMD is :-
// LEFTLINK  FORWARD_POLYGONS_LIST_TEMRINATED_BY_-1  BACKWARD_POLYGONS_LIST_TEMRINATED_BY_-1  RIGHTLINK

#define GETBSPTRIANGLE(polyid) (&(s->polys[(polyid)]))

static BOOL _imd_load_bsp( char **ppFileData, iIMDShape *s, UWORD BSPNodeCount )
{
	char *pFileData = *ppFileData;
	int cnt;
	UWORD Node;
	PSBSPTREENODE NodeList;	// An pointer to an array of  nodes
	iIMDPoly *IMDTri;			// pointer to a polygon ... for handling the link list in the bsp
	iV_DEBUG1("imd[_load_bsp] = number of nodes =%d\n",BSPNodeCount);

	if (s->npolys >	BSPPOLYID_MAXPOLYID) {
		iV_Error(0xff,"(_imd_load_bsp) Too many polygons in IMD for BSP to handle");
	}

	// Build table of nodes - we sort out the links later
	NodeList = (BSPTREENODE*)malloc((sizeof(BSPTREENODE))*BSPNodeCount);	// Allocate the entire node tree

	memset(NodeList,0,(sizeof(BSPTREENODE))*BSPNodeCount);	// Zero it out ... we need to make all pointers NULL

	for (Node = 0; Node < BSPNodeCount; Node++) {
		BSPTREENODE *psNode;

		SDWORD NodeID;	// Temp storage area for a node ID
		SDWORD PolygonID,FirstPolygonID;	// Temp storage area for a polygon ID

		psNode = &(NodeList[Node]);

		FirstPolygonID=-1;	// This indicates the first polygon in the forward facing BSP list

		InitNode(psNode);

		if (sscanf(pFileData,"%d%n",&NodeID,&cnt) != 1)	// Check that we read 1 parameter ok
		{
			iV_Error(0xff,"(_load_bsp) - needed a left node!");
			return FALSE;
		}
		pFileData += cnt;
		psNode->link[LEFT]=(PSBSPTREENODE)NodeID;	// This could be -1 indicating an empty node

		// Get forward facing polygon list - never empty apart from root node
		while(1) {
			if (sscanf(pFileData,"%d%n",&PolygonID,&cnt) != 1) 	// Get a valid polygon number
			{
				iV_Error(0xff,"(_load_bsp) - needed a polygon number");
				return FALSE;
			}
			pFileData += cnt;

			if (PolygonID==-1)	break;

			if ((PolygonID<0) || (PolygonID >= s->npolys)) {
				iV_Error(0xff,"(_load_bsp) - bad polygon number");
				return FALSE;
			}

			if (FirstPolygonID==-1) FirstPolygonID=PolygonID;

			IMDTri=GETBSPTRIANGLE(PolygonID);
			if (IMDTri->BSP_NextPoly != BSPPOLYID_TERMINATE) {
				iV_Error(0xff,"(_load_bsp) - Polygon is mentioned more than once in the BSP");
			}

			IMDTri->BSP_NextPoly=psNode->TriSameDir;
			psNode->TriSameDir=PolygonID;
//			list_Add( psNode->psTriSameDir , &(s->polys[PolygonID]) );
		}

		// Generate the plane equation - if weve got any polygons
		if (FirstPolygonID != -1) {
			GetPlane(s, FirstPolygonID, &(psNode->Plane));
		} else {
			memset((char *)&(psNode->Plane),0,sizeof(PLANE));	// Clear the plane equation
		}

		// Get reverse facing polygon list - frequently empty
		while(1) {
			if (sscanf(pFileData,"%d%n",&PolygonID,&cnt) != 1) 	// Get a valid polygon number
			{
				iV_Error(0xff,"(_load_bsp) - needed a polygon number");
				return FALSE;
			}
			pFileData += cnt;

			if (PolygonID == -1) {
				break;
			}
			if ((PolygonID < 0) || (PolygonID >= s->npolys)) {
				iV_Error(0xff,"(_load_bsp) - bad polygon number");
				return FALSE;
			}

			// Insert into the list
			IMDTri=GETBSPTRIANGLE(PolygonID);
			if (IMDTri->BSP_NextPoly != BSPPOLYID_TERMINATE) {
				iV_Error(0xff,"(_load_bsp) - Polygon is mentioned more than once in the BSP");
			}

			IMDTri->BSP_NextPoly=psNode->TriOppoDir;
			psNode->TriOppoDir=PolygonID;

//			list_Add( psNode->psTriOppoDir , &(s->polys[PolygonID]) );
		}

		if (sscanf(pFileData,"%d%n",&NodeID,&cnt) != 1)	// Check that we read 1 parameter ok
		{
			iV_Error(0xff,"(_load_bsp) - needed a right node!");
			return FALSE;
		}
		pFileData += cnt;
		psNode->link[RIGHT]=(PSBSPTREENODE)NodeID;	// This could be -1 indicating an empty node
	}

	// Now fix all the links
	for (Node = 0; Node < BSPNodeCount; Node++) {
		BSPTREENODE *psNode;
		int NodeID;

		psNode = &(NodeList[Node]);

		if ((SDWORD)(psNode->link[LEFT]) == -1) {
			psNode->link[LEFT]=0;	// if its zero then its an empty link
		} else {
			NodeID = (int) psNode->link[LEFT];
			psNode->link[LEFT] = &NodeList[NodeID];
		}

		if ((SDWORD)(psNode->link[RIGHT]) == -1) {
			psNode->link[RIGHT]=0;	// if its zero then its an empty link
		} else {
			NodeID = (int) psNode->link[RIGHT];
			psNode->link[RIGHT] = &NodeList[NodeID];
		}
	}

	// Set the shape node list to the root node ... this can be used to
  // FREE up the BSP memory if we needed to
	s->BSPNode = &NodeList[0];
	iV_DEBUG0("BSP Loaded AOK\n");

	*ppFileData = pFileData;
	return TRUE;
}
#endif


static BOOL ReadPoints( char **ppFileData, iIMDShape *s )
{
	char *pFileData = *ppFileData;
	int cnt;
	int i;
	Vector3i *p;
	int lastPoint,match,j;
	SDWORD newX,newY,newZ;

	p = s->points;

	lastPoint = 0;

	for (i = 0; i < s->npoints; i++) {
		if (sscanf(pFileData, "%d %d %d%n", &(newX), &(newY), &(newZ), &cnt) != 3) {
			iV_Error(0xff,"(_load_points) file corrupt -K");
			return FALSE;
		}
		pFileData += cnt;

//		DBPRINTF(("%d) x=%d y=%x z=%d\n",i,newX,newY,newZ));
		//check for duplicate points
		match = -1;
		j = 0;

		// scan through list upto the number of points added (lastPoint) ... not up to the number of points scanned in (i)  (which will include duplicates)
		while((j < lastPoint) && (match == -1))
//		while((j < i) && (match == -1))
		{
			if (newX == p[j].x) {
				if (newY == p[j].y) {
					if (newZ == p[j].z) {
						match = j;
					}
				}
			}
			j++;
		}

		if (match == -1) {
			// new point
			p[lastPoint].x=newX;
			p[lastPoint].y=newY;
			p[lastPoint].z=newZ;
			vertexTable[i] = lastPoint;
			lastPoint++;
		} else {
			vertexTable[i] = match;
		}
	}

	//clear remaining table
	for (i = s->npoints; i < iV_IMD_MAX_POINTS; i++) {
		vertexTable[i] = -1;
	}

	s->npoints = lastPoint;

	*ppFileData = pFileData;
	return(TRUE);
}


//*************************************************************************
//*** load shape level vertices
//*
//* pre		fp open
//*			s allocated, s->npoints set
//*
//* params	fp 		= currently open shape file pointer
//*			s			= pointer to shape level
//*
//* on exit	s->points allocated (iVector * s->npoints
//* returns	FALSE on error (memory allocation failure/bad file format)
//*
//******
static BOOL _imd_load_points( char **ppFileData, iIMDShape *s )
{
	int i ;
	Vector3i *p;
	Sint32 tempXMax, tempXMin, tempZMax, tempZMin, extremeX, extremeZ;
	Sint32 xmax, ymax, zmax;
	double dx, dy, dz, rad_sq, rad, old_to_p_sq, old_to_p, old_to_new;
	double xspan, yspan, zspan, maxspan;
	Vector3f dia1, dia2, cen;
	Vector3f vxmin = { 0, 0, 0 }, vymin = { 0, 0, 0 }, vzmin = { 0, 0, 0 },
	         vxmax = { 0, 0, 0 }, vymax = { 0, 0, 0 }, vzmax = { 0, 0, 0 };

	//load the points then pass through a second time to setup bounding datavalues

	IMDPoints+=s->npoints;

	s->points = p = (Vector3i *) malloc(sizeof(Vector3i) * s->npoints);
	if (p == NULL) {
		return FALSE;
	}

	// Read in points and remove duplicates (!)
	if ( ReadPoints( ppFileData, s ) == FALSE )
	{
		return FALSE;
	}

	s->xmax = s->ymax = s->zmax = tempXMax = tempZMax = -FP12_MULTIPLIER;
	s->xmin = s->ymin = s->zmin = tempXMin = tempZMin = FP12_MULTIPLIER;

	vxmax.x = vymax.y = vzmax.z = (double) -FP12_MULTIPLIER;
	vxmin.x = vymin.y = vzmin.z = (double) FP12_MULTIPLIER;

	// set up bounding data for minimum number of vertices
	for (i = 0; i < s->npoints; i++, p++) {
			if (p->x > s->xmax)
				s->xmax = p->x;
			if (p->x < s->xmin)
				s->xmin = p->x;

			/* Biggest x coord so far within our height window? */
			if( (p->x > tempXMax) && (p->y > DROID_VIS_LOWER) && (p->y < DROID_VIS_UPPER) )
			{
				tempXMax = p->x;
			}

			/* Smallest x coord so far within our height window? */
			if( (p->x < tempXMin) && (p->y > DROID_VIS_LOWER) && (p->y < DROID_VIS_UPPER) )
			{
				tempXMin = p->x;
			}

			if (p->y > s->ymax)
				s->ymax = p->y;
			if (p->y < s->ymin)
				s->ymin = p->y;

			if (p->z > s->zmax)
				s->zmax = p->z;
			if (p->z < s->zmin)
				s->zmin = p->z;

			/* Biggest z coord so far within our height window? */
			if( (p->z > tempZMax) && (p->y > DROID_VIS_LOWER) && (p->y < DROID_VIS_UPPER) )
			{
				tempZMax = p->z;
			}

			/* Smallest z coord so far within our height window? */
			if( (p->z < tempZMax) && (p->y > DROID_VIS_LOWER) && (p->y < DROID_VIS_UPPER) )
			{
				tempZMin = p->z;
			}

			// for tight sphere calculations

			if ((double) p->x < vxmin.x) {
				vxmin.x = (double) p->x;
				vxmin.y = (double) p->y;
				vxmin.z = (double) p->z;
			}

			if ((double) p->x > vxmax.x) {
				vxmax.x = (double) p->x;
				vxmax.y = (double) p->y;
				vxmax.z = (double) p->z;
			}

			if ((double) p->y < vymin.y) {
				vymin.x = (double) p->x;
				vymin.y = (double) p->y;
				vymin.z = (double) p->z;
			}

			if ((double) p->y > vymax.y) {
				vymax.x = (double) p->x;
				vymax.y = (double) p->y;
				vymax.z = (double) p->z;
			}

			if ((double) p->z < vzmin.z) {
				vzmin.x = (double) p->x;
				vzmin.y = (double) p->y;
				vzmin.z = (double) p->z;
			}


			if ((double) p->z > vzmax.z) {
				vzmax.x = (double) p->x;
				vzmax.y = (double) p->y;
				vzmax.z = (double) p->z;
			}
		}

		/* Centered about origin I can do the '-' thing below!! */
		extremeX = pie_MAX(tempXMax,-tempXMin);
		extremeZ = pie_MAX(tempZMax,-tempZMin);

		s->visRadius = pie_MAX(extremeX,extremeZ);
		// no need to scale an IMD shape (only FSD)

		xmax = pie_MAX(s->xmax,-s->xmin);
		ymax = pie_MAX(s->ymax,-s->ymin);
		zmax = pie_MAX(s->zmax,-s->zmin);

		s->radius = pie_MAX(xmax,(pie_MAX(ymax,zmax)));


		s->sradius = (SDWORD)((float)sqrt( xmax*xmax + ymax*ymax + zmax*zmax));

// START: tight bounding sphere

		// set xspan = distance between 2 points xmin & xmax (squared)

		dx = vxmax.x - vxmin.x;
		dy = vxmax.y - vxmin.y;
		dz = vxmax.z - vxmin.z;
		xspan = dx*dx + dy*dy + dz*dz;

		// same for yspan

		dx = vymax.x - vymin.x;
		dy = vymax.y - vymin.y;
		dz = vymax.z - vymin.z;
		yspan = dx*dx + dy*dy + dz*dz;

		// and ofcourse zspan

		dx = vzmax.x - vzmin.x;
		dy = vzmax.y - vzmin.y;
		dz = vzmax.z - vzmin.z;
		zspan = dx*dx + dy*dy + dz*dz;

		// set points dia1 & dia2 to maximally seperated pair

		// assume xspan biggest

		dia1 = vxmin;
		dia2 = vxmax;
		maxspan = xspan;

		if (yspan>maxspan) {
			maxspan = yspan;
			dia1 = vymin;
			dia2 = vymax;
		}

		if (zspan>maxspan) {
			maxspan = zspan;
			dia1 = vzmin;
			dia2 = vzmax;
		}

		// dia1, dia2 diameter of initial sphere

		cen.x = (dia1.x + dia2.x) / 2.;
		cen.y = (dia1.y + dia2.y) / 2.;
		cen.z = (dia1.z + dia2.z) / 2.;

		// calc initial radius

		dx = dia2.x - cen.x;
		dy = dia2.y - cen.y;
		dz = dia2.z - cen.z;

		rad_sq = dx*dx + dy*dy + dz*dz;
		rad = sqrt(rad_sq);

		// second pass (find tight sphere)

		for (p = s->points, i=0; i<s->npoints; i++, p++) {
			dx = p->x - cen.x;
			dy = p->y - cen.y;
			dz = p->z - cen.z;
			old_to_p_sq = dx*dx + dy*dy + dz*dz;

			// do r**2 first
			if (old_to_p_sq>rad_sq) {
				// this point outside current sphere
				old_to_p = sqrt(old_to_p_sq);
				// radius of new sphere
				rad = (rad + old_to_p) / 2.;
				// rad**2 for next compare
				rad_sq = rad*rad;
				old_to_new = old_to_p - rad;
				// centre of new sphere
				cen.x = (rad*cen.x + old_to_new*p->x) / old_to_p;
				cen.y = (rad*cen.y + old_to_new*p->y) / old_to_p;
				cen.z = (rad*cen.z + old_to_new*p->z) / old_to_p;
				iV_DEBUG4("NEW SPHERE: cen,rad = %d %d %d,  %d\n",(Sint32) cen.x, (Sint32) cen.y, (inSint32t32) cen.z, (Sint32) rad);
			}
		}

		s->ocen.x = (Sint32) cen.x;
		s->ocen.y = (Sint32) cen.y;
		s->ocen.z = (Sint32) cen.z;
		s->oradius = (Sint32) rad;
		iV_DEBUG2("radius, sradius, %d, %d\n", s->radius, s->sradius);
		iV_DEBUG4("SPHERE: cen,rad = %d %d %d,  %d\n", s->ocen.x, s->ocen.y, s->ocen.z, s->oradius);

// END: tight bounding sphere
	return TRUE;
}


static BOOL _imd_load_connectors(char **ppFileData, iIMDShape *s)
{
	char *pFileData = *ppFileData;
	int cnt;
	int i;
	Vector3i *p;
	SDWORD newX,newY,newZ;

	IMDConnectors+=s->nconnectors;

	if ((s->connectors = (Vector3i *) malloc(sizeof(Vector3i) * s->nconnectors)) == NULL)
	{
		iV_Error(0xff,"(_load_connectors) MALLOC fail");
		return FALSE;
	}

	p = s->connectors;

	for (i=0; i<s->nconnectors; i++, p++)
	{
		if (sscanf(pFileData,"%d %d %d%n",&(newX),&(newY),&(newZ),&cnt) != 3)
		{
			iV_Error(0xff,"(_load_connectors) file corrupt -M");
			return FALSE;
		}
                pFileData += cnt;
		p->x=newX;
		p->y=newY;
		p->z=newZ;
	}

	*ppFileData = pFileData;
	return TRUE;
}


//*************************************************************************
//*** load shape levels recurrsively
//*
//* pre		fp open
//*
//* params	fp 		= currently open shape file pointer
//*			s			= pointer to shape level
//*			texpage	= texture page number if iV_IMD_TEX
//*
//* on exit	s allocated
//* returns	pointer to iFSDShape structure (or NULL on error)
//*
//******
static iIMDShape *_imd_load_level(char **ppFileData, char *FileDataEnd, int nlevels, int texpage)
{
	char *pFileData = *ppFileData;
	int cnt;
	iIMDShape *s;
	char buffer[MAX_FILE_PATH];
//	int level;
	int n;
	int npolys;

#ifdef BSPIMD
//		UWORD NumberOfParameters;
//		UDWORD count;
#endif

	if (nlevels == 0)
		return NULL;

	s = (iIMDShape *) malloc(sizeof(iIMDShape));

	if (s) {
		s->points = NULL;
		s->polys = NULL;
		s->connectors = NULL;
		s->texanims = NULL;
		s->next=NULL;

		s->shadowEdgeList = NULL;
		s->nShadowEdges = 0;

// if we can be sure that there is no bsp ... the we check for level number at this point
#ifndef BSPIMD
		if (sscanf(pFileData,"%s %d%n",buffer,&level,&cnt) != 2) {
			debug(LOG_ERROR, "_imd_load_level: file corrupt");
			return NULL;
		}
		pFileData += cnt;

		if (strcmp(buffer,"LEVEL") != 0) {
			debug(LOG_ERROR, "_imd_load_level: excepting 'LEVEL' directive");
			return NULL;
		}
#endif

		s->flags = _IMD_FLAGS;
		s->texpage = texpage;

		if (sscanf(pFileData,"%s %d%n",buffer,&n,&cnt) != 2) {
			debug(LOG_ERROR, "_imd_load_level(2): file corrupt");
			return NULL;
		}
		pFileData += cnt;

		// load texture anims if specified

		// load points

		if (strcmp(buffer,"POINTS") != 0) {
			debug(LOG_ERROR, "_imd_load_level: expecting 'POINTS' directive");
			return NULL;
		}

		if (n>iV_IMD_MAX_POINTS) {
			debug(LOG_ERROR, "_imd_load_level: too many points in IMD");
		}

		s->npoints = n;

//	DBPRINTF(("%s %d , %d \n",buffer,n,s->npoints));

		// Some imd/pie's were greater than the max number of points causing all sorts of memory overflows (blfact2)
		//
		// There was no check / error handling!
		//

		_imd_load_points( &pFileData, s );

		if (sscanf(pFileData,"%s %d%n",buffer,&npolys,&cnt) != 2) {
			debug(LOG_ERROR, "_imd_load_level(3): file corrupt");
			return NULL;
		}
		pFileData += cnt;

		s->npolys=npolys;

		if (strcmp(buffer,"POLYGONS") != 0) {
			debug(LOG_ERROR,"_imd_load_level: expecting 'POLYGONS' directive");
			return NULL;
		}

//			DBPRINTF(("loading polygons - %d\n",s->npolys));
		_imd_load_polys( &pFileData, s );


//NOW load optional stuff
		{
			BOOL OptionalsCompleted;
#ifdef BSPIMD
			s->BSPNode=NULL;	// Zero the bsp node pointer to zero as a default
#endif

			s->nconnectors = 0;	// Default number of connectors must be 0 ( this was'nt being done PBD. )

			OptionalsCompleted=FALSE;

			while(OptionalsCompleted == FALSE) {

//				DBPRINTF(("current file pos = %p (%x)(%x)(%x)  - endoffile = %p\n",*ppFileData,**ppFileData,*((*ppFileData)+1),*((*ppFileData)+2),FileDataEnd));

				// check for end of file (give or take white space)
				if (AtEndOfFile(*&pFileData,FileDataEnd)==TRUE)
				{
					OptionalsCompleted=TRUE;
					break;
				}

				// Scans in the line ... if we don't get 2 parameters then quit
				if (sscanf(pFileData,"%s %d%n",buffer,&n,&cnt) != 2)
				{
					OptionalsCompleted=TRUE;
					break;
				}
                                pFileData += cnt;


				// check for next level ... or might be a BSP      - This should handle an imd if it has a BSP tree attached to it
				// might be "BSP" or "LEVEL"
				if (strcmp(buffer,"LEVEL") == 0)
				{
					iV_DEBUG2("imd[_load_level] = npoints %d, npolys %d\n",s->npoints,s->npolys);
					s->next = _imd_load_level(&pFileData,FileDataEnd,nlevels-1,texpage);
				}
#ifdef BSPIMD
				else if (strcmp(buffer,"BSP") == 0)
				{
					_imd_load_bsp( &pFileData, s, (UWORD)n );
				}
#endif
				else if (strcmp(buffer,"CONNECTORS") == 0)
				{
					//load connector stuff
					s->nconnectors = n;
					_imd_load_connectors( &pFileData, s );
				}
				else
				{
//				DBPRINTF(("1) current file pos = %p (%x)  - endoffile = %p\n",*ppFileData,**ppFileData,FileDataEnd));
					iV_Error(0xff,"(_load_level) unexpected directive %s %d",buffer,&n);
					OptionalsCompleted=TRUE;
					break;
				}

			}
		}
	} else {
		/* Failed to allocate memory for s */
		debug(LOG_ERROR, "_imd_load_level: Memory allocation error");
	}
	*ppFileData = pFileData;
	return s;
}


