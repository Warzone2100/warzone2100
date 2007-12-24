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
 */
#ifndef _pie_internal_h
#define _pie_internal_h

#include "wzglobal.h"
#include "pie_types.h"

#include "imdloader.h"
#include "texture.h"

#include "gui.h"

enum {
	PIE_VERTICE = 0,
	PIE_POLYGON,
	PIE_CONNECTOR,
};

static const uint32_t MAX_TEX_PAGES = 128;
static const uint32_t MAX_FILE_NAME_LENGTH = 128;
static const uint32_t MAX_PIES = 64;
static const uint32_t MAX_ANIM_FRAMES = 128;

static const uint32_t MAX_SHARED_VERTICES = 64;

static const float VERTICE_SELECT_RADIUS = 0.0003f;

///Callback struct
typedef struct _pie_internal_cb {
	void	*pInstance;
	uint32_t	Id;
} PIE_INTERNAL_CB;

typedef struct _vertice_list {
	uint16_t	id;
	bool	selected;
	Vector3f vertice;
	struct _pie_internal_cb callback;
} VERTICE_LIST;

typedef struct _imd_poly_list {
	uint16_t	id;
	bool	selected;
	uint16_t	frame;
	bool	hasVBO;
	uint32_t	VBOId;
	struct iIMDPoly polygon;
	struct _pie_internal_cb callback;
} IMD_POLY_LIST;

typedef struct _shared_vertice {
	uint16_t	numShared;
	uint16_t	shared[MAX_SHARED_VERTICES];
} SHARED_VERTICE;

typedef struct _connector_list {
	uint16_t	id;
	bool	selected;
	Vector3f connector;
	struct _pie_internal_cb callback;
} CONNECTOR_LIST;

typedef struct _pie_frame_info {
	bool	died;
	bool	visible;
	uint32_t	frameId;
	float	offsetX;
	float	offsetY;
	float	offsetZ;
	float	rotateX;
	float	rotateY;
	float	rotateZ;
	float	scaleX;
	float	scaleY;
	float	scaleZ;
	struct _pie_internal_cb callback;
	char	BarName[255];
	TwBar	*Bar;
} PIE_FRAME_INFO;

typedef struct _Interleaved_T2F_V3F {
	Vector2f	t;
	Vector3f	v;
} INTERLEAVED_T2F_V3F;

///Monolithic pie monster O_O
class CPieInternal {
public:
	bool	died;

	// Begin of Frame
	bool	m_IsSub;

	bool	m_Visible;
	bool	m_saveNow;
	bool	m_drawAnim;
	bool	m_controlSubAnim;
	bool	m_framesVisible;

	char	m_Name[255];

	uint16_t	uid;

	float	m_newVerticeX;
	float	m_newVerticeY;
	float	m_newVerticeZ;
	float	m_VelocityX;
	float	m_VelocityY;
	float	m_VelocityZ;
	float	m_duplicateX;
	float	m_duplicateY;
	float	m_duplicateZ;

	int32_t	m_TexpageId;

	///Constructed from a imd
	CPieInternal(uint16_t uid, iIMDShape *imd, const char *name, bool isSub = false, CPieInternal *parent = NULL);

	///Newly generated CPieInteral
	CPieInternal(uint16_t uid, const char *name, int32_t textureId, bool isSub = false, CPieInternal *parent = NULL);

	~CPieInternal(void);

	CPieInternal	*getInstance(void);
	void			setInstance(void);

	///Convert to iIMDShape
	iIMDShape*		ToIMD(void);

	bool			ToFile(const char *filename, bool isOld = false);

	uint16_t			findFreeSlot(uint8_t type);
	bool			isVerticeDuplicated(Vector3f v);

	PIE_INTERNAL_CB	m_InstanceCallback;
	PIE_INTERNAL_CB	m_AddVerticeCB;
	PIE_INTERNAL_CB	m_AddConnectorCB;

	bool			addVertice(uint8_t type);

	bool			removeVerticeAt(uint16_t position);

	bool			removeConnectorAt(uint16_t position);


	void			selectAll(void);
	void			unselectAll(void);

