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

#include "resmaster.h"

#ifdef _WIN32
	#include <windows.h>	// required by gl.h
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL/SDL_opengl.h>
#include <SDL/SDL_image.h>

#include "texture.h"

#include "screen.h"
#include "game_io.h"
#include "config.h"

// Resource Manager
CResMaster *ResMaster;

///Callback to create new pie
void TW_CALL newPieCB(void *clientData) {
	CPieInternal	*tmpPieInternal, *tmp2;
	Uint16			i;
	char			name[255];

	tmpPieInternal = ResMaster->getPies();

	for (i = 0;i < ResMaster->m_pieCount;i++)
	{
		tmp2 = tmpPieInternal + i;
		if (tmp2 != NULL)
		{
			delete tmp2;
			tmp2 = NULL;
		}
	}

	ResMaster->m_pieCount = 0;
	snprintf(name, 255, "newpoly%3u.pie", ResMaster->m_numNewPies);
	ResMaster->addPie(NULL, name);
	ResMaster->m_numNewPies++;
}

///Callback to active linker
void TW_CALL activeLinkerCB(void *clientData) {
	ResMaster->activeLinker();
}

///Callback to deactive linker
void TW_CALL deactiveLinkerCB(void *clientData) {
	ResMaster->deactiveLinker();
}

//discards clientData
void TW_CALL savePiesCB(void *clientData) {
	CPieInternal	*tmpPieInternal;
	Uint16			i;

	tmpPieInternal = ResMaster->getPies();

	for (i = 0;i < ResMaster->m_pieCount;i++)
	{
		tmpPieInternal->ToFile(tmpPieInternal[i].m_Name);
	}
}

void TW_CALL startMapTextureCB(void *clientData) {
	ResMaster->startMapTexture();
}

void TW_CALL stopMapTextureCB(void *clientData) {
	ResMaster->stopMapTexture();
}

/* Return the number of newlines in a file buffer */
static int numCR(char *pFileBuffer, int fileSize)
{
	int  lines=0;

	while (fileSize-- > 0)
	{
		if (*pFileBuffer++ == '\n')
		{
			lines++;
		}
	}

	return lines;
}

CResMaster::~CResMaster() {
	Uint16	i;

	for (i = 0;i < m_pieCount;i++)
	{
		if (m_Pies[i] != NULL)
		{
			delete m_Pies[i];
			m_Pies[i] = NULL;
		}
	}

	TwDeleteBar(m_utilBar);
	TwDeleteBar(m_textureBar);

	this->freeTexPages();
	this->deactiveLinker();
	this->stopMapTexture();
}

void	CResMaster::activeLinker(void) {
	this->m_PolyLinker.m_Up = true;
	this->m_PolyLinker.m_MakePolygon = false;
	this->m_PolyLinker.m_LinkedIndex = 0;
	this->m_PolyLinker.m_Target = -1;
}
void	CResMaster::deactiveLinker(void) {
	this->m_PolyLinker.m_Up = false;
}

bool	CResMaster::readTextureList(const char *filename) {
	FILE *file;
	char *buffer, *bufferTmp;
	Uint32 cnt, size;

	file = fopen(filename, "rb");

	if (file == NULL) {
		fprintf(stderr, "unable to read config file %s\n", filename);
		return false;
	}

	buffer = (char*)malloc(1024*1024);
	bufferTmp = buffer;
	size = fread(buffer, 1, 1024*1024, file);

	size = numCR(buffer, size);

	while (m_textureToProcess < size)
	{
		sscanf(buffer, "%s%n", m_TexPageNames[m_textureToProcess], &cnt);
		buffer += cnt;
		while (*buffer == ' ')
		{
			m_TexPageNames[m_textureToProcess][cnt] = ' ';
			sscanf(buffer, "%s%n", &m_TexPageNames[m_textureToProcess][cnt + 1], &cnt);
			buffer += cnt;
		}
		// Increment number of texture to process
		m_textureToProcess++;
		buffer = strchr(buffer, '\n') + 1;
	}

	free(bufferTmp);

	return true;
}

bool	CResMaster::loadTexPages(void) {
	iV_Image *tmpImage;
	Uint32	i;

	tmpImage = (iV_Image*)malloc(sizeof(iV_Image));

	for (i = 0;i < m_textureToProcess;i++)
	{
		if (!iV_loadImage_PNG(m_TexPageNames[i], tmpImage))
		{
			return false;
		}

		pie_AddTexPage(tmpImage, m_TexPageNames[i], i);

		_TEX_PAGE[i].w = tmpImage->width;
		_TEX_PAGE[i].h = tmpImage->height;

		free(tmpImage->bmp);

		memset(tmpImage, 0, sizeof(iV_Image));
	}

	free(tmpImage);
	return true;
}

