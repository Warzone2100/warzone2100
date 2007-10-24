#include "pie_internal.h"

#ifdef _WIN32
	#include <windows.h>	// required by gl.h
#endif
#include <GL/gl.h>
#include <GL/glu.h>

#include "screen.h"
#include "game_io.h"
#include "config.h"

TwType g_pieVertexType;
TwType g_pieVector2fType;

//// Callbacks ////

//clientData as struct(includes instance of object and int info)
void TW_CALL addVerticeCB(void *clientData) {
	PIE_INTERNAL_CB	*pie_internal_cb = (PIE_INTERNAL_CB*)clientData;
	CPieInternal	*instance = reinterpret_cast<CPieInternal*>(pie_internal_cb->pInstance);
	Uint8	i = pie_internal_cb->Id;

	if (!instance->addVertice(i))
	{
		fprintf(stderr, "adding vertice type %d failed\n", i);
	}
	instance->updateGUI();
}

//clientData as struct(includes instance of object and int info)
void TW_CALL removeSelectedCB(void *clientData) {
	PIE_INTERNAL_CB	*pie_internal_cb = (PIE_INTERNAL_CB*)clientData;
	CPieInternal	*instance = reinterpret_cast<CPieInternal*>(pie_internal_cb->pInstance);

	instance->removeSelected();
	instance->updateGUI();
}

//clientData as struct(includes instance of object and int info)
void TW_CALL removeVerticeAtCB(void *clientData) {
	PIE_INTERNAL_CB	*pie_internal_cb = (PIE_INTERNAL_CB*)clientData;
	CPieInternal	*instance = reinterpret_cast<CPieInternal*>(pie_internal_cb->pInstance);
	Uint8	i = pie_internal_cb->Id;

	if (!instance->removeVerticeAt(i))
	{
		fprintf(stderr, "remove vertice at %d failed\n", i);
	}
}

//clientData as struct(includes instance of object and int info)
void TW_CALL removeConnectorAtCB(void *clientData) {
	PIE_INTERNAL_CB	*pie_internal_cb = (PIE_INTERNAL_CB*)clientData;
	CPieInternal	*instance = reinterpret_cast<CPieInternal*>(pie_internal_cb->pInstance);
	Uint8	i = pie_internal_cb->Id;

	if (!instance->removeConnectorAt(i))
	{
		fprintf(stderr, "remove connector at %d failed\n", i);
	}
	instance->updateGUI();
}

//clientData as struct(includes instance of object and int info)
void TW_CALL moveSelectedVertices(void *clientData) {
	PIE_INTERNAL_CB	*pie_internal_cb = (PIE_INTERNAL_CB*)clientData;
	CPieInternal	*instance = reinterpret_cast<CPieInternal*>(pie_internal_cb->pInstance);

	instance->moveSelected();
}

//clientData as struct(includes instance of object and int info)
void TW_CALL removeSelectedVertices(void *clientData) {
	PIE_INTERNAL_CB	*pie_internal_cb = (PIE_INTERNAL_CB*)clientData;
	CPieInternal	*instance = reinterpret_cast<CPieInternal*>(pie_internal_cb->pInstance);

	instance->removeSelected();
	instance->updateGUI();
}

//clientData as struct(includes instance of object and int info)
void TW_CALL symmetricSelectedVertices(void *clientData) {
	PIE_INTERNAL_CB	*pie_internal_cb = (PIE_INTERNAL_CB*)clientData;
	CPieInternal	*instance = reinterpret_cast<CPieInternal*>(pie_internal_cb->pInstance);
	Uint8	axis = pie_internal_cb->Id;

	instance->symmetricSelected(axis);
	instance->updateGUI();
}

//clientData as struct(includes instance of object and int info)
void TW_CALL hideGUICB(void *clientData) {
	PIE_INTERNAL_CB	*pie_internal_cb = (PIE_INTERNAL_CB*)clientData;
	CPieInternal	*instance = reinterpret_cast<CPieInternal*>(pie_internal_cb->pInstance);

	instance->hideGUI();
}

//clientData as struct(includes instance of object and int info)
void TW_CALL showGUICB(void *clientData) {
	PIE_INTERNAL_CB	*pie_internal_cb = (PIE_INTERNAL_CB*)clientData;
	CPieInternal	*instance = reinterpret_cast<CPieInternal*>(pie_internal_cb->pInstance);

	instance->showGUI();
}

//// End of Callbacks ////

void	CPolygonLinker::flush(void) {
	m_LinkedIndex = 0;
	m_Target = 0;
	m_MakePolygon = false;
}

void	CPolygonLinker::setTarget(Sint32 target) {
	m_Target = target;
}

Uint16	CPolygonLinker::getTarget(void) {
	return m_Target;
}

bool	CPolygonLinker::isUp(void) {
	return (m_Up);
}

bool	CPolygonLinker::isDuplicated(Uint16 vertId) {
	Uint8 i = 0;

	if (!m_LinkedIndex)
	{
		//not linked vertices yet
		return false;
	}

	for (i = 0;i < m_LinkedIndex + 1;i++)
	{
		if (m_LinkedVertices[i] == vertId)
		{
			return true;
		}
	}
	return false;
}

bool	CPolygonLinker::canLink(Uint16 vertId) {
	//Skips self-link(dot) and line link(2 points line)
	if (isDuplicated(vertId) && m_LinkedIndex < 3)
	{
		return false;
	}

	return true;
}

bool	CPolygonLinker::Link(Uint16 vertId) {
	if(!canLink(vertId))
	{
		return false;
	}

	if (isDuplicated(vertId))
	{
		m_MakePolygon = true;
		return true;
	}

	m_LinkedVertices[m_LinkedIndex] = vertId;

	m_LinkedIndex++;

	return true;
}

void	CPolygonLinker::draw(CPieInternal *target) {
	Uint16		i;
	Uint8		color[4] = {128, 1, 1, 255};

	for (i = 0;i < m_LinkedIndex;i++)
	{
		if ((i + 1) >= m_LinkedIndex)
		{
			break;
		}
		glColor4ub(color[0], color[1]+i*20, color[2], color[3]);
		glBegin(GL_LINES);
		glVertex3f(target->m_Vertices[m_LinkedVertices[i]]->vertice.x,
					target->m_Vertices[m_LinkedVertices[i]]->vertice.y,
					target->m_Vertices[m_LinkedVertices[i]]->vertice.z);
		glVertex3f(target->m_Vertices[m_LinkedVertices[i+1]]->vertice.x,
					target->m_Vertices[m_LinkedVertices[i+1]]->vertice.y,
					target->m_Vertices[m_LinkedVertices[i+1]]->vertice.z);
		glEnd();
	}

	glColor4ub(255, 255, 255, 255);
}

void	CPolygonLinker::makePolygon(CPieInternal *target) {
	Uint8		i;
	Uint16		polyIndex;

	polyIndex = target->findFreeSlot(1);

	target->m_Polygons[polyIndex] = (IMD_POLY_LIST*)malloc(sizeof(IMD_POLY_LIST));

	memset(target->m_Polygons[polyIndex], 0, sizeof(IMD_POLY_LIST));

	target->m_Polygons[polyIndex]->polygon.pTexAnim = NULL;
	target->m_Polygons[polyIndex]->polygon.pindex = (VERTEXID*)malloc(sizeof(VERTEXID) * pie_MAX_VERTICES_PER_POLYGON);
	target->m_Polygons[polyIndex]->polygon.texCoord = (Vector2f*)malloc(sizeof(Vector2f) * pie_MAX_VERTICES_PER_POLYGON);
	memset(target->m_Polygons[polyIndex]->polygon.texCoord, 0, sizeof(Vector2f) * pie_MAX_VERTICES_PER_POLYGON);
	target->m_Polygons[polyIndex]->polygon.npnts = m_LinkedIndex;

	for (i = 0;i < m_LinkedIndex;i++)
	{
		target->m_Polygons[polyIndex]->polygon.pindex[i] = (VERTEXID)m_LinkedVertices[i];
	}

	if (polyIndex == target->m_polyCount)
	{
		target->m_polyCount++;
	}

	m_MakePolygon = false;
	m_LinkedIndex = 0;
}

