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
#include "gui.h"
#include "config.h"
#include "screen.h"

TwType g_tw_pieVertexType;
TwType g_tw_pieVector2fType;

void	CAntTweakBarTextField::doFunction() {}

void	CAntTweakBarTextField::addTextBox(const char *name)
{
	if (m_Up)
	{
		return;
	}

	char	def[255];
	char	barAttr[255];

    fprintf(stderr, "%s\n", name);

	m_textBar = TwNewBar(name);

	strncpy(m_name, name, strlen(name));

	snprintf(barAttr, 255, "%s color = '%d %d %d %d' size = '200 100'", name, guiMajorAlpha, guiMajorRed, guiMajorGreen, guiMajorBlue);
	TwDefine(barAttr);

	Uint16	positionX = g_Screen.m_width / 2 - 100;
	Uint16	positionY = g_Screen.m_height / 2 - 50;
	snprintf(barAttr, 255, "%s position = '%d %d'", name, positionX, positionY);
	TwDefine(barAttr);

	m_Up = true;

	m_textIndex = 0;

	snprintf(def, 255, " label='%s'", name);
	TwAddVarRO(m_textBar, name, TW_TYPE_INT32, &m_textIndex, def);
}

void	CAntTweakBarTextField::incrementChar(Uint16 key)
{
	if (m_textIndex < 255 - 1)
	{
		char str = (Uint8)key;

		strncpy(&m_text[m_textIndex], &str, 1);
		m_textIndex++;
	}
}

void	CAntTweakBarTextField::decrementChar()
{
	if (m_textIndex > 0)
	{
		m_text[m_textIndex] = '\0';
		m_textIndex--;
	}
}

void	CAntTweakBarTextField::deleteTextBox()
{
	/*copies chars from mychars to text_pointer location if both
	values are not null */
	/*
	if (text_pointer && mychars_index)
	{
		int size = mychars_index;
		strncpy(text_pointer, mychars, size);
		text_pointer = NULL;
	}
	*/

	memset(m_text, '\0', 255);
	memset(m_name, '\0', 255);

	TwDeleteBar(m_textBar);
	m_Up = false;
}

void	CAntTweakBarTextField::updateTextBox(void)
{
	char test[255] = " label='";

	TwRemoveVar(m_textBar, m_name);

	snprintf(&test[8], m_textIndex, "%s", &m_text[0]);
	snprintf(&test[8 + m_textIndex], 1, "\'");

	TwAddVarRO(m_textBar, m_name, TW_TYPE_INT32, &m_textIndex, test);
}
