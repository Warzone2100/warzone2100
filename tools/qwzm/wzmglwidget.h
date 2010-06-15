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

	You should have received a copy of the GNU Lesser General Public
	License along with this program. If not, see
	<http://www.gnu.org/licenses/>.
*/
#ifndef QWZMGL_H
#define QWZMGL_H

#include <qglviewer.h>
extern "C"
{
#include "wzmutils.h"
}

class WZMOpenGLWidget : public QGLViewer
{
	Q_OBJECT

public:
	WZMOpenGLWidget(QWidget *parent);
	~WZMOpenGLWidget();
	void setModel(MODEL *model);
	void setTeam(int index);
	void setAnimation(bool value);
	void draw();
	void selectMesh(int meshSelect);

protected:
	void init();
//	void resizeGL(int w, int h);
	QString helpString() const;

private:
	MODEL *psModel;
	int teamIndex;
	int selectedMesh;
	bool animation;
	QTime timer;
	int now;
};

#endif