//the in game pie_Draw3DShape2 function for reference's sake
#if 0
static void pie_Draw3DShape2(iIMDShape *shape, int frame, PIELIGHT colour, PIELIGHT specular, int pieFlag, int pieFlagData)
{
	unsigned int n;
	Vector3f *pVertices, *pPixels, scrPoints[pie_MAX_VERTICES];
	iIMDPoly *pPolys;
	PIEPOLY piePoly;
	VERTEXID *index;
	bool light = lighting;

	/* Set tranlucency */
	if (pieFlag & pie_ADDITIVE)
	{ //Assume also translucent
		//pie_SetRendMode(REND_ADDITIVE_TEX);
		colour.byte.a = (UBYTE)pieFlagData;
		//pie_SetBilinear(true);
		light = false;
	}
	else if (pieFlag & pie_TRANSLUCENT)
	{
		//pie_SetRendMode(REND_ALPHA_TEX);
		colour.byte.a = (UBYTE)pieFlagData;
		//pie_SetBilinear(false);//never bilinear with constant alpha, gives black edges
		light = false;
	}
	else
	{
		//pie_SetRendMode(REND_GOURAUD_TEX);
		//if hardware fog then alpha is set else unused in decal mode
		if (pieFlag & pie_NO_BILINEAR)
		{
			//pie_SetBilinear(false);
		}
		else
		{
			//pie_SetBilinear(true);
		}
	}

	pie_SetTexturePage(shape->texpage);

	//now draw the shape
	//rotate and project points from shape->points to scrPoints
	for (pVertices = shape->points, pPixels = scrPoints;
			pVertices < shape->points + shape->npoints;
			pVertices++, pPixels++)
	{
		pPixels->x = pVertices->x;
		pPixels->y = pVertices->y;
		pPixels->z = pVertices->z;
	}

	for (pPolys = shape->polys;
			pPolys < shape->polys + shape->npolys;
			pPolys++)
	{
		piePoly.flags = pPolys->flags;

		if (pieFlag & pie_TRANSLUCENT)
		{
			/* There are no PIE files with PIE_ALPHA set, this is the only user, and
			 * this flag is never checked anywhere, except we check below that _some_
			 * flag is set. This is weird. FIXME. - Per */
			piePoly.flags |= PIE_ALPHA;
		}
		else if (pieFlag & pie_ADDITIVE)
		{
			piePoly.flags &= (0xffffffff-PIE_COLOURKEYED);//dont treat additive images as colour keyed
		}

		for (n = 0, index = pPolys->pindex;
				n < pPolys->npnts;
				n++, index++)
		{
			pieVrts[n].x = scrPoints[*index].x;
			pieVrts[n].y = scrPoints[*index].y;
			pieVrts[n].z = scrPoints[*index].z;
			pieVrts[n].u = pPolys->texCoord[n].x;
			pieVrts[n].v = pPolys->texCoord[n].y;
			pieVrts[n].light.argb = colour.argb;
			pieVrts[n].specular.argb = specular.argb;
		}
		piePoly.nVrts = pPolys->npnts;
		piePoly.pVrts = pieVrts;

		piePoly.pTexAnim = pPolys->pTexAnim;

		if (piePoly.flags > 0)
		{
			pie_PiePolyFrame(&piePoly, frame, light); // draw the polygon ...
		}
	}

	if (pieFlag & pie_BUTTON)
	{
		pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);
	}
}
#endif


///Constructed from a imd
CPieInternal::CPieInternal(Uint16 uid, iIMDShape *imd, const char *name) {
	Uint32 i, j, vertIndex = 0;

	strncpy(&m_Name[0], name, 255);

	m_polyCount = m_vertCount = m_connCount = 0;
	m_newVerticeX = m_newVerticeY = m_newVerticeZ = 0.0f;
	m_VelocityX = m_VelocityY = m_VelocityZ = 0.0f;

	this->uid = uid;
	this->setInstance();
	this->m_InstanceCallback.pInstance = (void*)this;
	this->m_InstanceCallback.Id = 0;

	this->m_AddVerticeCB.pInstance = (void*)this;
	this->m_AddVerticeCB.Id = PIE_VERTICE;

	this->m_AddConnectorCB.pInstance = (void*)this;
	this->m_AddConnectorCB.Id = PIE_CONNECTOR;

	this->m_numInstances = 0;

	this->m_TexpageId = imd->texpage;

	for (i = 0;i < imd->npolys;i++)
	{
		m_Polygons[i] = (IMD_POLY_LIST*)malloc(sizeof(IMD_POLY_LIST));
		memset(&m_Polygons[i]->polygon, 0, sizeof(iIMDPoly));

		if (imd->polys[i].pTexAnim != NULL)
		{
			m_Polygons[i]->polygon.pTexAnim = (iTexAnim*)malloc(sizeof(iTexAnim));
			memcpy(m_Polygons[i]->polygon.pTexAnim, imd->polys[i].pTexAnim, sizeof(iTexAnim));
		}
		else
		{
			m_Polygons[i]->polygon.pTexAnim = NULL;
		}

		m_Polygons[i]->polygon.texCoord = (Vector2f*)malloc(sizeof(Vector2f) * pie_MAX_VERTICES_PER_POLYGON);

		m_Polygons[i]->polygon.pindex = (VERTEXID*)malloc(sizeof(VERTEXID) * pie_MAX_VERTICES_PER_POLYGON);

		for (j = 0;
			j < imd->polys[i].npnts;
			j++)
		{
			memcpy(&m_Polygons[i]->polygon.pindex[j], &imd->polys[i].pindex[j], sizeof(VERTEXID));
			m_Polygons[i]->polygon.texCoord[j].x = imd->polys[i].texCoord[j].x;
			m_Polygons[i]->polygon.texCoord[j].y = imd->polys[i].texCoord[j].y;
		}

		m_Polygons[i]->polygon.normal = imd->polys[i].normal;
		m_Polygons[i]->polygon.npnts = imd->polys[i].npnts;
		m_Polygons[i]->polygon.zcentre = imd->polys[i].zcentre;
		m_Polygons[i]->polygon.flags = imd->polys[i].flags;
		m_Polygons[i]->id = i;
		m_Polygons[i]->selected = false;

		m_polyCount++;
	}

	for (i = 0;i < imd->npoints;i++)
	{
		m_Vertices[vertIndex] = (VERTICE_LIST*)malloc(sizeof(VERTICE_LIST));
		m_Vertices[vertIndex]->vertice.x = imd->points[vertIndex].x;
		m_Vertices[vertIndex]->vertice.y = imd->points[vertIndex].y;
		m_Vertices[vertIndex]->vertice.z = imd->points[vertIndex].z;
		m_Vertices[vertIndex]->id = vertIndex;
		m_Vertices[vertIndex]->selected = false;
		m_vertCount++;
		vertIndex++;
	}

	for (i = 0;i < imd->nconnectors;i++)
	{
		m_Connectors[i] = (CONNECTOR_LIST*)malloc(sizeof(CONNECTOR_LIST));
		m_Connectors[i]->connector.x = imd->connectors[i].x;
		m_Connectors[i]->connector.y = imd->connectors[i].y;
		m_Connectors[i]->connector.z = imd->connectors[i].z;
		m_Connectors[i]->id = m_connCount;
		m_Connectors[i]->selected = false;
		m_connCount++;
	}

	this->constructSharedVerticeList();

	this->addGUI();
}

///Newly generated CPieInternal
CPieInternal::CPieInternal(Uint16 uid, const char *name, Sint32 textureId) {
	m_polyCount = m_vertCount = m_connCount = 0;
	m_newVerticeX = m_newVerticeY = m_newVerticeZ = 0.0f;
	m_VelocityX = m_VelocityY = m_VelocityZ = 0.0f;

	this->uid = uid;
	this->setInstance();
	this->m_InstanceCallback.pInstance = (void*)this;
	this->m_InstanceCallback.Id = 0;
	this->m_numInstances = 0;
	this->m_TexpageId = textureId;

	// Sets m_Name's length to 0
	m_Name[0] = '\0';
	strncpy(m_Name, name, 255);

	this->addGUI();
}

