/*
	Copyright (C) Warzone 2100 Project

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this program. If not, see
	<http://www.gnu.org/licenses/>.
*/
#ifndef __3DS2PIE_H
#define __3DS2PIE_H

#define POLYGON_SIZE_3DS 3 // Should be triangles

struct WZ_FACE {
	unsigned int index[POLYGON_SIZE_3DS];
	float texCoord[POLYGON_SIZE_3DS][2];
};

struct WZ_POSITION {
	float x, y, z;
};

struct WZ_PIE_LEVEL {
	unsigned int numFaces;
	WZ_FACE * faceL;
	unsigned int numVertexes;
	WZ_POSITION *posL;
	char *texName;
};

//TODO: There should be a WZ_PIE structure with a number of levels and a level list

struct PIE_OPTIONS {
	char *page;
	bool swapYZ;
	bool invertUV;
	bool reverseWinding;
	bool twoSidedPolys;
	float scaleFactor;
	bool exportPIE3;
	bool useTCMask;
};

#include <lib3ds/file.h>

void dump_3ds_to_pie(Lib3dsFile *f, WZ_PIE_LEVEL *pie, PIE_OPTIONS options);
void dump_pie_file(WZ_PIE_LEVEL *pie, FILE *o, PIE_OPTIONS options);

#endif
