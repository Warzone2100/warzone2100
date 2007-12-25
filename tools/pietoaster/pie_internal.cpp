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
#include "pie_internal.h"

#include <SDL_opengl.h>

#include "screen.h"
#include "game_io.h"
#include "config.h"

#include "gui.h"

///cache between pie and va/vbo
static INTERLEAVED_T2F_V3F vaCache[pie_MAX_VERTICES];

//default submodel id for GUI
static Uint32 g_SubModelIndex = 8000;

// this is a joke actually the result of 65536.0f / 360.f * 22.5f / 4096.0f is ~1
static inline float WTFScale(float x)
{
	//return (x * 65536.0f / 360.f * 22.5f / 4096.0f);
	return x;
}

CAddSubModelDialog	AddSubModelDialog;
CAddSubModelFileDialog	AddSubModelFileDialog;
CReadAnimFileDialog	ReadAnimFileDialog;
CWriteAnimFileDialog WriteAnimFileDialog;

void	CAddSubModelDialog::doFunction() {
	CPieInternal	*instance = reinterpret_cast<CPieInternal*>(m_userInfo);

	if (!instance->addSub(m_text))
	{
		fprintf(stderr, "Error adding sub model with name:%s\n", m_text);
	}
	this->deleteTextBox();
	this->m_Up = false;
}

void	CAddSubModelFileDialog::doFunction() {
	CPieInternal	*instance = reinterpret_cast<CPieInternal*>(m_userInfo);

	if (!instance->addSubFromFile(m_text))
	{
		fprintf(stderr, "Error adding sub model with name:%s\n", m_text);
	}
	this->deleteTextBox();
	this->m_Up = false;
}

void	CReadAnimFileDialog::doFunction() {
	CPieInternal	*instance = reinterpret_cast<CPieInternal*>(m_userInfo);

	instance->readAnimFile(m_text);
	instance->updateGUI();
	// Updates sub model gui as well
	CPieInternal	*temp = instance->m_nextSub;
	while (temp)
	{
		temp->updateGUI();
		temp = temp->m_nextSub;
	}
	this->deleteTextBox();
	this->m_Up = false;
}

void	CWriteAnimFileDialog::doFunction() {
	CPieInternal	*instance = reinterpret_cast<CPieInternal*>(m_userInfo);

	if (instance->m_animMode == instance->ANIM3DTRANS)
	{
		if (!instance->writeAnimFileTrans(m_text))
		{
			fprintf(stderr, "Error writing anim file name:%s\n", m_text);
		}
	}
	else if (instance->m_animMode == instance->ANIM3DFRAMES)
	{
		if (!instance->writeAnimFileFrames(m_text))
		{
			fprintf(stderr, "Error writing anim file name:%s\n", m_text);
		}
	}
	this->deleteTextBox();
	this->m_Up = false;
}

//// Callbacks ////

/* borked CB's
void TW_CALL setVarCB(const void *value, void *clientData) {
}

void TW_CALL getVarCB(void *value, void *clientData) {
	PIE_INTERNAL_CB	*pie_internal_cb = (PIE_INTERNAL_CB*)clientData;
	CPieInternal	*instance = reinterpret_cast<CPieInternal*>(pie_internal_cb->pInstance);

	instance->buildHLCache();
}
*/

//clientData as struct(includes instance of object and int info)
void TW_CALL addVerticeCB(void *clientData) {
	PIE_INTERNAL_CB	*pie_internal_cb = (PIE_INTERNAL_CB*)clientData;
	CPieInternal	*instance = reinterpret_cast<CPieInternal*>(pie_internal_cb->pInstance);
	Uint8	i = pie_internal_cb->Id;

	//fprintf(stderr, "address of instance:%d", instance);
	//fprintf(stderr, "type of id:%c", i);

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
	Uint16	i = pie_internal_cb->Id;

	if (!instance->removeVerticeAt(i))
	{
		fprintf(stderr, "remove vertice at %d failed\n", i);
	}
}

//clientData as struct(includes instance of object and int info)
void TW_CALL removeConnectorAtCB(void *clientData) {
	PIE_INTERNAL_CB	*pie_internal_cb = (PIE_INTERNAL_CB*)clientData;
	CPieInternal	*instance = reinterpret_cast<CPieInternal*>(pie_internal_cb->pInstance);
	Uint16	i = pie_internal_cb->Id;

	if (!instance->removeConnectorAt(i))
	{
		fprintf(stderr, "remove connector at %d failed\n", i);
	}
	instance->updateGUI();
}

//clientData as struct(includes instance of object and int info)
void TW_CALL selectAllCB(void *clientData) {
	PIE_INTERNAL_CB	*pie_internal_cb = (PIE_INTERNAL_CB*)clientData;
	CPieInternal	*instance = reinterpret_cast<CPieInternal*>(pie_internal_cb->pInstance);

	instance->selectAll();
}