bool	CResMaster::freeTexPages(void) {
	Uint32	i;

	for (i = 0;i < m_textureToProcess;i++)
	{
		glDeleteTextures(1, (GLuint*)&_TEX_PAGE[i].id);
	}
	return true;
}

void	CResMaster::enableTexture(bool value) {
	if (value && m_drawTexture)
	{
		glEnable(GL_TEXTURE_2D);
		m_drawTexture = value;
	}
	else if (m_drawTexture)
	{
		glDisable(GL_TEXTURE_2D);
		m_drawTexture = value;
	}
}

bool	CResMaster::addPie(iIMDShape *imd, const char *name) {
	if (imd)
	{
		m_Pies[m_pieCount] = new CPieInternal(m_pieCount, imd, name);
	}
	else
	{
		m_Pies[m_pieCount] = new CPieInternal(m_pieCount, name, m_newPieTextureId);
	}
	m_pieCount++;

	return true;
}

CPieInternal* CResMaster::getPies(void) {
	return m_Pies[0];
}

bool	CResMaster::addGUI(void) {
	char	defineString[255];
	snprintf(defineString, 255, "utilBar position = '0 0' color = '%d %d %d %d' size = '%d %d'",
				guiMajorAlpha, guiMajorRed, guiMajorGreen, guiMajorBlue, guiMajorWidth, guiMajorHeight);

	m_utilBar = TwNewBar("utilBar");
	TwDefine(defineString);

	TwAddButton(m_utilBar, "new pie", newPieCB, NULL, "label = 'New Pie'");
	TwAddButton(m_utilBar, "texturemapper start", startMapTextureCB, NULL, "label = 'Map Texture'");
	TwAddButton(m_utilBar, "texturemapper stop", stopMapTextureCB, NULL, "label = 'Stop Map Texture'");

	TwAddButton(m_utilBar, "polylinker start", activeLinkerCB, NULL, "label = 'Link Polygon'");
	TwAddButton(m_utilBar, "polylinker stop", deactiveLinkerCB, NULL, "label = 'Stop Link Polygon'");
	TwAddVarRW(m_utilBar, "polygon mode", TW_TYPE_UINT32, &m_polygonMode, "label = 'Polygon Mode' min=0 max=2 step=1");
	TwAddVarRW(m_utilBar, "shade mode", TW_TYPE_UINT32, &m_shadeMode, "label = 'Shade Mode' min=0 max=1 step=1");

	TwAddVarRW(m_utilBar, "draw axis", TW_TYPE_BOOLCPP, &m_drawAxis, "label = 'Draw Axis'");
	TwAddVarRW(m_utilBar, "draw texture", TW_TYPE_BOOLCPP, &m_drawTexture, "label = 'Draw Texture'");
	TwAddVarRW(m_utilBar, "draw newvertice", TW_TYPE_BOOLCPP, &m_drawNewVertice, "label = 'Draw New Vertice'");
	TwAddVarRW(m_utilBar, "hl selected", TW_TYPE_BOOLCPP, &m_highLightSelected, "label = 'HL Selected'");
	TwAddVarRW(m_utilBar, "hl vertices", TW_TYPE_BOOLCPP, &m_highLightVertices, "label = 'HL Vertices'");
	TwAddVarRW(m_utilBar, "hl connectors", TW_TYPE_BOOLCPP, &m_highLightConnectors, "label = 'HL Connectors'");
	TwAddButton(m_utilBar, "save to file", savePiesCB, NULL, "label = 'Save'");

	m_textureBar = TwNewBar("textureBar");
	char	defineString2[255];
	snprintf(defineString2, 255, "textureBar position = '%d %d' color = '%d %d %d %d' size = '%d %d'",
				g_Screen.m_width/2 - 200, g_Screen.m_height/2 + 200,
				guiMajorAlpha, guiMajorRed, guiMajorGreen, guiMajorBlue, guiMajorWidth, guiMajorHeight);

	TwDefine(defineString2);

	Uint32	i;
	for (i = 0;i < m_textureToProcess;i++)
	{
		char	tmpName[255];
		char	tmpName2[255];
		snprintf(tmpName, 255, "pageId%d", i);
		snprintf(tmpName2, 255, "pageName%d", i);
		TwAddVarRO(m_textureBar, tmpName, TW_TYPE_UINT32, &_TEX_PAGE[i].id, "label = 'Page Id'");
		TwAddVarRO(m_textureBar, tmpName2, TW_TYPE_CSSTRING(255), &_TEX_PAGE[i].name, "label = 'Page Name'");
	}
	return true;
}

