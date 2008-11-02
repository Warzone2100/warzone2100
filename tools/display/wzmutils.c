/*
	This file is part of Warzone 2100.
	Copyright (C) 2007-2008  Warzone Resurrection Project

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

#include "wzmutils.h"

#include <png.h>

#if (defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
#  define strcasecmp _stricmp
#  define strncasecmp _strnicmp
#  define inline __inline
#  define alloca _alloca
#  define fileno _fileno
#endif

void prepareModel(MODEL *psModel)
{
	if (!psModel || !psModel->pixmap || psModel->pixmap->w <= 0 || psModel->pixmap->h <= 0 || !psModel->pixmap->pixels)
	{
		printf("Bad model passed to prepareModel!\n");
		exit(EXIT_FAILURE);
	}
	glGenTextures(1, &psModel->texture);
	glBindTexture(GL_TEXTURE_2D, psModel->texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, psModel->pixmap->w, psModel->pixmap->h, 0, GL_RGBA,
	             GL_UNSIGNED_BYTE, psModel->pixmap->pixels);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

// Adjust the vector in vec1 with the vector to be in vec2 by fractional value in
// fraction which indicates how far we've come toward vec2. The result is put into
// the result vector.
static void interpolateVectors(Vector3f vec1, Vector3f vec2, Vector3f *result, double fraction)
{
	result->x = vec1.x * (1.0 - fraction) + vec2.x * fraction;
	result->y = vec1.y * (1.0 - fraction) + vec2.y * fraction;
	result->z = vec1.z * (1.0 - fraction) + vec2.z * fraction;
}

void drawModel(MODEL *psModel, int now)
{
	int i;

	assert(psModel && psModel->mesh);

	// Update animation frames
	for (i = 0; i < psModel->meshes; i++)
	{
		MESH *psMesh = &psModel->mesh[i];
		FRAME *psFrame;

		if (!psMesh->frameArray)
		{
			continue;
		}
		psFrame = &psMesh->frameArray[psMesh->currentFrame];

		assert(psMesh->currentFrame < psMesh->frames && psMesh->currentFrame >= 0);
		if (psFrame->timeSlice != 0 && psFrame->timeSlice * 1000 + psMesh->lastChange < now)
		{
			psMesh->lastChange = now;
			psMesh->currentFrame++;
			if (psMesh->currentFrame >= psMesh->frames)
			{
				psMesh->currentFrame = 0;	// loop
			}
		}
	}

	// Draw model
	glColor3f(1.0f, 1.0f, 1.0f);
	for (i = 0; i < psModel->meshes; i++)
	{
		MESH *psMesh = &psModel->mesh[i];

		assert(psMesh);
		if (psMesh->frameArray)
		{
			FRAME *psFrame = &psMesh->frameArray[psMesh->currentFrame];
			FRAME *nextFrame = psFrame;
			double fraction = 1.0f / (psFrame->timeSlice * 1000) * (now - psMesh->lastChange); // until next frame
			Vector3f vec;

			glPushMatrix();	// save matrix state

			assert(psMesh->currentFrame < psMesh->frames);

			if (psMesh->currentFrame == psMesh->frames - 1)
			{
				nextFrame = &psMesh->frameArray[0];	// wrap around
			}
			else
			{
				nextFrame = &psMesh->frameArray[psMesh->currentFrame + 1];
			}

			// Try to avoid crap drivers from taking down the entire system
			assert(finitef(psFrame->translation.x) && finitef(psFrame->translation.y) && finitef(psFrame->translation.z));
			assert(psFrame->rotation.x >= -360.0f && psFrame->rotation.y >= -360.0f && psFrame->rotation.z >= -360.0f);
			assert(psFrame->rotation.x <= 360.0f && psFrame->rotation.y <= 360.0f && psFrame->rotation.z <= 360.0f);

			// Translate
			interpolateVectors(psFrame->translation, nextFrame->translation, &vec, fraction);
			glTranslatef(vec.x, vec.z, vec.y);	// z and y flipped

			// Rotate
			interpolateVectors(psFrame->rotation, nextFrame->rotation, &vec, fraction);
			glRotatef(vec.x, 1, 0, 0);
			glRotatef(vec.z, 0, 1, 0);	// z and y flipped again...
			glRotatef(vec.y, 0, 0, 1);

			// Morph
			if (!psMesh->teamColours)
			{
				psMesh->currentTextureArray = psFrame->textureArray;
			}
		}

		glTexCoordPointer(2, GL_FLOAT, 0, psMesh->textureArray[psMesh->currentTextureArray]);
		glVertexPointer(3, GL_FLOAT, 0, psMesh->vertexArray);

		glDrawElements(GL_TRIANGLES, psMesh->faces * 3, GL_UNSIGNED_INT, psMesh->indexArray);
		if (psMesh->frameArray)
		{
			glPopMatrix();	// restore position for next mesh
		}
	}
}

MODEL *createModel(int meshes, int now)
{
	MODEL *psModel = malloc(sizeof(MODEL));
	int i;

	psModel->meshes = meshes;
	psModel->mesh = malloc(sizeof(MESH) * meshes);
	psModel->texPath[0] = '\0';
	psModel->pixmap = NULL;
	for (i = 0; i < meshes; i++)
	{
		MESH *psMesh = &psModel->mesh[i];
		int j;

		psMesh->faces = 0;
		psMesh->frames = 0;
		psMesh->vertices = 0;
		psMesh->textureArrays = 0;
		psMesh->connectors = 0;
		psMesh->teamColours = false;
		psMesh->vertexArray = NULL;
		psMesh->indexArray = NULL;
		for (j = 0; j < MAX_TEXARRAYS; j++)
		{
			psMesh->textureArray[j] = NULL;
		}
		psMesh->frameArray = NULL;
		psMesh->currentFrame = 0;
		psMesh->lastChange = now;
		psMesh->currentTextureArray = 0;
	}

	return psModel;
}

PIXMAP *readPixmap(const char *filename)
{
	PIXMAP *gfx;
	png_structp pngp;
	png_infop infop;
	png_uint_32 width, height;
	png_int_32 y, stride;
	int bit_depth, color_type, interlace_type;
	FILE *fp;
	png_bytep *row_pointers;
	const unsigned int sig_length = 8;
	unsigned char header[8];
	unsigned char *image_data;
	unsigned int result;

	if (PNG_LIBPNG_VER_MAJOR != 1 || PNG_LIBPNG_VER_MINOR < 2)
	{
		printf("libpng 1.2.6 or higher required!\n");
		exit(EXIT_FAILURE);
	}
	if (!(fp = fopen(filename, "rb")))
	{
		printf("%s won't open!\n", filename);
		exit(EXIT_FAILURE);
	}
	result = fread(header, 1, sig_length, fp);
	if (result != sig_length)
	{
		printf("Bad file %s\n", filename);
		exit(EXIT_FAILURE);
	}
	if (png_sig_cmp(header, 0, sig_length))
	{
		printf("%s is not a PNG file!\n", filename);
		exit(EXIT_FAILURE);
	}
	if (!(pngp = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)))
	{
		printf("Failed creating PNG struct reading %s.\n", filename);
		exit(EXIT_FAILURE);
	}
	if (!(infop = png_create_info_struct(pngp)))
	{
		printf("Failed creating PNG info struct reading %s.\n", filename);
		exit(EXIT_FAILURE);
	}
	if (setjmp(pngp->jmpbuf))
	{
		printf("Failed while reading PNG file: %s\n", filename);
		exit(EXIT_FAILURE);
	}
	png_init_io(pngp, fp);
	png_set_sig_bytes(pngp, sig_length);
	png_read_info(pngp, infop);

	/* Transformations to ensure we end up with 32bpp, 4 channel RGBA */
	png_set_strip_16(pngp);
	png_set_gray_to_rgb(pngp);
	png_set_packing(pngp);
	png_set_palette_to_rgb(pngp);
	png_set_tRNS_to_alpha(pngp);
	png_set_filler(pngp, 0xFF, PNG_FILLER_AFTER);
	png_set_gray_1_2_4_to_8(pngp);

	png_read_update_info(pngp, infop);
	png_get_IHDR(pngp, infop, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);

	stride = png_get_rowbytes(pngp, infop);
	row_pointers = malloc(sizeof(png_bytep) * height);

	image_data = malloc(height * width * 4);
	for (y = 0; y < (int)height; y++)
	{
		row_pointers[y] = image_data + (y * width * 4);
	}

	png_read_image(pngp, row_pointers);
	png_read_end(pngp, infop);
	fclose(fp);

	gfx = malloc(sizeof(*gfx));
	gfx->w = width;
	gfx->h = height;
	gfx->pixels = (char *)image_data;

	png_destroy_read_struct(&pngp, &infop, NULL);
	free(row_pointers);

	return gfx;
}

