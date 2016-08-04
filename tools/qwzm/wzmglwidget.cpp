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

#include <GL/glu.h>
#include "wzmglwidget.h"

#define gl_errors() really_report_gl_errors(__FILE__, __LINE__)
static void really_report_gl_errors(const char *file, int line)
{
	GLenum error = glGetError();

	if (error != GL_NO_ERROR)
	{
		qFatal("Oops, GL error caught: %s %s:%i", gluErrorString(error), file, line);
	}
}

WZMOpenGLWidget::WZMOpenGLWidget(QWidget *parent)
		: QGLViewer(parent), psModel(NULL), teamIndex(0),
		selectedMesh(-1), animation(false), now(0)
{
	if (!QGLFormat::hasOpenGL())
	{
		qWarning("This system has no OpenGL support!");
		exit(EXIT_FAILURE);
	}
	timer.start();
	setAxisIsDrawn(true);
}

WZMOpenGLWidget::~WZMOpenGLWidget()
{
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	freeModel(psModel);
}

void WZMOpenGLWidget::init()
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
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GEQUAL, 0.05f);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	gl_errors();
}

void WZMOpenGLWidget::draw()
{
	if (psModel)
	{
		if (animation)
		{
			now = timer.elapsed();
		}

		drawModel(psModel, now, selectedMesh);
	}
}

void WZMOpenGLWidget::selectMesh(int index)
{
	selectedMesh = index - 1;
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
}

void WZMOpenGLWidget::setModel(MODEL *model)
{
	double dimension;

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
	setSceneRadius(dimension);
	showEntireScene();
	timer.start();
}

QString WZMOpenGLWidget::helpString() const
{
	QString text("<h2>The Warzone Model Post-production Program</h2>");
	text += "Welcome! This help needs be a lot more helpful!";
	return text;
}
