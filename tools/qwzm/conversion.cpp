#include <lib3ds/file.h>
#include <lib3ds/mesh.h>
#include <lib3ds/vector.h>
#include <lib3ds/material.h>

#include "qwzm.h"

// Load PIE files
#define iV_IMD_TEX	0x00000200
#define iV_IMD_XNOCUL	0x00002000
#define iV_IMD_TEXANIM	0x00004000
#define MAX_POLYGON_SIZE 16

typedef struct {
	int index[MAX_POLYGON_SIZE];
	int texCoord[MAX_POLYGON_SIZE][2];
	int vertices;
	int frames, rate, width, height; // animation data
	bool cull;
} WZ_FACE;

typedef struct {
	int x, y, z;
} WZ_POSITION;

void QWzmViewer::loadPIE(QString inputFile)
{
	const bool swapYZ = false;
	const bool reverseWinding = false;
	const bool invertUV = false;
	const bool assumeAnimation = false;
	int num, x, y, z, levels, level;
	char s[200];
	const char *input = inputFile.toAscii().constData();
	FILE *fp = fopen(input, "r");

	if (!fp)
	{
		qWarning("Cannot open \"%s\" for reading: %s", input, strerror(errno));
		return;
	}

	num = fscanf(fp, "PIE %d\n", &x);
	if (num != 1)
	{
		fprintf(stderr, "Bad PIE file %s\n", input);
		exit(1);
	}

	num = fscanf(fp, "TYPE %d\n", &x); // ignore
	if (num != 1)
	{
		fprintf(stderr, "Bad TYPE directive in %s\n", input);
		exit(1);
	}

	num = fscanf(fp, "TEXTURE %d %s %d %d\n", &z, s, &x, &y);
	if (num != 4)
	{
		fprintf(stderr, "Bad TEXTURE directive in %s\n", input);
		exit(1);
	}

	num = fscanf(fp, "LEVELS %d\n", &levels);
	if (num != 1)
	{
		fprintf(stderr, "Bad LEVELS directive in %s\n", input);
		exit(1);
	}

	// Recreate model
	if (psModel)
	{
		freeModel(psModel);
		psModel = NULL;
	}
	psModel = createModel(levels, 0);
	if (!psModel)
	{
		qFatal("Out of memory");
	}
	strcpy(psModel->texPath, s);

	for (level = 0; level < levels; level++)
	{
		MESH *psMesh = &psModel->mesh[level];
		int j, points, faces, facesWZM, faceCount, pointsWZM, pointCount, textureArrays = 1;
		WZ_FACE *faceList;
		WZ_POSITION *posList;

		num = fscanf(fp, "\nLEVEL %d\n", &x);
		if (num != 1 || level + 1 != x)
		{
			fprintf(stderr, "Bad LEVEL directive in %s, was %d should be %d.\n", input, x, level + 1);
			exit(1);
		}

		num = fscanf(fp, "POINTS %d\n", &points);
		if (num != 1)
		{
			fprintf(stderr, "Bad POINTS directive in %s, level %d.\n", input, level);
			exit(1);
		}
		posList = (WZ_POSITION*)malloc(sizeof(WZ_POSITION) * points);
		for (j = 0; j < points; j++)
		{
			if (swapYZ)
			{
				num = fscanf(fp, "%d %d %d\n", &posList[j].x, &posList[j].z, &posList[j].y);
			}
			else
			{
				num = fscanf(fp, "%d %d %d\n", &posList[j].x, &posList[j].y, &posList[j].z);
			}
			if (num != 3)
			{
				fprintf(stderr, "Bad POINTS entry level %d, number %d\n", level, j);
				exit(1);
			}
		}

		num = fscanf(fp, "POLYGONS %d", &faces);
		if (num != 1)
		{
			fprintf(stderr, "Bad POLYGONS directive in %s, level %d.\n", input, level);
			exit(1);			
		}
		facesWZM = faces;	// for starters
		faceList = (WZ_FACE*)malloc(sizeof(WZ_FACE) * faces);
		pointsWZM = 0;
		for (j = 0; j < faces; ++j)
		{
			int k;
			unsigned int flags;

			num = fscanf(fp, "\n%x", &flags);
			if (num != 1)
			{
				fprintf(stderr, "Bad POLYGONS texture flag entry level %d, number %d\n", level, j);
				exit(1);
			}
			if (!(flags & iV_IMD_TEX))
			{
				fprintf(stderr, "Bad texture flag entry level %d, number %d - no texture flag!\n", level, j);
				exit(1);
			}

			if (flags & iV_IMD_XNOCUL)
			{
				faceList[j].cull = true;
				facesWZM++;	// must add additional face that is faced in the opposite direction
			}
			else
			{
				faceList[j].cull = false;
			}

			num = fscanf(fp, "%d", &faceList[j].vertices);
			if (num != 1 || faceList[j].vertices < 0)
			{
				fprintf(stderr, "Bad POLYGONS vertices entry level %d, number %d\n", level, j);
				exit(1);
			}
			assert(faceList[j].vertices <= MAX_POLYGON_SIZE); // larger polygons not supported
			assert(faceList[j].vertices >= 3); // points and lines not supported
			if (faceList[j].vertices > 3)
			{
				// since they are triangle fans already, we get to do easy tessellation
				facesWZM += (faceList[j].vertices - 3);
				if (faceList[j].cull)
				{
					facesWZM += (faceList[j].vertices - 3); // double the number of extra faces needed
				}
			}
			pointsWZM += faceList[j].vertices;

			// Read in vertex indices and texture coordinates
			for (k = 0; k < faceList[j].vertices; k++)
			{
				num = fscanf(fp, "%d", &faceList[j].index[k]);
				if (num != 1)
				{
					fprintf(stderr, "Bad vertex position entry level %d, number %d\n", level, j);
					exit(1);
				}
			}
			if (flags & iV_IMD_TEXANIM)
			{
				num = fscanf(fp, "%d %d %d %d", &faceList[j].frames, &faceList[j].rate, &faceList[j].width, &faceList[j].height);
				if (num != 4)
				{
					fprintf(stderr, "Bad texture animation entry level %d, number %d.\n", level, j);
					exit(1);
				}
				if (faceList[j].frames <= 1)
				{
					fprintf(stderr, "Level %d, polygon %d has a single animation frame. That makes no sense.\n", level, j);
				}
				if (textureArrays < faceList[j].frames)
				{
					textureArrays = faceList[j].frames;
				}
			}
			else
			{
				faceList[j].frames = 0;
				faceList[j].rate = 0;
				faceList[j].width = 1; // to avoid division by zero
				faceList[j].height = 1;
			}
			for (k = 0; k < faceList[j].vertices; k++)
			{
				num = fscanf(fp, "%d %d", &faceList[j].texCoord[k][0], &faceList[j].texCoord[k][1]);
				if (num != 2)
				{
					fprintf(stderr, "Bad texture coordinate entry level %d, number %d\n", level, j);
					exit(1);
				}
			}
		}
		if (textureArrays == 8 && !assumeAnimation)	// guesswork
		{
			psMesh->teamColours = true;
		}
		else
		{
			psMesh->teamColours = false;
		}

		// Calculate position list. Since positions hold texture coordinates in WZM, unlike in Warzone,
		// we need to use some black magic to transfer them over here.
		pointCount = 0;
		psMesh->vertices = pointsWZM;
		psMesh->faces = facesWZM;
		psMesh->vertexArray = (GLfloat*)malloc(sizeof(GLfloat) * psMesh->vertices * 3);
		psMesh->indexArray = (GLuint*)malloc(sizeof(GLuint) * psMesh->vertices * 3);
		psMesh->textureArrays = textureArrays;
		for (j = 0; j < textureArrays; j++)
		{
			psMesh->textureArray[j] = (GLfloat*)malloc(sizeof(GLfloat) * psMesh->vertices * 2);
		}

		for (z = 0, j = 0; j < faces; j++)
		{
			for (int k = 0; k < faceList[j].vertices; k++, pointCount++)
			{
				int pos = faceList[j].index[k];

				// Generate new position
				psMesh->vertexArray[z++] = posList[pos].x;
				psMesh->vertexArray[z++] = posList[pos].y;
				psMesh->vertexArray[z++] = posList[pos].z;

				// Use the new position
				faceList[j].index[k] = pointCount;
			}
		}

		// Handle texture animation or team colours. In either case, add multiple texture arrays.
		for (z = 0; z < textureArrays; z++)
		{
			for (x = 0, j = 0; j < faces; j++)
			{
				for (int k = 0; k < faceList[j].vertices; k++)
				{
					// This works because wrap around is only permitted if you start the animation at the
					// left border of the texture. What a horrible hack this was.
					int framesPerLine = 256 / faceList[j].width;
					int frameH = z % framesPerLine;
					int frameV = z / framesPerLine;
					double width = faceList[j].texCoord[k][0] + faceList[j].width * frameH;
					double height = faceList[j].texCoord[k][1] + faceList[j].height * frameV;
					double u = width / 256.0f;
					double v = height / 256.0f;

					if (invertUV)
					{
						v = 1.0f - v;
					}
					psMesh->textureArray[z][x++] = u;
					psMesh->textureArray[z][x++] = v;
				}
			}
		}

		faceCount = 0;

		for (z = 0, j = 0; j < faces; j++)
		{
			int k, key, previous;

			key = faceList[j].index[0];
			previous = faceList[j].index[2];
			faceCount++;
			if (reverseWinding || faceList[j].cull)
			{
				GLuint *v = &psMesh->indexArray[z];

				z += 3;
				v[0] = key;
				v[1] = previous;
				v[2] = faceList[j].index[1];
			}
			if (!reverseWinding || faceList[j].cull)
			{
				GLuint *v = &psMesh->indexArray[z];

				z += 3;
				v[0] = key;
				v[2] = previous;
				v[1] = faceList[j].index[1];
			}

			// Generate triangles from the Warzone triangle fans (PIEs, get it?)
			for (k = 3; k < faceList[j].vertices; k++)
			{
				if (reverseWinding || faceList[j].cull)
				{
					GLuint *v = &psMesh->indexArray[z];

					z += 3;
					v[0] = key;
					v[2] = previous;
					v[1] = faceList[j].index[k];
				}
				if (!reverseWinding || faceList[j].cull)
				{
					GLuint *v = &psMesh->indexArray[z];

					z += 3;
					v[0] = key;
					v[1] = previous;
					v[2] = faceList[j].index[k];
				}
				previous = faceList[j].index[k];
			}
		}

		// We only handle texture animation here, so leave bone heap animation out of it for now.
		if (textureArrays == 1 || (textureArrays == 8 && !assumeAnimation))
		{
			psMesh->frames = 0;
		}
		else
		{
			psMesh->frames = textureArrays;
			psMesh->frameArray = (FRAME*)malloc(sizeof(FRAME) * psMesh->frames);
			for (j = 0; j < textureArrays; j++)
			{
				FRAME *psFrame = &psMesh->frameArray[j];

				psFrame->timeSlice = (float)faceList[j].rate;
				psFrame->textureArray = j;
				psFrame->translation.x = 0;
				psFrame->translation.y = 0;
				psFrame->translation.z = 0;
				psFrame->rotation.x = 0;
				psFrame->rotation.y = 0;
				psFrame->rotation.z = 0;
			}
		}

		num = fscanf(fp, "\nCONNECTORS %d", &psMesh->connectors);
		if (num == 1 && psMesh->connectors > 0)
		{
			psMesh->connectorArray = (CONNECTOR *)malloc(sizeof(CONNECTOR) * psMesh->connectors);

			for (j = 0; j < psMesh->connectors; j++)
			{
				CONNECTOR *conn = &psMesh->connectorArray[j];
				int a, b, c;

				num = fscanf(fp, "\n%d %d %d", &a, &b, &c);
				if (num != 3)
				{
					fprintf(stderr, "Bad CONNECTORS directive in %s entry level %d, number %d\n", input, level, j);
					exit(1);
				}
				conn->pos.x = a;
				conn->pos.y = b;
				conn->pos.z = c;
				conn->type = 0;	// generic type of connector, only type supported in PIE
			}
		}

		free(faceList);
		free(posList);
	}
}

