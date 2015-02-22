/*
 *	This file is part of Warzone 2100.
 *	Copyright (C) 2013-2015  Warzone 2100 Project
 * 
 *	Warzone 2100 is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 * 
 *	Warzone 2100 is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *	GNU General Public License for more details.
 * 
 *	You should have received a copy of the GNU General Public License
 *	along with Warzone 2100; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _LIB_FRAMEWORK_GEOMETRY_H
#define _LIB_FRAMEWORK_GEOMETRY_H

#include "frame.h"
#include "vector.h"
#include <limits>

/**
 * Fixed point 3D affine transformation.
 */
class Affine3F {

	/* Compact 4x4 matrix where the 4th row is assumed to be [0 0 0 1]
	 * since we're representing an affine transformation
	 */
	int32_t  m[3][4];

public:
	static const int FRAC_BITS = 12;
	static const int INT_BITS = std::numeric_limits<int32_t>::digits - FRAC_BITS;
	static const int FP_MULTIPLIER = 1 << FRAC_BITS;

	Affine3F() {
		m[0][0]=FP_MULTIPLIER;	m[0][1]=0;				m[0][2]=0;				m[0][3]=0;
		m[1][0]=0;				m[1][1]=FP_MULTIPLIER;	m[1][2]=0;				m[1][3]=0;
		m[2][0]=0;				m[2][1]=0;				m[2][2]=FP_MULTIPLIER;	m[2][3]=0;
	}
	Affine3F& RotX(uint16_t);
	Affine3F& RotY(uint16_t);
	Affine3F& RotZ(uint16_t);
	Affine3F& RotZXY(Rotation);
	Affine3F& RotInvZXY(Rotation);
	Affine3F& RotYXZ(Rotation);
	Affine3F& RotInvYXZ(Rotation);
	Affine3F& Trans(int32_t,int32_t,int32_t);
	Affine3F& Trans(Vector3i);
	Affine3F& Scale(int32_t); // Note: parameter assummed to be a scaled integer (scaled by FP_MULTIPLIER)
	Affine3F& InvRot();
	Vector3i translation() const;
	Vector3i operator*(const Vector3i) const;
	Affine3F operator*(const Affine3F&) const;
	Vector3i InvRot(const Vector3i) const;
};

#endif // _LIB_FRAMEWORK_GEOMETRY_H