//clientData as struct(includes instance of object and int info)
void TW_CALL unselectAllCB(void *clientData) {
	PIE_INTERNAL_CB	*pie_internal_cb = (PIE_INTERNAL_CB*)clientData;
	CPieInternal	*instance = reinterpret_cast<CPieInternal*>(pie_internal_cb->pInstance);

	instance->unselectAll();
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
void TW_CALL duplicateSelectedVertices(void *clientData) {
	PIE_INTERNAL_CB	*pie_internal_cb = (PIE_INTERNAL_CB*)clientData;
	CPieInternal	*instance = reinterpret_cast<CPieInternal*>(pie_internal_cb->pInstance);

	if (!instance->duplicateSelected())
	{
		fprintf(stderr, "Error duplicating selected vertices\n");
	}
	instance->updateGUI();
}

//clientData as struct(includes instance of object and int info)
void TW_CALL duplicatePolyCB(void *clientData) {
	PIE_INTERNAL_CB	*pie_internal_cb = (PIE_INTERNAL_CB*)clientData;
	CPieInternal	*instance = reinterpret_cast<CPieInternal*>(pie_internal_cb->pInstance);
	Uint16			id = pie_internal_cb->Id;

	if (!instance->duplicatePoly(id))
	{
		fprintf(stderr, "Error duplicating Poly %d\n", id);
	}
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

//clientData as struct(includes instance of object and int info)
void TW_CALL destructCB(void *clientData) {
	PIE_INTERNAL_CB	*pie_internal_cb = (PIE_INTERNAL_CB*)clientData;
	CPieInternal	*instance = reinterpret_cast<CPieInternal*>(pie_internal_cb->pInstance);

	instance->died = true;
}

//clientData as struct(includes instance of object and int info)
void TW_CALL addSubModelCB(void *clientData) {
	PIE_INTERNAL_CB	*pie_internal_cb = (PIE_INTERNAL_CB*)clientData;

	AddSubModelDialog.addTextBox("AddSubModel");
	AddSubModelDialog.addUserInfo(pie_internal_cb->pInstance);
}

//clientData as struct(includes instance of object and int info)
void TW_CALL addSubModelFileCB(void *clientData) {
	PIE_INTERNAL_CB	*pie_internal_cb = (PIE_INTERNAL_CB*)clientData;

	AddSubModelFileDialog.addTextBox("AddSubModelFile");
	AddSubModelFileDialog.addUserInfo(pie_internal_cb->pInstance);
}

//clientData as struct(includes instance of object and int info)
void TW_CALL readAnimFileCB(void *clientData) {
	PIE_INTERNAL_CB	*pie_internal_cb = (PIE_INTERNAL_CB*)clientData;

	ReadAnimFileDialog.addTextBox("ReadAnimFile");
	ReadAnimFileDialog.addUserInfo(pie_internal_cb->pInstance);
}

//clientData as struct(includes instance of object and int info)
void TW_CALL writeAnimFileCB(void *clientData) {
	PIE_INTERNAL_CB	*pie_internal_cb = (PIE_INTERNAL_CB*)clientData;

	WriteAnimFileDialog.addTextBox("WriteAnimFile");
	WriteAnimFileDialog.addUserInfo(pie_internal_cb->pInstance);
}

//clientData as struct(includes instance of object and int info)
void TW_CALL addFrameCB(void *clientData) {
	PIE_INTERNAL_CB	*pie_internal_cb = (PIE_INTERNAL_CB*)clientData;
	CPieInternal	*instance = reinterpret_cast<CPieInternal*>(pie_internal_cb->pInstance);

	instance->addFrame();
}

//clientData as struct(PIE_FRAME_INFO)
void TW_CALL removeFrameCB(void *clientData) {
	PIE_FRAME_INFO	*frame = (PIE_FRAME_INFO*)clientData;

	frame->died = true;
}

//clientData as struct(includes instance and frameId to duplicate)
void TW_CALL duplicateFrameCB(void *clientData) {
	PIE_INTERNAL_CB	*pie_internal_cb = (PIE_INTERNAL_CB*)clientData;
	CPieInternal	*instance = reinterpret_cast<CPieInternal*>(pie_internal_cb->pInstance);
	Uint16			id = pie_internal_cb->Id;

	instance->duplicateFrame(id);
}

void TW_CALL saveNowCB(void *clientData) {
	PIE_INTERNAL_CB	*pie_internal_cb = (PIE_INTERNAL_CB*)clientData;
	CPieInternal	*instance = reinterpret_cast<CPieInternal*>(pie_internal_cb->pInstance);

	instance->m_saveNow = true;
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
	Uint8		color[4] = {0, 0, 64, 255};
	bool		bTextured = false;

	glDisable(GL_DEPTH_TEST);

	if (glIsEnabled(GL_TEXTURE_2D))
	{
		glDisable(GL_TEXTURE_2D);
		bTextured = true;
	}

	for (i = 0;i < m_LinkedIndex;i++)
	{
		if ((i + 1) >= m_LinkedIndex)
		{
			break;
		}
		glColor4ub(color[0], color[1], color[2]+i*32, color[3]);
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

	glEnable(GL_DEPTH_TEST);

	if (bTextured)
	{
		glEnable(GL_TEXTURE_2D);
	}
}

void	CPolygonLinker::makePolygon(CPieInternal *target) {
	Uint8		i;
	Uint16		polyIndex;

	polyIndex = target->findFreeSlot(1);

	target->m_Polygons[polyIndex] = (IMD_POLY_LIST*)malloc(sizeof(IMD_POLY_LIST));

	memset(target->m_Polygons[polyIndex], 0, sizeof(IMD_POLY_LIST));

	//TODO:make flags customizable?
	target->m_Polygons[polyIndex]->polygon.flags = 0x00000200;

	target->m_Polygons[polyIndex]->polygon.pTexAnim = NULL;
	target->m_Polygons[polyIndex]->polygon.pindex = (VERTEXID*)malloc(sizeof(VERTEXID) * pie_MAX_VERTICES_PER_POLYGON);
	target->m_Polygons[polyIndex]->polygon.texCoord = (Vector2f*)malloc(sizeof(Vector2f) * pie_MAX_VERTICES_PER_POLYGON);
	memset(target->m_Polygons[polyIndex]->polygon.texCoord, 0, sizeof(Vector2f) * pie_MAX_VERTICES_PER_POLYGON);
	target->m_Polygons[polyIndex]->polygon.npnts = m_LinkedIndex;
	target->m_Polygons[polyIndex]->callback.Id = polyIndex;
	target->m_Polygons[polyIndex]->callback.pInstance = (void*)target;

	for (i = 0;i < m_LinkedIndex;i++)
	{
		target->m_Polygons[polyIndex]->polygon.pindex[i] = (VERTEXID)m_LinkedVertices[i];
	}

	target->m_Polygons[polyIndex]->id = polyIndex;
	target->m_Polygons[polyIndex]->frame = 0;	//set default frame to 0

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
		colour.byte.a = (Uint8)pieFlagData;
		//pie_SetBilinear(true);
		light = false;
	}
	else if (pieFlag & pie_TRANSLUCENT)
	{
		//pie_SetRendMode(REND_ALPHA_TEX);
		colour.byte.a = (Uint8)pieFlagData;
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
CPieInternal::CPieInternal(Uint16 uid, iIMDShape *imd, const char *name, bool isSub, CPieInternal *parent) {
	Uint32 i, j, vertIndex = 0;

	memset(this->m_Polygons, 0, sizeof(IMD_POLY_LIST) * pie_MAX_POLYGONS);
	memset(this->m_Vertices, 0, sizeof(VERTICE_LIST) * pie_MAX_VERTICES);
	memset(this->m_Connectors, 0, sizeof(CONNECTOR_LIST) * pie_MAX_VERTICES);

	m_IsSub = isSub;
	m_Visible = true;
	m_saveNow = false;
	m_drawAnim = m_framesVisible = m_controlSubAnim = false;
	m_parent = parent;
	m_numAnimFrames = 0; //imd->numFrames; need to load ani file
	m_frameInterval = 0;
	m_currAnimFrame = 0xFFFFFFFF;
	m_levels = 0;

	strncpy(&m_Name[0], name, 255);

	died = false;

	m_polyCount = m_vertCount = m_connCount = 0;
	m_newVerticeX = m_newVerticeY = m_newVerticeZ = 0.0f;
	m_VelocityX = m_VelocityY = m_VelocityZ = 0.0f;
	m_duplicateX = m_duplicateY = m_duplicateZ = 0.0f;

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
		m_Polygons[i]->frame = 0;	//set default frame to 0

		m_Polygons[i]->polygon.texCoord = (Vector2f*)malloc(sizeof(Vector2f) * pie_MAX_VERTICES_PER_POLYGON);

		m_Polygons[i]->polygon.pindex = (VERTEXID*)malloc(sizeof(VERTEXID) * pie_MAX_VERTICES_PER_POLYGON);

		for (j = 0;
			j < imd->polys[i].npnts;
			j++)
		{
			m_Polygons[i]->polygon.pindex[j] = imd->polys[i].pindex[j];
			m_Polygons[i]->polygon.texCoord[j].x = imd->polys[i].texCoord[j].x;
			m_Polygons[i]->polygon.texCoord[j].y = imd->polys[i].texCoord[j].y;
		}

		m_Polygons[i]->polygon.normal = imd->polys[i].normal;
		m_Polygons[i]->polygon.npnts = imd->polys[i].npnts;
		m_Polygons[i]->polygon.zcentre = imd->polys[i].zcentre;
		m_Polygons[i]->polygon.flags = imd->polys[i].flags;

		m_Polygons[i]->callback.Id = i;
		m_Polygons[i]->callback.pInstance = (void*)this;

		m_Polygons[i]->id = i;
		m_Polygons[i]->selected = false;
		m_Polygons[i]->hasVBO = false;

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

		m_Vertices[vertIndex]->callback.pInstance = this->getInstance();
		m_Vertices[vertIndex]->callback.Id = vertIndex;
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

		m_Connectors[i]->callback.pInstance = this->getInstance();
		m_Connectors[i]->callback.Id = i;
		m_connCount++;
	}

	this->addGUI();

	if (imd->next)
	{
		g_SubModelIndex++;
		if (this->m_parent)
		{
			this->m_nextSub = new CPieInternal(g_SubModelIndex, imd->next, "sub", true, this->m_parent);
			this->m_parent->m_levels++;
		}
		else
		{
			this->m_nextSub = new CPieInternal(g_SubModelIndex, imd->next, "sub", true, this);
		}
	}
	else
	{
		this->m_nextSub = NULL;
	}

	if (this->m_levels > 0)
	{
		this->readAnimFile(this->m_Name);
		this->updateGUI();
		// Updates sub model gui as well
		CPieInternal	*temp = this->m_nextSub;
		while (temp)
		{
			temp->updateGUI();
			temp = temp->m_nextSub;
		}
	}
}

///Newly generated CPieInternal
CPieInternal::CPieInternal(Uint16 uid, const char *name, Sint32 textureId, bool isSub, CPieInternal *parent) {
	memset(this->m_Polygons, 0, sizeof(IMD_POLY_LIST) * pie_MAX_POLYGONS);
	memset(this->m_Vertices, 0, sizeof(VERTICE_LIST) * pie_MAX_VERTICES);
	memset(this->m_Connectors, 0, sizeof(CONNECTOR_LIST) * pie_MAX_VERTICES);

	died = false;
	m_IsSub = isSub;
	m_Visible = true;
	m_saveNow = false;
	m_drawAnim = m_framesVisible = m_controlSubAnim = false;
	m_parent = parent;
	m_numAnimFrames = 0;
	m_frameInterval = 0;
	m_currAnimFrame = 0xFFFFFFFF;
	m_levels = 0;

	if (parent)
	{
		parent->m_levels++;
	}

	m_levels = m_polyCount = m_vertCount = m_connCount = 0;
	m_newVerticeX = m_newVerticeY = m_newVerticeZ = 0.0f;
	m_VelocityX = m_VelocityY = m_VelocityZ = 0.0f;
	m_duplicateX = m_duplicateY = m_duplicateZ = 0.0f;

	this->uid = uid;
	this->setInstance();
	this->m_InstanceCallback.pInstance = (void*)this;
	this->m_InstanceCallback.Id = 0;

	this->m_AddVerticeCB.pInstance = (void*)this;
	this->m_AddVerticeCB.Id = PIE_VERTICE;

	this->m_AddConnectorCB.pInstance = (void*)this;
	this->m_AddConnectorCB.Id = PIE_CONNECTOR;

	this->m_numInstances = 0;
	this->m_TexpageId = textureId;

	// Sets newly generated poly's next sub to null
	this->m_nextSub = NULL;

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

			if (m_Polygons[i]->hasVBO)
			{
				g_Screen.glDeleteBuffersARB(1, &m_Polygons[i]->VBOId);
			}

			free(m_Polygons[i]);
			m_Polygons[i] = NULL;
		}
	}

	if (this->m_IsSub)
	{
		/* do nothing handled in ResMaster now
		//assert(this->m_parent != NULL);
		CPieInternal	*next = this->m_parent;
		while (next)
		{
			if (next->m_nextSub == this)
			{
				next->m_nextSub = next->m_nextSub->m_nextSub;
				break;
			}
			next = next->m_nextSub;
		}
		//fprintf(stderr, "No Sub model found at %d in model at %d\n", this->m_parent, this);
		*/
	}
	else
	{
		// Deletes sub models
		CPieInternal	*next = this->m_nextSub;
		while (next)
		{
			CPieInternal	*temp = next->m_nextSub;
			delete next;
			next = temp;
		}
	}

	for (i = 0;i < this->m_numAnimFrames;i++)
	{
		TwDeleteBar(this->m_frames[i].Bar);
		this->m_frames[i].Bar = NULL;
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

	/*
	if (g_Screen.m_useVBO)
	{
		g_Screen.glDeleteBuffersARB(1, &m_VerticesHLVBOId);
		g_Screen.glDeleteBuffersARB(1, &m_ConnectorsHLVBOId);
	}
	*/
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
	Uint32 i, j, vertIndex = 0, newPolyIndex, newVertIndex, newConnIndex;

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

		newIMD->polys[newPolyIndex].pindex = (VERTEXID*)malloc(sizeof(VERTEXID) * pie_MAX_VERTICES_PER_POLYGON);
		newIMD->polys[newPolyIndex].texCoord = (Vector2f*)malloc(sizeof(Vector2f) * pie_MAX_VERTICES_PER_POLYGON);
		newIMD->polys[newPolyIndex].flags = m_Polygons[i]->polygon.flags;
		newIMD->polys[newPolyIndex].normal = m_Polygons[i]->polygon.normal;
		newIMD->polys[newPolyIndex].npnts = m_Polygons[i]->polygon.npnts;
		newIMD->polys[newPolyIndex].zcentre = m_Polygons[i]->polygon.zcentre;
		//TODO:TexAnim
		newIMD->polys[newPolyIndex].pTexAnim = (iTexAnim*)malloc(sizeof(iTexAnim));

		//memcpy(newIMD->polys[newPolyIndex], m_Polygons[i]->polygon, sizeof(iIMDPoly));
		//newIMD->polys[newPolyIndex] = m_Polygons[i]->polygon;
		if (m_Polygons[i]->polygon.pTexAnim)
		{
			memcpy(newIMD->polys[newPolyIndex].pTexAnim, m_Polygons[i]->polygon.pTexAnim, sizeof(iTexAnim));
		}
		else
		{
			newIMD->polys[newPolyIndex].pTexAnim = NULL;
		}
		for (j = 0;j < m_Polygons[i]->polygon.npnts;j++)
		{
			//memcpy(&newIMD->polys[newPolyIndex].pindex[j], &m_Polygons[i]->polygon.pindex[j], sizeof(VERTEXID));
			newIMD->polys[newPolyIndex].pindex[j] = m_Polygons[i]->polygon.pindex[j];
			newIMD->polys[newPolyIndex].texCoord[j] = m_Polygons[i]->polygon.texCoord[j];
			//newIMD->polys[newPolyIndex].pTexAnim[j] = m_Polygons[i]->polygon.pTexAnim[j];
		}
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
			// Paranoid check
			int	polys, points;
			for (polys = 0;polys < newIMD->npolys;polys++)
			{
				for (points = 0;points < newIMD->polys[polys].npnts;points++)
				{
					if (newIMD->polys[polys].pindex[points] > vertIndex)
					{
						newIMD->polys[polys].pindex[points] -= 1;
					}
				}
			}
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

	//Recursively save the multilevel pies
	CPieInternal	*sub = this->m_nextSub;
	if (sub)
	{
		newIMD->next = sub->ToIMD();
	}

	return newIMD;
}

bool	CPieInternal::ToFile(const char *filename, bool isOld) {
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

	if (isOld)
	{
		iV_IMDSaveOld(filename, tmpIMD, true);
	}
	else
	{
		iV_IMDSave(filename, tmpIMD, true);
	}

	int	i;
	for (i = 0;i < tmpIMD->npolys;i++)
	{
		if(tmpIMD->polys[i].pindex)
		{
			free(tmpIMD->polys[i].pindex);
			tmpIMD->polys[i].pindex = NULL;
		}
		if(tmpIMD->polys[i].pindex)
		{
			free(tmpIMD->polys[i].texCoord);
			tmpIMD->polys[i].texCoord = NULL;
		}
		if(tmpIMD->polys[i].pTexAnim)
		{
			free(tmpIMD->polys[i].pTexAnim);
			tmpIMD->polys[i].pindex = NULL;
		}
	}

	if (tmpIMD->polys)
	{
		free(tmpIMD->polys);
		tmpIMD->polys = NULL;
	}
	if (tmpIMD->points)
	{
		free(tmpIMD->points);
		tmpIMD->points = NULL;
	}
	if (tmpIMD->connectors)
	{
		free(tmpIMD->connectors);
		tmpIMD->connectors = NULL;
	}

	free(tmpIMD);
	tmpIMD = NULL;

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

			m_Vertices[slot]->id = slot;
			m_Vertices[slot]->selected = false;

			m_Vertices[slot]->callback.pInstance = this->getInstance();
			m_Vertices[slot]->callback.Id = slot;

			if (slot == m_vertCount)
			{
				m_vertCount++;
			}

			// Now rebuilds every frame due to gui
			// Re-construct the shared vertice list
			//this->constructSharedVerticeList();
			// Re-build highlight cache
			//this->buildHLCache();

			return true;
		case PIE_CONNECTOR:
			m_Connectors[slot] = (CONNECTOR_LIST*)malloc(sizeof(CONNECTOR_LIST));

			m_Connectors[slot]->connector.x = m_newVerticeX;
			m_Connectors[slot]->connector.y = m_newVerticeY;
			m_Connectors[slot]->connector.z = m_newVerticeZ;

			m_Connectors[slot]->id = slot;
			m_Connectors[slot]->selected = false;

			m_Connectors[slot]->callback.pInstance = this->getInstance();
			m_Connectors[slot]->callback.Id = slot;

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

	// Traverse and remove all polygons using this vertice
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

					if (m_Polygons[i]->hasVBO)
					{
						g_Screen.glDeleteBuffersARB(1, &m_Polygons[i]->VBOId);
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
		//this->buildHLCache();
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
		//this->buildHLCache();
		return true;
	}

	fprintf(stderr, "connector in position %d doesnt exist\n", position);
	return false;
}

void	CPieInternal::selectAll(void) {
	Uint16	i;

	for (i = 0;i < m_vertCount;i++)
	{
		if (m_Vertices[i])
		{
			m_Vertices[i]->selected = true;
		}
	}

	for (i = 0;i < m_connCount;i++)
	{
		if (m_Connectors[i])
		{
			m_Connectors[i]->selected = true;
		}
	}
}

void	CPieInternal::unselectAll(void) {
	Uint16	i;

	for (i = 0;i < m_vertCount;i++)
	{
		if (m_Vertices[i])
		{
			m_Vertices[i]->selected = false;
		}
	}

	for (i = 0;i < m_connCount;i++)
	{
		if (m_Connectors[i])
		{
			m_Connectors[i]->selected = false;
		}
	}
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
			//this->buildHLCache();
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

bool	CPieInternal::duplicateSelected(void) {
	Uint16	i, count;

	count = 0;
	for (i = 0;i < m_vertCount;i++)
	{
		if (m_Vertices[i] && m_Vertices[i]->selected)
		{
			Uint16	slot;

			slot = this->findFreeSlot(PIE_VERTICE);

			if (slot == 0xFFFF)
			{
				fprintf(stderr, "no slot left for duplicate at %d\n", i);
				return false;
			}

			m_Vertices[slot] = (VERTICE_LIST*)malloc(sizeof(VERTICE_LIST));

			m_Vertices[slot]->vertice.x = m_Vertices[i]->vertice.x + m_duplicateX;
			m_Vertices[slot]->vertice.y = m_Vertices[i]->vertice.y + m_duplicateY;
			m_Vertices[slot]->vertice.z = m_Vertices[i]->vertice.z + m_duplicateZ;

			m_Vertices[slot]->id = slot;
			m_Vertices[slot]->selected = false;

			m_Vertices[slot]->callback.pInstance = this->getInstance();
			m_Vertices[slot]->callback.Id = slot;

			m_vertCount++;

			count++;
		}
	}

	fprintf(stderr, "%d vertices duplicated.\n", count);
	return true;
}

bool	CPieInternal::duplicatePoly(Uint16 index) {
	Uint16	i;
	Uint16	newPolyIndex;

	newPolyIndex = this->findFreeSlot(PIE_POLYGON);

	if (this->m_Polygons[newPolyIndex] != NULL)
	{
		fprintf(stderr, "Error when duplicating polygon, polygon slot %d is used.\n", newPolyIndex);
		return false;
	}

	this->m_Polygons[newPolyIndex] = (IMD_POLY_LIST*)malloc(sizeof(IMD_POLY_LIST));

	memset(this->m_Polygons[newPolyIndex], 0, sizeof(IMD_POLY_LIST));

	//TODO:make flags customizable?
	m_Polygons[newPolyIndex]->polygon.flags = 0x00000200;

	m_Polygons[newPolyIndex]->polygon.pTexAnim = NULL;
	m_Polygons[newPolyIndex]->polygon.pindex = (VERTEXID*)malloc(sizeof(VERTEXID) * pie_MAX_VERTICES_PER_POLYGON);
	m_Polygons[newPolyIndex]->polygon.texCoord = (Vector2f*)malloc(sizeof(Vector2f) * pie_MAX_VERTICES_PER_POLYGON);
	memset(m_Polygons[newPolyIndex]->polygon.texCoord, 0, sizeof(Vector2f) * pie_MAX_VERTICES_PER_POLYGON);
	m_Polygons[newPolyIndex]->polygon.npnts = m_Polygons[index]->polygon.npnts;
	m_Polygons[newPolyIndex]->polygon.normal = m_Polygons[index]->polygon.normal;

	m_Polygons[newPolyIndex]->id = newPolyIndex;
	m_Polygons[newPolyIndex]->frame = 0;	//set default frame to 0

	m_Polygons[newPolyIndex]->hasVBO = false;

	for (i = 0;i < m_Polygons[index]->polygon.npnts;i++)
	{
		Uint16	slot;

		slot = findFreeSlot(PIE_VERTICE);

		if (slot == 0xFFFF)
		{
			fprintf(stderr, "no slot left for polygon duplicate vertice at %d\n", i);
			free(m_Polygons[index]);
			m_Polygons[index] = NULL;
			return false;
		}

		m_Vertices[slot] = (VERTICE_LIST*)malloc(sizeof(VERTICE_LIST));

		m_Vertices[slot]->vertice.x = m_Vertices[i]->vertice.x + m_duplicateX;
		m_Vertices[slot]->vertice.y = m_Vertices[i]->vertice.y + m_duplicateY;
		m_Vertices[slot]->vertice.z = m_Vertices[i]->vertice.z + m_duplicateZ;

		m_Vertices[slot]->id = slot;
		m_Vertices[slot]->selected = false;

		m_Vertices[slot]->callback.pInstance = this->getInstance();
		m_Vertices[slot]->callback.Id = slot;

		m_vertCount++;

		m_Polygons[newPolyIndex]->polygon.pindex[i] = (VERTEXID)slot;
		m_Polygons[newPolyIndex]->polygon.texCoord[i].x = m_Polygons[index]->polygon.texCoord[i].x;
		m_Polygons[newPolyIndex]->polygon.texCoord[i].y = m_Polygons[index]->polygon.texCoord[i].y;
		m_Polygons[newPolyIndex]->callback.Id = newPolyIndex;
		m_Polygons[newPolyIndex]->callback.pInstance = (void*)this;
	}

	m_polyCount++;
	return true;
}

void	CPieInternal::checkSelection(float x1, float y1, float z1,
									 float x2, float y2, float z2) {
	Uint16	i, chosen;
	float	mag, minDist;
	float	xdiff = x2 - x1, ydiff = y2 - y1, zdiff = z2 - z1;
	Vector3f	slope = {xdiff, ydiff, zdiff};

	if (!m_Visible)
	{
		goto CheckNextSubSelection;
	}

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
		return;
	}

CheckNextSubSelection:
	if (this->m_nextSub)
	{
		this->m_nextSub->checkSelection(x1, y1, z1,
										x2, y2, z2);
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

void	CPieInternal::buildHLCache(void) {
	Sint32	i, index;

	index = - 1;
	m_vertHLCount = 0;
	for (i = 0;i < m_vertCount;i++)
	{
		if (m_Vertices[i])
		{
			++index;
			m_VerticesHLCache[index].x = m_Vertices[i]->vertice.x;
			m_VerticesHLCache[index].y = m_Vertices[i]->vertice.y + 1.0f;
			m_VerticesHLCache[index].z = m_Vertices[i]->vertice.z;

			++index;
			m_VerticesHLCache[index].x = m_Vertices[i]->vertice.x + 1.0f;
			m_VerticesHLCache[index].y = m_Vertices[i]->vertice.y;
			m_VerticesHLCache[index].z = m_Vertices[i]->vertice.z;

			++index;
			m_VerticesHLCache[index].x = m_Vertices[i]->vertice.x;
			m_VerticesHLCache[index].y = m_Vertices[i]->vertice.y - 1.0f;
			m_VerticesHLCache[index].z = m_Vertices[i]->vertice.z;

			++index;
			m_VerticesHLCache[index].x = m_Vertices[i]->vertice.x - 1.0f;
			m_VerticesHLCache[index].y = m_Vertices[i]->vertice.y;
			m_VerticesHLCache[index].z = m_Vertices[i]->vertice.z;
			m_vertHLCount++;
		}
	}

	index = - 1;
	m_connHLCount = 0;
	for (i = 0;i < m_connCount;i++)
	{
		if (m_Connectors[i])
		{
			++index;
			m_ConnectorsHLCache[index].x = m_Connectors[i]->connector.x;
			m_ConnectorsHLCache[index].y = m_Connectors[i]->connector.z + 1.0f;
			m_ConnectorsHLCache[index].z = m_Connectors[i]->connector.y;

			++index;
			m_ConnectorsHLCache[index].x = m_Connectors[i]->connector.x + 1.0f;
			m_ConnectorsHLCache[index].y = m_Connectors[i]->connector.z - 1.0f;
			m_ConnectorsHLCache[index].z = m_Connectors[i]->connector.y;

			++index;
			m_ConnectorsHLCache[index].x = m_Connectors[i]->connector.x - 1.0f;
			m_ConnectorsHLCache[index].y = m_Connectors[i]->connector.z - 1.0f;
			m_ConnectorsHLCache[index].z = m_Connectors[i]->connector.y;

			m_connHLCount++;
		}
	}

#if 0
	if (g_Screen.m_useVBO)
	{
		Sint32 test;

		if (!bRebuild)
		{
			if (m_vertHLCount)
			{
				glGenBuffersARB(1, &m_VerticesHLVBOId);
				glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_VerticesHLVBOId);
				glBufferDataARB(GL_ARRAY_BUFFER_ARB, m_vertHLCount * 4, m_VerticesHLCache, GL_STATIC_DRAW_ARB);
			}

			if (m_connHLCount)
			{
				glGenBuffersARB(1, &m_ConnectorsHLVBOId);
				glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_ConnectorsHLVBOId);
				glBufferDataARB(GL_ARRAY_BUFFER_ARB, m_connHLCount * 3, m_ConnectorsHLCache, GL_STATIC_DRAW_ARB);

			}

			glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
			bRebuild = true;
		}
		else
		{
			//Deletes and recreate there is no need to map buffer cos HL lists may resize
			glDeleteBuffersARB(1, &m_VerticesHLVBOId);
			glDeleteBuffersARB(1, &m_ConnectorsHLVBOId);
			if (m_vertHLCount)
			{
				glGenBuffersARB(1, &m_VerticesHLVBOId);
				glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_VerticesHLVBOId);
				glBufferDataARB(GL_ARRAY_BUFFER_ARB, m_vertHLCount * 4, m_VerticesHLCache, GL_STATIC_DRAW_ARB);

				glGetBufferParameterivARB( GL_ARRAY_BUFFER_ARB, GL_BUFFER_SIZE_ARB, &test );
			}

			if (m_connHLCount)
			{
				glGenBuffersARB(1, &m_ConnectorsHLVBOId);
				glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_ConnectorsHLVBOId);
				glBufferDataARB(GL_ARRAY_BUFFER_ARB, m_connHLCount * 3, m_ConnectorsHLCache, GL_STATIC_DRAW_ARB);

				glGetBufferParameterivARB( GL_ARRAY_BUFFER_ARB, GL_BUFFER_SIZE_ARB, &test );
			}
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		}
	}
#endif
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
			if (m_Vertices[count2 - 1] && m_Vertices[i])
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
			}
			count2--;
		}
		count++;
	}
}

