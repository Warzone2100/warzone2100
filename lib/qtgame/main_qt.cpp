#include <QtGui/QMessageBox>

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/pieclip.h"
#include "src/warzoneconfig.h"
#include "lib/framework/frameint.h"
#include "wzapp_qt.h"


int realmain(int argc, char *argv[]);
int main(int argc, char *argv[])
{
	return realmain(argc, argv);
}

QApplication *appPtr;
WzMainWindow *mainWindowPtr;

void wzMain(int &argc, char **argv)
{
	appPtr = new QApplication(argc, argv);
}

bool wzMain2()
{
	debug(LOG_MAIN, "Qt initialization");
	QGL::setPreferredPaintEngine(QPaintEngine::OpenGL); // Workaround for incorrect text rendering on nany platforms.

	// Setting up OpenGL
	QGLFormat format;
	format.setDoubleBuffer(true);
	format.setAlpha(true);
	int w = pie_GetVideoBufferWidth();
	int h = pie_GetVideoBufferHeight();

	if (war_getFSAA())
	{
		format.setSampleBuffers(true);
		format.setSamples(war_getFSAA());
	}
	mainWindowPtr = new WzMainWindow(QSize(w, h), format);
	WzMainWindow &mainwindow = *mainWindowPtr;
	mainwindow.setMinimumResolution(QSize(800, 600));
	if (!mainwindow.context()->isValid())
	{
		QMessageBox::critical(NULL, "Oops!", "Warzone2100 failed to create an OpenGL context. This probably means that your graphics drivers are out of date. Try updating them!");
		return false;
	}

	screenWidth = w;
	screenHeight = h;
	if (war_getFullscreen())
	{
		mainwindow.resize(w,h);
		mainwindow.showFullScreen();
		if(w>mainwindow.width()) {
			w = mainwindow.width();
		}
		if(h>mainwindow.height()) {
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

	mainwindow.setSwapInterval(war_GetVsync());
	war_SetVsync(mainwindow.swapInterval() > 0);

	mainwindow.setReadyToPaint();

	return true;
}

void wzMain3()
{
	QApplication &app = *appPtr;
	WzMainWindow &mainwindow = *mainWindowPtr;
	mainwindow.update(); // kick off painting, needed on macosx
	app.exec();
}

void wzShutdown()
{
	delete mainWindowPtr;
	mainWindowPtr = NULL;
	delete appPtr;
	appPtr = NULL;
}

void wzToggleFullscreen()
{
}

QList<QSize> wzAvailableResolutions()
{
	return WzMainWindow::instance()->availableResolutions();
}

void wzSetSwapInterval(bool swap)
{
	WzMainWindow::instance()->setSwapInterval(swap);
}

bool wzGetSwapInterval()
{
	return WzMainWindow::instance()->swapInterval() > 0;
}