void	CResMaster::draw(void) {
	Uint32	i;

	if (this->m_TextureMapper.m_Up)
	{
		if (m_polygonMode != FILL)
		{
			m_polygonMode = FILL;
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		this->m_TextureMapper.draw();
		return;
	}

	if (m_polygonMode == FILL)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	else if (m_polygonMode == LINE)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else if (m_polygonMode == POINT)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	}

	if (m_shadeMode == SMOOTH)
	{
		glShadeModel(GL_SMOOTH);
	}
	else if (m_polygonMode == FLAT)
	{
		glShadeModel(GL_FLAT);
	}

	glRotatef(m_rotateX, 1.0f, 0.0f, 0.0f);
	glRotatef(m_rotateY, 0.0f, 1.0f, 0.0f);

	if (m_drawAxis)
	{
		this->drawAxis();
	}

	glScalef(m_scale, m_scale, m_scale);

	if (m_polygonMode == LINE || !m_drawTexture)
	{
		this->enableTexture(false);
	}
	else
	{
		this->enableTexture(true);
	}

	if (m_highLightVertices && m_highLightConnectors && m_highLightSelected)
	{
		for (i = 0;i < m_pieCount;i++)
		{
			this->m_Pies[i]->draw();
			this->m_Pies[i]->highLightVertices();
			this->m_Pies[i]->highLightConnectors();
			this->m_Pies[i]->highLightSelected();
		}
	}
	else if (m_highLightVertices && m_highLightConnectors)
	{
		for (i = 0;i < m_pieCount;i++)
		{
			this->m_Pies[i]->draw();
			this->m_Pies[i]->highLightVertices();
			this->m_Pies[i]->highLightConnectors();
		}
	}
	else if (m_highLightVertices)
	{
		for (i = 0;i < m_pieCount;i++)
		{
			this->m_Pies[i]->draw();
			this->m_Pies[i]->highLightVertices();
		}
	}
	else if (m_highLightConnectors)
	{
		for (i = 0;i < m_pieCount;i++)
		{
			this->m_Pies[i]->draw();
			this->m_Pies[i]->highLightConnectors();
		}
	}
	else
	{
		for (i = 0;i < m_pieCount;i++)
		{
			this->m_Pies[i]->draw();
		}
	}

	if (m_drawNewVertice)
	{
		for (i = 0;i < m_pieCount;i++)
		{
			this->m_Pies[i]->drawNewVertice();
		}
	}

	if (this->m_PolyLinker.m_Up && this->m_PolyLinker.m_Target >= 0)
	{
		this->m_PolyLinker.draw(this->m_Pies[this->m_PolyLinker.getTarget()]);
	}
}

void	CResMaster::drawAxis(void) {
	static const float xFar = 1999.0f, yFar = 1999.0f, zFar = 1999.0f;
	static const float xNear = -1999.0f, yNear = -1999.0f, zNear = -1999.0f;

	glDisable(GL_DEPTH_TEST);

	glTranslatef(0.0f, 0.0f, 0.0f);
	glBegin(GL_LINES);

	glColor4ub(0, 128, 0, 255);
	glVertex3f(xFar, 0.0f, 0.0f);
	glVertex3f(xNear, 0.0f, 0.0f);

	glColor4ub(128, 0, 0, 255);
	glVertex3f(0.0f, yFar, 0.0f);
	glVertex3f(0.0f, yNear, 0.0f);

	glColor4ub(0, 0, 128, 255);
	glVertex3f(0.0f, 0.0f, zFar);
	glVertex3f(0.0f, 0.0f, zNear);
	glEnd();

	glColor4ub(255, 255, 255, 255);

	glEnable(GL_DEPTH_TEST);
}

/**
* Gets 2 points(near and far) to do raycast
**/
void	CResMaster::unprojectMouseXY(double *newX1, double *newY1, double *newZ1,
									 double *newX2,	double *newY2, double *newZ2) {
	GLint		viewport[4];
	GLdouble	modelview[16];
	GLdouble	projection[16];
	GLfloat		screenX, screenY;

	glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
	glGetDoublev( GL_PROJECTION_MATRIX, projection );
	glGetIntegerv( GL_VIEWPORT, viewport );

	screenX = (float)MouseX;
	screenY = (float)viewport[3] - (float)MouseY;

	gluUnProject( screenX, screenY, 0.0f, modelview, projection, viewport, (GLdouble*)newX1, (GLdouble*)newY1, (GLdouble*)newZ1);
	gluUnProject( screenX, screenY, 1.0f, modelview, projection, viewport, (GLdouble*)newX2, (GLdouble*)newY2, (GLdouble*)newZ2);
}

