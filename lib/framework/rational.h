/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2011  Warzone 2100 Project

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
/*!
 * \file
 * \brief Rational number classes.
 */

#ifndef RATIONAL_H
#define RATIONAL_H

#include <algorithm>


static inline int gcd(int a, int b)
{
	while (b != 0)
	{
		a %= b;
		std::swap(a, b);
	}
	return abs(a);
}

/// Simple rational numbers, not bignum.
struct Rational
{
	Rational() {}
	Rational(int numerator, int denominator = 1)
	{
		int g = gcd(numerator, denominator);
		g *= denominator > 0? 1 : -1;
		n = numerator/g;
		d = denominator/g;
	}
	bool operator ==(Rational const &b) const { return n*b.d == b.n*d; }
	bool operator !=(Rational const &b) const { return n*b.d != b.n*d; }
	bool operator < (Rational const &b) const { return n*b.d <  b.n*d; }
	bool operator <=(Rational const &b) const { return n*b.d <= b.n*d; }
	bool operator > (Rational const &b) const { return n*b.d >  b.n*d; }
	bool operator >=(Rational const &b) const { return n*b.d >= b.n*d; }
	Rational operator +(Rational const &b) const { return Rational(n*b.d + b.n*d, d*b.d); }
	Rational operator -(Rational const &b) const { return Rational(n*b.d - b.n*d, d*b.d); }
	Rational operator *(Rational const &b) const { return Rational(n*b.n, d*b.d); }
	Rational operator /(Rational const &b) const { return Rational(n*b.d, d*b.n); }
	Rational operator -() const { return Rational(-n, d); }
	Rational &operator +=(Rational const &b) { return *this = *this + b; }
	Rational &operator -=(Rational const &b) { return *this = *this - b; }
	Rational &operator *=(Rational const &b) { return *this = *this * b; }
	Rational &operator /=(Rational const &b) { return *this = *this / b; }
	int floor() const { return n >= 0? n/d : (n - (d - 1))/d; }
	int ceil() const { return n >= 0? (n + (d - 1))/d : n/d; }

	// If int16_t isn't big enough, the comparison operations might overflow.
	int16_t n;
	int16_t d;

private:
	Rational(float);   // Disable.
	Rational(double);  // Disable.
};

#endif //RATIONAL_H