void	CPieInternal::drawNewVertice(void) {
	bool	bTextured = false;

	if (!this->m_Visible) {
		return;
	}

	glDisable(GL_DEPTH_TEST);

	if (glIsEnabled(GL_TEXTURE_2D))
	{
		glDisable(GL_TEXTURE_2D);
		bTextured = true;
	}

	glColor4ub(255, 0, 0, 255);
	glBegin(GL_LINES);

	glVertex3f(m_newVerticeX + 0.3f, m_newVerticeY, m_newVerticeZ);
	glVertex3f(m_newVerticeX + 2.0f, m_newVerticeY, m_newVerticeZ);

	glVertex3f(m_newVerticeX, m_newVerticeY, m_newVerticeZ + 0.3f);
	glVertex3f(m_newVerticeX, m_newVerticeY, m_newVerticeZ + 2.0f);

	glVertex3f(m_newVerticeX - 0.3f, m_newVerticeY, m_newVerticeZ);
	glVertex3f(m_newVerticeX - 2.0f, m_newVerticeY, m_newVerticeZ);

	glVertex3f(m_newVerticeX, m_newVerticeY, m_newVerticeZ - 0.3f);
	glVertex3f(m_newVerticeX, m_newVerticeY, m_newVerticeZ - 2.0f);

	glVertex3f(m_newVerticeX, m_newVerticeY + 0.3f, m_newVerticeZ);
	glVertex3f(m_newVerticeX, m_newVerticeY + 2.0f, m_newVerticeZ);

	glVertex3f(m_newVerticeX, m_newVerticeY - 0.3f, m_newVerticeZ);
	glVertex3f(m_newVerticeX, m_newVerticeY - 2.0f, m_newVerticeZ);

	glEnd();

	glColor4ub(255, 255, 255, 255);

	if (bTextured)
	{
		glEnable(GL_TEXTURE_2D);
	}

	glEnable(GL_DEPTH_TEST);

	if (this->m_nextSub)
	{
		this->m_nextSub->drawNewVertice();
	}
}

void	CPieInternal::highLightVertices(void) {
    bool	bTextured = false;

	if (!this->m_vertCount ||
		!this->m_Visible) {
	    goto NextSubHighLightVertices;
	}

	glDisable(GL_DEPTH_TEST);

	if (glIsEnabled(GL_TEXTURE_2D))
	{
		glDisable(GL_TEXTURE_2D);
		bTextured = true;
	}

	glColor4ub(128, 0, 0, 255);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, this->m_VerticesHLCache);
	glDrawArrays(GL_QUADS, 0, m_vertHLCount * 4);
	//glDrawElements(GL_QUADS, m_vertHLCount, GL_FLOAT, this->m_VerticesHLCache);
	glDisableClientState(GL_VERTEX_ARRAY);

	/*
    Uint16	i;

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
	*/

	glColor4ub(255, 255, 255, 255);

	glEnable(GL_DEPTH_TEST);

	if (bTextured)
	{
		glEnable(GL_TEXTURE_2D);
	}

NextSubHighLightVertices:
	if (this->m_nextSub)
	{
		this->m_nextSub->highLightVertices();
	}
}

void	CPieInternal::highLightConnectors(void) {
	bool	bTextured = false;

	if (!this->m_connCount ||
		!this->m_Visible) {
	    goto NextSubHighLightConnectors;
	}

	glDisable(GL_DEPTH_TEST);

	if (glIsEnabled(GL_TEXTURE_2D))
	{
		glDisable(GL_TEXTURE_2D);
		bTextured = true;
	}

	glColor4ub(0, 128, 0, 255);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, this->m_ConnectorsHLCache);
	//glDrawElements(GL_TRIANGLES, m_connHLCount, GL_FLOAT, this->m_ConnectorsHLCache);
	glDrawArrays(GL_TRIANGLES, 0, m_connHLCount * 3);
	glDisableClientState(GL_VERTEX_ARRAY);

/*
	Uint16	i;

	for (i = 0;i < m_connCount;i++)
	{
		if (m_Connectors[i])
		{
			glBegin(GL_TRIANGLES);
			glVertex3f(m_Connectors[i]->connector.x, m_Connectors[i]->connector.z + 1.0f, m_Connectors[i]->connector.y);
			glVertex3f(m_Connectors[i]->connector.x + 1.0f, m_Connectors[i]->connector.z - 1.0f, m_Connectors[i]->connector.y);
			glVertex3f(m_Connectors[i]->connector.x - 1.0f, m_Connectors[i]->connector.z - 1.0f , m_Connectors[i]->connector.y);
			glEnd();
		}
	}
*/
	glColor4ub(255, 255, 255, 255);

	glEnable(GL_DEPTH_TEST);

	if (bTextured)
	{
		glEnable(GL_TEXTURE_2D);
	}

NextSubHighLightConnectors:
	if (this->m_nextSub)
	{
		this->m_nextSub->highLightConnectors();
	}
}

void	CPieInternal::highLightSelected(void) {
	Uint16	i;
    bool	bTextured = false;

	if (!this->m_Visible) {
		goto NextSubHighLightSelected;
	}

	glDisable(GL_DEPTH_TEST);

	if (glIsEnabled(GL_TEXTURE_2D))
	{
		glDisable(GL_TEXTURE_2D);
		bTextured = true;
	}

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

	if (bTextured)
	{
		glEnable(GL_TEXTURE_2D);
	}

NextSubHighLightSelected:
	if (this->m_nextSub)
	{
		this->m_nextSub->highLightSelected();
	}
}

void	CPieInternal::bindTexture(Sint32 num)
{
	glBindTexture(GL_TEXTURE_2D, _TEX_PAGE[num].id);
}

void	CPieInternal::logic(void) {
	this->constructSharedVerticeList();
	this->buildHLCache();

	Uint16	i;
	for (i = 0;i < m_numAnimFrames;i++)
	{
		if (m_frames[i].died)
		{
			this->removeFrame(i);
			break;
		}
	}

	if (this->m_nextSub)
	{
		this->m_nextSub->logic();
	}
}

void	CPieInternal::drawInterleaved(INTERLEAVED_T2F_V3F *a_array, Uint32 count) {
	glInterleavedArrays(GL_T2F_V3F, 0, a_array);
	glDrawArrays(GL_TRIANGLE_FAN, 0, count);
}

void	CPieInternal::cacheVBOPoly(Uint32 count, Uint32 polyId) {
	if (m_Polygons[polyId]->hasVBO)
	{
		g_Screen.glDeleteBuffersARB(1, &m_Polygons[polyId]->VBOId);
	}

	if (g_Screen.glGenBuffersARB == NULL)
	{
		g_Screen.glGenBuffersARB = (PFNGLGENBUFFERSARBPROC)SDL_GL_GetProcAddress("glGenBuffersARB");
	}

	g_Screen.glGenBuffersARB(1, &m_Polygons[polyId]->VBOId);
	g_Screen.glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_Polygons[polyId]->VBOId);
	g_Screen.glBufferDataARB(GL_ARRAY_BUFFER_ARB, count * sizeof(INTERLEAVED_T2F_V3F), vaCache, GL_STATIC_DRAW_ARB);
	g_Screen.glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

	m_Polygons[polyId]->hasVBO = true;
}

void	CPieInternal::drawVBOPoly(Uint32 polyId) {
	g_Screen.glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_Polygons[polyId]->VBOId);
	glTexCoordPointer(2, GL_FLOAT, sizeof(INTERLEAVED_T2F_V3F), (char*)0);
	glVertexPointer(3, GL_FLOAT, sizeof(INTERLEAVED_T2F_V3F), (char*)(0 + sizeof(Vector2f)));
	glDrawArrays(GL_TRIANGLE_FAN, 0, m_Polygons[polyId]->polygon.npnts);
	g_Screen.glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
}

