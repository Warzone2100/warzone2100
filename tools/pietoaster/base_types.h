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
#ifndef _base_types_h
#define _base_types_h

#include <SDL.h>
#include <math.h>

#include "wzglobal.h"

typedef struct _named_matrices_4f {
	float	m11, m12, m13, m14;
	float	m21, m22, m23, m24;
	float	m31, m32, m33, m34;
	float	m41, m42, m43, m44;
} NAMED_MATRICES_4F;

typedef struct _named_matrices_4d {
	double	m11, m12, m13, m14;
	double	m21, m22, m23, m24;
	double	m31, m32, m33, m34;
	double	m41, m42, m43, m44;
} NAMED_MATRICES_4D;

class CPieMatrix4F {
public:
	union {
		float	um[16];
		NAMED_MATRICES_4F	m;
	};

	/*
	CPieMatrix4F	operator+(CPieMatrix4F matrix) = {
		CPieMatrix4F(this->m.m11 + matrix.m.m11,
		this->m.m12 + matrix.m.m12,
		this->m.m13 + matrix.m.m13,
		this->m.m14 + matrix.m.m14,
		this->m.m21 + matrix.m.m21,
		this->m.m22 + matrix.m.m22,
		this->m.m23 + matrix.m.m23,
		this->m.m24 + matrix.m.m24,
		this->m.m31 + matrix.m.m31,
		this->m.m32 + matrix.m.m32,
		this->m.m33 + matrix.m.m33,
		this->m.m34 + matrix.m.m34,
		this->m.m41 + matrix.m.m41,
		this->m.m42 + matrix.m.m42,
		this->m.m43 + matrix.m.m43,
		this->m.m44 + matrix.m.m44)
	};
	*/
};

class CPieMatrix4D {
public:
	union {
		double	um[16];
		NAMED_MATRICES_4D	m;
	};
};

#endif
