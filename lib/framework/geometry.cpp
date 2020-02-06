/*
	This file is part of Warzone 2100.
	Copyright (C) 2011-2020  Warzone 2100 Project

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

#include "geometry.h"

#include <algorithm>
#include "trig.h"

Affine3F &Affine3F::RotX(uint16_t x)
{
	if (x != 0)
	{
		int32_t tmp;
		const int64_t cra = iCos(x), sra = iSin(x);

		tmp = static_cast<int32_t>((cra * m[0][1] + sra * m[0][2]) >> 16);
		m[0][2] = static_cast<int32_t>((cra * m[0][2] - sra * m[0][1]) >> 16);
		m[0][1] = tmp;

		tmp = static_cast<int32_t>((cra * m[1][1] + sra * m[1][2]) >> 16);
		m[1][2] = static_cast<int32_t>((cra * m[1][2] - sra * m[1][1]) >> 16);
		m[1][1] = tmp;

		tmp = static_cast<int32_t>((cra * m[2][1] + sra * m[2][2]) >> 16);
		m[2][2] = static_cast<int32_t>((cra * m[2][2] - sra * m[2][1]) >> 16);
		m[2][1] = tmp;
	}
	return *this;
}

Affine3F &Affine3F::RotY(uint16_t y)
{
	if (y != 0)
	{
		int32_t tmp;
		int64_t cra = iCos(y), sra = iSin(y);

		tmp = static_cast<int32_t>((cra * m[0][0] - sra * m[0][2]) >> 16);
		m[0][2] = static_cast<int32_t>((sra * m[0][0] + cra * m[0][2]) >> 16);
		m[0][0] = tmp;

		tmp = static_cast<int32_t>((cra * m[1][0] - sra * m[1][2]) >> 16);
		m[1][2] = static_cast<int32_t>((sra * m[1][0] + cra * m[1][2]) >> 16);
		m[1][0] = tmp;

		tmp = static_cast<int32_t>((cra * m[2][0] - sra * m[2][2]) >> 16);
		m[2][2] = static_cast<int32_t>((sra * m[2][0] + cra * m[2][2]) >> 16);
		m[2][0] = tmp;

	}
	return *this;
}

Affine3F &Affine3F::RotZ(uint16_t z)
{
	if (z != 0)
	{
		int32_t tmp;
		int64_t cra = iCos(z), sra = iSin(z);

		tmp = static_cast<int32_t>((cra * m[0][0] + sra * m[0][1]) >> 16);
		m[0][1] = static_cast<int32_t>((cra * m[0][1] - sra * m[0][0]) >> 16);
		m[0][0] = tmp;

		tmp = static_cast<int32_t>((cra * m[1][0] + sra * m[1][1]) >> 16);
		m[1][1] = static_cast<int32_t>((cra * m[1][1] - sra * m[1][0]) >> 16);
		m[1][0] = tmp;

		tmp = static_cast<int32_t>((cra * m[2][0] + sra * m[2][1]) >> 16);
		m[2][1] = static_cast<int32_t>((cra * m[2][1] - sra * m[2][0]) >> 16);
		m[2][0] = tmp;
	}
	return *this;
}

Affine3F &Affine3F::RotZXY(Rotation r)
{
	return (*this).RotZ(r.direction).RotX(r.pitch).RotY(r.roll);
}

Affine3F &Affine3F::RotInvZXY(Rotation r)
{
	(*this) = (*this) * (Affine3F().RotZ(r.direction).RotX(r.pitch).RotY(r.roll).InvRot());
	return *this;
}

Affine3F &Affine3F::RotYXZ(Rotation r)
{
	return (*this).RotY(r.roll).RotX(r.pitch).RotZ(r.direction);
}

Affine3F &Affine3F::RotInvYXZ(Rotation r)
{
	(*this) = (*this) * (Affine3F().RotY(r.roll).RotX(r.pitch).RotZ(r.direction));
	return *this;
}

Affine3F &Affine3F::Trans(int32_t x, int32_t y, int32_t z)
{
	m[0][3] += x * m[0][0] + y * m[0][1] + z * m[0][2];
	m[1][3] += x * m[1][0] + y * m[1][1] + z * m[1][2];
	m[2][3] += x * m[2][0] + y * m[2][1] + z * m[2][2];
	return *this;
}

Affine3F &Affine3F::Trans(Vector3i tr)
{
	return (*this).Trans(tr.x, tr.y, tr.z);
}

Affine3F &Affine3F::Scale(int32_t s)
{
	m[0][0] = ((int64_t)m[0][0] * s) / FP_MULTIPLIER;
	m[0][1] = ((int64_t)m[0][1] * s) / FP_MULTIPLIER;
	m[0][2] = ((int64_t)m[0][2] * s) / FP_MULTIPLIER;
	m[1][0] = ((int64_t)m[1][0] * s) / FP_MULTIPLIER;
	m[1][1] = ((int64_t)m[1][1] * s) / FP_MULTIPLIER;
	m[1][2] = ((int64_t)m[1][2] * s) / FP_MULTIPLIER;
	m[2][0] = ((int64_t)m[2][0] * s) / FP_MULTIPLIER;
	m[2][1] = ((int64_t)m[2][1] * s) / FP_MULTIPLIER;
	m[2][2] = ((int64_t)m[2][2] * s) / FP_MULTIPLIER;
	return *this;
}

Affine3F &Affine3F::InvRot()
{
	std::swap(m[0][1], m[1][0]);
	std::swap(m[0][2], m[2][0]);
	std::swap(m[1][0], m[0][1]);
	std::swap(m[1][1], m[1][1]);
	std::swap(m[1][2], m[2][1]);
	std::swap(m[2][0], m[0][2]);
	std::swap(m[2][1], m[1][2]);
	std::swap(m[2][2], m[2][2]);
	return (*this);
}

Vector3i Affine3F::translation() const
{
	return Vector3i(m[0][3] / FP_MULTIPLIER, m[1][3] / FP_MULTIPLIER, m[2][3] / FP_MULTIPLIER);
}

Vector3i Affine3F::operator*(const Vector3i v) const
{
	Vector3i s;
	s.x = ((int64_t)v.x * m[0][0] + (int64_t)v.y * m[0][1] + (int64_t)v.z * m[0][2] + m[0][3]) / FP_MULTIPLIER;
	s.y = ((int64_t)v.x * m[1][0] + (int64_t)v.y * m[1][1] + (int64_t)v.z * m[1][2] + m[1][3]) / FP_MULTIPLIER;
	s.z = ((int64_t)v.x * m[2][0] + (int64_t)v.y * m[2][1] + (int64_t)v.z * m[2][2] + m[2][3]) / FP_MULTIPLIER;
	return s;
}

Affine3F Affine3F::operator*(const Affine3F &rhs) const
{
	Affine3F tr;
#define i64 int64_t
	tr.m[0][0] = ((i64)m[0][0] * rhs.m[0][0] + (i64)m[0][1] * rhs.m[1][0] + (i64)m[0][2] * rhs.m[2][0]) / FP_MULTIPLIER;
	tr.m[0][1] = ((i64)m[0][0] * rhs.m[0][1] + (i64)m[0][1] * rhs.m[1][1] + (i64)m[0][2] * rhs.m[2][1]) / FP_MULTIPLIER;
	tr.m[0][2] = ((i64)m[0][0] * rhs.m[0][2] + (i64)m[0][1] * rhs.m[1][2] + (i64)m[0][2] * rhs.m[2][2]) / FP_MULTIPLIER;
	tr.m[0][3] = ((i64)m[0][0] * rhs.m[0][3] + (i64)m[0][1] * rhs.m[1][3] + (i64)m[0][2] * rhs.m[2][3]) / FP_MULTIPLIER + m[0][3];

	tr.m[1][0] = ((i64)m[1][0] * rhs.m[0][0] + (i64)m[1][1] * rhs.m[1][0] + (i64)m[1][2] * rhs.m[2][0]) / FP_MULTIPLIER;
	tr.m[1][1] = ((i64)m[1][0] * rhs.m[0][1] + (i64)m[1][1] * rhs.m[1][1] + (i64)m[1][2] * rhs.m[2][1]) / FP_MULTIPLIER;
	tr.m[1][2] = ((i64)m[1][0] * rhs.m[0][2] + (i64)m[1][1] * rhs.m[1][2] + (i64)m[1][2] * rhs.m[2][2]) / FP_MULTIPLIER;
	tr.m[1][3] = ((i64)m[1][0] * rhs.m[0][3] + (i64)m[1][1] * rhs.m[1][3] + (i64)m[1][2] * rhs.m[2][3]) / FP_MULTIPLIER  + m[1][3];

	tr.m[2][0] = ((i64)m[2][0] * rhs.m[0][0] + (i64)m[2][1] * rhs.m[1][0] + (i64)m[2][2] * rhs.m[2][0]) / FP_MULTIPLIER;
	tr.m[2][1] = ((i64)m[2][0] * rhs.m[0][1] + (i64)m[2][1] * rhs.m[1][1] + (i64)m[2][2] * rhs.m[2][1]) / FP_MULTIPLIER;
	tr.m[2][2] = ((i64)m[2][0] * rhs.m[0][2] + (i64)m[2][1] * rhs.m[1][2] + (i64)m[2][2] * rhs.m[2][2]) / FP_MULTIPLIER;
	tr.m[2][3] = ((i64)m[2][0] * rhs.m[0][3] + (i64)m[2][1] * rhs.m[1][3] + (i64)m[2][2] * rhs.m[2][3]) / FP_MULTIPLIER  + m[2][3];
#undef i64
	return tr;
}

Vector3i Affine3F::InvRot(const Vector3i v) const
{
	Vector3i s;
	s.x = ((int64_t)v.x * m[0][0] + (int64_t)v.y * m[1][0] + (int64_t)v.z * m[2][0]) / FP_MULTIPLIER;
	s.y = ((int64_t)v.x * m[0][1] + (int64_t)v.y * m[1][1] + (int64_t)v.z * m[2][1]) / FP_MULTIPLIER;
	s.z = ((int64_t)v.x * m[0][2] + (int64_t)v.y * m[1][2] + (int64_t)v.z * m[2][2]) / FP_MULTIPLIER;
	return s;
}