///Constructed from a imd
CPieInternal::~CPieInternal() {
	Uint32 i;

	for (i = 0;i < m_polyCount;i++)
	{
		if (m_Polygons[i])
		{
			if (!TwDeleteBar(m_polyBars[i]))
			{
				fprintf(stderr, TwGetLastError());
			}
			m_polyBars[i] = NULL;

			if (m_Polygons[i]->polygon.pTexAnim != NULL)
			{
				free(m_Polygons[i]->polygon.pTexAnim);
				m_Polygons[i]->polygon.pTexAnim = NULL;

			}

			if (m_Polygons[i]->polygon.texCoord != NULL)
			{
				free(m_Polygons[i]->polygon.texCoord);
				m_Polygons[i]->polygon.texCoord = NULL;

			}

			if (m_Polygons[i]->polygon.pindex != NULL)
			{
				free(m_Polygons[i]->polygon.pindex);
				m_Polygons[i]->polygon.pindex = NULL;

			}

			free(m_Polygons[i]);
			m_Polygons[i] = NULL;
		}
	}

	if (!TwDeleteBar(m_toolBar))
	{
		fprintf(stderr, TwGetLastError());
	}
	//TwDeleteBar(m_vertBar);
	m_toolBar = NULL;
	if (!TwDeleteBar(m_vertBar))
	{
		fprintf(stderr, TwGetLastError());
	}
	//TwDeleteBar(m_vertBar);
	m_vertBar = NULL;
	if (!TwDeleteBar(m_connBar))
	{
		fprintf(stderr, TwGetLastError());
	}
	//TwDeleteBar(m_connBar);
	m_connBar = NULL;

	for (i = 0;i < m_vertCount;i++)
	{
		if (m_Vertices[i])
		{
			free(m_Vertices[i]);
			m_Vertices[i] = NULL;
		}
	}

	for (i = 0;i < m_connCount;i++)
	{
		if (m_Connectors[i])
		{
			free(m_Connectors[i]);
			m_Connectors[i] = NULL;
		}
	}
}

CPieInternal*	CPieInternal::getInstance(void) {
	return m_instance;
}

void	CPieInternal::setInstance(void) {
	m_instance = this;
}

///Convert to iIMDShape
iIMDShape* CPieInternal::ToIMD(void) {
	iIMDShape *newIMD = NULL;
	Uint32 i, vertIndex = 0, newPolyIndex, newVertIndex, newConnIndex;

	newIMD = (iIMDShape*)malloc(sizeof(iIMDShape));

	memset(newIMD, 0, sizeof(iIMDShape));

	newIMD->texpage = m_TexpageId;

	newIMD->npolys = 0;

	newIMD->polys = (iIMDPoly*)malloc(sizeof(iIMDPoly) * pie_MAX_POLYGONS);
	newIMD->points = (Vector3f*)malloc(sizeof(Vector3f) * pie_MAX_VERTICES);
	newIMD->connectors = (Vector3f*)malloc(sizeof(Vector3f) * pie_MAX_VERTICES);
	newIMD->npoints = 0;

	newPolyIndex = 0;
	for (i = 0;i < m_polyCount;i++)
	{
		if (m_Polygons[i] == NULL)
		{
			continue;
		}

		memcpy(&newIMD->polys[newPolyIndex], &m_Polygons[i]->polygon, sizeof(iIMDPoly));
		//newIMD->polys[i] = m_Polygons[i]->polygon;
		newIMD->npolys++;
		newPolyIndex++;

		/*
		newVertIndex = 0;
		for (j = 0;j < m_Polygons[i]->polygon.npnts;j++)
		{
			if (m_Vertices[vertIndex] == NULL)
			{
				fprintf(stderr, "WARNING:NULL vertice at index %d\n", vertIndex);
				vertIndex++;
				continue;
			}
			newIMD->points[vertIndex].x = m_Vertices[j]->vertice.x;
			newIMD->points[vertIndex].y = m_Vertices[j]->vertice.y;
			newIMD->points[vertIndex].z = m_Vertices[j]->vertice.z;
			newIMD->npoints++;

			vertIndex++;
		}
		*/
	}


	newVertIndex = 0;
	for (i = 0;i < m_vertCount;i++)
	{
		if (m_Vertices[vertIndex] == NULL)
		{
			fprintf(stderr, "WARNING:NULL vertice at index %d\n", vertIndex);
			vertIndex++;
			continue;
		}
		newIMD->points[newVertIndex].x = m_Vertices[i]->vertice.x;
		newIMD->points[newVertIndex].y = m_Vertices[i]->vertice.y;
		newIMD->points[newVertIndex].z = m_Vertices[i]->vertice.z;

		newVertIndex++;
		newIMD->npoints++;
		vertIndex++;
	}

	newConnIndex = 0;
	for (i = 0;i < m_connCount;i++)
	{
		if (m_Connectors[i] == NULL)
		{
			continue;
		}

		newIMD->connectors[newConnIndex].x = m_Connectors[i]->connector.x;
		newIMD->connectors[newConnIndex].y = m_Connectors[i]->connector.y;
		newIMD->connectors[newConnIndex].z = m_Connectors[i]->connector.z;
		newIMD->nconnectors++;
		newConnIndex++;
	}

	return newIMD;
}

bool	CPieInternal::ToFile(const char *filename) {
	FILE *file;
	iIMDShape	*tmpIMD;

	file = fopen(filename, "w+");
	if (file == NULL)
	{
		fprintf(stderr, "error when creating file %s\n", filename);
		return false;
	}

	tmpIMD = this->ToIMD();

	if (tmpIMD == NULL)
	{
		fprintf(stderr, "null imd when trying to save to file %s", filename);
		return false;
	}

	iV_IMDSave(filename, tmpIMD, true);

	free(tmpIMD);

	return true;
}

bool	CPieInternal::isVerticeDuplicated(Vector3f v) {
	Uint16	i;

	for (i = 0;i < m_vertCount;i++)
	{
		if (v.x == m_Vertices[i]->vertice.x &&
			v.y == m_Vertices[i]->vertice.y &&
			v.z == m_Vertices[i]->vertice.z)
		{
			return true;
		}
	}
	return false;
}

Uint16	CPieInternal::findFreeSlot(Uint8 type) {
	Uint16 i;

	switch(type)
	{
		case PIE_VERTICE:
			for (i = 0;i < m_vertCount;i++)
			{
				if (m_Vertices[i] == NULL)
				{
					return i;
				}
			}
			return m_vertCount;
		case PIE_POLYGON:
			for (i = 0;i < m_polyCount;i++)
			{
				if (m_Polygons[i] == NULL)
				{
					return i;
				}
			}
			return m_polyCount;
		case PIE_CONNECTOR:
			for (i = 0;i < m_connCount;i++)
			{
				if (m_Connectors[i] == NULL)
				{
					return i;
				}
			}
			return m_vertCount;
			return m_connCount;
		default:
			fprintf(stderr, "Unknown type when finding free slot\n");
			return false;
	}
}

bool	CPieInternal::addVertice(Uint8 type) {
	Uint16	slot;

	slot = this->findFreeSlot(type);

	if (slot == 0xFFFF)
	{
		return false;
	}

	switch(type)
	{
		case PIE_VERTICE:
			m_Vertices[slot] = (VERTICE_LIST*)malloc(sizeof(VERTICE_LIST));

			m_Vertices[slot]->vertice.x = m_newVerticeX;
			m_Vertices[slot]->vertice.y = m_newVerticeY;
			m_Vertices[slot]->vertice.z = m_newVerticeZ;

			m_Vertices[slot]->selected = false;

			m_Vertices[slot]->callback.pInstance = this->getInstance();

			if (slot == m_vertCount)
			{
				m_vertCount++;
			}

			// Re-construct the shared vertice list
			this->constructSharedVerticeList();

			return true;
		case PIE_CONNECTOR:
			m_Connectors[slot] = (CONNECTOR_LIST*)malloc(sizeof(CONNECTOR_LIST));

			m_Connectors[slot]->connector.x = m_newVerticeX;
			m_Connectors[slot]->connector.y = m_newVerticeY;
			m_Connectors[slot]->connector.z = m_newVerticeZ;

			m_Connectors[slot]->selected = false;

			if (slot == m_connCount)
			{
				m_connCount++;
			}

			return true;
		default:
			fprintf(stderr, "Unknown type when adding vertice\n");
			return false;
	}
}