MODEL *readModel(const char *filename, int now)
{
	FILE *fp = fopen(filename, "r");
	int num, x, meshes, mesh;
	char s[200];
	MODEL *psModel;

	if (!fp)
	{
		fprintf(stderr, "Cannot open \"%s\" for reading: %s", filename, strerror(errno));
		exit(1);
	}

	num = fscanf(fp, "WZM %d\n", &x);
	if (num != 1 || x != 1)
	{
		fprintf(stderr, "Bad WZM file %s\n", filename);
		exit(1);
	}

	num = fscanf(fp, "TEXTURE %s\n", s);
	if (num != 1)
	{
		fprintf(stderr, "Bad TEXTURE directive in %s\n", filename);
		exit(1);
	}

	num = fscanf(fp, "MESHES %d", &meshes);
	if (num != 1)
	{
		fprintf(stderr, "Bad MESHES directive in %s\n", filename);
		exit(1);
	}
	psModel = createModel(meshes, now);
	strcpy(psModel->texPath, s);

	// WZM does not support multiple meshes, nor importing them from PIE
	for (mesh = 0; mesh < meshes; mesh++)
	{
		MESH *psMesh = &psModel->mesh[mesh];
		int j;

		num = fscanf(fp, "\nMESH %d\n", &x);
		if (num != 1 || mesh != x)
		{
			fprintf(stderr, "Bad MESH directive in %s, was %d should be %d.\n", filename, x, mesh);
			exit(1);
		}

		num = fscanf(fp, "TEAMCOLOURS %d\n", &x);
		if (num != 1 || x > 1 || x < 0)
		{
			fprintf(stderr, "Bad TEAMCOLOURS directive in %s, mesh %d.\n", filename, mesh);
			exit(1);
		}
		psMesh->teamColours = x;

		num = fscanf(fp, "VERTICES %d\n", &x);
		if (num != 1)
		{
			fprintf(stderr, "Bad VERTICES directive in %s, mesh %d.\n", filename, mesh);
			exit(1);
		}
		psMesh->vertices = x;
		psMesh->vertexArray = malloc(sizeof(GLfloat) * x * 3);

		num = fscanf(fp, "FACES %d\n", &x);
		if (num != 1)
		{
			fprintf(stderr, "Bad VERTICES directive in %s, mesh %d.\n", filename, mesh);
			exit(1);
		}
		psMesh->faces = x;
		psMesh->indexArray = malloc(sizeof(GLuint) * x * 3);

		num = fscanf(fp, "VERTEXARRAY");
		if (num == EOF)
		{
			fprintf(stderr, "No VERTEXARRAY directive in %s, mesh %d.\n", filename, mesh);
			exit(1);
		}

		for (j = 0; j < psMesh->vertices; j++)
		{
			GLfloat *v = &psMesh->vertexArray[j * 3];

			num = fscanf(fp, "%f %f %f\n", &v[0], &v[1], &v[2]);
			if (num != 3)
			{
				fprintf(stderr, "Bad VERTEXARRAY entry mesh %d, number %d\n", mesh, j);
				exit(1);
			}
		}

		num = fscanf(fp, "TEXTUREARRAYS %d", &x);
		if (num != 1 || x < 0)
		{
			fprintf(stderr, "Bad TEXTUREARRAYS directive in %s, mesh %d.\n", filename, mesh);
			exit(1);
		}
		psMesh->textureArrays = x;
		for (j = 0; j < psMesh->textureArrays; j++)
		{
			int k;

			num = fscanf(fp, "\nTEXTUREARRAY %d", &x);
			if (num != 1 || x < 0 || x != j)
			{
				fprintf(stderr, "Bad TEXTUREARRAY directive in %s, mesh %d, array %d.\n", filename, mesh, j);
				exit(1);
			}
			psMesh->textureArray[j] = malloc(sizeof(GLfloat) * psMesh->vertices * 2);
			for (k = 0; k < psMesh->vertices; k++)
			{
				GLfloat *v = &psMesh->textureArray[j][k * 2];

				num = fscanf(fp, "\n%f %f", &v[0], &v[1]);
				if (num != 2)
				{
					fprintf(stderr, "Bad TEXTUREARRAY entry mesh %d, array %d, number %d\n", mesh, j, k);
					exit(1);
				}
			}
		}

		num = fscanf(fp, "\nINDEXARRAY");
		if (num == EOF)
		{
			fprintf(stderr, "No INDEXARRAY directive in %s, mesh %d.\n", filename, mesh);
			exit(1);
		}

		for (j = 0; j < psMesh->faces; j++)
		{
			GLuint *v = &psMesh->indexArray[j * 3];

			num = fscanf(fp, "\n%u %u %u", &v[0], &v[1], &v[2]);
			if (num != 3)
			{
				fprintf(stderr, "Bad INDEXARRAY directive in mesh %d, number %d\n", mesh, j);
				exit(1);
			}
		}

		num = fscanf(fp, "\nFRAMES %d", &psMesh->frames);
		if (num != 1)
		{
			fprintf(stderr, "Bad FRAMES directive in mesh %d\n", mesh);
			exit(1);
		}

		// Read animation frames
		if (psMesh->frames)
		{
			psMesh->frameArray = malloc(sizeof(FRAME) * psMesh->frames);
		}
		for (j = 0; j < psMesh->frames; j++)
		{
			FRAME *psFrame = &psMesh->frameArray[j];

			num = fscanf(fp, "\n%f %d %f %f %f %f %f %f", &psFrame->timeSlice, &psFrame->textureArray, 
			             &psFrame->translation.x, &psFrame->translation.y, &psFrame->translation.z,
			             &psFrame->rotation.x, &psFrame->rotation.y, &psFrame->rotation.z);
			if (num != 8)
			{
				fprintf(stderr, "Bad FRAMES directive in mesh %d, number %d\n", mesh, j);
				exit(1);
			}
		}

		num = fscanf(fp, "\nCONNECTORS %d", &x);
		if (num != 1)
		{
			fprintf(stderr, "Bad CONNECTORS directive in mesh %d\n", mesh);
			exit(1);
		}

		// throw away for now
		for (j = 0; j < x; j++)
		{
			int pos[3];

			num = fscanf(fp, "\n%d %d %d", &pos[0], &pos[1], &pos[2]);
			if (num != 3)
			{
				fprintf(stderr, "Bad CONNECTORS directive in mesh %d, number %d\n", mesh, j);
				exit(1);
			}
		}
	}

	return psModel;
}

void freeModel(MODEL *psModel)
{
	int i;

	for (i = 0; i < psModel->meshes; i++)
	{
		MESH *psMesh = &psModel->mesh[i];
		int j;

		free(psMesh->vertexArray);
		free(psMesh->indexArray);
		for (j = 0; j < MAX_TEXARRAYS; j++)
		{
			free(psMesh->textureArray[j]);
		}
		free(psMesh->frameArray);
	}
	free(psModel->pixmap);
	free(psModel->mesh);
	free(psModel);
}
