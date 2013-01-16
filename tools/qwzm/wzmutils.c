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

static inline void drawMesh(MODEL *psModel, int now, int mesh)
{
	MESH *psMesh = &psModel->mesh[mesh];

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
void drawModel(MODEL *psModel, int now, int selectedMesh)
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
	if (selectedMesh >= 0 && selectedMesh < psModel->meshes)
	{
		drawMesh(psModel, now, selectedMesh);
	}
	else
	{
		for (i = 0; i < psModel->meshes; i++)
		{
			drawMesh(psModel, now, i);
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
		psMesh->connectorArray = NULL;
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

int saveModel(const char *filename, MODEL *psModel)
{
	FILE *fp = fopen(filename, "w");
	int mesh;

	if (!fp)
	{
		fprintf(stderr, "Cannot open \"%s\" for writing: %s", filename, strerror(errno));
		return -1;
	}
	fprintf(fp, "WZM %d\n", 1);
	fprintf(fp, "TEXTURE %s\n", psModel->texPath);
	fprintf(fp, "MESHES %d", psModel->meshes);
	for (mesh = 0; mesh < psModel->meshes; mesh++)
	{
		MESH *psMesh = &psModel->mesh[mesh];
		int j;

		fprintf(fp, "\nMESH %d\n", mesh);
		fprintf(fp, "TEAMCOLOURS %d\n", psMesh->teamColours);
		fprintf(fp, "VERTICES %d\n", psMesh->vertices);
		fprintf(fp, "FACES %d\n", psMesh->faces);
		fprintf(fp, "VERTEXARRAY\n");
		for (j = 0; j < psMesh->vertices; j++)
		{
			GLfloat *v = &psMesh->vertexArray[j * 3];

			fprintf(fp, "\t%g %g %g\n", v[0], v[1], v[2]);
		}

		fprintf(fp, "TEXTUREARRAYS %d", psMesh->textureArrays);
		for (j = 0; j < psMesh->textureArrays; j++)
		{
			int k;

			fprintf(fp, "\nTEXTUREARRAY %d", j);
			for (k = 0; k < psMesh->vertices; k++)
			{
				GLfloat *v = &psMesh->textureArray[j][k * 2];

				fprintf(fp, "\n\t%g %g", v[0], v[1]);
			}
		}

		fprintf(fp, "\nINDEXARRAY");
		for (j = 0; j < psMesh->faces; j++)
		{
			GLuint *v = &psMesh->indexArray[j * 3];

			fprintf(fp, "\n\t%u %u %u", v[0], v[1], v[2]);
		}

		fprintf(fp, "\nFRAMES %d", psMesh->frames);
		// Read animation frames
		for (j = 0; j < psMesh->frames; j++)
		{
			FRAME *psFrame = &psMesh->frameArray[j];

			fprintf(fp, "\n\t%g %d %g %g %g %g %g %g", psFrame->timeSlice, psFrame->textureArray,
					psFrame->translation.x, psFrame->translation.y, psFrame->translation.z,
					psFrame->rotation.x, psFrame->rotation.y, psFrame->rotation.z);
		}

		fprintf(fp, "\nCONNECTORS %d", psMesh->connectors);
		for (j = 0; j < psMesh->connectors; j++)
		{
			CONNECTOR *conn = &psMesh->connectorArray[j];

			fprintf(fp, "\n\t%g %g %g 0", conn->pos.x, conn->pos.y, conn->pos.z);
		}
	}
	fclose(fp);
	return 0;
}

MODEL *readModel(const char *filename, int now)
{
	FILE *fp = fopen(filename, "r");
	int num, x, meshes, mesh, version;
	char s[200];
	MODEL *psModel;

	if (!fp)
	{
		fprintf(stderr, "Cannot open \"%s\" for reading: %s", filename, strerror(errno));
		return NULL;
	}

	num = fscanf(fp, "WZM %d\n", &version);
	if (num != 1)
	{
		fprintf(stderr, "Bad WZM file or wrong version: %s\n", filename);
		fclose(fp);
		return NULL;
	}
	if (version != 1 && version != 2)
	{
		fprintf(stderr, "Bad WZM version %d in %s\n", version, filename);
		fclose(fp);
		return NULL;
	}

	num = fscanf(fp, "TEXTURE %s\n", s);
	if (num != 1)
	{
		fprintf(stderr, "Bad TEXTURE directive in %s\n", filename);
		fclose(fp);
		return NULL;
	}

	num = fscanf(fp, "MESHES %d", &meshes);
	if (num != 1)
	{
		fprintf(stderr, "Bad MESHES directive in %s\n", filename);
		fclose(fp);
		return NULL;
	}
	psModel = createModel(meshes, now);
	strcpy(psModel->texPath, s);

	for (mesh = 0; mesh < meshes; mesh++)
	{
		MESH *psMesh = &psModel->mesh[mesh];
		int j;

		num = fscanf(fp, "\nMESH %s\n", s);
		if (num != 1)
		{
			fprintf(stderr, "Bad MESH directive in %s, was \"%s\".\n", filename, s);
			fclose(fp);
			freeModel(psModel);
			return NULL;
		}

		num = fscanf(fp, "TEAMCOLOURS %d\n", &x);
		if (num != 1 || x > 1 || x < 0)
		{
			fprintf(stderr, "Bad TEAMCOLOURS directive in %s, mesh %d.\n", filename, mesh);
			fclose(fp);
			freeModel(psModel);
			return NULL;
		}
		psMesh->teamColours = x;

		num = fscanf(fp, "VERTICES %d\n", &x);
		if (num != 1 || x < 0)
		{
			fprintf(stderr, "Bad VERTICES directive in %s, mesh %d.\n", filename, mesh);
			fclose(fp);
			freeModel(psModel);
			return NULL;
		}
		psMesh->vertices = x;
		psMesh->vertexArray = malloc(sizeof(GLfloat) * x * 3);

		num = fscanf(fp, "FACES %d\n", &x);
		if (num != 1)
		{
			fprintf(stderr, "Bad VERTICES directive in %s, mesh %d.\n", filename, mesh);
			fclose(fp);
			freeModel(psModel);
			return NULL;
		}
		psMesh->faces = x;
		psMesh->indexArray = malloc(sizeof(GLuint) * x * 3);

		num = fscanf(fp, "VERTEXARRAY");
		if (num == EOF)
		{
			fprintf(stderr, "No VERTEXARRAY directive in %s, mesh %d.\n", filename, mesh);
			fclose(fp);
			freeModel(psModel);
			return NULL;
		}

		for (j = 0; j < psMesh->vertices; j++)
		{
			GLfloat *v = &psMesh->vertexArray[j * 3];

			num = fscanf(fp, "%f %f %f\n", &v[0], &v[1], &v[2]);
			if (num != 3)
			{
				fprintf(stderr, "Bad VERTEXARRAY entry mesh %d, number %d\n", mesh, j);
				fclose(fp);
				freeModel(psModel);
				return NULL;
			}
		}

		num = fscanf(fp, "TEXTUREARRAYS %d", &x);
		if (num != 1 || x < 0)
		{
			fprintf(stderr, "Bad TEXTUREARRAYS directive in %s, mesh %d.\n", filename, mesh);
			fclose(fp);
			freeModel(psModel);
			return NULL;
		}
		psMesh->textureArrays = x;
		for (j = 0; j < psMesh->textureArrays; j++)
		{
			int k;

			num = fscanf(fp, "\nTEXTUREARRAY %d", &x);
			if (num != 1 || x < 0 || x != j)
			{
				fprintf(stderr, "Bad TEXTUREARRAY directive in %s, mesh %d, array %d.\n", filename, mesh, j);
				fclose(fp);
				freeModel(psModel);
				return NULL;
			}
			psMesh->textureArray[j] = malloc(sizeof(GLfloat) * psMesh->vertices * 2);
			for (k = 0; k < psMesh->vertices; k++)
			{
				GLfloat *v = &psMesh->textureArray[j][k * 2];

				num = fscanf(fp, "\n%f %f", &v[0], &v[1]);
				if (num != 2)
				{
					fprintf(stderr, "Bad TEXTUREARRAY entry mesh %d, array %d, number %d\n", mesh, j, k);
					fclose(fp);
					freeModel(psModel);
					return NULL;
				}
			}
		}

		num = fscanf(fp, "\nINDEXARRAY");
		if (num == EOF)
		{
			fprintf(stderr, "No INDEXARRAY directive in %s, mesh %d.\n", filename, mesh);
			fclose(fp);
			freeModel(psModel);
			return NULL;
		}

		for (j = 0; j < psMesh->faces; j++)
		{
			GLuint *v = &psMesh->indexArray[j * 3];

			num = fscanf(fp, "\n%u %u %u", &v[0], &v[1], &v[2]);
			if (num != 3)
			{
				fprintf(stderr, "Bad INDEXARRAY entry in mesh %d, number %d\n", mesh, j);
				fclose(fp);
				freeModel(psModel);
				return NULL;
			}
		}

		// Read animation frames
		num = fscanf(fp, "\nFRAMES %d", &psMesh->frames);
		if (num != 1 || psMesh->frames < 0)
		{
			fprintf(stderr, "Bad FRAMES directive in mesh %d\n", mesh);
			fclose(fp);
			freeModel(psModel);
			return NULL;
		}
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
				fprintf(stderr, "Bad FRAMES entry in mesh %d, number %d\n", mesh, j);
				fclose(fp);
				freeModel(psModel);
				return NULL;
			}
		}

		// Read connectors
		num = fscanf(fp, "\nCONNECTORS %d", &psMesh->connectors);
		if (num != 1 || psMesh->connectors < 0)
		{
			fprintf(stderr, "Bad CONNECTORS directive in mesh %d\n", mesh);
			fclose(fp);
			freeModel(psModel);
			return NULL;
		}
		if (psMesh->connectors)
		{
			psMesh->connectorArray = malloc(sizeof(CONNECTOR) * psMesh->connectors);
		}
		for (j = 0; j < psMesh->connectors; j++)
		{
			CONNECTOR *conn = &psMesh->connectorArray[j];
			int angle, angler1, angler2;

			if (version == 1)
			{
				num = fscanf(fp, "\n%f %f %f %d", &conn->pos.x, &conn->pos.y, &conn->pos.z, &conn->type);
			}
			else if (version == 2)
			{
				num = fscanf(fp, "\n%s %f %f %f %d %d %d", s, &conn->pos.x, &conn->pos.y, &conn->pos.z, &angle, &angler1, &angler2);
				conn->type = 0;	// TODO
			}
			if (num != 4)
			{
				fprintf(stderr, "Bad CONNECTORS entry in mesh %d, number %d\n", mesh, j);
				fclose(fp);
				freeModel(psModel);
				return NULL;
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
