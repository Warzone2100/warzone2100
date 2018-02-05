/*
	This file is part of Warzone 2100.
	Copyright (C) 2013-2017  Warzone 2100 Project

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
 * @file main_qt.cpp
 *
 * Qt backend
 */
#include <QtWidgets/QMessageBox>

#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/ivis_opengl/pieclip.h"
#include "lib/ivis_opengl/screen.h"
#include "wzapp_qt.h"

// used in crash reports & version info
const char *BACKEND = "Qt";

int realmain(int argc, char *argv[]);
int main(int argc, char *argv[])
{
	return realmain(argc, argv);
}

QApplication *appPtr;
WzMainWindow *mainWindowPtr;

void wzMain(int &argc, char **argv)
{
	debug(LOG_MAIN, "Qt initialization");

	appPtr = new QApplication(argc, argv);
}

bool wzMainScreenSetup(int antialiasing, bool fullscreen, bool vsync, bool highDPI)
{
	debug(LOG_MAIN, "Qt initialization");
	//QGL::setPreferredPaintEngine(QPaintEngine::OpenGL); // Workaround for incorrect text rendering on many platforms, doesn't exist in Qt5…

	// Setting up OpenGL
	QGLFormat format;
	format.setDoubleBuffer(true);
	//format.setAlpha(true);
	int w = pie_GetVideoBufferWidth();
	int h = pie_GetVideoBufferHeight();

	if (antialiasing)
	{
		format.setSampleBuffers(true);
		format.setSamples(antialiasing);
	}
	mainWindowPtr = new WzMainWindow(QSize(w, h), format);
	WzMainWindow &mainwindow = *mainWindowPtr;
	mainwindow.setMinimumResolution(QSize(800, 600));
	if (!mainwindow.context()->isValid())
	{
		QMessageBox::critical(nullptr, "Oops!", "Warzone2100 failed to create an OpenGL context. This probably means that your graphics drivers are out of date. Try updating them!");
		return false;
	}

	screenWidth = w;
	screenHeight = h;
	if (fullscreen)
	{
		mainwindow.resize(w, h);
		mainwindow.showFullScreen();
		if (w > mainwindow.width())
		{
			w = mainwindow.width();
		}
		if (h > mainwindow.height())
		{
			h = mainwindow.height();
		}
		pie_SetVideoBufferWidth(w);
		pie_SetVideoBufferHeight(h);
	}
	else
	{
		mainwindow.show();
		mainwindow.setMinimumSize(w, h);
		mainwindow.setMaximumSize(w, h);
	}

	mainwindow.setSwapInterval(vsync);
	mainwindow.setReadyToPaint();

	return true;
}

void wzMainEventLoop()
{
	QApplication &app = *appPtr;
	WzMainWindow &mainwindow = *mainWindowPtr;
	mainwindow.update(); // kick off painting, needed on macosx
	app.exec();
}

void wzShutdown()
{
	delete mainWindowPtr;
	mainWindowPtr = nullptr;
	delete appPtr;
	appPtr = nullptr;
}

bool wzIsFullscreen()
{
	return false; // for relevant intents and purposes, we are never in that kind of fullscreen
}

void wzToggleFullscreen()
{
}

std::vector<screeninfo> wzAvailableResolutions()
{
	std::vector<screeninfo> res;
	for (auto const &r : WzMainWindow::instance()->availableResolutions())
	{
		screeninfo info;
		info.width = r.width();
		info.height = r.height();
		info.refresh_rate = 0;
		info.screen = 0;
		res.push_back(info);
	}
	return res;
}

std::vector<unsigned int> wzAvailableDisplayScales()
{
	// TODO: Currently, Qt backend only supports 100% display scale.
	static const unsigned int wzDisplayScales[] = { 100 };
	return std::vector<unsigned int>(wzDisplayScales, wzDisplayScales + (sizeof(wzDisplayScales) / sizeof(wzDisplayScales[0])));
}

void wzGetGameToRendererScaleFactor(float *horizScaleFactor, float *vertScaleFactor)
{
	// TODO: Support high-DPI with Qt backend
	if (horizScaleFactor != nullptr)
	{
		*horizScaleFactor = 1.0f
	}
	if (vertScaleFactor != nullptr)
	{
		*vertScaleFactor = 1.0f
	}
}

void wzSetWindowIsResizable(bool resizable)
{
	// TODO: Implement
}

bool wzIsWindowResizable()
{
	// TODO: Implement
	return false;
}

bool wzSupportsLiveResolutionChanges()
{
	// TODO: Implement support for live resolution changes (several of the other functions with TODOs here)
	return false;
}

bool wzChangeDisplayScale(unsigned int displayScale)
{
	// TODO: Implement
	return false;
}

bool wzChangeWindowResolution(int screen, unsigned int width, unsigned int height)
{
	// TODO: Implement support for live resolution changes
	return false;
}

unsigned int wzGetMaximumDisplayScaleForWindowSize(unsigned int windowWidth, unsigned int windowHeight)
{
	// TODO: Implement
	return 100;
}

unsigned int wzGetCurrentDisplayScale()
{
	// TODO: Implement
	return 100;
}

void wzGetWindowResolution(int *screen, unsigned int *width, unsigned int *height)
{
	assert(mainWindowPtr != nullptr);
	WzMainWindow &mainwindow = *mainWindowPtr;
	if (screen != nullptr)
	{
		// FIXME: Determine which screen the window is on
		*screen = 0;
	}
	if (width != nullptr)
	{
		*width = mainwindow.width();
	}
	if (height != nullptr)
	{
		*height = mainwindow.height();
	}
}

void wzSetSwapInterval(int swap)
{
	WzMainWindow::instance()->setSwapInterval(swap);
}

int wzGetSwapInterval()
{
	return WzMainWindow::instance()->swapInterval();
}

void StartTextInput()
{
	// Something started?
}

void StopTextInput()
{
	// Whatever it was, it stopped…
}
