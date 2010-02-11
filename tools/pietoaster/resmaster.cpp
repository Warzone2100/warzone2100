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
#include "resmaster.h"

#include <SDL_opengl.h>
#include <SDL_image.h>

#include "texture.h"

#include "screen.h"
#include "game_io.h"
#include "config.h"

void	COpenFileDialog::doFunction() {
	//Add new pie with filename
	iIMDShape	*openIMD = iV_ProcessIMD(m_text);

	if (openIMD != NULL)
	{
		ResMaster.addPie(openIMD, m_text);
	}
	else
	{
		fprintf(stderr, "Unable to find file with name:%s\n", m_text);
	}
	this->deleteTextBox();
	this->m_Up = false;
}

// Resource Manager
CResMaster ResMaster;
// Open File Dialog
COpenFileDialog	OpenFileDialog;

// the dumbest shift kmod char handling function on planet earth :)
Uint16 shiftChar(Uint16 key)
{
	switch (key)
	{
	case '`':
		key = '~';
		break;
	case '1':
		key = '!';
		break;
	case '2':
		key = '@';
		break;
	case '3':
		key = '!';
		break;
	case '4':
		key = '$';
		break;
	case '5':
		key = '%';
		break;
	case '6':
		key = '^';
		break;
	case '7':
		key = '&';
		break;
	case '8':
		key = '*';
		break;
	case '9':
		key = '(';
		break;
	case '0':
		key = ')';
		break;
	case '-':
		key = '_';
		break;
	case '[':
		key = '{';
		break;
	case ']':
		key = '}';
		break;
	case ';':
		key = ';';
		break;
	case '\'':
		key = '\"';
		break;
	case ',':
		key = '<';
		break;
	case '.':
		key = '>';
		break;
	case '/':
		key = '?';
		break;
	default:
		break;
	}
	return key;
}

///Callback to create new pie
void TW_CALL openPieCB(void *clientData) {
	char	*name = (char*)clientData;

	OpenFileDialog.addTextBox(name);
}

void TW_CALL newPieCB(void *clientData) {
	CPieInternal	*tmpPieInternal;
	Uint16			i;
	char			name[255];

	for (i = 0;i < ResMaster.m_pieCount;i++)
	{
		tmpPieInternal = ResMaster.getPieAt(i);
		if (tmpPieInternal != NULL)
		{
			tmpPieInternal->ToFile(tmpPieInternal->m_Name, ResMaster.m_oldFormat);
		}
	}

	ResMaster.m_pieCount = 0;
	snprintf(name, 255, "newpoly%3u.pie", ResMaster.m_numNewPies);
	ResMaster.addPie(NULL, name);
	ResMaster.m_numNewPies++;
}

///Callback to active linker
void TW_CALL activeLinkerCB(void *clientData) {
	ResMaster.activeLinker();
}

///Callback to deactive linker
void TW_CALL deactiveLinkerCB(void *clientData) {
	ResMaster.deactiveLinker();
}

//discards clientData
void TW_CALL savePiesCB(void *clientData) {
	CPieInternal	*tmpPieInternal;
	Uint16			i;

	for (i = 0;i < ResMaster.m_pieCount;i++)
	{
		tmpPieInternal = ResMaster.getPieAt(i);
		if (tmpPieInternal != NULL)
		{
			tmpPieInternal->ToFile(tmpPieInternal->m_Name, ResMaster.m_oldFormat);
		}
	}
}

void TW_CALL mergePiesCB(void *clientData) {
	ResMaster.mergePies();
}

void TW_CALL startMapTextureCB(void *clientData) {
	ResMaster.startMapTexture();
}