bool	CPieInternal::removeVerticeAt(Uint16 position) {
	Uint16		i, j;
	VERTEXID	*vertexId;

	for (i = 0;i < m_polyCount;i++)
	{
		if (m_Polygons[i])
		{
			vertexId = m_Polygons[i]->polygon.pindex;
			for (j = 0;j < m_Polygons[i]->polygon.npnts;j++,vertexId++)
			{
				if (*vertexId == (VERTEXID)position)
				{
					if (m_Polygons[i]->polygon.pTexAnim != NULL)
					{
						free(m_Polygons[i]->polygon.pTexAnim);
						m_Polygons[i]->polygon.pTexAnim = NULL;
					}

					if (m_Polygons[i]->polygon.texCoord != NULL)
					{
						free(m_Polygons[i]->polygon.texCoord);
						m_Polygons[i]->polygon.texCoord = NULL;
					}

					if (m_Polygons[i]->polygon.pindex != NULL)
					{
						free(m_Polygons[i]->polygon.pindex);
						m_Polygons[i]->polygon.pindex = NULL;
					}

					free(m_Polygons[i]);
					m_Polygons[i] = NULL;
					break;
				}
			}
		}
	}

	if (m_Vertices[position])
	{
		free(m_Vertices[position]);
		m_Vertices[position] = NULL;
		m_SharedVertices[position].numShared = 0;
		return true;
	}

	fprintf(stderr, "vertice in position %d doesnt exist\n", position);
	return false;
}

bool	CPieInternal::removeConnectorAt(Uint16 position) {
	if (m_Connectors[position])
	{
		free(m_Connectors[position]);
		m_Connectors[position] = NULL;
		return true;
	}

	fprintf(stderr, "connector in position %d doesnt exist\n", position);
	return false;
}

void	CPieInternal::moveSelected(void) {
	Uint16	i;

	for (i = 0;i < m_vertCount;i++)
	{
		if (m_Vertices[i] && m_Vertices[i]->selected)
		{
			m_Vertices[i]->vertice.x += m_VelocityX;
			m_Vertices[i]->vertice.y += m_VelocityY;
			m_Vertices[i]->vertice.z += m_VelocityZ;
		}
	}
}

///Remove selected vertices and connectors
void	CPieInternal::removeSelected(void) {
	Uint16	i;
	for (i = 0;i < m_vertCount;i++)
	{
		if (m_Vertices[i] && m_Vertices[i]->selected)
		{
			this->removeVerticeAt(i);
		}
	}

	for (i = 0;i < m_connCount;i++)
	{
		if (m_Connectors[i] && m_Connectors[i]->selected)
		{
			this->removeConnectorAt(i);
		}
	}
}

//TODO:finish this
void	CPieInternal::symmetricSelected(Uint8	Axis) {

}