void	CPieInternal::flushVBOPolys(void) {
	Uint32	i;
	for (i = 0;i < m_polyCount;i++)
	{
		if (m_Polygons[i])
		{
			if (m_Polygons[i]->hasVBO)
			{
				g_Screen.glDeleteBuffersARB(1, &m_Polygons[i]->VBOId);
				m_Polygons[i]->VBOId = 0xABABABAB;
				m_Polygons[i]->hasVBO = false;
			}
		}
	}

	if (this->m_nextSub)
	{
		this->m_nextSub->flushVBOPolys();
	}
}

void	CPieInternal::draw(void) {
	//iIMDPoly	*pPolys;
	int			i, n, vaCount = 0;
	VERTEXID	*index;
	bool		bVerticeDrawn[pie_MAX_VERTICES];
	bool		bDrawTexture = false;
	GLdouble	matrix[16];

	if (!m_Visible)
	{
		if (this->m_nextSub)
		{
			if (m_animMode == ANIM3DFRAMES)
			{
				if (!m_drawAnim && !m_IsSub)
				{
					CPieInternal	*temp = this->m_nextSub;
					while (temp)
					{
						temp->draw();
						temp = temp->m_nextSub;
					}
				}
			}
			else
			{
				this->m_nextSub->draw();
			}
		}
		return;
	}

	if (m_framesVisible)
	{
		Uint32	frame;
		for (frame = 0;frame < this->m_numAnimFrames;frame++)
		{
			if (m_frames[frame].visible)
			{
				glGetDoublev(GL_MODELVIEW_MATRIX, matrix);
				glTranslatef(m_frames[frame].offsetX / 1000.0f,
							m_frames[frame].offsetZ / 1000.0f,
							m_frames[frame].offsetY / 1000.0f);
				glRotatef(-WTFScale(m_frames[frame].rotateX) / 1000.0f, 1.0, 0.0f, 0.0f);
				glRotatef(-WTFScale(m_frames[frame].rotateZ) / 1000.0f, 0.0, 1.0f, 0.0f);
				glRotatef(-WTFScale(m_frames[frame].rotateY) / 1000.0f, 0.0, 0.0f, 1.0f);
				glScalef(m_frames[frame].scaleX / 1000.0f,
							m_frames[frame].scaleZ / 1000.0f,
							m_frames[frame].scaleY / 1000.0f);

				glEnableClientState(GL_VERTEX_ARRAY);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);

				for (i = 0;i < m_polyCount;i++)
				{
					//piePoly.flags = pPolys->flags;

					if (m_Polygons[i])
					{
						vaCount = 0;
						// No texture animation and has VBO
						if (g_Screen.m_useVBO && m_Polygons[i]->frame == 0 && m_Polygons[i]->hasVBO)
						{
							this->drawVBOPoly(i);
							continue;
						}

						for (n = 0, index = m_Polygons[i]->polygon.pindex;
								n < m_Polygons[i]->polygon.npnts;
								n++, index++)
						{
							if (m_Polygons[i]->frame > 0)
							{
//#define PROPER_TEXTURE_COORDS
#ifdef PROPER_TEXTURE_COORDS
								vaCache[vaCount].t.x = m_Polygons[i]->polygon.texCoord[n].x + (m_Polygons[i]->polygon.pTexAnim->textureWidth * m_Polygons[i]->frame)/_TEX_PAGE[m_TexpageId].w;
								vaCache[vaCount].t.y = m_Polygons[i]->polygon.texCoord[n].y + (m_Polygons[i]->polygon.pTexAnim->textureHeight * m_Polygons[i]->frame)/_TEX_PAGE[m_TexpageId].h;
#else
								vaCache[vaCount].t.x = m_Polygons[i]->polygon.texCoord[n].x + (m_Polygons[i]->polygon.pTexAnim->textureWidth * m_Polygons[i]->frame)/256.0f;
								vaCache[vaCount].t.y = m_Polygons[i]->polygon.texCoord[n].y + (m_Polygons[i]->polygon.pTexAnim->textureHeight * m_Polygons[i]->frame)/256.0f;
#endif
							}
							else
							{
//#define PROPER_TEXTURE_COORDS
#ifdef PROPER_TEXTURE_COORDS
								vaCache[vaCount].t.x = m_Polygons[i]->polygon.texCoord[n].x/_TEX_PAGE[m_TexpageId].w;
								vaCache[vaCount].t.y = m_Polygons[i]->polygon.texCoord[n].y/_TEX_PAGE[m_TexpageId].h;
#else
								vaCache[vaCount].t.x = m_Polygons[i]->polygon.texCoord[n].x/256.0f;
								vaCache[vaCount].t.y = m_Polygons[i]->polygon.texCoord[n].y/256.0f;
#endif
							}
							vaCache[vaCount].v.x = m_Vertices[*index]->vertice.x;
							vaCache[vaCount].v.y = m_Vertices[*index]->vertice.y;
							vaCache[vaCount].v.z = m_Vertices[*index]->vertice.z;
							vaCount++;
						}
						//TODO:flags
						if (!g_Screen.m_useVBO)
						{
							this->drawInterleaved(vaCache, vaCount);
						}
						else
						{
							this->cacheVBOPoly(vaCount, i);
							this->drawVBOPoly(i);
						}

#if 0
						glBegin(GL_TRIANGLE_FAN);
						for (n = 0, index = m_Polygons[i]->polygon.pindex;
								n < m_Polygons[i]->polygon.npnts;
								n++, index++)
						{
							if (m_Polygons[i]->frame > 0)
							{
//#define PROPER_TEXTURE_COORDS
#ifdef PROPER_TEXTURE_COORDS
								glTexCoord2f(m_Polygons[i]->polygon.texCoord[n].x + (m_Polygons[i]->polygon.pTexAnim->textureWidth * m_Polygons[i]->frame)/_TEX_PAGE[m_TexpageId].w,
												m_Polygons[i]->polygon.texCoord[n].y + (m_Polygons[i]->polygon.pTexAnim->textureHeight * m_Polygons[i]->frame)/_TEX_PAGE[m_TexpageId].h);
#else
								glTexCoord2f(m_Polygons[i]->polygon.texCoord[n].x + (m_Polygons[i]->polygon.pTexAnim->textureWidth * m_Polygons[i]->frame)/256.0f,
												m_Polygons[i]->polygon.texCoord[n].y + (m_Polygons[i]->polygon.pTexAnim->textureHeight * m_Polygons[i]->frame)/256.0f);
#endif
							}
							else
							{
//#define PROPER_TEXTURE_COORDS
#ifdef PROPER_TEXTURE_COORDS
								glTexCoord2f(m_Polygons[i]->polygon.texCoord[n].x/_TEX_PAGE[m_TexpageId].w,
												m_Polygons[i]->polygon.texCoord[n].y/_TEX_PAGE[m_TexpageId].h);
#else
								glTexCoord2f(m_Polygons[i]->polygon.texCoord[n].x/256.0f,
												m_Polygons[i]->polygon.texCoord[n].y/256.0f);
#endif
							}
							glVertex3f(m_Vertices[*index]->vertice.x,
										m_Vertices[*index]->vertice.y,
										m_Vertices[*index]->vertice.z);
						}
						glEnd();
#endif
					}
				}

				glDisableClientState(GL_VERTEX_ARRAY);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);

				glLoadMatrixd(matrix);
			}
		}
		//Dont draw base model as it's already drawn by frame 0
		return;
	}

	if (m_drawAnim)
	{
		if (m_animMode == ANIM3DTRANS && m_controlSubAnim)
		{
			CPieInternal	*temp = this->m_nextSub;
			while (temp)
			{
				temp->m_drawAnim = true;
				temp = temp->m_nextSub;
			}
		}

        this->bindTexture(m_TexpageId);

		startAnim();
		glGetDoublev(GL_MODELVIEW_MATRIX, matrix);
		glTranslatef(m_frames[m_currAnimFrame].offsetX / 1000.0f,
					m_frames[m_currAnimFrame].offsetZ / 1000.0f,
					m_frames[m_currAnimFrame].offsetY / 1000.0f);
		glRotatef(-WTFScale(m_frames[m_currAnimFrame].rotateX) / 1000.0f, 1.0, 0.0f, 0.0f);
		glRotatef(-WTFScale(m_frames[m_currAnimFrame].rotateZ) / 1000.0f, 0.0, 1.0f, 0.0f);
		glRotatef(-WTFScale(m_frames[m_currAnimFrame].rotateY) / 1000.0f, 0.0, 0.0f, 1.0f);
		glScalef(m_frames[m_currAnimFrame].scaleX / 1000.0f,
					m_frames[m_currAnimFrame].scaleZ / 1000.0f,
					m_frames[m_currAnimFrame].scaleY / 1000.0f);
	}
	else
	{
		if (m_animMode == ANIM3DTRANS && m_controlSubAnim)
		{
			CPieInternal	*temp = this->m_nextSub;
			while (temp)
			{
				temp->m_drawAnim = false;
				temp = temp->m_nextSub;
			}
		}
		stopAnim();
	}

	if (m_animMode == ANIM3DFRAMES && m_drawAnim && !m_IsSub)
	{
		Uint32	targetFrame = 0;
		CPieInternal	*temp = this;

		if (m_currAnimFrame)
		{
			while (targetFrame < m_currAnimFrame)
			{
				temp = temp->m_nextSub;
				targetFrame++;
				if (temp == NULL)
				{
					fprintf(stderr, "WARNING:frame %d doesn't exist anymore\n", targetFrame);
					return;
				}
			}
			temp->draw();
			return;
		}
	}

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


	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	for (i = 0;i < m_polyCount;i++)
	{
		//piePoly.flags = pPolys->flags;

		if (m_Polygons[i])
		{
			vaCount = 0;
			// No texture animation and has VBO
			if (g_Screen.m_useVBO && m_Polygons[i]->frame == 0 && m_Polygons[i]->hasVBO)
			{
				// Also make sure vertices drawn flag is set
				for (n = 0, index = m_Polygons[i]->polygon.pindex;
						n < m_Polygons[i]->polygon.npnts;
						n++, index++)
				{
					bVerticeDrawn[*index] = true;
				}
				this->drawVBOPoly(i);
				continue;
			}

			for (n = 0, index = m_Polygons[i]->polygon.pindex;
					n < m_Polygons[i]->polygon.npnts;
					n++, index++)
			{
				if (m_Polygons[i]->frame > 0)
				{
//#define PROPER_TEXTURE_COORDS
#ifdef PROPER_TEXTURE_COORDS
					vaCache[vaCount].t.x = m_Polygons[i]->polygon.texCoord[n].x + (m_Polygons[i]->polygon.pTexAnim->textureWidth * m_Polygons[i]->frame)/_TEX_PAGE[m_TexpageId].w,
					vaCache[vaCount].t.y = m_Polygons[i]->polygon.texCoord[n].y + (m_Polygons[i]->polygon.pTexAnim->textureHeight * m_Polygons[i]->frame)/_TEX_PAGE[m_TexpageId].h);
#else
					vaCache[vaCount].t.x = m_Polygons[i]->polygon.texCoord[n].x + (m_Polygons[i]->polygon.pTexAnim->textureWidth * m_Polygons[i]->frame)/256.0f,
					vaCache[vaCount].t.y = m_Polygons[i]->polygon.texCoord[n].y + (m_Polygons[i]->polygon.pTexAnim->textureHeight * m_Polygons[i]->frame)/256.0f;
#endif
				}
				else
				{
//#define PROPER_TEXTURE_COORDS
#ifdef PROPER_TEXTURE_COORDS
					vaCache[vaCount].t.x = m_Polygons[i]->polygon.texCoord[n].x/_TEX_PAGE[m_TexpageId].w,
					vaCache[vaCount].t.y = m_Polygons[i]->polygon.texCoord[n].y/_TEX_PAGE[m_TexpageId].h);
#else
					vaCache[vaCount].t.x = m_Polygons[i]->polygon.texCoord[n].x/256.0f,
					vaCache[vaCount].t.y = m_Polygons[i]->polygon.texCoord[n].y/256.0f;
				}
#endif
				vaCache[vaCount].v.x = m_Vertices[*index]->vertice.x;
				vaCache[vaCount].v.y = m_Vertices[*index]->vertice.y;
				vaCache[vaCount].v.z = m_Vertices[*index]->vertice.z;
				vaCount++;

				bVerticeDrawn[*index] = true;
			}
			//TODO:flags
			if (!g_Screen.m_useVBO)
			{
				this->drawInterleaved(vaCache, vaCount);
			}
			else
			{
				this->cacheVBOPoly(vaCount, i);
				this->drawVBOPoly(i);
			}
		}
	}
#if 0
			glBegin(GL_TRIANGLE_FAN);
			for (n = 0, index = m_Polygons[i]->polygon.pindex;
					n < m_Polygons[i]->polygon.npnts;
					n++, index++)
			{
				if (m_Polygons[i]->frame > 0)
				{
//#define PROPER_TEXTURE_COORDS
#ifdef PROPER_TEXTURE_COORDS
					glTexCoord2f(m_Polygons[i]->polygon.texCoord[n].x/_TEX_PAGE[m_TexpageId].w,
								m_Polygons[i]->polygon.texCoord[n].y/_TEX_PAGE[m_TexpageId].h);
#else
					glTexCoord2f(m_Polygons[i]->polygon.texCoord[n].x + (m_Polygons[i]->polygon.pTexAnim->textureWidth * m_Polygons[i]->frame)/256.0f,
								m_Polygons[i]->polygon.texCoord[n].y + (m_Polygons[i]->polygon.pTexAnim->textureHeight * m_Polygons[i]->frame)/256.0f);
#endif
				}
				else
				{
//#define PROPER_TEXTURE_COORDS
#ifdef PROPER_TEXTURE_COORDS
					glTexCoord2f(m_Polygons[i]->polygon.texCoord[n].x/_TEX_PAGE[m_TexpageId].w,
									m_Polygons[i]->polygon.texCoord[n].y/_TEX_PAGE[m_TexpageId].h);
#else
					glTexCoord2f(m_Polygons[i]->polygon.texCoord[n].x/256.0f,
					m_Polygons[i]->polygon.texCoord[n].y/256.0f);
				}
#endif

				glVertex3f(m_Vertices[*index]->vertice.x,
							m_Vertices[*index]->vertice.y,
							m_Vertices[*index]->vertice.z);

				bVerticeDrawn[*index] = true;
			}
			glEnd();