void QWzmViewer::load3DS(QString input)
{
	const bool swapYZ = true;
	const bool reverseWinding = true;
	const bool invertUV = true;
	const float scaleFactor = 1.0f;
	Lib3dsFile *f = lib3ds_file_load(input.toAscii().constData());
	Lib3dsMaterial *material = f->materials;
	int meshes = 0;
	Lib3dsMesh *m;
	int meshIdx;

	if (!f)
	{
		qWarning("Loading 3DS file %s failed", input.toAscii().constData());
		return;
	}

	for (meshes = 0, m = f->meshes; m; m = m->next, meshes++) ;	// count the meshes
	if (psModel)
	{
		freeModel(psModel);
		psModel = NULL;
	}
	psModel = createModel(meshes, 0);
	if (!psModel)
	{
		qFatal("Out of memory");
	}

	// Grab texture name
	for (int j = 0; material; material = material->next, j++)
	{
		Lib3dsTextureMap *texture = &material->texture1_map;
		QString texName(texture->name);

		if (j > 0)
		{
			qWarning("Texture %d %s: More than one texture currently not supported!", j, texture->name);
			continue;
		}
		strcpy(psModel->texPath, texName.toLower().toAscii().constData());
	}

	// Grab each mesh
	for (meshIdx = 0, m = f->meshes; m; m = m->next, meshIdx++)
	{
		MESH *psMesh = &psModel->mesh[meshIdx];

		psMesh->vertices = m->points;
		psMesh->faces = m->faces;
		psMesh->vertexArray = (GLfloat*)malloc(sizeof(GLfloat) * psMesh->vertices * 3);
		psMesh->indexArray = (GLuint*)malloc(sizeof(GLuint) * psMesh->faces * 3);
		psMesh->textureArrays = 1;	// only one supported from 3DS
		psMesh->textureArray[0] = (GLfloat*)malloc(sizeof(GLfloat) * psMesh->vertices * 2);

		for (unsigned int i = 0; i < m->points; i++)
		{
			Lib3dsVector pos;

			lib3ds_vector_copy(pos, m->pointL[i].pos);

			if (swapYZ)
			{
				psMesh->vertexArray[i * 3 + 0] = pos[0] * scaleFactor;
				psMesh->vertexArray[i * 3 + 1] = pos[2] * scaleFactor;
				psMesh->vertexArray[i * 3 + 2] = pos[1] * scaleFactor;
			}
			else
			{
				psMesh->vertexArray[i * 3 + 0] = pos[0] * scaleFactor;
				psMesh->vertexArray[i * 3 + 1] = pos[1] * scaleFactor;
				psMesh->vertexArray[i * 3 + 2] = pos[2] * scaleFactor;
			}
		}

		for (unsigned int i = 0; i < m->points; i++)
		{
			GLfloat *v = &psMesh->textureArray[0][i * 2];

			v[0] = m->texelL[i][0];
			if (invertUV)
			{
				v[1] = 1.0f - m->texelL[i][1];
			}
			else
			{
				v[1] = m->texelL[i][1];
			}
		}

		for (unsigned int i = 0; i < m->faces; ++i)
		{
			Lib3dsFace *face = &m->faceL[i];
			GLuint *v = &psMesh->indexArray[i * 3];

			if (reverseWinding)
			{
				v[0] = face->points[2];
				v[1] = face->points[1];
				v[2] = face->points[0];
			}
			else
			{
				v[0] = face->points[0];
				v[1] = face->points[1];
				v[2] = face->points[2];
			}
		}
	}

	lib3ds_file_free(f);
}