void TW_CALL stopMapTextureCB(void *clientData) {
	ResMaster.stopMapTexture();
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

CResMaster::CResMaster() {
	m_oldFormat = false;
	m_numExtensions = 0;
	m_textureToProcess = 0;
	m_pieCount = 0;
	m_numNewPies = 0;
	m_rotateX = 0.0f;
	m_rotateY = 0.0f;
	m_scale = 1.0f;
	m_polygonMode = 0;
	m_highLightVertices = true;
	m_highLightConnectors = true;
	m_highLightSelected = true;
	m_drawAxis = true;
	m_drawGrids = true;
	m_drawTexture = true;
	m_drawNewVertice = true;
	m_drawTexts = true;
	m_shadeMode = 0;
	m_gamma = 1.0f;
	m_newPieTextureId = 0;

	memset(m_Pies, 0, sizeof(CPieInternal*)*MAX_PIES);

	m_PolyLinker.m_Up = false;
	m_TextureMapper.m_Up = false;
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

	for (i = 0;i < m_textureToProcess;i++)
	{
		if (m_TexSurfaces[i])
		{
			SDL_FreeSurface(m_TexSurfaces[i]);
			m_TexSurfaces[i] = NULL;
		}
	}

	if (g_Screen.m_useVBO)
	{
		g_Screen.glDeleteBuffersARB(1, &m_GridVBOId);
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
		//process every white space in filename manually
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

///Load texture from files and upload the loaded bmp to opengl
bool	CResMaster::loadTexPages(void) {
	iV_Image	*tmpImage;
	Uint32		i;
	static bool bReload = false;

	tmpImage = (iV_Image*)malloc(sizeof(iV_Image));

	if (!bReload)
	{
		for (i = 0;i < m_textureToProcess;i++)
		{
			SDL_Surface *surface = IMG_Load(m_TexPageNames[i]);

			if (surface == NULL)
			{
				printf("%s: %s\n", IMG_GetError(),m_TexPageNames[i]);
				fprintf(stderr, "warning; failed to load file %s id %d\n", m_TexPageNames[i], i);
				i++;
				continue;
			}

			//Copy bmp info from surface
			tmpImage->width = surface->w;
			tmpImage->height = surface->h;
			tmpImage->depth = surface->format->BytesPerPixel;
			tmpImage->bmp = (unsigned char*)surface->pixels;

			pie_AddTexPage(tmpImage, m_TexPageNames[i], i);

			_TEX_PAGE[i].w = tmpImage->width;
			_TEX_PAGE[i].h = tmpImage->height;

			//Copy it to system memory
			m_TexSurfaces[i] = surface;
		}
		bReload = true;
	}
	else
	{
		for (i = 0;i < m_textureToProcess;i++)
		{
			SDL_Surface *surface = m_TexSurfaces[i];

			if (surface == NULL)
			{
				fprintf(stderr, "warning; failed to locate surface in syste memory slot %d\n", i);
				i++;
				continue;
			}

			//Copy bmp info from surface
			tmpImage->width = surface->w;
			tmpImage->height = surface->h;
			tmpImage->depth = surface->format->BytesPerPixel;
			tmpImage->bmp = (unsigned char*)surface->pixels;

			pie_AddTexPage(tmpImage, m_TexPageNames[i], i);

			_TEX_PAGE[i].w = tmpImage->width;
			_TEX_PAGE[i].h = tmpImage->height;
		}
	}
	free(tmpImage);
	return true;
}

bool	CResMaster::freeTexPages(void) {
	Uint32	i;

	for (i = 0;i < m_textureToProcess;i++)
	{
		glDeleteTextures(1, &_TEX_PAGE[i].id);
	}
	return true;
}

void	CResMaster::enableTexture(bool value) {
	if (value)
	{
		glEnable(GL_TEXTURE_2D);
	}
	else
	{
		glDisable(GL_TEXTURE_2D);
	}
	m_drawTexture = value;
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

	if (m_Pies[m_pieCount] != NULL)
	{
		m_pieCount++;
		return true;
	}

	return false;
}

bool	CResMaster::removePieAt(Uint16 i) {
	if (m_Pies[i])
	{
		delete m_Pies[i];
		m_Pies[i] = NULL;
	}
	fprintf(stderr, "Pie with id %d doesn't exist\n", i);
	return false;
}

CPieInternal* CResMaster::getPieAt(Uint16 i) {
	return m_Pies[i];
}

bool	CResMaster::addGUI(void) {
	char	defineString[255];
	snprintf(defineString, 255, "utilBar position = '0 0' color = '%d %d %d %d' size = '%d %d'",
				guiMajorAlpha, guiMajorRed, guiMajorGreen, guiMajorBlue, guiMajorWidth, guiMajorHeight);

	m_utilBar = TwNewBar("utilBar");
	TwDefine(defineString);

	if (g_Screen.m_useVBO)
	{
		TwAddVarRW(m_utilBar, "useVBO", TW_TYPE_BOOLCPP, &g_Screen.m_useVBO, "label = 'Use VBO' group = 'VBO'");
	}

	TwAddButton(m_utilBar, "new pie", newPieCB, NULL, "label = 'New Pie' group = 'File'");
	TwAddButton(m_utilBar, "open file", openPieCB, (void*)"OpenPie", "label = 'Open File' group = 'File'");
	TwAddVarRW(m_utilBar, "save int pie", TW_TYPE_BOOLCPP, &m_oldFormat, "label = 'Save Integer Pie' group = 'File'");
	TwAddButton(m_utilBar, "save to file", savePiesCB, NULL, "label = 'Save All' group = 'File'");
	TwAddButton(m_utilBar, "merge pie", mergePiesCB, NULL, "label = 'Merge All' group = 'File'");

	TwAddButton(m_utilBar, "texturemapper start", startMapTextureCB, NULL, "label = 'Map Texture' group = 'Texture Mapper'");
	TwAddButton(m_utilBar, "texturemapper stop", stopMapTextureCB, NULL, "label = 'Stop Map Texture' group = 'Texture Mapper'");

	TwAddButton(m_utilBar, "polylinker start", activeLinkerCB, NULL, "label = 'Link Polygon' group = 'Link Polygon'");
	TwAddButton(m_utilBar, "polylinker stop", deactiveLinkerCB, NULL, "label = 'Stop Link Polygon' group = 'Link Polygon'");

	TwAddVarRW(m_utilBar, "polygon mode", TW_TYPE_UINT32, &m_polygonMode, "label = 'Polygon Mode' min=0 max=2 step=1 group = 'Visual Options'");
	TwAddVarRW(m_utilBar, "shade mode", TW_TYPE_UINT32, &m_shadeMode, "label = 'Shade Mode' min=0 max=1 step=1 group = 'Visual Options'");

	TwAddVarRW(m_utilBar, "draw axis", TW_TYPE_BOOLCPP, &m_drawAxis, "label = 'Draw Axis' group = 'Visual Options'");
	TwAddVarRW(m_utilBar, "draw grids", TW_TYPE_BOOLCPP, &m_drawGrids, "label = 'Draw Grids' group = 'Visual Options'");
	//TwAddVarRW(m_utilBar, "draw texts", TW_TYPE_BOOLCPP, &m_drawTexts, "label = 'Draw Texts' group = 'Visual Options'");
	TwAddVarRW(m_utilBar, "draw texture", TW_TYPE_BOOLCPP, &m_drawTexture, "label = 'Draw Texture' group = 'Visual Options'");
	TwAddVarRW(m_utilBar, "draw newvertice", TW_TYPE_BOOLCPP, &m_drawNewVertice, "label = 'Draw New Vertice' group = 'Visual Options'");
	TwAddVarRW(m_utilBar, "hl selected", TW_TYPE_BOOLCPP, &m_highLightSelected, "label = 'HL Selected' group = 'Visual Options'");
	TwAddVarRW(m_utilBar, "hl vertices", TW_TYPE_BOOLCPP, &m_highLightVertices, "label = 'HL Vertices' group = 'Visual Options'");
	TwAddVarRW(m_utilBar, "hl connectors", TW_TYPE_BOOLCPP, &m_highLightConnectors, "label = 'HL Connectors' group = 'Visual Options'");


	m_textureBar = TwNewBar("textureBar");
	char	defineString2[255];
	snprintf(defineString2, 255, "textureBar position = '%d %d' color = '%d %d %d %d' size = '%d %d'",
				g_Screen.m_width/2 - 150, g_Screen.m_height,
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
		TwAddVarRO(m_textureBar, tmpName2, TW_TYPE_CSSTRING(iV_TEXNAME_MAX), &_TEX_PAGE[i].name, "label = 'Page Name'");
	}
	return true;
}

void	CResMaster::logic(void) {
	Uint32	i;

	if (this->m_pieCount)
	{
		for (i = 0;i < m_pieCount;i++)
		{
			if (this->m_Pies[i])
			{
				CPieInternal	*base = this->m_Pies[i];
				while (base)
				{
					CPieInternal	*temp = base->m_nextSub;
					if (temp && temp->died)
					{
						CPieInternal	*newNext;
						newNext = temp->m_nextSub;
						delete temp;
						temp = NULL;
						base->m_nextSub = newNext;
					}
					base = base->m_nextSub;
				}

				if (this->m_Pies[i]->died)
				{
					delete this->m_Pies[i];
					this->m_Pies[i] = NULL;
					continue;
				}

				if (this->m_Pies[i]->m_saveNow)
				{
					this->m_Pies[i]->ToFile(this->m_Pies[i]->m_Name, this->m_oldFormat);
					this->m_Pies[i]->m_saveNow = false;
				}
			}
		}
	}
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

	glScalef(m_scale, m_scale, m_scale);

	if (m_drawGrids)
	{
		this->drawGrids();
	}

	if (m_drawAxis)
	{
		this->drawAxis();
	}

	if (m_polygonMode == LINE || !m_drawTexture)
	{
		this->enableTexture(false);
	}
	else
	{
		this->enableTexture(true);
	}

	if (m_pieCount == 0)
	{
		return;
	}

	if (m_highLightVertices && m_highLightConnectors && m_highLightSelected)
	{
		for (i = 0;i < m_pieCount;i++)
		{
			if (this->m_Pies[i])
			{
				if (this->m_Pies[i])
				{
					this->m_Pies[i]->logic();
					this->m_Pies[i]->draw();
					this->m_Pies[i]->highLightVertices();
					this->m_Pies[i]->highLightConnectors();
					this->m_Pies[i]->highLightSelected();
				}
			}
		}
	}
	else if (m_highLightVertices && m_highLightConnectors)
	{
		for (i = 0;i < m_pieCount;i++)
		{
			if (this->m_Pies[i])
			{
				this->m_Pies[i]->logic();
				this->m_Pies[i]->draw();
				this->m_Pies[i]->highLightVertices();
				this->m_Pies[i]->highLightConnectors();
			}
		}
	}
	else if (m_highLightVertices)
	{
		for (i = 0;i < m_pieCount;i++)
		{
			if (this->m_Pies[i])
			{
				this->m_Pies[i]->logic();
				this->m_Pies[i]->draw();
				this->m_Pies[i]->highLightVertices();
			}
		}
	}
	else if (m_highLightConnectors)
	{
		for (i = 0;i < m_pieCount;i++)
		{
			if (this->m_Pies[i])
			{
				this->m_Pies[i]->logic();
				this->m_Pies[i]->draw();
				this->m_Pies[i]->highLightConnectors();
			}
		}
	}
	else
	{
		for (i = 0;i < m_pieCount;i++)
		{
			if (this->m_Pies[i])
			{
				this->m_Pies[i]->logic();
				this->m_Pies[i]->draw();
			}
		}
	}

	if (m_drawNewVertice)
	{
		for (i = 0;i < m_pieCount;i++)
		{
			if (this->m_Pies[i])
			{
				this->m_Pies[i]->drawNewVertice();
			}
		}
	}

	if (m_drawTexts)
	{
		this->drawTexts();
	}

	if (this->m_PolyLinker.m_Up && this->m_PolyLinker.m_Target >= 0)
	{
		this->m_PolyLinker.draw(this->m_Pies[this->m_PolyLinker.getTarget()]);
	}
}

void	CResMaster::drawAxis(void) {
	static const float xFar = 1999.0f, yFar = 1999.0f, zFar = 1999.0f;
	static const float xNear = -1999.0f, yNear = -1999.0f, zNear = -1999.0f;

	if (m_drawTexture)
	{
		glDisable(GL_TEXTURE_2D);
	}

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

	if (m_drawTexture)
	{
		glEnable(GL_TEXTURE_2D);
	}
}

///Caches (1920/32)^2 grid quads into array
void	CResMaster::cacheGridsVertices(void) {
	float	x, z;
	Sint32	index = -1;
	static const float xMax = 1920.0f, zMax = 1920.0f;
	static const float xMin = -1920.0f, zMin = -1920.0f;
	static bool bRecache = false;

	if (!bRecache)
	{
		m_GridCacheCount = 0;
		for (z = zMin;z < zMax;z += 32.0f)
		{
			for (x= xMin;x < xMax;x += 32.0f)
			{
				++index;
				m_GridCacheVertices[index].x = x;
				m_GridCacheVertices[index].y = 0.0f;
				m_GridCacheVertices[index].z = z;

				++index;
				m_GridCacheVertices[index].x = x;
				m_GridCacheVertices[index].y = 0.0f;
				m_GridCacheVertices[index].z = z + 32.0f;

				++index;
				m_GridCacheVertices[index].x = x + 32.0f;
				m_GridCacheVertices[index].y = 0.0f;
				m_GridCacheVertices[index].z = z + 32.0f;

				++index;
				m_GridCacheVertices[index].x = x + 32.0f;
				m_GridCacheVertices[index].y = 0.0f;
				m_GridCacheVertices[index].z = z;
				m_GridCacheCount++;
			}
		}
		bRecache = true;
	}

	if (g_Screen.m_useVBO)
	{
		g_Screen.glGenBuffersARB(1, &m_GridVBOId);
		g_Screen.glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_GridVBOId);
		g_Screen.glBufferDataARB(GL_ARRAY_BUFFER_ARB, m_GridCacheCount * 4 * sizeof(Vector3f), m_GridCacheVertices, GL_STATIC_DRAW_ARB);
		Sint32	allocated;
		g_Screen.glGetBufferParameterivARB( GL_ARRAY_BUFFER_ARB, GL_BUFFER_SIZE_ARB, &allocated );
		if (allocated)
		{
			fprintf(stderr, "Allocated bytes in VBO %d: %d\n", m_GridVBOId, allocated);
		}
	}

}

void	CResMaster::drawGrids(void) {
	glDisable(GL_DEPTH_TEST);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glColor4ub(0, 0, 128, 255);

	if (m_drawTexture)
	{
		glDisable(GL_TEXTURE_2D);
	}

	glEnableClientState(GL_VERTEX_ARRAY);
	if (g_Screen.m_useVBO)
	{
		g_Screen.glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_GridVBOId);
		glVertexPointer(3, GL_FLOAT, 0, (char*)0);
		glDrawArrays(GL_QUADS, 0, m_GridCacheCount * 4);
		g_Screen.glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	}
	else
	{
		glVertexPointer(3, GL_FLOAT, 0, m_GridCacheVertices);
		glDrawArrays(GL_QUADS, 0, m_GridCacheCount * 4);
	}

	glDisableClientState(GL_VERTEX_ARRAY);

	glColor4ub(255, 255, 255, 255);

	// 128*128 one grid marker
	glColor4ub(128, 0, 0, 255);

	glBegin(GL_QUADS);
	glVertex3f(-64.0f, 0.0f, -64.0f);
	glVertex3f(-64.0f, 0.0f, 64.0f);
	glVertex3f(64.0f, 0.0f, 64.0f);
	glVertex3f(64.0f, 0.0f, -64.0f);
	glEnd();

	glColor4ub(255, 255, 255, 255);

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

	if (m_drawTexture)
	{
		glEnable(GL_TEXTURE_2D);
	}

	glEnable(GL_DEPTH_TEST);

#if 0
	static const int xMax = 2560, zMax = 2560;
	static const int xMin = -2560, zMin = -2560;
	static bool bDList = false;
	static GLuint list;

	float	x, z;

	if (bDList)
	{
		glCallList(list);
	}
	else
	{
		list = glGenLists(1);
		glNewList(list, GL_COMPILE);
		glDisable(GL_DEPTH_TEST);

		glTranslatef(0.0f, 0.0f, 0.0f);

		glColor4ub(255, 255, 64, 255);
		for (z = zMin;z < zMax;z += 32)
		{
			for (x= xMin;x < xMax;x += 32)
			{
				glBegin(GL_LINE_LOOP);
				glVertex3f(x, 0.0f, z);
				glVertex3f(x, 0.0f, z + 32);
				glVertex3f(x + 32, 0.0f, z + 32);
				glVertex3f(x + 32, 0.0f, z);
				glEnd();
			}
		}

		// 128*128 one grid marker
		glColor4ub(64, 255, 255, 255);

		glBegin(GL_LINE_LOOP);
		glVertex3f(-64.0f, 0.0f, -64.0f);
		glVertex3f(-64.0f, 0.0f, 64.0f);
		glVertex3f(64.0f, 0.0f, 64.0f);
		glVertex3f(64.0f, 0.0f, -64.0f);
		glEnd();

		glColor4ub(255, 255, 255, 255);

		glEnable(GL_DEPTH_TEST);
		glEndList();

		glCallList(list);
		bDList = true;
	}
#endif
}

void	CResMaster::drawTexts(void) {
	Uint16	i, j;

	for (i = 0;i < m_pieCount;i++)
	{
		if (m_Pies[i])
		{
			for (j = 0;j < m_Pies[i]->m_polyCount;j++)
			{
				if (m_Pies[i]->m_Polygons[j] && m_Pies[i]->m_Polygons[j]->selected)
				{
					char	text[255];
					SDL_Color	color;
					Uint8	style = 1;

					color.r = 0;
					color.g = 0;
					color.b = 128;

					snprintf(text, 255, "Poly:%d", m_Pies[i]->m_Polygons[j]->id);

					this->drawText(text, m_Pies[i]->m_Polygons[j]->polygon.normal.x,
									m_Pies[i]->m_Polygons[j]->polygon.normal.y,
									m_Pies[i]->m_Polygons[j]->polygon.normal.z,
									color, style);
				}
			}

			for (j = 0;j < m_Pies[i]->m_vertCount;j++)
			{
				if (m_Pies[i]->m_Vertices[j] && m_Pies[i]->m_Vertices[j]->selected)
				{
					char	text[255];
					SDL_Color	color;
					Uint8	style = 1;

					color.r = 0;
					color.g = 128;
					color.b = 128;

					snprintf(text, 255, "V%d", m_Pies[i]->m_Vertices[j]->id);

					this->drawText(text, m_Pies[i]->m_Vertices[j]->vertice.x,
									m_Pies[i]->m_Vertices[j]->vertice.y,
									m_Pies[i]->m_Vertices[j]->vertice.z,
									color, style);
				}
			}

			for (j = 0;j < m_Pies[i]->m_connCount;j++)
			{
				if (m_Pies[i]->m_Connectors[j] && m_Pies[i]->m_Connectors[j]->selected)
				{
					char	text[255];
					SDL_Color	color;
					Uint8	style = 1;

					color.r = 0;
					color.g = 128;
					color.b = 0;

					snprintf(text, 255, "V%d", m_Pies[i]->m_Connectors[j]->id);

					this->drawText(text, m_Pies[i]->m_Connectors[j]->connector.x,
									m_Pies[i]->m_Connectors[j]->connector.y,
									m_Pies[i]->m_Connectors[j]->connector.z,
									color, style);
				}
			}
		}
	}
}

#ifdef SDL_TTF_TEST
bool	CResMaster::initFont(void) {
	if (TTF_Init() >= 0)
	{
		return true;
	}
	fprintf(stderr, "error initializing SDL_ttf\n");
	return false;
}

bool	CResMaster::loadFont(const char *name, Uint8 size) {
	if (m_Font = TTF_OpenFont(name, size))
	{
		return true;
	}
	fprintf(stderr, "error opening font file %s fontsize %d\n", name, size);
	return false;
}
#endif

void	CResMaster::drawText(const char *text, float objX, float objY, float objZ, SDL_Color color, Uint8 style) {
	GLdouble	screenX, screenY, screenZ;
	GLint		viewport[4];
	GLdouble	modelview[16];
	GLdouble	projection[16];

	glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
	glGetDoublev( GL_PROJECTION_MATRIX, projection );
	glGetIntegerv( GL_VIEWPORT, viewport );

	gluProject(objX, objY, objZ, modelview, projection, viewport, &screenX, &screenY, &screenZ);

	glDisable(GL_TEXTURE_2D);
	glColor3fv((GLfloat*)&color);
	//glMatrixMode(GL_PROJECTION);
	//glPushMatrix();
	//glLoadIdentity();
	//glTranslatef(0.0f, 0.0f, 0.0f);
	//glOrtho(0, g_Screen.m_width, 0, g_Screen.m_height, -1, 1);

	glEnable(GL_TEXTURE_2D);
	//glPopMatrix();
	glColor4ub(255, 255, 255, 255);

#ifdef SDL_TTF_TEST
	SDL_Surface	*textSurface;

	textSurface = TTF_RenderText_Solid( m_Font, text, color);

	//glRasterPos2d(screenX, screenY);
	//glDrawPixels(textSurface->w, textSurface->h, GL_LUMINANCE, GL_UNSIGNED_BYTE, textSurface->pixels);

	SDL_Rect	srcrect, dstrect;

	srcrect.x = 0;
	srcrect.y = 0;
	srcrect.w = textSurface->w;
	srcrect.h = textSurface->h;

	dstrect.x = (Sint16)screenX;
	dstrect.y = (Sint16)screenY;
	dstrect.w = textSurface->w;
	dstrect.h = textSurface->h;

	if (SDL_BlitSurface(textSurface, &srcrect, g_Screen.m_Surface, &dstrect))
	{
		fprintf(stderr, "SDLBlit failed: %s\n", SDL_GetError());
	}

	srcrect.x = 0;
	srcrect.y = 0;
	srcrect.w = 333;
	srcrect.h = 333;

	dstrect.x = 222;
	dstrect.y = 222;
	dstrect.w = 333;
	dstrect.h = 333;

	SDL_FillRect(g_Screen.m_Surface, &dstrect, 0x33445566);
#endif
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
			if (m_scale > 0.05f)
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
			if (this->m_Pies[i])
			{
				this->m_Pies[i]->checkSelection((float)x1, (float)y1, (float)z1,
												(float)x2, (float)y2, (float)z2);
			}
		}

		if (this->m_PolyLinker.isUp())
		{
			if (this->m_PolyLinker.m_Target < 0)
			{
				for (i = 0;i < m_pieCount;i++)
				{
					if (this->m_Pies[i])
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
}

void	CResMaster::startMapTexture(void) {
	Uint16	i, j;

	for (i = 0;i < this->m_pieCount;i++)
	{
		if (this->m_Pies[i])
		{
			CPieInternal	*temp = this->m_Pies[i];
			while (temp)
			{
				temp->hideGUI();
				for (j = 0;j < temp->getPolyCount();j++)
				{
					if (temp->m_Polygons[j] &&
						temp->m_Polygons[j]->selected)
					{
						//TODO:fix 512,512
						this->m_TextureMapper.addTargets(512, 512, temp->m_TexpageId, &temp->m_Polygons[j]->polygon, 1);
						break;
					}
				}
				temp = temp->m_nextSub;
			}
		}
	}
}

void	CResMaster::stopMapTexture(void) {
	Uint16	i;

	this->m_TextureMapper.removeTargets();

	for (i = 0;i < this->m_pieCount;i++)
	{
		if (this->m_Pies[i])
		{
			CPieInternal	*temp = this->m_Pies[i];
			while (temp)
			{
				temp->showGUI();
				temp = temp->m_nextSub;
			}
		}
	}
}

void	CResMaster::mergePies(void) {
	Uint16	i, j, k;
	Uint32	totalVertices = 0, totalConnectors = 0, totalPolygons = 0;

	//Always use page 0 texture name
	this->m_MergedPie = new CPieInternal(0xFFFF, "merged.pie", 0) ;

	for (i = 0;i < this->m_pieCount;i++)
	{
		if (this->m_Pies[i])
		{
			if (totalVertices > pie_MAX_VERTICES)
			{
				fprintf(stderr, "exceeded max vertices when merging pies aborting...\n");
				return;
			}
			if (totalPolygons > pie_MAX_POLYGONS)
			{
				fprintf(stderr, "exceeded max polygons when merging pies aborting...\n");
				return;
			}

			for (j = 0;j < this->m_Pies[i]->m_polyCount;j++)
			{
				if (this->m_Pies[i]->m_Polygons[j])
				{
					this->m_MergedPie->m_Polygons[totalPolygons] = (IMD_POLY_LIST*)malloc(sizeof(IMD_POLY_LIST));
					memset(&this->m_MergedPie->m_Polygons[totalPolygons]->polygon, 0, sizeof(iIMDPoly));

					if (this->m_Pies[i]->m_Polygons[j]->polygon.pTexAnim != NULL)
					{
						this->m_MergedPie->m_Polygons[totalPolygons]->polygon.pTexAnim = (iTexAnim*)malloc(sizeof(iTexAnim));
						memcpy(this->m_MergedPie->m_Polygons[totalPolygons]->polygon.pTexAnim, this->m_Pies[i]->m_Polygons[j]->polygon.pTexAnim, sizeof(iTexAnim));
					}
					else
					{
						this->m_MergedPie->m_Polygons[totalPolygons]->polygon.pTexAnim = NULL;
					}

					this->m_MergedPie->m_Polygons[totalPolygons]->polygon.texCoord = (Vector2f*)malloc(sizeof(Vector2f) * pie_MAX_VERTICES_PER_POLYGON);

					this->m_MergedPie->m_Polygons[totalPolygons]->polygon.pindex = (VERTEXID*)malloc(sizeof(VERTEXID) * pie_MAX_VERTICES_PER_POLYGON);

					for (k = 0;
						k < this->m_Pies[i]->m_Polygons[j]->polygon.npnts;
						k++)
					{
						this->m_MergedPie->m_Polygons[totalPolygons]->polygon.pindex[k] = totalVertices + this->m_Pies[i]->m_Polygons[j]->polygon.pindex[k];
						this->m_MergedPie->m_Polygons[totalPolygons]->polygon.texCoord[k].x = this->m_Pies[i]->m_Polygons[j]->polygon.texCoord[k].x;
						this->m_MergedPie->m_Polygons[totalPolygons]->polygon.texCoord[k].y = this->m_Pies[i]->m_Polygons[j]->polygon.texCoord[k].y;
					}

					this->m_MergedPie->m_Polygons[totalPolygons]->polygon.normal = this->m_Pies[i]->m_Polygons[j]->polygon.normal;
					this->m_MergedPie->m_Polygons[totalPolygons]->polygon.npnts = this->m_Pies[i]->m_Polygons[j]->polygon.npnts;
					this->m_MergedPie->m_Polygons[totalPolygons]->polygon.zcentre = this->m_Pies[i]->m_Polygons[j]->polygon.zcentre;
					this->m_MergedPie->m_Polygons[totalPolygons]->polygon.flags = this->m_Pies[i]->m_Polygons[j]->polygon.flags;
					this->m_MergedPie->m_Polygons[totalPolygons]->id = totalPolygons;
					this->m_MergedPie->m_Polygons[totalPolygons]->selected = false;
					this->m_MergedPie->m_Polygons[totalPolygons]->hasVBO = this->m_Pies[i]->m_Polygons[j]->hasVBO;
					this->m_MergedPie->m_Polygons[totalPolygons]->VBOId = this->m_Pies[i]->m_Polygons[j]->VBOId;
				}
				totalPolygons++;
			}

			for (j = 0;j < this->m_Pies[i]->m_vertCount;j++)
			{
				if (this->m_Pies[i]->m_Vertices[j])
				{
					this->m_MergedPie->m_Vertices[totalVertices] = (VERTICE_LIST*)malloc(sizeof(VERTICE_LIST));
					this->m_MergedPie->m_Vertices[totalVertices]->vertice.x = this->m_Pies[i]->m_Vertices[j]->vertice.x;
					this->m_MergedPie->m_Vertices[totalVertices]->vertice.y = this->m_Pies[i]->m_Vertices[j]->vertice.y;
					this->m_MergedPie->m_Vertices[totalVertices]->vertice.z = this->m_Pies[i]->m_Vertices[j]->vertice.z;
					this->m_MergedPie->m_Vertices[totalVertices]->id = totalVertices;
				}
				totalVertices++;
			}

			for (j = 0;j < this->m_Pies[i]->m_connCount;j++)
			{
				if (this->m_Pies[i]->m_Connectors[j])
				{
					this->m_MergedPie->m_Connectors[totalConnectors] = (CONNECTOR_LIST*)malloc(sizeof(CONNECTOR_LIST));
					this->m_MergedPie->m_Connectors[totalConnectors]->connector.x = this->m_Pies[i]->m_Connectors[j]->connector.x;
					this->m_MergedPie->m_Connectors[totalConnectors]->connector.y = this->m_Pies[i]->m_Connectors[j]->connector.y;
					this->m_MergedPie->m_Connectors[totalConnectors]->connector.z = this->m_Pies[i]->m_Connectors[j]->connector.z;
					this->m_MergedPie->m_Connectors[totalConnectors]->id = totalConnectors;
				}
				totalConnectors++;
			}

			//memcpy(&this->m_MergedPie->m_Vertices[totalVertices], &this->m_Pies[i]->m_Vertices[0], sizeof(VERTICE_LIST)*this->m_Pies[i]->m_vertCount);
			//memcpy(&this->m_MergedPie->m_Connectors[totalConnectors], &this->m_Pies[i]->m_Vertices[0], sizeof(VERTICE_LIST)*this->m_Pies[i]->m_connCount);
			//memcpy(&this->m_MergedPie->m_Polygons[totalPolygons], &this->m_Pies[i]->m_Vertices[0], sizeof(VERTICE_LIST)*this->m_Pies[i]->m_polyCount);

			//totalVertices += this->m_Pies[i]->m_vertCount;
			//totalConnectors += this->m_Pies[i]->m_connCount;
			//totalPolygons += this->m_Pies[i]->m_polyCount;
		}
	}

	this->m_MergedPie->m_vertCount = totalVertices;
	this->m_MergedPie->m_connCount = totalConnectors;
	this->m_MergedPie->m_polyCount = totalPolygons;

	this->m_MergedPie->ToFile("merged.pie", this->m_oldFormat);
	delete this->m_MergedPie;
	this->m_MergedPie = NULL;
}

void	CResMaster::getOGLExtensionString(void) {
	Uint32	num = 0, read = 0, extLength, length;

	const char *extensions = (const char*)glGetString(GL_EXTENSIONS);

	length = strlen(extensions);

	while (read < length)
	{
		sscanf(&extensions[read], "%[^' ']%n", m_oglExtensions[num].string, &extLength);
		read += extLength + 1;
		num++;
	}
	m_numExtensions = num;

#if 0
	FILE	*file = fopen("openglEXTS.txt", "w+");

	fprintf(file, "Supported Opengl Extensions:\n");
	fprintf(file, "Number extensions: %d\n", num);

	for (num = 0;num < m_numExtensions;num++)
	{
		fprintf(file, "%s\n", m_oglExtensions[num].string);
	}
	fclose(file);
#endif
}

bool	CResMaster::isOGLExtensionAvailable(const char* extension) {
	Uint32	i;

	for (i = 0;i < m_numExtensions;i++)
	{
		if (!strncmp(m_oglExtensions[i].string, extension, 255))
		{
			return true;
		}
	}
	return false;
}
