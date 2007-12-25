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
#ifndef _resmaster_h
#define _resmaster_h

#include <SDL_opengl.h>

#include "wzglobal.h"
#include "pie_types.h"

#include "imdloader.h"
#include "texture.h"

#include "pie_internal.h"

#include "texture_mapper.h"

#include "AntTweakBar.h"

extern Uint16 shiftChar(Uint16 key);

typedef struct _ogl_extension_string {
	char	string[255];
} OGL_EXTENSION_STRING;

class CResMaster {
public:
	enum {
		FILL = 0,
		LINE = 1,
		POINT = 2,
		SMOOTH = 0,
		FLAT = 1
	};

	static const Uint32 MAX_GL_EXTENSIONS = 500;

	OGL_EXTENSION_STRING	m_oglExtensions[MAX_GL_EXTENSIONS];
	Uint32	m_numExtensions;

	Uint32	m_textureToProcess;
	Uint32	m_pieCount;
	Uint32	m_numNewPies;

	float	m_rotateX;	///<rotation of models X
	float	m_rotateY;	///<rotation of models X
	float	m_scale;	///<scale of models
	Uint32	m_polygonMode;	///<polygon mode to render polygons
	bool	m_highLightVertices;	///<highlight vertice or not
	bool	m_highLightConnectors;	///<highlight connector or not
	bool	m_highLightSelected;
	bool	m_drawAxis;	///<whether to draw X,Y,Z axis or not
	bool	m_drawGrids;
	bool	m_drawTexture;	///<whether to draw textures or not
	bool	m_drawNewVertice;	///<whether to draw new vertice position or not
	bool	m_drawTexts;	///<whether to draw selected vertice/poly/connector id text or not
	Uint32	m_shadeMode;	///<shade mode of polygons
	float	m_gamma;	///<gamma correction of renderer
	Sint32	m_newPieTextureId;	///<new pie textureId
	bool	m_oldFormat;	///old integer format

	CResMaster();

	~CResMaster();
	bool	isLinkerUp(void) {return m_PolyLinker.m_Up;};
	void	activeLinker(void);
	void	deactiveLinker(void);
	bool	readTextureList(const char *filename);
	bool	loadTexPages(void);
	bool	freeTexPages(void);

	void	enableTexture(bool value);

	bool	addPie(iIMDShape *imd, const char *name);
	bool	removePieAt(Uint16 i);
	CPieInternal *getPieAt(Uint16 i);

	bool	addGUI(void);

	void	logic(void);

	void	draw(void);

	void	drawAxis(void);
	void	drawGrids(void);
	void	drawTexts(void);

#ifdef	SDL_TTF_TEST
	TTF_Font		*m_Font;

	bool	initFont(void);
	bool	loadFont(const char *name, Uint8 size);
#endif
	void	drawText(const char *text, float objX, float objY, float ObjZ, SDL_Color color, Uint8 style);

	void	unprojectMouseXY(double *newX1, double *newY1, double *newZ1,
								double *newX2, double *newY2, double *newZ2);

	void	updateInput(void);

	bool	isTextureMapperUp(void) {return m_TextureMapper.m_Up;};
	void	startMapTexture(void);
	void	stopMapTexture(void);

	void	cacheGridsVertices(void);

	void	mergePies(void);

	void	getOGLExtensionString(void);
	bool	isOGLExtensionAvailable(const char *extension);
private:
	CPieInternal	*m_Pies[MAX_PIES];
	CPieInternal	*m_MergedPie;
	CPolygonLinker	m_PolyLinker;
	CTextureMapper	m_TextureMapper;
	char			m_TexPageNames[MAX_TEX_PAGES][MAX_FILE_NAME_LENGTH];
	SDL_Surface		*m_TexSurfaces[MAX_TEX_PAGES];

	Vector3f		m_GridCacheVertices[65536];
	Uint32			m_GridCacheCount;
	GLuint			m_GridVBOId;

	TwBar			*m_utilBar;
	TwBar			*m_textureBar;
};

extern CResMaster ResMaster;

#endif
