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

#ifndef _texture_mapper_h
#define _texture_mapper_h

#include "wzglobal.h"
#include "pie_types.h"

#include "imdloader.h"
#include "texture.h"

#include "pie_internal.h"

class CTextureMapper {
public:
	bool	m_Up;

	///the radius of each point's selection circle
	static const Uint16 m_selectionRadius = 5;

	CTextureMapper() : m_Up(false) {};

	void	addTargets(Uint16 width, Uint16 height, Uint16 textureId, iIMDPoly *target, Uint16 quantity);
	void	removeTargets(void);
	void	draw(void);

	void	addGUI(void);
	void	updateGUI(void);
private:
	Sint32	translateX(float texCoord);
	Sint32	translateY(float texCoord);

	/*
	 (X0,Y0)-------------(X1,Y1)
			|			|
			|			|
			|			|
	 (X3,Y3)-------------(X2,Y2)
	*/
	//Texture page rasterization positions
	Sint16	m_rasterX0;
	Sint16	m_rasterY0;
	Sint16	m_rasterX1;
	Sint16	m_rasterY1;
	Sint16	m_rasterX2;
	Sint16	m_rasterY2;
	Sint16	m_rasterX3;
	Sint16	m_rasterY3;

	Uint16	m_width;
	Uint16	m_height;
	Uint16	m_textureId;
	///Pointer to the polys
	iIMDPoly	*m_targets[pie_MAX_POLYGONS];
	///Quantity of polys
	Uint16	m_quantity;
	///selected or not
	bool	m_Selected[pie_MAX_POLYGONS][pie_MAX_VERTICES_PER_POLYGON];

	TwBar	*m_GUI;

	friend class CPieInternal;
};

#endif
