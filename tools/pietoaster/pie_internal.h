/* 
 *  PieToaster is an OpenGL application to edit 3D models in
 *  Warzone 2100's (an RTS game) PIE 3D model format, which is heavily
 *  inspired by PieSlicer created by stratadrake.
 *  Copyright (C) 2007  Carl Hee
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  $Revision$
 *  $Id$
 *  $HeadURL$
 */

#ifndef _pie_internal_h
#define _pie_internal_h

#include "pie_types.h"

#include "imdloader.h"
#include "texture.h"

#include <AntTweakBar.h>

extern TwType g_pieVertexType;
extern TwType g_pieVector2fType;

enum {
	PIE_VERTICE = 0,
	PIE_POLYGON,
	PIE_CONNECTOR,
};

static const Uint32 MAX_TEX_PAGES = 128;
static const Uint32 MAX_FILE_NAME_LENGTH = 128;
static const Uint32 MAX_PIES = 64;

static const Uint32 MAX_SHARED_VERTICES = 64;

static const float VERTICE_SELECT_RADIUS = 0.0005f;

///Callback struct
typedef struct _pie_internal_cb {
	void	*pInstance;
	Uint32	Id;
} PIE_INTERNAL_CB;

typedef struct _vertice_list {
	Uint16	id;
	bool	selected;
	Vector3f vertice;
	struct _pie_internal_cb callback;
} VERTICE_LIST;

typedef struct _imd_poly_list {
	Uint16	id;
	bool	selected;
	struct iIMDPoly polygon;
	struct _pie_internal_cb callback;
} IMD_POLY_LIST;

typedef struct _shared_vertice {
	Uint16	numShared;
	Uint16	shared[MAX_SHARED_VERTICES];
} SHARED_VERTICE;

typedef struct _connector_list {
	Uint16	id;
	bool	selected;
	Vector3f connector;
	struct _pie_internal_cb callback;
} CONNECTOR_LIST;

///Monolithic pie monster O_O
class CPieInternal {
public:
	char	m_Name[255];

	Uint16	uid;

	float	m_newVerticeX;
	float	m_newVerticeY;
	float	m_newVerticeZ;
	float	m_VelocityX;
	float	m_VelocityY;
	float	m_VelocityZ;

	Sint32	m_TexpageId;

	///Constructed from a imd
	CPieInternal(Uint16 uid, iIMDShape *imd, const char *name);

	///Newly generated CPieInteral
	CPieInternal(Uint16 uid, const char *name, Sint32 textureId);

	~CPieInternal(void);

	CPieInternal	*getInstance(void);
	void			setInstance(void);

	///Convert to iIMDShape
	iIMDShape*		ToIMD(void);

	bool			ToFile(const char *filename);

	Uint16			findFreeSlot(Uint8 type);
	bool			isVerticeDuplicated(Vector3f v);

	PIE_INTERNAL_CB	m_InstanceCallback;
	PIE_INTERNAL_CB	m_AddVerticeCB;
	PIE_INTERNAL_CB	m_AddConnectorCB;
	bool			addVertice(Uint8 type);

	bool			removeVerticeAt(Uint16 position);

	bool			removeConnectorAt(Uint16 position);

	void			moveSelected(void);
	void			removeSelected(void);
	void			symmetricSelected(Uint8	Axis);

	void			checkSelection(float x1, float y1, float z1,
									float x2, float y2, float z2);

	VERTICE_LIST	*getVertices(void);
	IMD_POLY_LIST	*getPolys(void);
	CONNECTOR_LIST	*getConnectors(void);
	void			setPosition(float x, float y, float z);

	void			drawNewVertice(void);
	void			highLightVertices(void);
	void			highLightConnectors(void);
	void			highLightSelected(void);

	void			constructSharedVerticeList(void);

	void			logic(void);

	void			bindTexture(Sint32 num);

	void			draw(void);

	bool			addGUI(void);
	void			updateGUI(void);
	void			hideGUI(void);
	void			showGUI(void);

	Uint16			getVertCount(void) {return(m_vertCount);};
	Uint16			getConnCount(void) {return(m_connCount);};
	Uint16			getPolyCount(void) {return(m_polyCount);};
private:
	CPieInternal	*m_instance;
	Uint32			m_numInstances;

	VERTICE_LIST	*m_Vertices[pie_MAX_VERTICES];
	IMD_POLY_LIST	*m_Polygons[pie_MAX_POLYGONS];
	CONNECTOR_LIST	*m_Connectors[pie_MAX_VERTICES];
	SHARED_VERTICE	m_SharedVertices[pie_MAX_VERTICES];

	Uint16			m_vertCount;
	Uint16			m_polyCount;
	Uint16			m_connCount;
	Uint16			m_sharedCount;
	Vector3f		m_position;

	TwBar			*m_toolBar;	///<tool bar
	TwBar			*m_vertBar;	///<vertices bar
	TwBar			*m_polyBars[pie_MAX_POLYGONS];	///<polygons bars
	TwBar			*m_connBar;	///<connectors bar

	friend class CResMaster;
	friend class CPolygonLinker;
};

///Links polygon and shytz YO!
class CPolygonLinker {
public:
	bool	m_Up;

	bool	m_MakePolygon;

	void	flush(void);

	void	setTarget(Sint32 target);
	Uint16	getTarget(void);

	bool	isUp(void);
	bool	isDuplicated(Uint16 vertId);
	bool	canLink(Uint16 vertId);
	bool	Link(Uint16 vertId);

	void	draw(CPieInternal *target);

	void	makePolygon(CPieInternal *target);
private:
	Sint32	m_Target;
	Uint8	m_LinkedIndex;
	Uint16	m_LinkedVertices[pie_MAX_VERTICES_PER_POLYGON];

	friend class CResMaster;
};

#endif
