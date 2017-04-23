/*
	This file is part of Warzone 2100.
	Copyright (C) 2007-2017  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef VECTOR_H
#define VECTOR_H

#if defined(WZ_CC_MSVC)
#pragma warning( disable : 4201)
#endif
#define GLM_SWIZZLE

#include <stdint.h>

#include "wzglobal.h"
#include "frame.h"
#include "lib/framework/types.h"
#include "glm/core/setup.hpp"
#include "glm/core/type.hpp"

using Vector3i = glm::ivec3;
using Vector2i = glm::ivec2;
using Vector2f = glm::vec2;
using Vector3f = glm::vec3;
struct Rotation
{
	Rotation()
	{
		direction = 0;
		pitch = 0;
		roll = 0;
	}
	Rotation(int direction, int pitch, int roll) : direction(direction), pitch(pitch), roll(roll) {}
	Rotation(Vector3i xyz) : direction(xyz.x), pitch(xyz.y), roll(xyz.z) {}
	uint16_t direction, pitch, roll;  ///< Object rotation in 0..64k range
};
typedef Vector3i Position;  ///< Map position in world coordinates

static inline Vector3i toVector(Rotation const &r)
{
	return Vector3i(r.direction, r.pitch, r.roll);
}

// vector * vector -> scalar
// Note: glm doesn't provide dot operator for integral vector.
static inline WZ_DECL_PURE int operator *(Vector2i const &a, Vector2i const &b)
{
	return a.x * b.x + a.y * b.y;
}
static inline WZ_DECL_PURE int operator *(Vector3i const &a, Vector3i const &b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

// iSinCosR(angle, scalar) -> 2d_vector
static inline WZ_DECL_PURE Vector2i iSinCosR(uint16_t a, int32_t r)
{
	return Vector2i(iSinR(a, r), iCosR(a, r));
}

// iAtan2(2d_vector) -> angle
static inline WZ_DECL_PURE int iAtan2(Vector2i const &a)
{
	return iAtan2(a.x, a.y);
}

// iHypot(vector) -> scalar
static inline WZ_DECL_PURE int iHypot(Vector2i const &a)
{
	return iHypot(a.x, a.y);
}
static inline WZ_DECL_PURE int iHypot(Vector3i const &a)
{
	return iHypot3(a.x, a.y, a.z);
}

/*!
 * Rotate v
 * \param v vector to rotate
 * \param angle the amount * 32768/π to rotate in counterclockwise direction
 * \return Result
 */
static inline WZ_DECL_PURE Vector2f Vector2f_Rotate2f(Vector2f v, int angle)
{
	Vector2f result;
	result.x = (v.x * iCos(angle) - v.y * iSin(angle)) / 65536;
	result.y = (v.x * iSin(angle) + v.y * iCos(angle)) / 65536;

	return result;
}


/*!
 * Much the same as Vector2i_InCircle except that it works in 3-axis by discarding the z-component and with
 * circles.
 * \param v Vector to test
 * \param c Vector containing the centre of the circle
 * \param r The radius of the circle
 * \return If v falls within the circle
 */
static inline bool WZ_DECL_PURE Vector3i_InCircle(Vector3i v, Vector3i c, unsigned r)
{
	Vector2i delta = Vector3i(v - c).xy;
	// Explictily cast to "unsigned int" because this number never can be
	// negative, due to the fact that these numbers are squared. Still GCC
	// warns about a comparison of a comparison between an unsigned and a
	// signed integer.
	return (unsigned int)(delta * delta) < r * r;
}


/*!
 * Much the same as Vector2i_InCircle except that it works in 3-axis and with
 * spheres.
 * The equation used is also ever so slightly different:
 * (x - a)^2 + (y - b)^2 + (z - c)^2 = r^2. Notice how it is still squared and
 * _not_ cubed!
 * \param v Vector to test
 * \param c Vector containing the centre of the sphere
 * \param r The radius of the sphere
 * \return If v falls within the sphere
 */
static inline bool WZ_DECL_PURE Vector3i_InSphere(Vector3i v, Vector3i c, unsigned r)
{
	Vector3i delta = v - c;
	// Explictily cast to "unsigned int" because this number never can be
	// negative, due to the fact that these numbers are squared. Still GCC
	// warns about a comparison of a comparison between an unsigned and a
	// signed integer.
	return (unsigned int)(delta * delta) < r * r;
}

#endif // VECTOR_H
