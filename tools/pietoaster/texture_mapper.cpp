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
#include "texture_mapper.h"
#include "texture.h"
#include "screen.h"

void	CTextureMapper::addTargets(Uint16 width, Uint16 height, Uint16 textureId, iIMDPoly *target, Uint16 quantity) {
	Uint16	i;

	if (this->m_Up)
	{
		return;
	}

	m_width = width;
	m_height = height;
	m_textureId = textureId;
	m_quantity = quantity;

	for (i = 0;i < m_quantity;i++)
	{
		m_targets[i] = &target[i];
	}

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, g_Screen.m_width, 0, g_Screen.m_height, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	//assert(g_screen.m_width >= m_width && g_screen.m_height >= m_height);

	// Work out the rasterization position
	m_rasterX0 = m_rasterX3 = - m_width/2;
	m_rasterX1 = m_rasterX2 = m_rasterX0 + m_width;
	m_rasterY0 = m_rasterY1 = -m_height/2;
	m_rasterY2 = m_rasterY3 = m_rasterY0 + m_height;

	this->addGUI();

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	m_Up = true;
}

void	CTextureMapper::removeTargets() {
	Uint16	i;

	if (!this->m_Up)
	{
		return;
	}

	TwDeleteBar(m_GUI);

	for (i = 0;i < m_quantity;i++)
	{
		m_targets[i] = NULL;
	}

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_TEXTURE_2D);

	glTranslatef(0.0f, 0.0f, 0.0f);

	m_Up = false;
}

void	CTextureMapper::draw(void) {
	glBindTexture(GL_TEXTURE_2D, _TEX_PAGE[m_textureId].id);

	GLint x0 = m_rasterX0, x1 = m_rasterX1, x2 = m_rasterX2, x3 = m_rasterX3;
	GLint y0 = m_rasterY0, y1 = m_rasterY1, y2 = m_rasterY2, y3 = m_rasterY3;

	glScalef(0.25f, 0.25f, 0.25f);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 1.0f);
	glVertex2i(x0, y0);
	glTexCoord2f(1.0f, 1.0f);
	glVertex2i(x1, y1);
	glTexCoord2f(1.0f, 0.0f);
	glVertex2i(x2, y2);
	glTexCoord2f(0.0f, 0.0f);
	glVertex2i(x3, y3);
	glEnd();

	//glBindTexture(GL_TEXTURE_2D, -1);
	glDisable(GL_TEXTURE_2D);

	//Draw polygon vertices/points
	Sint32	i, x, y;
	glColor4ub(255, 192, 255, 255);
	for (i = 0;i < m_targets[0]->npnts;i++)
	{

#ifdef PROPER_TEXTURE_COORDS
		x = this->translateX(m_targets[0]->texCoord[i].x/m_width);
		y = this->translateY(m_targets[0]->texCoord[i].y/m_height);
#else
		x = this->translateX(m_targets[0]->texCoord[i].x/256.0f);
		y = this->translateY(m_targets[0]->texCoord[i].y/256.0f);
#endif

		glBegin(GL_LINES);
		glVertex2i(x + 2 + x0, -(y + 2 + y0));
		glVertex2i(x - 2 + x0, -(y - 2 + y0));
		glVertex2i(x + 2 + x0, -(y - 2 + y0));
		glVertex2i(x - 2 + x0, -(y + 2 + y0));
		glEnd();
	}

	glBegin(GL_LINE_LOOP);
	for (i = 1;i < m_targets[0]->npnts + 1;i++)
	{
		//links to point 0 when reaching last point
		if (i == m_targets[0]->npnts)
		{

#ifdef PROPER_TEXTURE_COORDS
			x = this->translateX(m_targets[0]->texCoord[i-1].x/m_width);
			y = this->translateY(m_targets[0]->texCoord[i-1].y/m_height);
#else
			x = this->translateX(m_targets[0]->texCoord[i-1].x/256.0f);
			y = this->translateY(m_targets[0]->texCoord[i-1].y/256.0f);
#endif

			glVertex2i(x + x0, -(y + y0));

#ifdef PROPER_TEXTURE_COORDS
			x = this->translateX(m_targets[0]->texCoord[0].x/m_width);
			y = this->translateY(m_targets[0]->texCoord[0].y/m_height);
#else
			x = this->translateX(m_targets[0]->texCoord[0].x/256.0f);
			y = this->translateY(m_targets[0]->texCoord[0].y/256.0f);
#endif

			glVertex2i(x + x0, -(y + y0));
		}
		else
		{

#ifdef PROPER_TEXTURE_COORDS
			x = this->translateX(m_targets[0]->texCoord[i-1].x/m_width);
			y = this->translateY(m_targets[0]->texCoord[i-1].y/m_height);
#else
			x = this->translateX(m_targets[0]->texCoord[i-1].x/256.0f);
			y = this->translateY(m_targets[0]->texCoord[i-1].y/256.0f);
#endif

			glVertex2i(x + x0, -(y + y0));

#ifdef PROPER_TEXTURE_COORDS
			x = this->translateX(m_targets[0]->texCoord[i].x/m_width);
			y = this->translateY(m_targets[0]->texCoord[i].y/m_height);
#else
			x = this->translateX(m_targets[0]->texCoord[i].x/256.0f);
			y = this->translateY(m_targets[0]->texCoord[i].y/256.0f);
#endif
			glVertex2i(x + x0, -(y + y0));
		}
	}
	glEnd();
	glColor4ub(255, 255, 255, 255);
	glEnable(GL_TEXTURE_2D);
}

///Translate texture coord X in float into screen coords in int
Sint32	CTextureMapper::translateX(float texCoord) {
	return (Sint32)(texCoord * m_width);
}

///Translate texture coord Y in float into screen coords in int
Sint32	CTextureMapper::translateY(float texCoord) {
	return (Sint32)(texCoord * m_height);
}

void	CTextureMapper::addGUI(void) {
	m_GUI = TwNewBar("TextureMapper");

	char	tmDefine[255];
	snprintf(tmDefine, 255, "TextureMapper ");

	Uint16	i, j;

	//assert(m_quantity < 10 && m_targets[i]->npnts < 10)

	for (i = 0;i < m_quantity;i++)
	{
		for (j = 0;j < m_targets[i]->npnts;j++)
		{
			char	varName[255];
			char	varDefine[255];

			snprintf(varName, 255, "target%uN%u", i, j);
			//TODO:non-power-of-two support?
			snprintf(varDefine, 255, "label = 'vertice%u'", j);

			TwAddVarRW(m_GUI, varName, g_tw_pieVector2fType, &m_targets[i]->texCoord[j], varDefine);
		}
	}
}