	void			moveSelected(void);
	void			removeSelected(void);
	void			symmetricSelected(uint8_t	Axis);

	bool			duplicateSelected(void);
	bool			duplicatePoly(uint16_t index);

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

	void			buildHLCache(void);
	void			constructSharedVerticeList(void);

	void			bindTexture(int32_t num);

	void			logic(void);

	void			drawInterleaved(INTERLEAVED_T2F_V3F *a_array, uint32_t count);
	void			cacheVBOPoly(uint32_t count, uint32_t polyId);
	void			drawVBOPoly(uint32_t polyId);
	void			flushVBOPolys(void);

	void			draw(void);

	bool			addGUI(void);
	void			updateGUI(void);
	void			hideGUI(void);
	void			showGUI(void);

	bool			addSub(const char *name);
	bool			addSubFromFile(const char *filename);
	bool			addFrame(void);
	bool			removeFrame(uint16_t id);
	bool			duplicateFrame(uint16_t id);

	uint16_t			getVertCount(void) {return(m_vertCount);};
	uint16_t			getConnCount(void) {return(m_connCount);};
	uint16_t			getPolyCount(void) {return(m_polyCount);};

	bool			readAnimFile(const char *filename);
	bool			writeAnimFileTrans(const char *filename);
	bool			writeAnimFileFrames(const char *filename);

	void			startAnim(void);
	void			stopAnim(void);

	enum {
		ANIM3DFRAMES = 0,
		ANIM3DTRANS = 1
	};

	uint32_t			m_animMode;
	uint32_t			m_currAnimFrame;
	uint32_t			m_animStartTime;
	uint32_t			m_numAnimFrames;
	uint32_t			m_frameInterval;
	uint16_t			m_levels;	///<Levels of submodel
	CPieInternal	*m_nextSub;
	
	CPieInternal	*m_parent;	///<Parent of submodel NULL if non-sub
private:
	CPieInternal	*m_instance;
	uint32_t			m_numInstances;

	VERTICE_LIST	*m_Vertices[pie_MAX_VERTICES];
	IMD_POLY_LIST	*m_Polygons[pie_MAX_POLYGONS];
	CONNECTOR_LIST	*m_Connectors[pie_MAX_VERTICES];
	SHARED_VERTICE	m_SharedVertices[pie_MAX_VERTICES];

	uint16_t			m_vertCount;
	uint16_t			m_polyCount;
	uint16_t			m_connCount;
	uint16_t			m_sharedCount;
	Vector3f		m_position;

	Vector3f		m_VerticesHLCache[pie_MAX_VERTICES * 4];
	Vector3f		m_ConnectorsHLCache[pie_MAX_VERTICES * 4];
	uint32_t			m_VerticesHLVBOId;
	uint32_t			m_ConnectorsHLVBOId;

	uint16_t			m_vertHLCount;
	uint16_t			m_connHLCount;

	PIE_FRAME_INFO	m_frames[MAX_ANIM_FRAMES];

	TwBar			*m_toolBar;	///<tool bar
	TwBar			*m_vertBar;	///<vertices bar
	TwBar			*m_polyBars[pie_MAX_POLYGONS];	///<polygons bars
	TwBar			*m_connBar;	///<connectors bar
	TwBar			*m_frameBars[MAX_ANIM_FRAMES];	///<frame animation bars

	friend class CResMaster;
	friend class CPolygonLinker;
};

///Links polygon and shytz YO!
class CPolygonLinker {
public:
	bool	m_Up;

	bool	m_MakePolygon;

	CPolygonLinker() : m_Up(false) {};

	void	flush(void);

	void	setTarget(int32_t target);
	uint16_t	getTarget(void);

	bool	isUp(void);
	bool	isDuplicated(uint16_t vertId);
	bool	canLink(uint16_t vertId);
	bool	Link(uint16_t vertId);

	void	draw(CPieInternal *target);

	void	makePolygon(CPieInternal *target);
private:
	int32_t	m_Target;
	uint8_t	m_LinkedIndex;
	uint16_t	m_LinkedVertices[pie_MAX_VERTICES_PER_POLYGON];

	friend class CResMaster;
};

#endif