///updates ResMaster input
void	CResMaster::updateInput(void) {
	if (this->m_TextureMapper.m_Up)
	{
		//Ignores input when texture mapper is up
		return;
	}

	//hold mouse middle button to scale model
	if (isMouseButtonHold(2))
	{
		if (MouseMoveY > 0)
		{
			if (m_scale < 3.0f)
			{
				m_scale += 0.01f;
			}
		}

		if (MouseMoveY < 0)
		{
			if (m_scale > 0.2f)
			{
				m_scale -= 0.01f;
			}
		}
	}

	//hold right mouse button to rotate model
	if (isMouseButtonHold(3))
	{
		if (MouseMoveY > 0)
		{
			m_rotateX += (0.1f * MouseMoveY);
			if (m_rotateX >= 360.0f)
			{
				m_rotateX -= 360.0f;
			}
		}
		else if (MouseMoveY < 0)
		{
			m_rotateX += (0.1f * MouseMoveY);
			if (m_rotateX <= -360.0f)
			{
				m_rotateX += 360.0f;
			}
		}

		if (MouseMoveX > 0)
		{
			m_rotateY += (0.1f * MouseMoveX);
			if (m_rotateY >= 360.0f)
			{
				m_rotateY -= 360.0f;
			}
		}
		else if (MouseMoveX < 0)
		{
			m_rotateY += (0.1f * MouseMoveX);
			if (m_rotateY <= -360.0f)
			{
				m_rotateY += 360.0f;
			}
		}
	}

	if (this->m_PolyLinker.isUp())
	{
		if (this->m_PolyLinker.m_MakePolygon)
		{
			this->m_PolyLinker.makePolygon(this->m_Pies[this->m_PolyLinker.getTarget()]);
			this->m_PolyLinker.m_Up = false;
			this->m_Pies[this->m_PolyLinker.getTarget()]->updateGUI();
			return;
		}
	}


	// Checks vertice selection upon left-click event
	if (isMouseButtonDown(1))
	{
		double x1, y1, z1, x2, y2, z2;
		Uint32	i, j;

		this->unprojectMouseXY(&x1, &y1, &z1, &x2, &y2, &z2);

		for (i = 0;i < m_pieCount;i++)
		{
			this->m_Pies[i]->checkSelection((float)x1, (float)y1, (float)z1,
											(float)x2, (float)y2, (float)z2);
		}

		if (this->m_PolyLinker.isUp())
		{
			if (this->m_PolyLinker.m_Target < 0)
			{
				for (i = 0;i < m_pieCount;i++)
				{
					for (j = 0;j < this->m_Pies[i]->m_vertCount;j++)
					{
						if (this->m_Pies[i]->m_Vertices[j] &&
							this->m_Pies[i]->m_Vertices[j]->selected)
						{
							this->m_Pies[i]->m_Vertices[j]->selected = false;
							this->m_PolyLinker.setTarget(i);
							this->m_PolyLinker.Link(j);
							break;
						}
					}
				}
			}
			else
			{
				for (i = 0;i < this->m_Pies[this->m_PolyLinker.getTarget()]->m_vertCount;i++)
				{
					if (this->m_Pies[this->m_PolyLinker.getTarget()]->m_Vertices[i] &&
						this->m_Pies[this->m_PolyLinker.getTarget()]->m_Vertices[i]->selected)
					{
						this->m_Pies[this->m_PolyLinker.getTarget()]->m_Vertices[i]->selected = false;
						this->m_PolyLinker.Link(i);
						break;
					}
				}
			}
		}
	}

	// Checks connector selection
}

void CResMaster::startMapTexture(void) {
	Uint16	i, j;

	for (i = 0;i < this->m_pieCount;i++)
	{
		if (this->m_Pies[i])
		{
			this->m_Pies[i]->hideGUI();
			for (j = 0;j < this->m_Pies[i]->getPolyCount();j++)
			{
				if (this->m_Pies[i]->m_Polygons[j] &&
					this->m_Pies[i]->m_Polygons[j]->selected)
				{
					//TODO:fix 512,512
					this->m_TextureMapper.addTargets(512, 512, this->m_Pies[i]->m_TexpageId, &this->m_Pies[i]->m_Polygons[j]->polygon, 1);
					break;
				}
			}
		}
	}
}

void CResMaster::stopMapTexture(void) {
	Uint16	i;

	ResMaster->m_TextureMapper.removeTargets();

	for (i = 0;i < this->m_pieCount;i++)
	{
		if (this->m_Pies[i])
		{
			this->m_Pies[i]->showGUI();
		}
	}
}