#endif

	glDisable(GL_DEPTH_TEST);
	glColor4ub(32, 0, 128, 255);
	for (i = 0;i < m_polyCount;i++)
	{
		if (m_Polygons[i])
		{
			for (n = 0, index = m_Polygons[i]->polygon.pindex;
					n < m_Polygons[i]->polygon.npnts;
					n++, index++)
			{
				if (m_Polygons[i]->selected)
				{
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
				}
			}
		}
	}
	glColor4ub(255, 255, 255, 255);
	glEnable(GL_DEPTH_TEST);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	if (glIsEnabled(GL_TEXTURE_2D))
	{
		glDisable(GL_TEXTURE_2D);
		bDrawTexture = true;
	}

	for (i = 0;i < m_vertCount;i++)
	{
		if (m_Vertices[i] && !bVerticeDrawn[i])
		{
			glBegin(GL_POINTS);
			glVertex3f(m_Vertices[i]->vertice.x, m_Vertices[i]->vertice.y, m_Vertices[i]->vertice.z);
			glEnd();
		}
	}

	if (bDrawTexture)
	{
		glEnable(GL_TEXTURE_2D);
	}

	if (m_drawAnim)
	{
		glLoadMatrixd(matrix);
	}

	if (this->m_nextSub)
	{
		if (m_animMode == ANIM3DFRAMES)
		{
			if (!m_drawAnim && !m_IsSub)
			{
				CPieInternal	*temp = this->m_nextSub;
				while (temp)
				{
					temp->draw();
					temp = temp->m_nextSub;
				}
			}
		}
		else
		{
			this->m_nextSub->draw();
		}
	}
}

