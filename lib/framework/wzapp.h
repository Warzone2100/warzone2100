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
#ifndef WZAPP_H
#define WZAPP_H

#include <QtGui/QApplication>
#include <QtGui/QImageReader>
#include <QtCore/QTimer>
#include <QtOpenGL/QGLWidget>
#include <QtCore/QBuffer>
#include <QtCore/QTime>
#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QSemaphore>
#include <QtCore/QSettings>
#include <physfs.h>

// Get platform defines before checking for them.
// Qt headers MUST come before platform specific stuff!
#include "lib/framework/frame.h"
#include "lib/framework/cursors.h"
#include "lib/ivis_opengl/textdraw.h"
#include "input.h"

class WzConfig : public QSettings
{
public:
	WzConfig(const QString &name, QObject *parent = 0) : QSettings(QString("wz::") + name, QSettings::IniFormat, parent) {}
	Vector3f vector3f(const QString &name);
	void setVector3f(const QString &name, const Vector3f &v);
	Vector3i vector3i(const QString &name);
	void setVector3i(const QString &name, const Vector3i &v);
	Vector2i vector2i(const QString &name);
	void setVector2i(const QString &name, const Vector2i &v);
};

class WzMainWindow : public QGLWidget
{
	Q_OBJECT

private:
	void loadCursor(CURSOR cursor, int x, int y, QImageReader &buffer);
	void mouseMoveEvent(QMouseEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void wheelEvent(QWheelEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);
	void inputMethodEvent(QInputMethodEvent *event);
	void realHandleKeyEvent(QKeyEvent *event, bool pressed);
	void focusOutEvent(QFocusEvent *event);
	MOUSE_KEY_CODE buttonToIdx(Qt::MouseButton button);

	QCursor *cursors[CURSOR_MAX];
	QTimer *timer;
	QTime tickCount;
	QFont regularFont, boldFont, smallFont, scaledFont;
	bool notReadyToPaint;  ///< HACK Don't draw during initial show(), since some global variables apparently aren't set up.
	static WzMainWindow *myself;

public:
	WzMainWindow(const QGLFormat &format, QWidget *parent = 0);
	~WzMainWindow();
	void initializeGL();
	void resizeGL(int w, int h);
	void paintGL();
	static WzMainWindow *instance();
	void setCursor(CURSOR index);
	void setCursor(QCursor cursor);
	void setFontType(enum iV_fonts FontID);
	void setFontSize(float size);
	int ticks() { return tickCount.elapsed(); }
	void setReadyToPaint() { notReadyToPaint = false; }
	void drawPixmap(int XPos, int YPos, QPixmap *pix);

public slots:
	void tick();
	void close();
};

struct _wzThread : public QThread
{
	_wzThread(int (*threadFunc_)(void *), void *data_) : threadFunc(threadFunc_), data(data_) {}
	void run()
	{
		ret = (*threadFunc)(data);
	}
	int (*threadFunc)(void *);
	void *data;
	int ret;
};

// This one couldn't be easier...
struct _wzMutex : public QMutex
{
};

struct _wzSemaphore : public QSemaphore
{
	_wzSemaphore(int startValue = 0) : QSemaphore(startValue) {}
};

#endif