void	CPieInternal::checkSelection(float x1, float y1, float z1,
									 float x2, float y2, float z2) {
	Uint16	i, chosen;
	float	mag, minDist;
	float	xdiff = x2 - x1, ydiff = y2 - y1, zdiff = z2 - z1;
	Vector3f	slope = {xdiff, ydiff, zdiff};

	mag = sqrtf(slope.x*slope.x + slope.y *slope.y + slope.z*slope.z);

	slope.x /= mag;
	slope.y /= mag;
	slope.z /= mag;

	minDist = 0.0f;
	chosen = 0xFFFF;
	for (i = 0;i < m_vertCount;i++)
	{
		if (m_Vertices[i])
		{
			//Begin of Ray Test
			xdiff =	m_Vertices[i]->vertice.x - x1;
			ydiff =	m_Vertices[i]->vertice.y - y1;
			zdiff =	m_Vertices[i]->vertice.z - z1;
			Vector3f	selectSlope = {xdiff, ydiff, zdiff};
			mag = sqrtf(selectSlope.x*selectSlope.x + selectSlope.y *selectSlope.y + selectSlope.z*selectSlope.z);

			selectSlope.x /= mag;
			selectSlope.y /= mag;
			selectSlope.z /= mag;

			xdiff = slope.x - selectSlope.x;
			ydiff = slope.y - selectSlope.y;
			zdiff = slope.z - selectSlope.z;

			float dist = xdiff*xdiff + ydiff*ydiff + zdiff*zdiff;

			if (dist <= VERTICE_SELECT_RADIUS)
			{
				//TODO:fix broken cycling through shared vertice code
#if 0
				if (isMouseButtonDoubleDown(1))
				{
					if (m_SharedVertices[i].numShared)
					{
						Uint16	index, newIndex;

						for (index = 0;index < m_SharedVertices[i].numShared;index++)
						{
							if (m_Vertices[m_SharedVertices[i].shared[index]]->selected)
							{
								newIndex = index + 1;
								if (newIndex >= m_SharedVertices[i].numShared)
								{
									newIndex = 0;
								}
								//assert(m_Vertices[m_SharedVertices[i].shared[newIndex]] != NULL);
								m_Vertices[m_SharedVertices[i].shared[index]]->selected = false;
								m_Vertices[m_SharedVertices[i].shared[newIndex]]->selected = true;
							}
						}
						return;
					}
					else
					{
						if (m_Vertices[i]->selected)
						{
							m_Vertices[i]->selected = false;
						}
						else
						{
							m_Vertices[i]->selected = true;
						}
						return;
					}
				}
				else
				{
#endif


				if (minDist <= 0.0f)
				{
					minDist = dist;
					chosen = i;
				}
				else
				{
					if (dist < minDist)
					{
						minDist = dist;
						chosen = i;
					}
					minDist = dist;
				}
			}
		}
	}

	if (chosen != 0xFFFF)
	{
		if (m_Vertices[chosen]->selected)
		{
			m_Vertices[chosen]->selected = false;
		}
		else
		{
			m_Vertices[chosen]->selected = true;
		}
		return;
	}

	minDist = 0.0f;
	chosen = 0xFFFF;
	for (i = 0;i < m_connCount;i++)
	{
		if (m_Connectors[i])
		{
			//Begin of Ray Test
			xdiff =	m_Connectors[i]->connector.x - x1;
			ydiff =	m_Connectors[i]->connector.z - y1;
			zdiff =	m_Connectors[i]->connector.y - z1;
			Vector3f	selectSlope = {xdiff, ydiff, zdiff};
			mag = sqrtf(selectSlope.x*selectSlope.x + selectSlope.y *selectSlope.y + selectSlope.z*selectSlope.z);

			selectSlope.x /= mag;
			selectSlope.y /= mag;
			selectSlope.z /= mag;

			xdiff = slope.x - selectSlope.x;
			ydiff = slope.y - selectSlope.y;
			zdiff = slope.z - selectSlope.z;

			float dist = xdiff*xdiff + ydiff*ydiff + zdiff*zdiff;

			if (dist <= VERTICE_SELECT_RADIUS)
			{
				if (minDist <= 0.0f)
				{
					minDist = dist;
					chosen = i;
				}
				else
				{
					if (dist < minDist)
					{
						minDist = dist;
						chosen = i;
					}
					minDist = dist;
				}
			}
		}
	}

	if (chosen != 0xFFFF)
	{
		if (m_Connectors[chosen]->selected)
		{
			m_Connectors[chosen]->selected = false;
		}
		else
		{
			m_Connectors[chosen]->selected = true;
		}
	}
}

VERTICE_LIST*	CPieInternal::getVertices(void) {
	return m_Vertices[0];
}

IMD_POLY_LIST*	CPieInternal::getPolys(void) {
	return m_Polygons[0];
}

CONNECTOR_LIST*	CPieInternal::getConnectors(void) {
	return m_Connectors[0];
}

void	CPieInternal::setPosition(float x, float y, float z) {
	m_position.x = x;
	m_position.y = y;
	m_position.z = z;
}

void	CPieInternal::constructSharedVerticeList(void) {
	Uint16	i, count, count2;

	count = count2 = 0;
	for (i = 0;i < m_vertCount;i++)
	{
		m_SharedVertices[i].shared[0] = i;
		m_SharedVertices[i].numShared = 1;

		count2 = count;
		while (count2)
		{
			if (m_Vertices[count2 - 1]->vertice.x == m_Vertices[i]->vertice.x &&
				m_Vertices[count2 - 1]->vertice.y == m_Vertices[i]->vertice.y &&
				m_Vertices[count2 - 1]->vertice.z == m_Vertices[i]->vertice.z)
			{
				m_SharedVertices[i].shared[m_SharedVertices[i].numShared] = count2 - 1;
				m_SharedVertices[i].numShared++;
				// duplicate the shared info
				memcpy(&m_SharedVertices[count2 - 1], &m_SharedVertices[i], sizeof(SHARED_VERTICE));
			}
			count2--;
		}
		count++;
	}
}

void	CPieInternal::drawNewVertice(void) {
	glDisable(GL_DEPTH_TEST);

	glColor4ub(255, 0, 0, 192);
	glBegin(GL_LINES);

	glVertex3f(m_newVerticeX + 0.1f, m_newVerticeY, m_newVerticeZ);
	glVertex3f(m_newVerticeX + 2.0f, m_newVerticeY, m_newVerticeZ);

	glVertex3f(m_newVerticeX, m_newVerticeY, m_newVerticeZ + 0.1f);
	glVertex3f(m_newVerticeX, m_newVerticeY, m_newVerticeZ + 2.0f);

	glVertex3f(m_newVerticeX - 0.1f, m_newVerticeY, m_newVerticeZ);
	glVertex3f(m_newVerticeX - 2.0f, m_newVerticeY, m_newVerticeZ);

	glVertex3f(m_newVerticeX, m_newVerticeY, m_newVerticeZ - 0.1f);
	glVertex3f(m_newVerticeX, m_newVerticeY, m_newVerticeZ - 2.0f);

	glVertex3f(m_newVerticeX, m_newVerticeY + 0.1f, m_newVerticeZ);
	glVertex3f(m_newVerticeX, m_newVerticeY + 2.0f, m_newVerticeZ);

	glVertex3f(m_newVerticeX, m_newVerticeY - 0.1f, m_newVerticeZ);
	glVertex3f(m_newVerticeX, m_newVerticeY - 2.0f, m_newVerticeZ);

	glEnd();

	glColor4ub(255, 255, 255, 255);

	glEnable(GL_DEPTH_TEST);
}

void	CPieInternal::highLightVertices(void) {
	Uint16	i;

	glDisable(GL_DEPTH_TEST);

	glColor4ub(128, 0, 0, 255);
	for (i = 0;i < m_vertCount;i++)
	{
		if (m_Vertices[i])
		{
			glBegin(GL_QUADS);
			glVertex3f(m_Vertices[i]->vertice.x, m_Vertices[i]->vertice.y + 1.0f, m_Vertices[i]->vertice.z);
			glVertex3f(m_Vertices[i]->vertice.x + 1.0f, m_Vertices[i]->vertice.y, m_Vertices[i]->vertice.z);
			glVertex3f(m_Vertices[i]->vertice.x, m_Vertices[i]->vertice.y - 1.0f , m_Vertices[i]->vertice.z);
			glVertex3f(m_Vertices[i]->vertice.x - 1.0f, m_Vertices[i]->vertice.y, m_Vertices[i]->vertice.z);
			glEnd();
		}
	}

	glColor4ub(255, 255, 255, 255);

	glEnable(GL_DEPTH_TEST);
}

void	CPieInternal::highLightConnectors(void) {
	Uint16	i;

	glDisable(GL_DEPTH_TEST);

	glColor4ub(0, 128, 0, 255);
	for (i = 0;i < m_connCount;i++)
	{
		if (m_Connectors[i])
		{
			glBegin(GL_TRIANGLE_FAN);
			glVertex3f(m_Connectors[i]->connector.x, m_Connectors[i]->connector.z + 1.0f, m_Connectors[i]->connector.y);
			glVertex3f(m_Connectors[i]->connector.x + 1.0f, m_Connectors[i]->connector.z - 1.0f, m_Connectors[i]->connector.y);
			glVertex3f(m_Connectors[i]->connector.x - 1.0f, m_Connectors[i]->connector.z - 1.0f , m_Connectors[i]->connector.y);
			glEnd();
		}
	}

	glColor4ub(255, 255, 255, 255);

	glEnable(GL_DEPTH_TEST);
}

void	CPieInternal::highLightSelected(void) {
	Uint16	i;

	glDisable(GL_DEPTH_TEST);

	glColor4ub(255, 255, 0, 255);

	for (i = 0;i < m_vertCount;i++)
	{
		if (m_Vertices[i] && m_Vertices[i]->selected)
		{
			glBegin(GL_QUADS);
			glVertex3f(m_Vertices[i]->vertice.x, m_Vertices[i]->vertice.y + 1.0f, m_Vertices[i]->vertice.z);
			glVertex3f(m_Vertices[i]->vertice.x + 1.0f, m_Vertices[i]->vertice.y, m_Vertices[i]->vertice.z);
			glVertex3f(m_Vertices[i]->vertice.x, m_Vertices[i]->vertice.y - 1.0f , m_Vertices[i]->vertice.z);
			glVertex3f(m_Vertices[i]->vertice.x - 1.0f, m_Vertices[i]->vertice.y, m_Vertices[i]->vertice.z);
			glEnd();
		}
	}

	for (i = 0;i < m_connCount;i++)
	{
		if (m_Connectors[i] && m_Connectors[i]->selected)
		{
			glBegin(GL_TRIANGLE_FAN);
			glVertex3f(m_Connectors[i]->connector.x, m_Connectors[i]->connector.z + 1.0f, m_Connectors[i]->connector.y);
			glVertex3f(m_Connectors[i]->connector.x + 1.0f, m_Connectors[i]->connector.z - 1.0f, m_Connectors[i]->connector.y);
			glVertex3f(m_Connectors[i]->connector.x - 1.0f, m_Connectors[i]->connector.z - 1.0f , m_Connectors[i]->connector.y);
			glEnd();
		}
	}
	glColor4ub(255, 255, 255, 255);

	glEnable(GL_DEPTH_TEST);
}

void	CPieInternal::bindTexture(Sint32 num)
{
	glBindTexture(GL_TEXTURE_2D, _TEX_PAGE[num].id);
}


void	CPieInternal::draw(void) {
	//iIMDPoly	*pPolys;
	int			i, n;
	VERTEXID	*index;
	bool		bVerticeDrawn[pie_MAX_VERTICES];

	memset(&bVerticeDrawn[0], 0, sizeof(bool) * m_vertCount);

	this->bindTexture(m_TexpageId);

	/*
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 50.0f);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(0.0f, 4.0f, 50.0f);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(4.0f, 4.0f, 50.0f);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(4.0f, 0.0f, 50.0f);
	glEnd();
	*/


	for (i = 0;i < m_polyCount;i++)
	{
		//piePoly.flags = pPolys->flags;

		if (m_Polygons[i])
		{
			glBegin(GL_TRIANGLE_FAN);
			for (n = 0, index = m_Polygons[i]->polygon.pindex;
					n < m_Polygons[i]->polygon.npnts;
					n++, index++)
			{
//#define PROPER_TEXTURE_COORDS
#ifdef PROPER_TEXTURE_COORDS
				glTexCoord2f(m_Polygons[i]->polygon.texCoord[n].x/_TEX_PAGE[m_TexpageId].w,
							m_Polygons[i]->polygon.texCoord[n].y/_TEX_PAGE[m_TexpageId].h);
#else
				glTexCoord2f(m_Polygons[i]->polygon.texCoord[n].x/256.0f,
							m_Polygons[i]->polygon.texCoord[n].y/256.0f);
#endif
				glVertex3f(m_Vertices[*index]->vertice.x,
							m_Vertices[*index]->vertice.y,
							m_Vertices[*index]->vertice.z);
				bVerticeDrawn[*index] = true;
			}
			glEnd();

			if (m_Polygons[i]->selected)
			{
				glDisable(GL_DEPTH_TEST);

				glColor4ub(0, 64, 0, 128);
				glBegin(GL_LINE_LOOP);
				for (n = 0, index = m_Polygons[i]->polygon.pindex;
						n < m_Polygons[i]->polygon.npnts;
						n++, index++)
				{
					glVertex3f(m_Vertices[*index]->vertice.x,
								m_Vertices[*index]->vertice.y,
								m_Vertices[*index]->vertice.z);
				}
				glEnd();
				glColor4ub(255, 255, 255, 255);
				glEnable(GL_DEPTH_TEST);
			}
		}
	}

	glDisable(GL_TEXTURE_2D);

	for (i = 0;i < m_vertCount;i++)
	{
		if (m_Vertices[i] && !bVerticeDrawn[i])
		{
			glBegin(GL_POINTS);
			glVertex3f(m_Vertices[i]->vertice.x, m_Vertices[i]->vertice.y, m_Vertices[i]->vertice.z);
			glEnd();
		}
	}
}

bool CPieInternal::addGUI(void) {
	Uint16		i, n, totalVertices = 0;
	VERTEXID	*index;

	//Utility bar
	char	toolBarName[255] = "toolbar";
	char	toolBarDefine[255];

	if (this->uid < 10)
	{
		_snprintf(&toolBarName[7], 1, "0");
		_snprintf(&toolBarName[8], 1, "%u", this->uid);
		strcpy(toolBarDefine, toolBarName);
		_snprintf(&toolBarDefine[9], 100, " label = 'Pie %u ToolBar' position = '0 200' color = '%d %d %d %d' size = '%d %d'",
					this->uid, guiMajorAlpha, guiMajorRed, guiMajorGreen, guiMajorBlue, guiMajorWidth, guiMajorHeight);
	}
	else if (this->uid < 100)
	{
		_snprintf(&toolBarName[7], 2, "%2u", this->uid);
		strcpy(toolBarDefine, toolBarName);
		_snprintf(&toolBarDefine[9], 100, " label = 'Pie %u ToolBar' position = '0 200' color = '%d %d %d %d' size = '%d %d'",
					this->uid, guiMajorAlpha, guiMajorRed, guiMajorGreen, guiMajorBlue, guiMajorWidth, guiMajorHeight);
	}
	else
	{
		_snprintf(&toolBarName[7], 3, "3%u", this->uid);
		strcpy(toolBarDefine, toolBarName);
		_snprintf(&toolBarDefine[10], 100, " label = 'Pie %u ToolBar' position = '0 200' color = '%d %d %d %d' size = '%d %d'",
					this->uid, guiMajorAlpha, guiMajorRed, guiMajorGreen, guiMajorBlue, guiMajorWidth, guiMajorHeight);
	}

	m_toolBar = TwNewBar(toolBarName);

	TwDefine(toolBarDefine);

	TwAddButton(m_toolBar, "showgui", showGUICB, &m_InstanceCallback, "label = 'Show GUI'");
	TwAddButton(m_toolBar, "hidegui", hideGUICB, &m_InstanceCallback, "label = 'Hide GUI'");
	TwAddVarRW(m_toolBar, "textureId", TW_TYPE_INT32, &m_TexpageId, "label = 'Page Id' min=0 max=100 step=1");
	TwAddButton(m_toolBar, "addVertice", addVerticeCB, &m_AddVerticeCB, "label = 'Add Vertice'");
	TwAddButton(m_toolBar, "addConnector", addVerticeCB, &m_AddConnectorCB, "label = 'Add Connector'");
	TwAddVarRW(m_toolBar, "newVerticeX", TW_TYPE_FLOAT, &m_newVerticeX, "label = 'New VerticeX' step = 0.1");
	TwAddVarRW(m_toolBar, "newVerticeY", TW_TYPE_FLOAT, &m_newVerticeY, "label = 'New VerticeY' step = 0.1");
	TwAddVarRW(m_toolBar, "newVerticeZ", TW_TYPE_FLOAT, &m_newVerticeZ, "label = 'New VerticeZ' step = 0.1");

	TwAddButton(m_toolBar, "moveVertices", moveSelectedVertices, &m_InstanceCallback, "label = 'Move Selected'");

	TwAddVarRW(m_toolBar, "VelocityX", TW_TYPE_FLOAT, &m_VelocityX, "label = 'VelocityX' step = 0.1");
	TwAddVarRW(m_toolBar, "VelocityY", TW_TYPE_FLOAT, &m_VelocityY, "label = 'VelocityY' step = 0.1");
	TwAddVarRW(m_toolBar, "VelocityZ", TW_TYPE_FLOAT, &m_VelocityZ, "label = 'VelocityZ' step = 0.1");
	TwAddButton(m_toolBar, "removeVertices", removeSelectedCB, &m_InstanceCallback, "label = 'Remove Selected'");

	//Vertices List bar
	char	vertBarName[255];
	char	vertBarDefine[255];

	strncpy(vertBarName, toolBarName, 255);
	strcat(vertBarName, "vertBar");

	_snprintf(vertBarDefine, 255, "%s label = 'Pie %u VerticeList' position = '%d %d' color = '%d %d %d %d' size = '%d %d'",
			vertBarName, this->uid, g_Screen.m_width - 100, 0, guiMajorAlpha, guiMajorRed,
			guiMajorGreen, guiMajorBlue, guiMajorWidth, guiMajorHeight);

	m_vertBar = TwNewBar(vertBarName);
	TwDefine(vertBarDefine);

	for (i = 0;i < m_vertCount;i++)
	{
		if (m_Vertices[i])
		{
			char	name[255] = "vert";
			_snprintf(&name[4], 3, "%3u", i);
			TwAddVarRW(m_vertBar, name, g_pieVertexType, m_Vertices[i], "");
		}
	}

	//Connector list bar
	char	connBarName[255];
	char	connBarDefine[255];

	strncpy(connBarName, toolBarName, 255);
	strcat(connBarName, "connBar");

	_snprintf(connBarDefine, 255, "%s label = 'Pie %u ConnectorList' position = '%d %d' color = '%d %d %d %d' size = '%d %d'",
			connBarName, this->uid, g_Screen.m_width - 100, 400, guiMajorAlpha, guiMajorRed,
			guiMajorGreen, guiMajorBlue, guiMajorWidth, guiMajorHeight);

	m_connBar = TwNewBar(connBarName);
	TwDefine(connBarDefine);

	for (i = 0;i < m_connCount;i++)
	{
		if (m_Connectors[i])
		{
			char	name[255] = "conn";
			_snprintf(&name[4], 3, "%3u", i);
			TwAddVarRW(m_connBar, name, g_pieVertexType, m_Connectors[i], "");
		}
	}

	for (i = 0;i < m_polyCount;i++)
	{
		//multiple polygon names format poly[pie uid][poly uid]
		char	tmpString[255] = "poly";
		char	tmpString2[255];

		//FIXME:capped to 1000(3 digi)
		//assert(this->uid < 1000);
		//utterly hack to add zero's to make Bar names unique -_-
		if (i < 10)
		{
			_snprintf(&tmpString[4], 1, "0");
			_snprintf(&tmpString[5], 1, "%u", i);

			if (this->uid < 10)
			{
				_snprintf(&tmpString[6], 1, "0");
				_snprintf(&tmpString[7], 1, "%u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[8], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
					this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
			}
			else if (this->uid < 100)
			{
				_snprintf(&tmpString[6], 2, "%2u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[8], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
					this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
			}
			else
			{
				_snprintf(&tmpString[6], 3, "3%u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[9], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
					this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
			}

			strcpy(tmpString2, tmpString);
			_snprintf(&tmpString2[8], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
					this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
		}
		else if (i < 100)
		{
			_snprintf(&tmpString[4], 2, "%2u", i);

			if (this->uid < 10)
			{
				_snprintf(&tmpString[6], 1, "0");
				_snprintf(&tmpString[7], 1, "%u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[8], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
					this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
			}
			else if (this->uid < 100)
			{
				_snprintf(&tmpString[6], 2, "%2u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[8], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
					this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
			}
			else
			{
				_snprintf(&tmpString[6], 3, "3%u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[9], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
					this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
			}
		}
		else
		{
			_snprintf(&tmpString[4], 3, "%3u", i);

			if (this->uid < 10)
			{
				_snprintf(&tmpString[7], 1, "0");
				_snprintf(&tmpString[8], 1, "%u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[9], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
					this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
			}
			else if (this->uid < 100)
			{
				_snprintf(&tmpString[7], 2, "%2u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[9], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
					this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
			}
			else
			{
				_snprintf(&tmpString[7], 3, "3%u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[10], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
					this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
			}
		}

		m_polyBars[i] = TwNewBar(tmpString);

		TwDefine(tmpString2);

		TwAddVarRW(m_polyBars[i], "polyselected", TW_TYPE_BOOLCPP, &m_Polygons[i]->selected, "label = 'Poly Selected'");

		for (n = m_Polygons[i]->polygon.npnts, index = m_Polygons[i]->polygon.pindex + (m_Polygons[i]->polygon.npnts - 1);
				n > 0;
				n--, index--)
		{
			char	stringTemp[255] = "vert";

			_snprintf(&stringTemp[4], 55, " id%3u uid%3u", *index, totalVertices++);

			TwAddVarRW(m_polyBars[i], stringTemp, g_pieVertexType, m_Vertices[*index], "");
#if 0
			char	stringX[255] = "verticeX";
			char	stringY[255] = "verticeY";
			char	stringZ[255] = "verticeZ";
			char	stringBool[255] = "bool";

			_snprintf(&stringX[8], 9, "vert%3u", ++totalVertices);
			_snprintf(&stringY[8], 9, "vert%3u", ++totalVertices);
			_snprintf(&stringZ[8], 9, "vert%3u", ++totalVertices);
			_snprintf(&stringBool[4], 9, "bool%3u", totalVertices);

			TwAddVarRW(m_polyBars[i], stringX, TW_TYPE_FLOAT, &m_Vertices[*index]->vertice.x, "label = 'Vertice X' step = 0.1");
			TwAddVarRW(m_polyBars[i], stringY, TW_TYPE_FLOAT, &m_Vertices[*index]->vertice.y, "label = 'Vertice Y' step = 0.1");
			TwAddVarRW(m_polyBars[i], stringZ, TW_TYPE_FLOAT, &m_Vertices[*index]->vertice.z, "label = 'Vertice Z' step = 0.1");
			TwAddVarRW(m_polyBars[i], stringBool, TW_TYPE_BOOLCPP, &m_Vertices[*index]->selected, "label = 'Selected'");
#endif
		}
	}

	return true;
}

void	CPieInternal::updateGUI(void) {
	Uint16		i, n, totalVertices = 0;
	VERTEXID	*index;

	//Utility bar
	char	toolBarName[255] = "toolbar";
	char	toolBarDefine[255];

	if (this->uid < 10)
	{
		_snprintf(&toolBarName[7], 1, "0");
		_snprintf(&toolBarName[8], 1, "%u", this->uid);
		strcpy(toolBarDefine, toolBarName);
		_snprintf(&toolBarDefine[9], 100, " label = 'Pie %u ToolBar' position = '0 200' color = '%d %d %d %d' size = '%d %d'",
					this->uid, guiMajorAlpha, guiMajorRed, guiMajorGreen, guiMajorBlue, guiMajorWidth, guiMajorHeight);
	}
	else if (this->uid < 100)
	{
		_snprintf(&toolBarName[7], 2, "%2u", this->uid);
		strcpy(toolBarDefine, toolBarName);
		_snprintf(&toolBarDefine[9], 100, " label = 'Pie %u ToolBar' position = '0 200' color = '%d %d %d %d' size = '%d %d'",
					this->uid, guiMajorAlpha, guiMajorRed, guiMajorGreen, guiMajorBlue, guiMajorWidth, guiMajorHeight);
	}
	else
	{
		_snprintf(&toolBarName[7], 3, "3%u", this->uid);
		strcpy(toolBarDefine, toolBarName);
		_snprintf(&toolBarDefine[10], 100, " label = 'Pie %u ToolBar' position = '0 200' color = '%d %d %d %d' size = '%d %d'",
					this->uid, guiMajorAlpha, guiMajorRed, guiMajorGreen, guiMajorBlue, guiMajorWidth, guiMajorHeight);
	}

	//Vertice list bar
	char	vertBarName[255];
	char	vertBarDefine[255];

	strncpy(vertBarName, toolBarName, 255);
	strcat(vertBarName, "vertBar");

	_snprintf(vertBarDefine, 255, "%s label = 'Pie %u VerticeList' position = '%d %d' color = '%d %d %d %d' size = '%d %d'",
			vertBarName, this->uid, g_Screen.m_width - 100, 0, guiMajorAlpha, guiMajorRed,
			guiMajorGreen, guiMajorBlue, guiMajorWidth, guiMajorHeight);

	TwDeleteBar(m_vertBar);
	m_vertBar = TwNewBar(vertBarName);
	TwDefine(vertBarDefine);

	for (i = 0;i < m_vertCount;i++)
	{
		if (m_Vertices[i])
		{
			char	name[255] = "vert";
			_snprintf(&name[4], 3, "%3u", i);
			if (!TwAddVarRW(m_vertBar, name, g_pieVertexType, m_Vertices[i], ""))
			{
				fprintf(stderr, TwGetLastError());
			}
		}
	}

	//Connector list bar
	char	connBarName[255];
	char	connBarDefine[255];

	strncpy(connBarName, toolBarName, 255);
	strcat(connBarName, "connBar");

	_snprintf(connBarDefine, 255, "%s label = 'Pie %u ConnectorList' position = '%d %d' color = '%d %d %d %d' size = '%d %d'",
			connBarName, this->uid, g_Screen.m_width - 100, 400, guiMajorAlpha, guiMajorRed,
			guiMajorGreen, guiMajorBlue, guiMajorWidth, guiMajorHeight);

	TwDeleteBar(m_connBar);
	m_connBar = TwNewBar(connBarName);
	TwDefine(connBarDefine);

	for (i = 0;i < m_connCount;i++)
	{
		if (m_Connectors[i])
		{
			char	name[255] = "conn";
			_snprintf(&name[4], 3, "%3u", i);
			TwAddVarRW(m_connBar, name, g_pieVertexType, m_Connectors[i], "");
		}
	}

	for (i = 0;i < m_polyCount;i++)
	{
		if (m_Polygons[i] == NULL)
		{
			//skip NULL polygons
			continue;
		}
		//multiple polygon names format poly[pie uid][poly uid]
		char	tmpString[255] = "poly";
		char	tmpString2[255];

		//FIXME:capped to 1000(3 digi)
		//assert(this->uid < 1000);
		//utterly hack to add zero's to make Bar names unique -_-
		if (i < 10)
		{
			_snprintf(&tmpString[4], 1, "0");
			_snprintf(&tmpString[5], 1, "%u", i);

			if (this->uid < 10)
			{
				_snprintf(&tmpString[6], 1, "0");
				_snprintf(&tmpString[7], 1, "%u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[8], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
					this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
			}
			else if (this->uid < 100)
			{
				_snprintf(&tmpString[6], 2, "%2u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[8], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
					this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
			}
			else
			{
				_snprintf(&tmpString[6], 3, "3%u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[9], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
					this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
			}

			strcpy(tmpString2, tmpString);
			_snprintf(&tmpString2[8], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
					this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
		}
		else if (i < 100)
		{
			_snprintf(&tmpString[4], 2, "%2u", i);

			if (this->uid < 10)
			{
				_snprintf(&tmpString[6], 1, "0");
				_snprintf(&tmpString[7], 1, "%u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[8], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
					this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
			}
			else if (this->uid < 100)
			{
				_snprintf(&tmpString[6], 2, "%2u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[8], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
					this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
			}
			else
			{
				_snprintf(&tmpString[6], 3, "3%u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[9], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
					this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
			}
		}
		else
		{
			_snprintf(&tmpString[4], 3, "%3u", i);

			if (this->uid < 10)
			{
				_snprintf(&tmpString[7], 1, "0");
				_snprintf(&tmpString[8], 1, "%u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[9], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
					this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
			}
			else if (this->uid < 100)
			{
				_snprintf(&tmpString[7], 2, "%2u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[9], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
					this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
			}
			else
			{
				_snprintf(&tmpString[7], 3, "3%u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[10], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
					this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
			}
		}

		TwDeleteBar(m_polyBars[i]);
		m_polyBars[i] = TwNewBar(tmpString);

		TwDefine(tmpString2);

		TwAddVarRW(m_polyBars[i], "polyselected", TW_TYPE_BOOLCPP, &m_Polygons[i]->selected, "label = 'Poly Selected'");

		for (n = m_Polygons[i]->polygon.npnts, index = m_Polygons[i]->polygon.pindex + (m_Polygons[i]->polygon.npnts - 1);
				n > 0;
				n--, index--)
		{
			char	stringTemp[255] = "vert";

			_snprintf(&stringTemp[4], 55, " id%3u uid%3u", *index, totalVertices++);

			TwAddVarRW(m_polyBars[i], stringTemp, g_pieVertexType, m_Vertices[*index], "");
		}
	}
}

void	CPieInternal::hideGUI(void) {
	Uint16		i;

	//Utility bar
	char	toolBarName[255] = "toolbar";
	char	toolBarDefine[255];

	if (this->uid < 10)
	{
		_snprintf(&toolBarName[7], 1, "0");
		_snprintf(&toolBarName[8], 1, "%u", this->uid);
		strcpy(toolBarDefine, toolBarName);
		_snprintf(&toolBarDefine[9], 100, " hide");
	}
	else if (this->uid < 100)
	{
		_snprintf(&toolBarName[7], 2, "%2u", this->uid);
		strcpy(toolBarDefine, toolBarName);
		_snprintf(&toolBarDefine[9], 100, " hide");
	}
	else
	{
		_snprintf(&toolBarName[7], 3, "3%u", this->uid);
		strcpy(toolBarDefine, toolBarName);
		_snprintf(&toolBarDefine[10], 100, " hide");
	}

	//Vertice list bar
	char	vertBarName[255];
	char	vertBarDefine[255];

	strncpy(vertBarName, toolBarName, 255);
	strcat(vertBarName, "vertBar");

	_snprintf(vertBarDefine, 255, "%s hide",
			vertBarName);

	TwDefine(vertBarDefine);

	//Connector list bar
	char	connBarName[255];
	char	connBarDefine[255];

	strncpy(connBarName, toolBarName, 255);
	strcat(connBarName, "connBar");

	_snprintf(connBarDefine, 255, "%s hide",
			connBarName);

	TwDefine(connBarDefine);

	for (i = 0;i < m_polyCount;i++)
	{
		if (m_Polygons[i] == NULL)
		{
			//skip NULL polygons
			continue;
		}
		//multiple polygon names format poly[pie uid][poly uid]
		char	tmpString[255] = "poly";
		char	tmpString2[255];

		//FIXME:capped to 1000(3 digi)
		//assert(this->uid < 1000);
		//utterly hack to add zero's to make Bar names unique -_-
		if (i < 10)
		{
			_snprintf(&tmpString[4], 1, "0");
			_snprintf(&tmpString[5], 1, "%u", i);

			if (this->uid < 10)
			{
				_snprintf(&tmpString[6], 1, "0");
				_snprintf(&tmpString[7], 1, "%u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[8], 100, " hide");
			}
			else if (this->uid < 100)
			{
				_snprintf(&tmpString[6], 2, "%2u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[8], 100, " hide");
			}
			else
			{
				_snprintf(&tmpString[6], 3, "3%u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[9], 100, " hide");
			}

			strcpy(tmpString2, tmpString);
			_snprintf(&tmpString2[8], 100, " hide");
		}
		else if (i < 100)
		{
			_snprintf(&tmpString[4], 2, "%2u", i);

			if (this->uid < 10)
			{
				_snprintf(&tmpString[6], 1, "0");
				_snprintf(&tmpString[7], 1, "%u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[8], 100, " hide");
			}
			else if (this->uid < 100)
			{
				_snprintf(&tmpString[6], 2, "%2u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[8], 100, " hide");
			}
			else
			{
				_snprintf(&tmpString[6], 3, "3%u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[9], 100, " hide");
			}
		}
		else
		{
			_snprintf(&tmpString[4], 3, "%3u", i);

			if (this->uid < 10)
			{
				_snprintf(&tmpString[7], 1, "0");
				_snprintf(&tmpString[8], 1, "%u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[9], 100, " hide");
			}
			else if (this->uid < 100)
			{
				_snprintf(&tmpString[7], 2, "%2u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[9], 100, " hide");
			}
			else
			{
				_snprintf(&tmpString[7], 3, "3%u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[10], 100, " hide");
			}
		}

		TwDefine(tmpString2);
	}
}

void	CPieInternal::showGUI(void) {
	Uint16		i;

	//Utility bar
	char	toolBarName[255] = "toolbar";
	char	toolBarDefine[255];

	if (this->uid < 10)
	{
		_snprintf(&toolBarName[7], 1, "0");
		_snprintf(&toolBarName[8], 1, "%u", this->uid);
		strcpy(toolBarDefine, toolBarName);
		_snprintf(&toolBarDefine[9], 100, " show");
	}
	else if (this->uid < 100)
	{
		_snprintf(&toolBarName[7], 2, "%2u", this->uid);
		strcpy(toolBarDefine, toolBarName);
		_snprintf(&toolBarDefine[9], 100, " show");
	}
	else
	{
		_snprintf(&toolBarName[7], 3, "3%u", this->uid);
		strcpy(toolBarDefine, toolBarName);
		_snprintf(&toolBarDefine[10], 100, " show");
	}

	TwDefine(toolBarDefine);

	//Vertice list bar
	char	vertBarName[255];
	char	vertBarDefine[255];

	strncpy(vertBarName, toolBarName, 255);
	strcat(vertBarName, "vertBar");

	_snprintf(vertBarDefine, 255, "%s show",
			vertBarName);

	TwDefine(vertBarDefine);

	//Connector list bar
	char	connBarName[255];
	char	connBarDefine[255];

	strncpy(connBarName, toolBarName, 255);
	strcat(connBarName, "connBar");

	_snprintf(connBarDefine, 255, "%s show",
			connBarName);

	TwDefine(connBarDefine);

	for (i = 0;i < m_polyCount;i++)
	{
		if (m_Polygons[i] == NULL)
		{
			//skip NULL polygons
			continue;
		}
		//multiple polygon names format poly[pie uid][poly uid]
		char	tmpString[255] = "poly";
		char	tmpString2[255];

		//FIXME:capped to 1000(3 digi)
		//assert(this->uid < 1000);
		//utterly hack to add zero's to make Bar names unique -_-
		if (i < 10)
		{
			_snprintf(&tmpString[4], 1, "0");
			_snprintf(&tmpString[5], 1, "%u", i);

			if (this->uid < 10)
			{
				_snprintf(&tmpString[6], 1, "0");
				_snprintf(&tmpString[7], 1, "%u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[8], 100, " show");
			}
			else if (this->uid < 100)
			{
				_snprintf(&tmpString[6], 2, "%2u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[8], 100, " show");
			}
			else
			{
				_snprintf(&tmpString[6], 3, "3%u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[9], 100, " show");
			}

			strcpy(tmpString2, tmpString);
			_snprintf(&tmpString2[8], 100, " show");
		}
		else if (i < 100)
		{
			_snprintf(&tmpString[4], 2, "%2u", i);

			if (this->uid < 10)
			{
				_snprintf(&tmpString[6], 1, "0");
				_snprintf(&tmpString[7], 1, "%u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[8], 100, " show");
			}
			else if (this->uid < 100)
			{
				_snprintf(&tmpString[6], 2, "%2u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[8], 100, " show");
			}
			else
			{
				_snprintf(&tmpString[6], 3, "3%u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[9], 100, " show");
			}
		}
		else
		{
			_snprintf(&tmpString[4], 3, "%3u", i);

			if (this->uid < 10)
			{
				_snprintf(&tmpString[7], 1, "0");
				_snprintf(&tmpString[8], 1, "%u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[9], 100, " show");
			}
			else if (this->uid < 100)
			{
				_snprintf(&tmpString[7], 2, "%2u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[9], 100, " show");
			}
			else
			{
				_snprintf(&tmpString[7], 3, "3%u", this->uid);
				strcpy(tmpString2, tmpString);
				_snprintf(&tmpString2[10], 100, " show");
			}
		}

		TwDefine(tmpString2);
	}
}
