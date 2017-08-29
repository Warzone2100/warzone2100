#include <QtGlobal>

#include <lib3ds/file.h>
#include <lib3ds/mesh.h>
#include <lib3ds/vector.h>
#include <lib3ds/material.h>

#include <sstream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <set>
#include <list>
#include <queue>
#include <cmath>

#include "qwzm.h"
#include "conversion.h"

MODEL *QWzmViewer::loadPIE(QString inputFile)
{
	const bool swapYZ = false;
	const bool reverseWinding = false;
	const bool invertUV = false;
	const bool assumeAnimation = false;
	int num, x, y, z, levels, level, pieVersion;
	char s[200];
	QByteArray inFile = inputFile.toAscii();
	const char *input = inFile.constData();
	MODEL *psModel = NULL;
	FILE *fp = fopen(input, "r");

	if (!fp)
	{
		qWarning("Cannot open \"%s\" for reading: %s", input, strerror(errno));
		return NULL;
	}

	num = fscanf(fp, "PIE %d\n", &pieVersion);
	if (num != 1)
	{
		qWarning("Bad PIE file %s\n", input);
		fclose(fp);
		freeModel(psModel);
		return NULL;
	}

	num = fscanf(fp, "TYPE %x\n", &x); // ignore
	if (num != 1)
	{
		qWarning("Bad TYPE directive in %s\n", input);
		fclose(fp);
		freeModel(psModel);
		return NULL;
	}

	num = fscanf(fp, "TEXTURE %d %s %d %d\n", &z, s, &x, &y);
	if (num != 4)
	{
		qWarning("Bad TEXTURE directive in %s\n", input);
		fclose(fp);
		freeModel(psModel);
		return NULL;
	}

	num = fscanf(fp, "LEVELS %d\n", &levels);
	if (num != 1)
	{
		qWarning("Bad LEVELS directive in %s\n", input);
		fclose(fp);
		freeModel(psModel);
		return NULL;
	}

	psModel = createModel(levels, 0);
	if (!psModel)
	{
		qFatal("Out of memory");
		fclose(fp);
		exit(EXIT_FAILURE);
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
			qWarning("Bad LEVEL directive in %s, was %d should be %d.\n", input, x, level + 1);
			fclose(fp);
			freeModel(psModel);
			return NULL;
		}

		num = fscanf(fp, "POINTS %d\n", &points);
		if (num != 1)
		{
			qWarning("Bad POINTS directive in %s, level %d.\n", input, level);
			fclose(fp);
			freeModel(psModel);
			return NULL;
		}
		posList = (WZ_POSITION*)malloc(sizeof(WZ_POSITION) * points);
		for (j = 0; j < points; j++)
		{
			if (swapYZ)
			{
				num = fscanf(fp, "%f %f %f\n", &posList[j].x, &posList[j].z, &posList[j].y);
			}
			else
			{
				num = fscanf(fp, "%f %f %f\n", &posList[j].x, &posList[j].y, &posList[j].z);
			}
			if (num != 3)
			{
				qWarning("Bad POINTS entry level %d, number %d\n", level, j);
				fclose(fp);
				freeModel(psModel);
				return NULL;
			}
		}

		num = fscanf(fp, "POLYGONS %d", &faces);
		if (num != 1)
		{
			qWarning("Bad POLYGONS directive in %s, level %d.\n", input, level);
			fclose(fp);
			freeModel(psModel);
			return NULL;
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
				qWarning("Bad POLYGONS texture flag entry level %d, number %d\n", level, j);
				fclose(fp);
				freeModel(psModel);
				return NULL;
			}
			if (!(flags & iV_IMD_TEX))
			{
				qWarning("Bad texture flag entry level %d, number %d - no texture flag!\n", level, j);
				fclose(fp);
				freeModel(psModel);
				return NULL;
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

			num = fscanf(fp, "%u", &faceList[j].vertices);
			if (num != 1 || faceList[j].vertices < 0)
			{
				qWarning("Bad POLYGONS vertices entry level %d, number %d\n", level, j);
				fclose(fp);
				freeModel(psModel);
				return NULL;
			}
			assert(faceList[j].vertices <= MAX_POLYGON_SIZE); // larger polygons not supported
			assert(faceList[j].vertices >= VERTICES_PER_TRIANGLE); // points and lines not supported
			if (faceList[j].vertices > VERTICES_PER_TRIANGLE)
			{
				// since they are triangle fans already, we get to do easy tessellation
				facesWZM += (faceList[j].vertices - VERTICES_PER_TRIANGLE);
				if (faceList[j].cull)
				{
					facesWZM += (faceList[j].vertices - VERTICES_PER_TRIANGLE); // double the number of extra faces needed
				}
			}
			pointsWZM += faceList[j].vertices;

			// Read in vertex indices and texture coordinates
			for (k = 0; k < faceList[j].vertices; k++)
			{
				num = fscanf(fp, "%d", &faceList[j].index[k]);
				if (num != 1)
				{
					qWarning("Bad vertex position entry level %d, number %d\n", level, j);
					fclose(fp);
					freeModel(psModel);
					return NULL;
				}
			}
			if (flags & iV_IMD_TEXANIM)
			{
				num = fscanf(fp, "%d %d %f %f", &faceList[j].frames, &faceList[j].rate, &faceList[j].width, &faceList[j].height);
				if (num != 4)
				{
					qWarning("Bad texture animation entry level %d, number %d.\n", level, j);
					fclose(fp);
					freeModel(psModel);
					return NULL;
				}
				if (faceList[j].frames <= 1)
				{
					qWarning("Level %d, polygon %d has a single animation frame. That makes no sense.\n", level, j);
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
				faceList[j].width = 0;
				faceList[j].height = 0;
			}
			for (k = 0; k < faceList[j].vertices; k++)
			{
				num = fscanf(fp, "%f %f", &faceList[j].texCoord[k][0], &faceList[j].texCoord[k][1]);
				if (num != 2)
				{
					qWarning("Bad texture coordinate entry level %d, number %d\n", level, j);
					fclose(fp);
					freeModel(psModel);
					return NULL;
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
		psMesh->vertexArray = (GLfloat*)malloc(sizeof(GLfloat) * psMesh->vertices * VERTICES_PER_TRIANGLE);
		psMesh->indexArray = (GLuint*)malloc(sizeof(GLuint) * psMesh->vertices * VERTICES_PER_TRIANGLE);
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
					double u;
					double v;
					// Fix for division by zero in pie 2
					int framesPerLine = OLD_TEXTURE_SIZE_FIX;

					if (pieVersion == 3)
					{
						if(faceList[j].width != 0)
						{
							framesPerLine = 1/ faceList[j].width;
						}
						else
						{
							// Fix for division by zero in pie 3
							framesPerLine = 1;
						}
					}
					else if (faceList[j].width != 0) // else pieVersion ==2
					{
						framesPerLine = OLD_TEXTURE_SIZE_FIX / faceList[j].width;
					}

					int frameH = z % framesPerLine;

					/*
					 * This works because wrap around is only permitted if you start the animation at the
					 * left border of the texture. What a horrible hack this was.
					 * Note: It is done the same way in the Warzone source.
					 */
					int frameV = z / framesPerLine;
					double width = faceList[j].texCoord[k][0] + faceList[j].width * frameH;
					double height = faceList[j].texCoord[k][1] + faceList[j].height * frameV;

					if (pieVersion == 2)
					{
						u = width / OLD_TEXTURE_SIZE_FIX;
						v = height / OLD_TEXTURE_SIZE_FIX;
					}
					else if (pieVersion == 3)
					{
						u = width;
						v = height;
					}

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
			for (k = VERTICES_PER_TRIANGLE; k < faceList[j].vertices; k++)
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
				GLfloat a, b, c;

				num = fscanf(fp, "\n%f %f %f", &a, &b, &c);
				if (num != 3)
				{
					qWarning("Bad CONNECTORS directive in %s entry level %d, number %d\n", input, level, j);
					fclose(fp);
					freeModel(psModel);
					return NULL;
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
	fclose(fp);
	return psModel;
}

MODEL *QWzmViewer::load3DS(QString input)
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
	MODEL *psModel;

	if (!f)
	{
		qWarning("Loading 3DS file %s failed", input.toAscii().constData());
		return NULL;
	}

	for (meshes = 0, m = f->meshes; m; m = m->next, meshes++) ;	// count the meshes

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
	return psModel;
}

int QWzmViewer::savePIE(const char *filename, const MODEL *psModel, int pieVersion, int type)
{
	std::fstream file;
	int i, j, k;

	/* variable ptOffset: Temporary storage of calculated index
	 *		for a give face OR vertex in the WZM vertex array.
	 */
	/* variable ptSetIndex: Index of a vertex in the set
	 *		of PIE points for a given PIE level.
	 */
	// variable polyFlag: Use to compose the flag for a PIE polygon
	unsigned int ptOffset, ptSetIndex, polyFlag;

	/* Temporary stringstream used to accommodate
	 * the order PIE polygon data is written
	 */
	std::stringstream animTmpSs;

	MESH *mesh;

	/* variable unitW: Width of the bounding box for
	 *		a single tex. anim. "frame" of a polygon.
	 * variable unitH: Same as unitW except represents
	 *		the height.
	 */
	/* variable t: represents the top of the bounding box
	 *		when estimating the tex. anim.'s
	 *		height.
	 * variable b: Same as t except represents the
	 *		bottom.
	 */
	double unitW, unitH, t, b;

	/* variable ptSet: for creating a unique set
	 *		of PIE points for a given PIE level.
	 */
	std::set< pie_Point> ptSet;

	/* variable textures: Temporary storage of
	 *		texture coordinates to avoid redundant
	 *		ptOffset calculations.
	 */
	std::queue< textCoords, std::list<textCoords> > textures;

	file.open(filename, std::fstream::out);

	if (!file.is_open())
	{
		qWarning("QWzmViewer::savePIE - Failed to open file.");
		return -1;
	}

	file << "PIE " << pieVersion << '\n';
	file << "TYPE " << std::hex << type << std::dec << "\n";

	// Texture directive:
	file << "TEXTURE 0 " << psModel->texPath;
	switch (pieVersion)
	{
	case 2:
		if (psModel->pixmap != NULL)
		{
			file << ' ' << psModel->pixmap->h << ' ' << psModel->pixmap->w << '\n';
		}
		else
		{
			file << " 256 256\n";
		}
		break;
	case 3:
		file << " 0 0\n";
		break;
	default:
		file.close();
		qWarning("QWzmViewer::savePIE - Unsupported pie version");
		return -1;
	}

	// LEVELS directive
	file << "LEVELS " << psModel->meshes << '\n';

	// For each WZM mesh a PIE level.
	for (i = 0; i < psModel->meshes; i++)
	{
		ptSet.clear();
		mesh = &(psModel->mesh[i]);

		// LEVEL directive
		file << "LEVEL " << i + 1 << "\n";

		// Create the set of unique points.
		for (j = 0; j < mesh->vertices; j++)
		{
			ptSet.insert(pie_Point(mesh->vertexArray[j*VERTICES_PER_TRIANGLE],
								   mesh->vertexArray[j*VERTICES_PER_TRIANGLE+1],
								   mesh->vertexArray[j*VERTICES_PER_TRIANGLE+2]));
		}

		if (ptSet.size() > pie_MAX_POLYGONS)
		{
			file.close();
			qWarning("QWzmViewer::savePIE - Model has too many vertices to save as PIE.");
			return -1;
		}

		// POINTS directive
		file << "POINTS " << ptSet.size() << '\n';

		// print out all the points in the set
		std::set< pie_Point>::iterator it;
		for (it = ptSet.begin(); it != ptSet.end(); it++)
		{
			GLfloat x, y, z;
			it->getXYZ(x, y, z);
			switch (pieVersion)
			{
			case 2:
				file << '\t' << (int)rintf(x) << ' ' << (int)rintf(y) << ' ' << (int)rintf(z) << '\n';
				break;
			case 3:
				file << '\t' << x << ' ' << y << ' ' << z << '\n';
				break;
			}
		}

		// POLYGONS directive
		file << "POLYGONS " << mesh->faces << '\n';

		for (j = 0; j < mesh->faces; j++)
		{
			polyFlag = iV_IMD_TEX; // Default flag
			animTmpSs.str(std::string());

			if (mesh->textureArrays > 1)
			{
				ptOffset = mesh->indexArray[j*VERTICES_PER_TRIANGLE];

				/*
				 * TODO:
				 * This _if_ statement isn't a robust way of checking
				 * for animations or team colours.
				 */
				if ((mesh->textureArray[0][ptOffset*2] < mesh->textureArray[1][ptOffset*2])
					|| (mesh->textureArray[0][ptOffset*2+1] < mesh->textureArray[mesh->textureArrays-1][ptOffset*2+1]))
				{
					/* Find height and width for team colours
					 * and animations.
					 * TODO: Check that the animations are legal
					 * for the PIE format.
					 */
					unitH = 0;

					// Try to find width the easy way
					unitW = mesh->textureArray[1][ptOffset*2] - mesh->textureArray[0][ptOffset*2];

					// Try to find height the easy way
					for (k = 0; k < mesh->textureArrays - 1; k++)
					{
						// Look for wrap around
						if (mesh->textureArray[k+1][ptOffset*2+1] > mesh->textureArray[k][ptOffset*2+1])
						{
							unitH = mesh->textureArray[k+1][ptOffset*2+1] - mesh->textureArray[k][ptOffset*2+1];
							break;
						}
					}

					// If the easy way failed
					if ( unitH <= 0)
					{
						// Find top and bottom, add pixels to the difference for good luck.
						b = 0; // Search for the bottom starting from the top
						t = 1; // Search for the top starting from the bottom

						// Try every V to find the min and max
						for (k = 0; k < VERTICES_PER_TRIANGLE; k++)
						{
							ptOffset = mesh->indexArray[j*VERTICES_PER_TRIANGLE+k];
							if (mesh->textureArray[0][ptOffset*2+1] < t)
							{
								t = mesh->textureArray[0][ptOffset*2+1];
							}
							if (mesh->textureArray[0][ptOffset*2+1] > b)
							{
								b = mesh->textureArray[0][ptOffset*2+1];
							}
						}

						unitH = fabs(b - t);

						if (psModel->pixmap != NULL && psModel->pixmap->h != 0)
						{
							unitH += 2 / psModel->pixmap->h;
						}
						else if (pieVersion == 2)
						{
							unitH += 2 / OLD_TEXTURE_SIZE_FIX;
						}
					}

					/* If the animations wraps around for each frame
					 * then the easy way of finding width would fail.
					 */
					if (unitW <= 0 && unitH ==0)
					{
						qWarning("QWzmViewer::savePIE - Texture animation appears to be illegal for PIE format.");
						file.close();
						return -1;
					}
					else if ( unitW <=0)
					{
						unitW = 1;
					}

					switch (pieVersion)
					{
						// TODO: Get playback rate and replace the literal " 1 " with it.
					case 2:
						animTmpSs << mesh->textureArrays << " 1 " << (int)rintf(OLD_TEXTURE_SIZE_FIX*unitW)
								<< ' ' << (int)rintf(OLD_TEXTURE_SIZE_FIX*unitH);
						break;
					case 3:
						animTmpSs << mesh->textureArrays << " 1 " << (GLfloat)unitW << ' ' << (GLfloat)unitH;
						break;
					}
					polyFlag |= iV_IMD_TEXANIM;
				}
			}

			// Write out the flag followed by the number of vertices
			file << "\t" << std::hex << polyFlag << std::dec << ' ' << VERTICES_PER_TRIANGLE; // Triangles only

			// Write the three polygon indices
			for (k = 0; k < VERTICES_PER_TRIANGLE; k++)
			{
				ptOffset = mesh->indexArray[j*VERTICES_PER_TRIANGLE+k];

				pie_Point thisPoint(mesh->vertexArray[ptOffset*VERTICES_PER_TRIANGLE],
									mesh->vertexArray[ptOffset*VERTICES_PER_TRIANGLE+1],
									mesh->vertexArray[ptOffset*VERTICES_PER_TRIANGLE+2]);

				ptSetIndex = distance(ptSet.begin(), ptSet.find(thisPoint));

				if (ptSetIndex == ptSet.size())
				{
					file.close();
					qWarning("QWzmViewer::savePIE - Internal error: Failed to find vertex in set.");
					return -1;
				}
				else
				{
					file << ' ' << ptSetIndex;

					// Store the texture coordinates for later
					textures.push(textCoords(mesh->textureArray[0][ptOffset*2],
											 mesh->textureArray[0][ptOffset*2+1]));
				}
			}

			// If the polygon has team colours or animations, write that data now
			if (polyFlag & iV_IMD_TEXANIM)
			{
				file << ' ' << animTmpSs.str();
			}

			// Write out all the texture coordinates
			while (!textures.empty())
			{
				switch (pieVersion)
				{
				case 2:
					file << ' ' << (int)rintf(OLD_TEXTURE_SIZE_FIX*textures.front().u);
					file << ' ' << (int)rintf(OLD_TEXTURE_SIZE_FIX*textures.front().v);
					break;
				case 3:
					file << ' ' << textures.front().u << ' ' << textures.front().v;
					break;
				}
				textures.pop();
			}
			file << '\n';
		}

		// In PIE the CONNECTORS directive is optional
		if (mesh->connectors > 0)
		{
			// CONNECTORS directive
			file << "CONNECTORS " << mesh->connectors << "\n";

			for (j = 0; j < mesh->connectors; j++)
			{
				switch (pieVersion)
				{
				case 2:
					file << "\t" << (int)rintf(mesh->connectorArray[j].pos.x)
							<< ' ' << (int)rintf(mesh->connectorArray[j].pos.y)
							<< ' ' << (int)rintf(mesh->connectorArray[j].pos.z) << '\n';
					break;
				case 3:
					file << "\t" << mesh->connectorArray[j].pos.x
							<< ' ' << mesh->connectorArray[j].pos.y
							<< ' ' << mesh->connectorArray[j].pos.z << '\n';
					break;
				}
			}
		}
	}

	if(file.bad())
	{
		file.close();
		return -1;
	}
	file.close();
	return 0;
}

QPieExportDialog::QPieExportDialog(QWidget *parent)
		: QDialog(parent), Ui_PieExportDialog()
{
	setupUi(this);
}

QPieExportDialog::~QPieExportDialog()
{
}

int QPieExportDialog::getPieVersion()
{
	return cbb_pieversion->currentIndex() + 2;
}

int QPieExportDialog::getFlags()
{
	int retVal = 0;
	if (cb_TCMask->checkState())
	{
		retVal |= iV_IMD_TCMASK;
	}
	if (cb_Tex->checkState())
	{
		retVal |= iV_IMD_TEX;
	}
	if (cb_Other->checkState())
	{
		retVal |= sb_OtherFlags->value();
	}
	return retVal;
}
