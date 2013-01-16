/*
	This file is part of Warzone 2100.
	Copyright (C) 2007-2013  Warzone 2100 Project

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

#include "wzglobal.h"

#include <assert.h>

#include "fixedpoint.h"
#include "frame.h"
#include "math_ext.h"

struct Rotation;
struct Vector3i;
struct Vector2i
{
	Vector2i() {}
	Vector2i(int x, int y) : x(x), y(y) {}
	Vector2i(Vector3i const &r); // discards the z value

	int x, y;
};
struct Vector2f
{
	Vector2f() {}
	Vector2f(float x, float y) : x(x), y(y) {}
	Vector2f(Vector2i const &v) : x(v.x), y(v.y) {}

	float x, y;
};
struct Vector3i
{
	Vector3i() {}
	Vector3i(int x, int y, int z) : x(x), y(y), z(z) {}
	Vector3i(Vector2i const &xy, int z) : x(xy.x), y(xy.y), z(z) {}
	Vector3i(Rotation const &r);
	int x, y, z;
};
struct Vector3f
{
	Vector3f() {}
	Vector3f(float x, float y, float z) : x(x), y(y), z(z) {}
	Vector3f(Vector3i const &v) : x(v.x), y(v.y), z(v.z) {}
	Vector3f(Vector2f const &xy, float z) : x(xy.x), y(xy.y), z(z) {}

	float x, y, z;
};
struct Rotation
{
	Rotation() { direction = 0; pitch = 0; roll = 0; }
	Rotation(int direction, int pitch, int roll) : direction(direction), pitch(pitch), roll(roll) {}
	Rotation(Vector3i xyz) : direction(xyz.x), pitch(xyz.y), roll(xyz.z) {}
	uint16_t direction, pitch, roll;  ///< Object rotation in 0..64k range
};
typedef Vector3i Position;  ///< Map position in world coordinates

inline Vector3i::Vector3i(Rotation const &r) : x(r.direction), y(r.pitch), z(r.roll) {}
inline Vector2i::Vector2i(Vector3i const &r) : x(r.x), y(r.y) {}

// removeZ(3d_vector) -> 2d_vector
static inline WZ_DECL_PURE Vector2i removeZ(Vector3i const &a) { return Vector2i(a.x, a.y); }
static inline WZ_DECL_PURE Vector2f removeZ(Vector3f const &a) { return Vector2f(a.x, a.y); }


// vector == vector -> bool
static inline WZ_DECL_PURE bool operator ==(Vector2i const &a, Vector2i const &b) { return a.x == b.x && a.y == b.y; }
static inline WZ_DECL_PURE bool operator ==(Vector2f const &a, Vector2f const &b) { return a.x == b.x && a.y == b.y; }
static inline WZ_DECL_PURE bool operator ==(Vector3i const &a, Vector3i const &b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
static inline WZ_DECL_PURE bool operator ==(Vector3f const &a, Vector3f const &b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
static inline WZ_DECL_PURE bool operator ==(Rotation const &a, Rotation const &b) { return a.direction == b.direction && a.pitch == b.pitch && a.roll == b.roll; }

// vector != vector -> bool
static inline WZ_DECL_PURE bool operator !=(Vector2i const &a, Vector2i const &b) { return a.x != b.x || a.y != b.y; }
static inline WZ_DECL_PURE bool operator !=(Vector2f const &a, Vector2f const &b) { return a.x != b.x || a.y != b.y; }
static inline WZ_DECL_PURE bool operator !=(Vector3i const &a, Vector3i const &b) { return a.x != b.x || a.y != b.y || a.z != b.z; }
static inline WZ_DECL_PURE bool operator !=(Vector3f const &a, Vector3f const &b) { return a.x != b.x || a.y != b.y || a.z != b.z; }
static inline WZ_DECL_PURE bool operator !=(Rotation const &a, Rotation const &b) { return a.direction != b.direction || a.pitch != b.pitch || a.roll != b.roll; }

// vector + vector -> vector
static inline WZ_DECL_PURE Vector2i operator +(Vector2i const &a, Vector2i const &b) { return Vector2i(a.x + b.x, a.y + b.y); }
static inline WZ_DECL_PURE Vector2f operator +(Vector2f const &a, Vector2f const &b) { return Vector2f(a.x + b.x, a.y + b.y); }
static inline WZ_DECL_PURE Vector3i operator +(Vector3i const &a, Vector3i const &b) { return Vector3i(a.x + b.x, a.y + b.y, a.z + b.z); }
static inline WZ_DECL_PURE Vector3f operator +(Vector3f const &a, Vector3f const &b) { return Vector3f(a.x + b.x, a.y + b.y, a.z + b.z); }
//static inline WZ_DECL_PURE Rotation operator +(Rotation const &a, Rotation const &b) { return Rotation((int16_t)a.direction + (int16_t)b.direction, (int16_t)a.pitch + (int16_t)b.pitch, (int16_t)a.roll + (int16_t)b.roll); }

// vector - vector -> vector
static inline WZ_DECL_PURE Vector2i operator -(Vector2i const &a, Vector2i const &b) { return Vector2i(a.x - b.x, a.y - b.y); }
static inline WZ_DECL_PURE Vector2f operator -(Vector2f const &a, Vector2f const &b) { return Vector2f(a.x - b.x, a.y - b.y); }
static inline WZ_DECL_PURE Vector3i operator -(Vector3i const &a, Vector3i const &b) { return Vector3i(a.x - b.x, a.y - b.y, a.z - b.z); }
static inline WZ_DECL_PURE Vector3f operator -(Vector3f const &a, Vector3f const &b) { return Vector3f(a.x - b.x, a.y - b.y, a.z - b.z); }
//static inline WZ_DECL_PURE Rotation operator -(Rotation const &a, Rotation const &b) { return Rotation((int16_t)a.direction - (int16_t)b.direction, (int16_t)a.pitch - (int16_t)b.pitch, (int16_t)a.roll - (int16_t)b.roll); }

// -vector -> vector
static inline WZ_DECL_PURE Vector2i operator -(Vector2i const &a) { return Vector2i(-a.x, -a.y); }
static inline WZ_DECL_PURE Vector2f operator -(Vector2f const &a) { return Vector2f(-a.x, -a.y); }
static inline WZ_DECL_PURE Vector3i operator -(Vector3i const &a) { return Vector3i(-a.x, -a.y, -a.z); }
static inline WZ_DECL_PURE Vector3f operator -(Vector3f const &a) { return Vector3f(-a.x, -a.y, -a.z); }

// vector * scalar -> vector
static inline WZ_DECL_PURE Vector2i operator *(Vector2i const &a, int   s) { return Vector2i(a.x*s, a.y*s); }
static inline WZ_DECL_PURE Vector2f operator *(Vector2f const &a, float s) { return Vector2f(a.x*s, a.y*s); }
static inline WZ_DECL_PURE Vector3i operator *(Vector3i const &a, int   s) { return Vector3i(a.x*s, a.y*s, a.z*s); }
static inline WZ_DECL_PURE Vector3f operator *(Vector3f const &a, float s) { return Vector3f(a.x*s, a.y*s, a.z*s); }
//static inline WZ_DECL_PURE Rotation operator *(Rotation const &a, int   s) { return Rotation((int16_t)a.direction*s, (int16_t)a.pitch*s, (int16_t)a.roll*s); }

// vector / scalar -> vector
static inline WZ_DECL_PURE Vector2i operator /(Vector2i const &a, int   s) { return Vector2i(a.x/s, a.y/s); }
static inline WZ_DECL_PURE Vector2f operator /(Vector2f const &a, float s) { return Vector2f(a.x/s, a.y/s); }
static inline WZ_DECL_PURE Vector3i operator /(Vector3i const &a, int   s) { return Vector3i(a.x/s, a.y/s, a.z/s); }
static inline WZ_DECL_PURE Vector3f operator /(Vector3f const &a, float s) { return Vector3f(a.x/s, a.y/s, a.z/s); }
//static inline WZ_DECL_PURE Rotation operator /(Rotation const &a, int   s) { return Rotation((int16_t)a.direction/s, (int16_t)a.pitch/s, (int16_t)a.roll/s); }

// vector * vector -> scalar
static inline WZ_DECL_PURE int   operator *(Vector2i const &a, Vector2i const &b) { return a.x*b.x + a.y*b.y; }
static inline WZ_DECL_PURE float operator *(Vector2f const &a, Vector2f const &b) { return a.x*b.x + a.y*b.y; }
static inline WZ_DECL_PURE int   operator *(Vector3i const &a, Vector3i const &b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline WZ_DECL_PURE float operator *(Vector3f const &a, Vector3f const &b) { return a.x*b.x + a.y*b.y + a.z*b.z; }

// normalise(vector) -> scalar
static inline WZ_DECL_PURE Vector2f normalise(Vector2f const &a) { float sq = a*a; if (sq == 0.0f) return Vector2f(0.0f, 0.0f);       return a / sqrtf(sq); }
static inline WZ_DECL_PURE Vector3f normalise(Vector3f const &a) { float sq = a*a; if (sq == 0.0f) return Vector3f(0.0f, 0.0f, 0.0f); return a / sqrtf(sq); }

// iSinCosR(angle, scalar) -> 2d_vector
static inline WZ_DECL_PURE Vector2i iSinCosR(uint16_t a, int32_t r) { return Vector2i(iSinR(a, r), iCosR(a, r)); }

// iAtan2(2d_vector) -> angle
static inline WZ_DECL_PURE int iAtan2(Vector2i const &a) { return iAtan2(a.x, a.y); }

// iHypot(vector) -> scalar
static inline WZ_DECL_PURE int iHypot(Vector2i const &a) { return iHypot(a.x, a.y); }
static inline WZ_DECL_PURE int iHypot(Vector3i const &a) { return iHypot3(a.x, a.y, a.z); }

// swapYZ(3d_vector) -> 3d_vector
static inline WZ_DECL_PURE Vector3i swapYZ(Vector3i a) { return Vector3i(a.x, a.z, a.y); }
static inline WZ_DECL_PURE Vector3f swapYZ(Vector3f a) { return Vector3f(a.x, a.z, a.y); }

// vector × vector -> scalar
static inline WZ_DECL_PURE Vector3i crossProduct(Vector3i const &a, Vector3i const &b) { return Vector3i(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x); }
static inline WZ_DECL_PURE Vector3f crossProduct(Vector3f const &a, Vector3f const &b) { return Vector3f(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x); }

// vector += vector
static inline Vector2i const &operator +=(Vector2i &a, Vector2i const &b) { return a = a + b; }
static inline Vector2f const &operator +=(Vector2f &a, Vector2f const &b) { return a = a + b; }
static inline Vector3i const &operator +=(Vector3i &a, Vector3i const &b) { return a = a + b; }
static inline Vector3f const &operator +=(Vector3f &a, Vector3f const &b) { return a = a + b; }

// vector -= vector
static inline Vector2i const &operator -=(Vector2i &a, Vector2i const &b) { return a = a - b; }
static inline Vector2f const &operator -=(Vector2f &a, Vector2f const &b) { return a = a - b; }
static inline Vector3i const &operator -=(Vector3i &a, Vector3i const &b) { return a = a - b; }
static inline Vector3f const &operator -=(Vector3f &a, Vector3f const &b) { return a = a - b; }


/*!
 * Rotate v
 * \param v vector to rotate
 * \param angle the amount * 32768/π to rotate in counterclockwise direction
 * \return Result
 */
static inline WZ_DECL_PURE Vector2f Vector2f_Rotate2f(Vector2f v, int angle)
{
	Vector2f result;
	result.x = (v.x*iCos(angle) - v.y*iSin(angle)) / 65536;
	result.y = (v.x*iSin(angle) + v.y*iCos(angle)) / 65536;

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
static inline WZ_DECL_CONST bool Vector3i_InCircle(const Vector3i v, const Vector3i c, const unsigned int r)
{
	Vector2i delta = removeZ(v - c);
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
static inline WZ_DECL_CONST bool Vector3i_InSphere (const Vector3i v, const Vector3i c, const unsigned int r)
{
	Vector3i delta = v - c;
	// Explictily cast to "unsigned int" because this number never can be
	// negative, due to the fact that these numbers are squared. Still GCC
	// warns about a comparison of a comparison between an unsigned and a
	// signed integer.
	return (unsigned int)(delta * delta) < r * r;
}


#endif // VECTOR_H
