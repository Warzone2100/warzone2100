/*
	Copyright (C) 2008 by Warzone Resurrection Team

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU Lessser General Public
	License along with this program. If not, see
	<http://www.gnu.org/licenses/>.
*/

#include "wzmglwidget.h"

#define gl_errors() really_report_gl_errors(__FILE__, __LINE__)
static void really_report_gl_errors (const char *file, int line)
{
	GLenum error = glGetError();

	if (error != GL_NO_ERROR)
	{
		qFatal("Oops, GL error caught: %s %s:%i", gluErrorString(error), file, line);
	}
}

WZMOpenGLWidget::WZMOpenGLWidget(QWidget *parent)
		: QGLWidget(parent)
{
	animation = false;
	dimension = 0.0;
	psModel = NULL;
	teamIndex = 0;
	if (!QGLFormat::hasOpenGL())
	{
		qWarning("This system has no OpenGL support!");
		exit(EXIT_FAILURE);
	}
	timer.start();
}

WZMOpenGLWidget::~WZMOpenGLWidget()
{
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	freeModel(psModel);
}

void WZMOpenGLWidget::initializeGL()
{
	qWarning("OpenGL version: %s", glGetString(GL_VERSION));
	qWarning("OpenGL renderer: %s", glGetString(GL_RENDERER));
	qWarning("OpenGL vendor: %s", glGetString(GL_VENDOR));

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_FOG);
	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glClearDepth(1.0f);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	gl_errors();
}

void WZMOpenGLWidget::resizeGL(int w, int h)
{
	if ( h == 0 ) h = 1;
	const float aspect = (float)w / (float)h;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, w, h);

	gluPerspective(45.0f, aspect, 0.1f, 500.0f);
	gl_errors();
}

void WZMOpenGLWidget::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, -40.0f, -50.0f + -(dimension * 2.0f));;
	gl_errors();
	if (psModel)
	{
		int now = timer.elapsed();

		// Animation support
		for (int i = 0; i < psModel->meshes && animation; i++)
		{
			MESH *psMesh = &psModel->mesh[i];
			FRAME *psFrame;

			if (!psMesh->frameArray)
			{
				continue;
			}
			psFrame = &psMesh->frameArray[psMesh->currentFrame];

			assert(psMesh->currentFrame < psMesh->frames && psMesh->currentFrame >= 0);
			if (psFrame->timeSlice != 0 && psFrame->timeSlice * 1000.0 + psMesh->lastChange < now)
			{
				psMesh->lastChange = now;
				psMesh->currentFrame++;
				if (psMesh->currentFrame >= psMesh->frames)
				{
					psMesh->currentFrame = 0;	// loop
				}
			}
		}

		drawModel(psModel, animation ? now : 0);
	}
	gl_errors();
}

void WZMOpenGLWidget::setTeam(int index)
{
	if (!psModel)
	{
		return;
	}
	if (index > 7 || index < 0)
	{
		qWarning("setTeam: Bad index %d", index);
		return;
	}
	for (int i = 0; i < psModel->meshes; i++)
	{
		MESH *psMesh = &psModel->mesh[i];

		if (!psMesh->teamColours)
		{
			continue;
		}
		psMesh->currentTextureArray = index;
	}
	teamIndex = index;
}

void WZMOpenGLWidget::setAnimation(bool value)
{
	animation = value;

	if (!animation && psModel)
	{
		for (int i = 0; i < psModel->meshes; i++)
		{
			MESH *psMesh = &psModel->mesh[i];

			psMesh->currentFrame = 0;
			psMesh->lastChange = 0;
		}
	}
}

void WZMOpenGLWidget::setModel(MODEL *model)
{
	if (!model)
	{
		return;
	}
	prepareModel(model);
	gl_errors();
	psModel = model;

	// Calculate best z offset
	for (int i = 0; i < psModel->meshes; i++)
	{
		MESH *psMesh = &psModel->mesh[i];

		for (int j = 0; j < psMesh->vertices * 3; j++)
		{
			dimension = MAX(fabs(psMesh->vertexArray[j]), dimension);
		}
	}
	timer.start();
}