bool CPieInternal::addGUI(void) {
	Uint16		i, n, totalVertices = 0;
	VERTEXID	*index;

	char	toolBarName[255] = "toolbar";
	char	toolBarDefine[255];

	//Utility bar
	if (!m_IsSub)
	{

		if (this->uid < 10)
		{
			snprintf(&toolBarName[7], 1, "0");
			snprintf(&toolBarName[8], 1, "%u", this->uid);
			strcpy(toolBarDefine, toolBarName);
			snprintf(&toolBarDefine[9], 100, " label = 'Pie %u ToolBar' position = '0 200' color = '%d %d %d %d' size = '%d %d'",
						this->uid, guiMajorAlpha, guiMajorRed, guiMajorGreen, guiMajorBlue, guiMajorWidth, guiMajorHeight);
		}
		else if (this->uid < 100)
		{
			snprintf(&toolBarName[7], 2, "%2u", this->uid);
			strcpy(toolBarDefine, toolBarName);
			snprintf(&toolBarDefine[9], 100, " label = 'Pie %u ToolBar' position = '0 200' color = '%d %d %d %d' size = '%d %d'",
						this->uid, guiMajorAlpha, guiMajorRed, guiMajorGreen, guiMajorBlue, guiMajorWidth, guiMajorHeight);
		}
		else
		{
			snprintf(&toolBarName[7], 3, "%3u", this->uid);
			strcpy(toolBarDefine, toolBarName);
			snprintf(&toolBarDefine[10], 100, " label = 'Pie %u ToolBar' position = '0 200' color = '%d %d %d %d' size = '%d %d'",
						this->uid, guiMajorAlpha, guiMajorRed, guiMajorGreen, guiMajorBlue, guiMajorWidth, guiMajorHeight);
		}
	}
	else
	{
			snprintf(toolBarName, 255, "SubModel%4u", this->uid);
			snprintf(toolBarDefine, 255, "%s label = 'Pie %u ToolBar' position = '0 200' color = '%d %d %d %d' size = '%d %d'",
						toolBarName, this->uid, guiMajorAlpha, guiMajorRed, guiMajorGreen, guiMajorBlue, guiMajorWidth, guiMajorHeight);
	}

	m_toolBar = TwNewBar(toolBarName);

	TwDefine(toolBarDefine);

	TwAddButton(m_toolBar, "saveNow", saveNowCB, &m_InstanceCallback, "label = 'Save Pie' group = 'General'");
	TwAddButton(m_toolBar, "removepie", destructCB, &m_InstanceCallback, "label = 'Remove Pie' group = 'General'");
	TwAddVarRW(m_toolBar, "visible", TW_TYPE_BOOLCPP, &m_Visible, "label = 'Visible' group = 'General'");
	TwAddVarRW(m_toolBar, "textureId", TW_TYPE_INT32, &m_TexpageId, "label = 'Page Id' min=0 max=100 step=1 group = 'General'");
	TwAddButton(m_toolBar, "showgui", showGUICB, &m_InstanceCallback, "label = 'Show GUI' group = 'General'");
	TwAddButton(m_toolBar, "hidegui", hideGUICB, &m_InstanceCallback, "label = 'Hide GUI' group = 'General'");
	// Only non sub model can do this
	if (!this->m_IsSub)
	{
		TwAddButton(m_toolBar, "addSubModel", addSubModelCB, &m_InstanceCallback, "label = 'Add SubModel' group = 'SubModel and Animation'");
		TwAddButton(m_toolBar, "addSubModelFile", addSubModelFileCB, &m_InstanceCallback, "label = 'SubModel From File' group = 'SubModel and Animation'");
		TwAddButton(m_toolBar, "readAnimFile", readAnimFileCB, &m_InstanceCallback, "label = 'Anim From File' group = 'SubModel and Animation'");
		TwAddButton(m_toolBar, "writeAnimFile", writeAnimFileCB, &m_InstanceCallback, "label = 'Write Anim File' group = 'SubModel and Animation'");
        TwAddVarRW(m_toolBar, "frameSpeed", TW_TYPE_INT32, &m_frameInterval, "label = 'Frame Speed' min=0 max=1000 step=1 group = 'SubModel and Animation'");
		TwAddVarRW(m_toolBar, "animMode", TW_TYPE_INT32, &m_animMode, "label = 'Anim Mode' min=0 max=1 step=1 group = 'Anim Mode'");
	}
	TwAddButton(m_toolBar, "addFrame", addFrameCB, &m_InstanceCallback, "label = 'Add Anim Frame' group = 'SubModel and Animation'");
	TwAddVarRW(m_toolBar, "controlSubAnim", TW_TYPE_BOOLCPP, &m_controlSubAnim, "label = 'Control SubModel Anim' group = 'SubModel and Animation'");
	TwAddVarRW(m_toolBar, "drawAnim", TW_TYPE_BOOLCPP, &m_drawAnim, "label = 'Draw Anim' group = 'SubModel and Animation'");
	TwAddVarRW(m_toolBar, "framesVisible", TW_TYPE_BOOLCPP, &m_framesVisible, "label = 'Frames Visible' group = 'SubModel and Animation'");

	TwAddButton(m_toolBar, "selectAll", selectAllCB, &m_InstanceCallback, "label = 'Select All' group = 'Selection and Move'");
	TwAddButton(m_toolBar, "unselectAll", unselectAllCB, &m_InstanceCallback, "label = 'Unselect All' group = 'Selection and Move'");
	TwAddButton(m_toolBar, "moveVertices", moveSelectedVertices, &m_InstanceCallback, "label = 'Move Selected' group = 'Selection and Move'");
	TwAddVarRW(m_toolBar, "VelocityX", TW_TYPE_FLOAT, &m_VelocityX, "label = 'VelocityX' step = 0.1 group = 'Selection and Move'");
	TwAddVarRW(m_toolBar, "VelocityY", TW_TYPE_FLOAT, &m_VelocityY, "label = 'VelocityY' step = 0.1 group = 'Selection and Move'");
	TwAddVarRW(m_toolBar, "VelocityZ", TW_TYPE_FLOAT, &m_VelocityZ, "label = 'VelocityZ' step = 0.1 group = 'Selection and Move'");
	TwAddButton(m_toolBar, "removeVertices", removeSelectedCB, &m_InstanceCallback, "label = 'Remove Selected' group = 'Selection and Move'");
	TwAddVarRW(m_toolBar, "DuplicateX", TW_TYPE_FLOAT, &m_duplicateX, "label = 'DuplicateX' step = 0.1 group = 'Selection and Move'");
	TwAddVarRW(m_toolBar, "DuplicateY", TW_TYPE_FLOAT, &m_duplicateY, "label = 'DuplicateY' step = 0.1 group = 'Selection and Move'");
	TwAddVarRW(m_toolBar, "DuplicateZ", TW_TYPE_FLOAT, &m_duplicateZ, "label = 'DuplicateZ' step = 0.1 group = 'Selection and Move'");
	TwAddButton(m_toolBar, "duplicateVertices", duplicateSelectedVertices, &m_InstanceCallback, "label = 'Duplicate Selected' group = 'Selection and Move'");

	TwAddVarRW(m_toolBar, "newVerticeX", TW_TYPE_FLOAT, &m_newVerticeX, "label = 'New VerticeX' step = 0.1 group = 'New Vertice'");
	TwAddVarRW(m_toolBar, "newVerticeY", TW_TYPE_FLOAT, &m_newVerticeY, "label = 'New VerticeY' step = 0.1 group = 'New Vertice'");
	TwAddVarRW(m_toolBar, "newVerticeZ", TW_TYPE_FLOAT, &m_newVerticeZ, "label = 'New VerticeZ' step = 0.1 group = 'New Vertice'");
	TwAddButton(m_toolBar, "addVertice", addVerticeCB, &m_AddVerticeCB, "label = 'Add Vertice' group = 'New Vertice'");
	TwAddButton(m_toolBar, "addConnector", addVerticeCB, &m_AddConnectorCB, "label = 'Add Connector' group = 'New Vertice'");




	//Vertices List bar
	char	vertBarName[255];
	char	vertBarDefine[255];

	strncpy(vertBarName, toolBarName, 255);
	strcat(vertBarName, "vertBar");

	snprintf(vertBarDefine, 255, "%s label = 'Pie %u VerticeList' position = '%d %d' color = '%d %d %d %d' size = '%d %d'",
			vertBarName, this->uid, g_Screen.m_width, 0, guiMajorAlpha, guiMajorRed,
			guiMajorGreen, guiMajorBlue, guiMajorWidth, guiMajorHeight);

	m_vertBar = TwNewBar(vertBarName);
	TwDefine(vertBarDefine);

	for (i = 0;i < m_vertCount;i++)
	{
		if (m_Vertices[i])
		{
			char	name[255] = "vert";
			snprintf(&name[4], 3, "%3u", i);
			TwAddVarRW(m_vertBar, name, g_tw_pieVertexType, m_Vertices[i], "");
			//TwAddVarCB(m_vertBar, name, g_tw_pieVertexType, setVarCB, getVarCB, &this->m_InstanceCallback, "");
		}
	}

	//Connector list bar
	char	connBarName[255];
	char	connBarDefine[255];

	strncpy(connBarName, toolBarName, 255);
	strcat(connBarName, "connBar");

	snprintf(connBarDefine, 255, "%s label = 'Pie %u ConnectorList' position = '%d %d' color = '%d %d %d %d' size = '%d %d'",
			connBarName, this->uid, g_Screen.m_width, g_Screen.m_height, guiMajorAlpha, guiMajorRed,
			guiMajorGreen, guiMajorBlue, guiMajorWidth, guiMajorHeight);

	m_connBar = TwNewBar(connBarName);
	TwDefine(connBarDefine);

	for (i = 0;i < m_connCount;i++)
	{
		if (m_Connectors[i])
		{
			char	name[255] = "conn";
			char	name2[255] = "delconn";
			snprintf(&name[4], 3, "%3u", i);
			snprintf(&name2[7], 3, "%3u", i);
			TwAddButton(m_connBar, name2, removeConnectorAtCB, &m_Connectors[i]->callback, "label = 'Remove'");
			TwAddVarRW(m_connBar, name, g_tw_pieVertexType, m_Connectors[i], "");
			//TwAddVarCB(m_connBar, name, g_tw_pieVertexType, setVarCB, getVarCB, &this->m_InstanceCallback, "");
		}
	}

	for (i = 0;i < m_polyCount;i++)
	{
		//multiple polygon names format poly[pie uid][poly uid]
		char	tmpString[255] = "poly";
		char	tmpString2[255];

		if (!this->m_IsSub)
		{
			//FIXME:capped to 1000(3 digi)
			//assert(this->uid < 1000);
			//utterly hack to add zero's to make Bar names unique -_-
			if (i < 10)
			{
				snprintf(&tmpString[4], 1, "0");
				snprintf(&tmpString[5], 1, "%u", i);

				if (this->uid < 10)
				{
					snprintf(&tmpString[6], 1, "0");
					snprintf(&tmpString[7], 1, "%u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[8], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
						this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
				}
				else if (this->uid < 100)
				{
					snprintf(&tmpString[6], 2, "%2u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[8], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
						this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
				}
				else
				{
					snprintf(&tmpString[6], 3, "3%u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[9], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
						this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
				}

				strcpy(tmpString2, tmpString);
				snprintf(&tmpString2[8], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
						this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
			}
			else if (i < 100)
			{
				snprintf(&tmpString[4], 2, "%2u", i);

				if (this->uid < 10)
				{
					snprintf(&tmpString[6], 1, "0");
					snprintf(&tmpString[7], 1, "%u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[8], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
						this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
				}
				else if (this->uid < 100)
				{
					snprintf(&tmpString[6], 2, "%2u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[8], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
						this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
				}
				else
				{
					snprintf(&tmpString[6], 3, "3%u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[9], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
						this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
				}
			}
			else
			{
				snprintf(&tmpString[4], 3, "%3u", i);

				if (this->uid < 10)
				{
					snprintf(&tmpString[7], 1, "0");
					snprintf(&tmpString[8], 1, "%u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[9], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
						this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
				}
				else if (this->uid < 100)
				{
					snprintf(&tmpString[7], 2, "%2u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[9], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
						this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
				}
				else
				{
					snprintf(&tmpString[7], 3, "3%u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[10], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
						this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
				}
			}
		}
		else
		{
			snprintf(tmpString, 255, "SubModel%dPoly%d", this->uid, i);
			snprintf(tmpString2, 255, "%s label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
						tmpString, this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
		}

		m_polyBars[i] = TwNewBar(tmpString);

		TwDefine(tmpString2);

		TwAddButton(m_polyBars[i], "duplicatePoly", duplicatePolyCB, &m_Polygons[i]->callback, "label = 'Duplicate this'");
		TwAddVarRW(m_polyBars[i], "polyselected", TW_TYPE_BOOLCPP, &m_Polygons[i]->selected, "label = 'Poly Selected'");
		TwAddVarRW(m_polyBars[i], "flags", TW_TYPE_UINT32, &m_Polygons[i]->polygon.flags, "label = 'Flags'");
		if (m_Polygons[i]->polygon.pTexAnim)
		{
			TwAddVarRW(m_polyBars[i], "texAnimFrame", TW_TYPE_UINT16, &m_Polygons[i]->frame, "label = 'TexAnim Frame' min = 0 max = 7 step = 1 group = 'TexAnim'");
			TwAddVarRW(m_polyBars[i], "texHeight", TW_TYPE_UINT16, &m_Polygons[i]->polygon.pTexAnim->textureHeight, "label = 'Height' min = 0 max = 31 step = 1 group = 'TexAnim'");
			TwAddVarRW(m_polyBars[i], "texWidth", TW_TYPE_UINT16, &m_Polygons[i]->polygon.pTexAnim->textureWidth, "label = 'Width' min = 0 max = 31 step = 1 group = 'TexAnim'");
			TwAddVarRW(m_polyBars[i], "textPlayRate", TW_TYPE_UINT16, &m_Polygons[i]->polygon.pTexAnim->playbackRate, "label = 'Height' min = 0  step = 1 group = 'TexAnim'");
			TwAddVarRW(m_polyBars[i], "texFrames", TW_TYPE_UINT16, &m_Polygons[i]->polygon.pTexAnim->nFrames, "label = 'Width' min = 0 max = 7 step = 1 group = 'TexAnim'");
		}

		for (n = m_Polygons[i]->polygon.npnts, index = m_Polygons[i]->polygon.pindex + (m_Polygons[i]->polygon.npnts - 1);
				n > 0;
				n--, index--)
		{
			char	stringTemp[255] = "vert";
			char	stringTemp2[255] = "delvert";

			snprintf(&stringTemp[4], 55, " id%3u uid%3u", *index, totalVertices++);

			snprintf(&stringTemp2[7], 55, " id%3u uid%3u", *index, totalVertices);

			TwAddButton(m_polyBars[i], stringTemp2, removeVerticeAtCB, &m_Vertices[*index]->callback, "label = 'Remove'");
			TwAddVarRW(m_polyBars[i], stringTemp, g_tw_pieVertexType, m_Vertices[*index], "");
			//TwAddVarCB(m_polyBars[i], stringTemp, g_tw_pieVertexType, setVarCB, getVarCB, &this->m_InstanceCallback, "");
#if 0
			char	stringX[255] = "verticeX";
			char	stringY[255] = "verticeY";
			char	stringZ[255] = "verticeZ";
			char	stringBool[255] = "bool";

			snprintf(&stringX[8], 9, "vert%3u", ++totalVertices);
			snprintf(&stringY[8], 9, "vert%3u", ++totalVertices);
			snprintf(&stringZ[8], 9, "vert%3u", ++totalVertices);
			snprintf(&stringBool[4], 9, "bool%3u", totalVertices);

			TwAddVarRW(m_polyBars[i], stringX, TW_TYPE_FLOAT, &m_Vertices[*index]->vertice.x, "label = 'Vertice X' step = 0.1");
			TwAddVarRW(m_polyBars[i], stringY, TW_TYPE_FLOAT, &m_Vertices[*index]->vertice.y, "label = 'Vertice Y' step = 0.1");
			TwAddVarRW(m_polyBars[i], stringZ, TW_TYPE_FLOAT, &m_Vertices[*index]->vertice.z, "label = 'Vertice Z' step = 0.1");
			TwAddVarRW(m_polyBars[i], stringBool, TW_TYPE_BOOLCPP, &m_Vertices[*index]->selected, "label = 'Selected'");
#endif
		}
	}

	//Animation Frame list bar
	char	animBarName[255];

	strncpy(animBarName, toolBarName, 255);
	strcat(animBarName, "animBar");


	for (i = 0;i < m_numAnimFrames;i++)
	{
		char	frameName[255];
		char	animBarDefine[255];

		snprintf(frameName, 255, "%sframe%d", animBarName, i);
		TwDeleteBar(m_frames[i].Bar);
		m_frames[i].Bar = TwNewBar(frameName);

		strncpy(m_frames[i].BarName, frameName, 255);

		snprintf(animBarDefine, 255, "%s label = 'Pie%u Frame%d' position = '%d %d' color = '%d %d %d %d' size = '%d %d'",
				frameName, this->uid, i, g_Screen.m_width, g_Screen.m_height, guiMajorAlpha, guiMajorRed,
				guiMajorGreen, guiMajorBlue, guiMajorWidth, guiMajorHeight);

		TwDefine(animBarDefine);

		strncpy(m_frames[i].BarName, frameName, 255);

		TwAddButton(m_frames[i].Bar, "removeFrame", removeFrameCB, &m_frames[i], "label = 'Remove Anim Frame'");
		TwAddVarRW(m_frames[i].Bar, "visible", TW_TYPE_BOOLCPP, &m_frames[i].visible, "label = 'Visible'");
		TwAddVarRW(m_frames[i].Bar, "id", TW_TYPE_UINT32, &m_frames[i].frameId, "label = 'FrameId'");
		TwAddVarRW(m_frames[i].Bar, "offsetX", TW_TYPE_FLOAT, &m_frames[i].offsetX, "label = 'OffsetX' step = 100");
		TwAddVarRW(m_frames[i].Bar, "offsetY", TW_TYPE_FLOAT, &m_frames[i].offsetY, "label = 'OffsetY' step = 100");
		TwAddVarRW(m_frames[i].Bar, "offsetZ", TW_TYPE_FLOAT, &m_frames[i].offsetZ, "label = 'OffsetZ' step = 100");
		TwAddVarRW(m_frames[i].Bar, "rotateX", TW_TYPE_FLOAT, &m_frames[i].rotateX, "label = 'rotateX' step = 100");
		TwAddVarRW(m_frames[i].Bar, "rotateY", TW_TYPE_FLOAT, &m_frames[i].rotateY, "label = 'rotateY' step = 100");
		TwAddVarRW(m_frames[i].Bar, "rotateZ", TW_TYPE_FLOAT, &m_frames[i].rotateZ, "label = 'rotateZ' step = 100");
		TwAddVarRW(m_frames[i].Bar, "scaleX", TW_TYPE_FLOAT, &m_frames[i].scaleX, "label = 'ScaleX' step = 10");
		TwAddVarRW(m_frames[i].Bar, "scaleY", TW_TYPE_FLOAT, &m_frames[i].scaleY, "label = 'ScaleY' step = 10");
		TwAddVarRW(m_frames[i].Bar, "scaleZ", TW_TYPE_FLOAT, &m_frames[i].scaleZ, "label = 'ScaleZ' step = 10");
	}

	return true;
}

void	CPieInternal::updateGUI(void) {
	Uint16		i, n, totalVertices = 0;
	VERTEXID	*index;

	//Utility bar
	char	toolBarName[255] = "toolbar";
	char	toolBarDefine[255];

		//Utility bar
	if (!m_IsSub)
	{

		if (this->uid < 10)
		{
			snprintf(&toolBarName[7], 1, "0");
			snprintf(&toolBarName[8], 1, "%u", this->uid);
			strcpy(toolBarDefine, toolBarName);
			snprintf(&toolBarDefine[9], 100, " label = 'Pie %u ToolBar' position = '0 200' color = '%d %d %d %d' size = '%d %d'",
						this->uid, guiMajorAlpha, guiMajorRed, guiMajorGreen, guiMajorBlue, guiMajorWidth, guiMajorHeight);
		}
		else if (this->uid < 100)
		{
			snprintf(&toolBarName[7], 2, "%2u", this->uid);
			strcpy(toolBarDefine, toolBarName);
			snprintf(&toolBarDefine[9], 100, " label = 'Pie %u ToolBar' position = '0 200' color = '%d %d %d %d' size = '%d %d'",
						this->uid, guiMajorAlpha, guiMajorRed, guiMajorGreen, guiMajorBlue, guiMajorWidth, guiMajorHeight);
		}
		else
		{
			snprintf(&toolBarName[7], 3, "%3u", this->uid);
			strcpy(toolBarDefine, toolBarName);
			snprintf(&toolBarDefine[10], 100, " label = 'Pie %u ToolBar' position = '0 200' color = '%d %d %d %d' size = '%d %d'",
						this->uid, guiMajorAlpha, guiMajorRed, guiMajorGreen, guiMajorBlue, guiMajorWidth, guiMajorHeight);
		}
	}
	else
	{
			snprintf(toolBarName, 255, "SubModel%4u", this->uid);
			snprintf(toolBarDefine, 255, "%s label = 'Pie %u ToolBar' position = '0 200' color = '%d %d %d %d' size = '%d %d'",
						toolBarName, this->uid, guiMajorAlpha, guiMajorRed, guiMajorGreen, guiMajorBlue, guiMajorWidth, guiMajorHeight);
	}

	//Vertice list bar
	char	vertBarName[255];
	char	vertBarDefine[255];

	strncpy(vertBarName, toolBarName, 255);
	strcat(vertBarName, "vertBar");

	snprintf(vertBarDefine, 255, "%s label = 'Pie %u VerticeList' position = '%d %d' color = '%d %d %d %d' size = '%d %d'",
			vertBarName, this->uid, g_Screen.m_width, 0, guiMajorAlpha, guiMajorRed,
			guiMajorGreen, guiMajorBlue, guiMajorWidth, guiMajorHeight);

	//TwDeleteBar(m_vertBar);
	if (!m_vertBar)
	{
		m_vertBar = TwNewBar(vertBarName);
		TwDefine(vertBarDefine);
	}

	TwRemoveAllVars(m_vertBar);

	for (i = 0;i < m_vertCount;i++)
	{
		if (m_Vertices[i])
		{
			char	name[255] = "vert";
			snprintf(&name[4], 3, "%3u", i);
			if (!TwAddVarRW(m_vertBar, name, g_tw_pieVertexType, m_Vertices[i], ""))
				//|| !TwAddVarCB(m_vertBar, name, g_tw_pieVertexType, setVarCB, getVarCB, &this->m_InstanceCallback, ""))
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

	snprintf(connBarDefine, 255, "%s label = 'Pie %u ConnectorList' position = '%d %d' color = '%d %d %d %d' size = '%d %d'",
			connBarName, this->uid, g_Screen.m_width, 400, guiMajorAlpha, guiMajorRed,
			guiMajorGreen, guiMajorBlue, guiMajorWidth, guiMajorHeight);

	//TwDeleteBar(m_connBar);
	if (!m_connBar)
	{
		m_connBar = TwNewBar(connBarName);
		TwDefine(connBarDefine);
	}

	TwRemoveAllVars(m_connBar);

	for (i = 0;i < m_connCount;i++)
	{
		if (m_Connectors[i])
		{
			char	name[255] = "conn";
			char	name2[255] = "delconn";
			snprintf(&name[4], 3, "%3u", i);
			snprintf(&name2[7], 3, "%3u", i);
			TwAddButton(m_connBar, name2, removeConnectorAtCB, &m_Connectors[i]->callback, "label = 'Remove'");
			TwAddVarRW(m_connBar, name, g_tw_pieVertexType, m_Connectors[i], "");
			//TwAddVarCB(m_connBar, name, g_tw_pieVertexType, setVarCB, getVarCB, &this->m_InstanceCallback, "");
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
		if (!this->m_IsSub)
		{
			if (i < 10)
			{
				snprintf(&tmpString[4], 1, "0");
				snprintf(&tmpString[5], 1, "%u", i);

				if (this->uid < 10)
				{
					snprintf(&tmpString[6], 1, "0");
					snprintf(&tmpString[7], 1, "%u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[8], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
						this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
				}
				else if (this->uid < 100)
				{
					snprintf(&tmpString[6], 2, "%2u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[8], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
						this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
				}
				else
				{
					snprintf(&tmpString[6], 3, "3%u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[9], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
						this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
				}

				strcpy(tmpString2, tmpString);
				snprintf(&tmpString2[8], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
						this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
			}
			else if (i < 100)
			{
				snprintf(&tmpString[4], 2, "%2u", i);

				if (this->uid < 10)
				{
					snprintf(&tmpString[6], 1, "0");
					snprintf(&tmpString[7], 1, "%u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[8], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
						this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
				}
				else if (this->uid < 100)
				{
					snprintf(&tmpString[6], 2, "%2u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[8], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
						this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
				}
				else
				{
					snprintf(&tmpString[6], 3, "3%u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[9], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
						this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
				}
			}
			else
			{
				snprintf(&tmpString[4], 3, "%3u", i);

				if (this->uid < 10)
				{
					snprintf(&tmpString[7], 1, "0");
					snprintf(&tmpString[8], 1, "%u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[9], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
						this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
				}
				else if (this->uid < 100)
				{
					snprintf(&tmpString[7], 2, "%2u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[9], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
						this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
				}
				else
				{
					snprintf(&tmpString[7], 3, "3%u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[10], 100, " label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
						this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
				}
			}
		}
		else
		{
			snprintf(tmpString, 255, "SubModel%dPoly%d", this->uid, i);
			snprintf(tmpString2, 255, "%s label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
						tmpString, this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
		}

		//TwDeleteBar(m_polyBars[i]);
		if (!m_polyBars[i])
		{
			m_polyBars[i] = TwNewBar(tmpString);

			TwDefine(tmpString2);
		}

		TwRemoveAllVars(m_polyBars[i]);

		TwAddButton(m_polyBars[i], "duplicatePoly", duplicatePolyCB, &m_Polygons[i]->callback, "label = 'Duplicate this'");
		TwAddVarRW(m_polyBars[i], "polyselected", TW_TYPE_BOOLCPP, &m_Polygons[i]->selected, "label = 'Poly Selected'");
		TwAddVarRW(m_polyBars[i], "flags", TW_TYPE_UINT32, &m_Polygons[i]->polygon.flags, "label = 'Flags'");
		if (m_Polygons[i]->polygon.pTexAnim)
		{
			TwAddVarRW(m_polyBars[i], "texAnimFrame", TW_TYPE_UINT32, &m_Polygons[i]->frame, "label = 'TexAnim Frame' min = 0 max = 7 step = 1 group = 'TexAnim'");
			TwAddVarRW(m_polyBars[i], "texHeight", TW_TYPE_UINT16, &m_Polygons[i]->polygon.pTexAnim->textureHeight, "label = 'Height' min = 0 max = 31 step = 1 group = 'TexAnim'");
			TwAddVarRW(m_polyBars[i], "texWidth", TW_TYPE_UINT16, &m_Polygons[i]->polygon.pTexAnim->textureWidth, "label = 'Width' min = 0 max = 31 step = 1 group = 'TexAnim'");
			TwAddVarRW(m_polyBars[i], "textPlayRate", TW_TYPE_UINT16, &m_Polygons[i]->polygon.pTexAnim->playbackRate, "label = 'Height' min = 0  step = 1 group = 'TexAnim'");
			TwAddVarRW(m_polyBars[i], "texFrames", TW_TYPE_UINT16, &m_Polygons[i]->polygon.pTexAnim->nFrames, "label = 'Width' min = 0 max = 7 step = 1 group = 'TexAnim'");
		}


		for (n = m_Polygons[i]->polygon.npnts, index = m_Polygons[i]->polygon.pindex + (m_Polygons[i]->polygon.npnts - 1);
				n > 0;
				n--, index--)
		{
			char	stringTemp[255] = "vert";
			char	stringTemp2[255] = "delvert";

			snprintf(&stringTemp[4], 55, " id%3u uid%3u", *index, totalVertices++);

			snprintf(&stringTemp2[7], 55, " id%3u uid%3u", *index, totalVertices);

			TwAddButton(m_polyBars[i], stringTemp2, removeVerticeAtCB, &m_Vertices[*index]->callback, "label = 'Remove'");
			TwAddVarRW(m_polyBars[i], stringTemp, g_tw_pieVertexType, m_Vertices[*index], "");
			//TwAddVarCB(m_polyBars[i], stringTemp, g_tw_pieVertexType, setVarCB, getVarCB, &this->m_InstanceCallback, "");
		}
	}

	//Animation Frame list bar
	char	animBarName[255];

	strncpy(animBarName, toolBarName, 255);
	strcat(animBarName, "animBar");

	for (i = 0;i < m_numAnimFrames;i++)
	{
		char	frameName[255];
		char	animBarDefine[255];

		snprintf(frameName, 255, "%sframe%d", animBarName, i);
		//TwDeleteBar(m_frames[i].Bar);
		if (!m_frames[i].Bar)
		{
			m_frames[i].Bar = TwNewBar(m_frames[i].BarName);

			//strncpy(m_frames[i].BarName, frameName, 255);

			snprintf(animBarDefine, 255, "%s label = 'Pie%u Frame%d' position = '%d %d' color = '%d %d %d %d' size = '%d %d'",
					m_frames[i].BarName, this->uid, i, g_Screen.m_width, g_Screen.m_height, guiMajorAlpha, guiMajorRed,
					guiMajorGreen, guiMajorBlue, guiMajorWidth, guiMajorHeight);

			TwDefine(animBarDefine);
		}

		if (m_frames[i].died)
		{
			continue;
		}

		TwRemoveAllVars(m_frames[i].Bar);
		TwAddButton(m_frames[i].Bar, "duplicateFrame", duplicateFrameCB, &m_frames[i].callback, "label = 'Duplicate Anim Frame'");
		TwAddButton(m_frames[i].Bar, "removeFrame", removeFrameCB, &m_frames[i], "label = 'Remove Anim Frame'");
		TwAddVarRW(m_frames[i].Bar, "visible", TW_TYPE_BOOLCPP, &m_frames[i].visible, "label = 'Visible'");
		TwAddVarRW(m_frames[i].Bar, "id", TW_TYPE_UINT32, &m_frames[i].frameId, "label = 'Frameid'");
		TwAddVarRW(m_frames[i].Bar, "offsetX", TW_TYPE_FLOAT, &m_frames[i].offsetX, "label = 'OffsetX' step = 100");
		TwAddVarRW(m_frames[i].Bar, "offsetY", TW_TYPE_FLOAT, &m_frames[i].offsetY, "label = 'OffsetY' step = 100");
		TwAddVarRW(m_frames[i].Bar, "offsetZ", TW_TYPE_FLOAT, &m_frames[i].offsetZ, "label = 'OffsetZ' step = 100");
		TwAddVarRW(m_frames[i].Bar, "rotateX", TW_TYPE_FLOAT, &m_frames[i].rotateX, "label = 'rotateX  step = 100");
		TwAddVarRW(m_frames[i].Bar, "rotateY", TW_TYPE_FLOAT, &m_frames[i].rotateY, "label = 'rotateY' step = 100");
		TwAddVarRW(m_frames[i].Bar, "rotateZ", TW_TYPE_FLOAT, &m_frames[i].rotateZ, "label = 'rotateZ' step = 100");
		TwAddVarRW(m_frames[i].Bar, "scaleX", TW_TYPE_FLOAT, &m_frames[i].scaleX, "label = 'ScaleX' step = 10");
		TwAddVarRW(m_frames[i].Bar, "scaleY", TW_TYPE_FLOAT, &m_frames[i].scaleY, "label = 'ScaleY' step = 10");
		TwAddVarRW(m_frames[i].Bar, "scaleZ", TW_TYPE_FLOAT, &m_frames[i].scaleZ, "label = 'ScaleZ' step = 10");
	}
}

void	CPieInternal::hideGUI(void) {
	Uint16		i;

	//Utility bar
	char	toolBarName[255] = "toolbar";
	char	toolBarDefine[255];

	if (!this->m_IsSub)
	{
		if (this->uid < 10)
		{
			snprintf(&toolBarName[7], 1, "0");
			snprintf(&toolBarName[8], 1, "%u", this->uid);
			strcpy(toolBarDefine, toolBarName);
			snprintf(&toolBarDefine[9], 100, " hide");
		}
		else if (this->uid < 100)
		{
			snprintf(&toolBarName[7], 2, "%2u", this->uid);
			strcpy(toolBarDefine, toolBarName);
			snprintf(&toolBarDefine[9], 100, " hide");
		}
		else
		{
			snprintf(&toolBarName[7], 3, "3%u", this->uid);
			strcpy(toolBarDefine, toolBarName);
			snprintf(&toolBarDefine[10], 100, " hide");
		}
	}
	else
	{
			snprintf(toolBarName, 255, "SubModel%4u", this->uid);
			snprintf(toolBarDefine, 255, "%s label = 'Pie %u ToolBar' position = '0 200' color = '%d %d %d %d' size = '%d %d'",
						toolBarName, this->uid, guiMajorAlpha, guiMajorRed, guiMajorGreen, guiMajorBlue, guiMajorWidth, guiMajorHeight);
	}

	//Vertice list bar
	char	vertBarName[255];
	char	vertBarDefine[255];

	strncpy(vertBarName, toolBarName, 255);
	strcat(vertBarName, "vertBar");

	snprintf(vertBarDefine, 255, "%s hide",
			vertBarName);

	TwDefine(vertBarDefine);

	//Connector list bar
	char	connBarName[255];
	char	connBarDefine[255];

	strncpy(connBarName, toolBarName, 255);
	strcat(connBarName, "connBar");

	snprintf(connBarDefine, 255, "%s hide",
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

		if (!this->m_IsSub)
		{
			//FIXME:capped to 1000(3 digi)
			//assert(this->uid < 1000);
			//utterly hack to add zero's to make Bar names unique -_-
			if (i < 10)
			{
				snprintf(&tmpString[4], 1, "0");
				snprintf(&tmpString[5], 1, "%u", i);

				if (this->uid < 10)
				{
					snprintf(&tmpString[6], 1, "0");
					snprintf(&tmpString[7], 1, "%u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[8], 100, " hide");
				}
				else if (this->uid < 100)
				{
					snprintf(&tmpString[6], 2, "%2u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[8], 100, " hide");
				}
				else
				{
					snprintf(&tmpString[6], 3, "3%u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[9], 100, " hide");
				}

				strcpy(tmpString2, tmpString);
				snprintf(&tmpString2[8], 100, " hide");
			}
			else if (i < 100)
			{
				snprintf(&tmpString[4], 2, "%2u", i);

				if (this->uid < 10)
				{
					snprintf(&tmpString[6], 1, "0");
					snprintf(&tmpString[7], 1, "%u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[8], 100, " hide");
				}
				else if (this->uid < 100)
				{
					snprintf(&tmpString[6], 2, "%2u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[8], 100, " hide");
				}
				else
				{
					snprintf(&tmpString[6], 3, "3%u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[9], 100, " hide");
				}
			}
			else
			{
				snprintf(&tmpString[4], 3, "%3u", i);

				if (this->uid < 10)
				{
					snprintf(&tmpString[7], 1, "0");
					snprintf(&tmpString[8], 1, "%u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[9], 100, " hide");
				}
				else if (this->uid < 100)
				{
					snprintf(&tmpString[7], 2, "%2u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[9], 100, " hide");
				}
				else
				{
					snprintf(&tmpString[7], 3, "3%u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[10], 100, " hide");
				}
			}
		}
		else
		{
			snprintf(tmpString, 255, "SubModel%dPoly%d", this->uid, i);
			snprintf(tmpString2, 255, "%s label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
						tmpString, this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
		}

		TwDefine(tmpString2);
	}
}

void	CPieInternal::showGUI(void) {
	Uint16		i;

	//Utility bar
	char	toolBarName[255] = "toolbar";
	char	toolBarDefine[255];

	if (!this->m_IsSub)
	{
		if (this->uid < 10)
		{
			snprintf(&toolBarName[7], 1, "0");
			snprintf(&toolBarName[8], 1, "%u", this->uid);
			strcpy(toolBarDefine, toolBarName);
			snprintf(&toolBarDefine[9], 100, " show");
		}
		else if (this->uid < 100)
		{
			snprintf(&toolBarName[7], 2, "%2u", this->uid);
			strcpy(toolBarDefine, toolBarName);
			snprintf(&toolBarDefine[9], 100, " show");
		}
		else
		{
			snprintf(&toolBarName[7], 3, "3%u", this->uid);
			strcpy(toolBarDefine, toolBarName);
			snprintf(&toolBarDefine[10], 100, " show");
		}
	}
	else
	{
			snprintf(toolBarName, 255, "SubModel%4u", this->uid);
			snprintf(toolBarDefine, 255, "%s label = 'Pie %u ToolBar' position = '0 200' color = '%d %d %d %d' size = '%d %d'",
						toolBarName, this->uid, guiMajorAlpha, guiMajorRed, guiMajorGreen, guiMajorBlue, guiMajorWidth, guiMajorHeight);
	}

	TwDefine(toolBarDefine);

	//Vertice list bar
	char	vertBarName[255];
	char	vertBarDefine[255];

	strncpy(vertBarName, toolBarName, 255);
	strcat(vertBarName, "vertBar");

	snprintf(vertBarDefine, 255, "%s show",
			vertBarName);

	TwDefine(vertBarDefine);

	//Connector list bar
	char	connBarName[255];
	char	connBarDefine[255];

	strncpy(connBarName, toolBarName, 255);
	strcat(connBarName, "connBar");

	snprintf(connBarDefine, 255, "%s show",
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

		if (!this->m_IsSub)
		{
			//FIXME:capped to 1000(3 digi)
			//assert(this->uid < 1000);
			//utterly hack to add zero's to make Bar names unique -_-
			if (i < 10)
			{
				snprintf(&tmpString[4], 1, "0");
				snprintf(&tmpString[5], 1, "%u", i);

				if (this->uid < 10)
				{
					snprintf(&tmpString[6], 1, "0");
					snprintf(&tmpString[7], 1, "%u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[8], 100, " show");
				}
				else if (this->uid < 100)
				{
					snprintf(&tmpString[6], 2, "%2u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[8], 100, " show");
				}
				else
				{
					snprintf(&tmpString[6], 3, "3%u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[9], 100, " show");
				}

				strcpy(tmpString2, tmpString);
				snprintf(&tmpString2[8], 100, " show");
			}
			else if (i < 100)
			{
				snprintf(&tmpString[4], 2, "%2u", i);

				if (this->uid < 10)
				{
					snprintf(&tmpString[6], 1, "0");
					snprintf(&tmpString[7], 1, "%u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[8], 100, " show");
				}
				else if (this->uid < 100)
				{
					snprintf(&tmpString[6], 2, "%2u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[8], 100, " show");
				}
				else
				{
					snprintf(&tmpString[6], 3, "3%u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[9], 100, " show");
				}
			}
			else
			{
				snprintf(&tmpString[4], 3, "%3u", i);

				if (this->uid < 10)
				{
					snprintf(&tmpString[7], 1, "0");
					snprintf(&tmpString[8], 1, "%u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[9], 100, " show");
				}
				else if (this->uid < 100)
				{
					snprintf(&tmpString[7], 2, "%2u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[9], 100, " show");
				}
				else
				{
					snprintf(&tmpString[7], 3, "3%u", this->uid);
					strcpy(tmpString2, tmpString);
					snprintf(&tmpString2[10], 100, " show");
				}
			}
		}
		else
		{
			snprintf(tmpString, 255, "SubModel%dPoly%d", this->uid, i);
			snprintf(tmpString2, 255, "%s label = 'Pie %u Poly %u' position = '0 400' color = '%d %d %d %d' size = '%d %d'",
						tmpString, this->uid, i , guiMinorAlpha, guiMinorRed, guiMinorGreen, guiMinorBlue, guiMinorWidth, guiMinorHeight);
		}

		TwDefine(tmpString2);
	}
}

bool	CPieInternal::addSub(const char *name) {
	CPieInternal	*temp = this;
	while(temp->m_nextSub)
	{
		temp = temp->m_nextSub;
	}

	// Reached the end of linked list
	CPieInternal	*newSub;
	newSub = new CPieInternal(g_SubModelIndex, name, this->m_TexpageId, true, this);

	if (newSub == NULL)
	{
		fprintf(stderr, "error creating submodel\n");
		return false;
	}
	temp->m_nextSub = newSub;
	g_SubModelIndex++;
	return true;
}

bool	CPieInternal::addSubFromFile(const char *filename) {
	CPieInternal	*temp = this;
	while(temp->m_nextSub)
	{
		temp = temp->m_nextSub;
	}

	// Reached the end of linked list
	CPieInternal	*newSub;
	iIMDShape		*newIMD = iV_ProcessIMD(filename);

	newSub = new CPieInternal(g_SubModelIndex, newIMD, filename, true, this);

	if (newSub == NULL)
	{
		fprintf(stderr, "error creating submodel from file %s\n", filename);
		return false;
	}
	temp->m_nextSub = newSub;
	g_SubModelIndex++;
	return true;
}


bool	CPieInternal::addFrame(void) {
	//assert(m_numAnimFrames < MAX_ANIM_FRAMES);
	m_frames[m_numAnimFrames].died = false;
	m_frames[m_numAnimFrames].visible = true;
	m_frames[m_numAnimFrames].frameId = m_numAnimFrames;
	m_frames[m_numAnimFrames].offsetX = 0.0f;
	m_frames[m_numAnimFrames].offsetY = 0.0f;
	m_frames[m_numAnimFrames].offsetZ = 0.0f;
	m_frames[m_numAnimFrames].rotateX = 0.0f;
	m_frames[m_numAnimFrames].rotateY = 0.0f;
	m_frames[m_numAnimFrames].rotateZ = 0.0f;
	m_frames[m_numAnimFrames].scaleX = 1000.0f;
	m_frames[m_numAnimFrames].scaleY = 1000.0f;
	m_frames[m_numAnimFrames].scaleZ = 1000.0f;
	m_frames[m_numAnimFrames].callback.Id = m_numAnimFrames;
	m_frames[m_numAnimFrames].callback.pInstance = (void*)this;
	snprintf(m_frames[m_numAnimFrames].BarName, 255, "Pie%dAnim%dBar", this->uid, m_frames[m_numAnimFrames].frameId);
	m_frames[m_numAnimFrames].Bar = NULL;

	m_numAnimFrames++;

	this->updateGUI();
	return true;
}

bool	CPieInternal::removeFrame(Uint16 id) {

	//assert(id < MAX_ANIM_FRAMES);
	//assert(m_numAnimFrames > 0);
	if (id >= MAX_ANIM_FRAMES)
	{
		fprintf(stderr, "frame id is too large\n");
		return false;
	}

	TwDeleteBar(m_frames[id].Bar);
	m_frames[id].Bar = NULL;
	m_frames[id].died = false;

	Uint16	i;
	for (i = m_numAnimFrames - 1;i > id;i--)
	{
		m_frames[i - 1].visible = m_frames[i].visible;
		m_frames[i - 1].offsetX = m_frames[i].offsetX;
		m_frames[i - 1].offsetY = m_frames[i].offsetY;
		m_frames[i - 1].offsetZ = m_frames[i].offsetZ;
		m_frames[i - 1].rotateX = m_frames[i].rotateX;
		m_frames[i - 1].rotateY = m_frames[i].rotateY;
		m_frames[i - 1].rotateZ = m_frames[i].rotateZ;
		m_frames[i - 1].scaleX = m_frames[i].scaleX;
		m_frames[i - 1].scaleY = m_frames[i].scaleY;
		m_frames[i - 1].scaleZ = m_frames[i].scaleZ;
		m_frames[i - 1].callback.Id = m_frames[i].callback.Id;
		m_frames[i - 1].callback.pInstance = m_frames[i].callback.pInstance;
		//strncpy(m_frames[i - 1].BarName, m_frames[i].BarName, 255);
		//m_frames[i - 1].Bar = m_frames[i].Bar;
		TwDeleteBar(m_frames[i].Bar);
		m_frames[i].Bar = NULL;
	}

	m_numAnimFrames--;
	this->updateGUI();
	return true;
}

bool	CPieInternal::duplicateFrame(Uint16 id) {
	//assert(id < MAX_ANIM_FRAMES);
	m_frames[m_numAnimFrames].died = false;
	m_frames[m_numAnimFrames].visible = true;
	m_frames[m_numAnimFrames].frameId = m_numAnimFrames;
	m_frames[m_numAnimFrames].offsetX = m_frames[id].offsetX;
	m_frames[m_numAnimFrames].offsetY = m_frames[id].offsetY;
	m_frames[m_numAnimFrames].offsetZ = m_frames[id].offsetZ;
	m_frames[m_numAnimFrames].rotateX = m_frames[id].rotateX;
	m_frames[m_numAnimFrames].rotateY = m_frames[id].rotateY;
	m_frames[m_numAnimFrames].rotateZ = m_frames[id].rotateZ;
	m_frames[m_numAnimFrames].scaleX = m_frames[id].scaleX;
	m_frames[m_numAnimFrames].scaleY = m_frames[id].scaleY;
	m_frames[m_numAnimFrames].scaleZ = m_frames[id].scaleZ;
	m_frames[m_numAnimFrames].callback.Id = m_numAnimFrames;
	m_frames[m_numAnimFrames].callback.pInstance = (void*)this;
	snprintf(m_frames[m_numAnimFrames].BarName, 255, "Pie%dAnim%dBar", this->uid, m_frames[m_numAnimFrames].frameId);
	m_frames[m_numAnimFrames].Bar = NULL;

	m_numAnimFrames++;

	this->updateGUI();
	return true;
}

bool	CPieInternal::readAnimFile(const char *filename)
{
	FILE	*file;
	char	newFileName[255];
	const char *extension = strstr(filename, ".pie");

	if (extension)
	{
		sscanf(filename, "%[^'.']", newFileName);
		strcat(newFileName, ".ani");
		file = fopen(newFileName, "rb");
	}
	else
	{
		strncpy(newFileName, filename, 255);
		file = fopen(newFileName, "rb");
	}

	if (!file)
	{
		fprintf(stderr, "Erorr opening anim file %s\n", newFileName);
		return false;
	}

	char	*buffer = (char*)malloc(sizeof(char) * 1024 * 1024); //1MB
	char	*temp = buffer;
	Uint32	count, quantity = fread(buffer, 1, 1024*1024, file);
	Uint32	objIndex;
	CPieInternal	*target = this;

	fclose(file);

	count = 0;
	while (count < quantity)
	{
		if (*buffer == '/' && *(buffer+1) == '*')
		{
			count += 2;
			buffer += 2;
			while (true)
			{
				if (*buffer == '\n')
				{
					count++;
					buffer++;
					break;
				}
				count++;
				buffer++;
			}
			continue;
		}

		if (*buffer == '\n')
		{
			count++;
			buffer++;
			continue;
		}

		char Line[512];
		Uint32	LineLength;
		sscanf(buffer, "%[^'\n']%n", Line, &LineLength);
		count += LineLength + 1;
		buffer += LineLength + 1;

		if (Line[0] == '{' || Line[1] == '{' ||
			Line[0] == '}' || Line[1] == '}')
		{
			continue;
		}
		else
		{
			char	tag[256], dummy[256], dummy2[256];
			sscanf(Line, "%s %s", tag, dummy);

			if (!strcmp(tag, "ANIM3DFRAMES"))
			{
				Uint32	count, total = 0;
				sscanf(Line, "%s%n", dummy, &count);
				total += count;
				sscanf(&Line[total], " \"%[^'\"']%n", dummy2, &count);
				total += count;
				sscanf(&Line[total], "\" %d %d", &target->m_numAnimFrames, &target->m_frameInterval);

				target->m_animMode = ANIM3DFRAMES;

				CPieInternal	*temp = target->m_nextSub;
				while(temp)
				{
					temp->m_animMode = ANIM3DFRAMES;
					temp->m_numAnimFrames = 0;
					temp->m_frameInterval = target->m_numAnimFrames;
					temp = temp->m_nextSub;
				}
			}
			else if (!strcmp(tag, "ANIM3DTRANS"))
			{
				Uint32	dummyInt, count, total = 0;
				sscanf(Line, "%s%n", dummy, &count);
				total += count;
				sscanf(&Line[total], " \"%[^'\"']%n", dummy2, &count);
				total += count;
				sscanf(&Line[total], "\" %d %d %d", &target->m_numAnimFrames, &target->m_frameInterval, &dummyInt);

				target->m_animMode = ANIM3DTRANS;

				CPieInternal	*temp = target->m_nextSub;
				while(temp)
				{
					temp->m_animMode = ANIM3DTRANS;
					temp->m_numAnimFrames = target->m_numAnimFrames;
					temp->m_frameInterval = target->m_numAnimFrames;
					temp = temp->m_nextSub;
				}
			}
			else if (!strcmp(tag, "ANIMOBJECT"))
			{
				sscanf(Line, "%s %d %s", dummy, &objIndex, dummy2);

				CPieInternal	*temp = this;
				Uint32			i = 0;

				while(i < objIndex)
				{
					temp = temp->m_nextSub;
					if (temp == NULL)
					{
						fprintf(stderr, "Pie %s Level %d doesnt exist\n", m_Name, objIndex);
						return false;
					}
					i++;
				}
				target = temp;
			}
			else
			{
				Uint32	frameId;
				float x, y, z, rotx, roty, rotz, scalex, scaley, scalez;

				sscanf(Line, "%d %f %f %f %f %f %f %f %f %f",
					&frameId, &x, &y, &z, &rotx, &roty, &rotz, &scalex, &scaley, &scalez);
				target->m_frames[frameId].died = false;
				target->m_frames[frameId].visible = true;
				target->m_frames[frameId].frameId = frameId;
				target->m_frames[frameId].offsetX = x;
				target->m_frames[frameId].offsetY = y;
				target->m_frames[frameId].offsetZ = z;
				target->m_frames[frameId].rotateX = rotx;
				target->m_frames[frameId].rotateY = roty;
				target->m_frames[frameId].rotateZ = rotz;
				target->m_frames[frameId].scaleX = scalex;
				target->m_frames[frameId].scaleY = scaley;
				target->m_frames[frameId].scaleZ = scalez;
				target->m_frames[frameId].callback.Id = frameId;
				target->m_frames[frameId].callback.pInstance = (void*)target;
				target->m_frames[frameId].Bar = NULL;
				snprintf(target->m_frames[frameId].BarName, 255, "Pie%dAnim%dBar", target->uid, m_frames[frameId].frameId);
			}
		}
	}
	free(temp);
	return true;
}

void	CPieInternal::startAnim() {
	if (m_currAnimFrame == 0xFFFFFFFF)
	{
		m_currAnimFrame = 0;
		m_animStartTime = SDL_GetTicks();
	}
	else
	{
		Uint32	currTime = SDL_GetTicks();

		if (currTime - m_animStartTime > m_frameInterval)
		{
			m_currAnimFrame++;
			if (m_currAnimFrame >= m_numAnimFrames)
			{
				m_currAnimFrame = 0;
			}
			m_animStartTime = currTime;
		}
	}
}

void	CPieInternal::stopAnim() {
	m_currAnimFrame = 0xFFFFFFFF;
	m_animStartTime = 0;
}

bool	CPieInternal::writeAnimFileTrans(const char *filename) {
	FILE	*file = fopen(filename, "w+");

	if (!file)
	{
		fprintf(stderr, "Error creating anim file %s\n", filename);
		return false;
	}

	CPieInternal	*temp = this;
	Uint32			index = 0;
	fprintf(file, "ANIM3DTRANS \"%s\" %d %d %d\n", temp->m_Name, temp->m_numAnimFrames, temp->m_frameInterval, temp->m_levels + 1);
	fprintf(file, "{\n");
	while (temp)
	{
		fprintf(file, "\tANIMOBJECT %d \"SubModel%d\"\n", index, index);
		fprintf(file, "\t{\n");
		Uint32	i;
		for (i = 0;i < temp->m_numAnimFrames;i++)
		{
			fprintf(file, "\t\t%d %d %d %d %d %d %d %d %d %d\n",
				i, (Sint32)temp->m_frames[i].offsetX, (Sint32)temp->m_frames[i].offsetY, (Sint32)temp->m_frames[i].offsetZ,
				(Sint32)temp->m_frames[i].rotateX, (Sint32)temp->m_frames[i].rotateY, (Sint32)temp->m_frames[i].rotateZ,
				(Sint32)temp->m_frames[i].scaleX, (Sint32)temp->m_frames[i].scaleY, (Sint32)temp->m_frames[i].scaleZ);
		}
		fprintf(file, "\t}\n\n");
		temp = temp->m_nextSub;
		index++;
	}
	fprintf(file, "}\n");
	fclose(file);
	return true;
}

bool	CPieInternal::writeAnimFileFrames(const char *filename) {
	FILE	*file = fopen(filename, "w+");

	if (!file)
	{
		fprintf(stderr, "Error creating anim file %s\n", filename);
		return false;
	}

	CPieInternal	*temp = this;
	Uint32			i;
	fprintf(file, "ANIM3DFRAMES \"%s\" %d %d\n", temp->m_Name, temp->m_numAnimFrames, temp->m_frameInterval);
	fprintf(file, "{\n");
	for (i = 0;i < temp->m_numAnimFrames;i++)
	{
		fprintf(file, "\t%d %d %d %d %d %d %d %d %d %d\n",
			i, (Sint32)temp->m_frames[i].offsetX, (Sint32)temp->m_frames[i].offsetY, (Sint32)temp->m_frames[i].offsetZ,
			(Sint32)temp->m_frames[i].rotateX, (Sint32)temp->m_frames[i].rotateY, (Sint32)temp->m_frames[i].rotateZ,
			(Sint32)temp->m_frames[i].scaleX, (Sint32)temp->m_frames[i].scaleY, (Sint32)temp->m_frames[i].scaleZ);
	}
	fprintf(file, "}\n");
	fclose(file);
	return true;
}
