/*
	This file is part of Warzone 2100.
	Copyright (C) 2013-2015  Warzone 2100 Project

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
/**
 * @file qtgame.h
 *
 *
 */
#ifndef QTGAME_H
#define QTGAME_H

#include <QtOpenGL/QGLWidget>

class QtGameWidget : public QGLWidget
{
	Q_OBJECT

private:
	QSize mOriginalResolution, mCurrentResolution, mWantedSize, mMinimumSize;
	int mOriginalRefreshRate, mCurrentRefreshRate;
	int mSwapInterval;
	int mOriginalDepth, mCurrentDepth;
	QList<QSize> mResolutions;
	bool mResolutionChanged;
	bool mCursorTrapped;

	void restoreResolution();
	void updateResolutionList();
	bool setResolution(const QSize res, int rate, int depth);

	QGLFormat adjustFormat(const QGLFormat &format);
protected:
	virtual void initializeGL();

public:
	QtGameWidget(QSize curResolution, const QGLFormat &format, QWidget *parent = 0, Qt::WindowFlags f = 0, const QGLWidget *shareWidget = 0);
	~QtGameWidget()
	{
		if (mResolutionChanged)
		{
			restoreResolution();
		}
	}
	QList<QSize> availableResolutions() const
	{
		return mResolutions;
	}
	bool isMouseTrapped()
	{
		return mCursorTrapped;
	}

	int swapInterval() const
	{
		return mSwapInterval;
	}
	void setSwapInterval(int interval);

public slots:
	void setMinimumResolution(QSize res);

	/* Not to be confused with grabMouse and releaseMouse... */
	void trapMouse();
	void freeMouse();

	/* Overloaded functions to protect sanity of window size */
	void show();
	void showFullScreen();
	void setWindowState(Qt::WindowStates windowState);
	void showMaximized();
	void showMinimized();
	void showNormal();
	void resize(QSize size);
	void resize(int w, int h)
	{
		resize(QSize(w, h));
	}
	void setFixedSize(QSize size)
	{
		resize(size);
	}
	void setFixedSize(int w, int h)
	{
		resize(w, h);
	}
	void setFixedHeight(int h)
	{
		resize(size().width(), h);
	}
	void setFixedWidth(int w)
	{
		resize(w, size().height());
	}
};

#endif
