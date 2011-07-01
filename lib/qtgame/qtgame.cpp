#include "qtgame.h"

#include "lib/framework/wzglobal.h"

#ifdef WZ_WS_X11
#include <X11/extensions/Xrandr.h>
#include <QX11Info>
#elif WZ_WS_WIN32
#define _WIN32_WINNT 0x0502
#include <windows.h>
#endif

/* Overloaded QWidget functions to handle resolution changing */

void QtGameWidget::trapMouse()
{
#ifdef WZ_WS_X11
	int result, count = 0;
	do
	{
		result = XGrabPointer(QX11Info::display(), winId(), False, 
		                      (uint)(ButtonPressMask | ButtonReleaseMask | PointerMotionMask | EnterWindowMask | LeaveWindowMask),
		                      GrabModeAsync, GrabModeAsync, winId(), None, CurrentTime);
		usleep(150);
		count++;
	} while (result != GrabSuccess && count < 15);
#elif WZ_WS_WIN32
	RECT lpRect;
	lpRect.top = y();
	lpRect.left = x();
	lpRect.bottom = y() + size().height();
	lpRect.rigth = x() + size().width();
	ClipCursor(&lpRect);
#endif
	mCursorTrapped = true;
}

void QtGameWidget::freeMouse()
{
#ifdef WZ_WS_X11
	XUngrabPointer(QX11Info::display(), CurrentTime);
#elif WZ_WS_WIN32
	ClipCursor(NULL);
#endif
	mCursorTrapped = false;
}

void QtGameWidget::resize(QSize res)
{
	if (!mResolutions.contains(res))
	{
		qWarning("(%d, %d) is not a supported resolution!", res.width(), res.height());
		return;
	}
	if (QGLWidget::windowState() & Qt::WindowFullScreen)
	{
		setResolution(res, -1, -1);
		move(0, 0);
		mResolutionChanged = true;
		if (pos() != QPoint(0, 0))
		{
			qWarning("Not at (0, 0) -- is at (%d, %d)!", pos().x(), pos().y()); // TODO - handle somehow?
		}
	}
	else
	{
		QGLWidget::setFixedSize(res);
	}
	mWantedSize = res;
}

void QtGameWidget::show()
{
	QGLWidget::setFixedSize(mWantedSize);
	QGLWidget::show();
}

void QtGameWidget::setMinimumResolution(QSize res)
{
	mMinimumSize = res;
	updateResolutionList();
}

void QtGameWidget::setWindowState(Qt::WindowStates windowState)
{
	Qt::WindowStates current = QGLWidget::windowState();

	// Only interested in capturing leaving/entering fullscreen
	if ((current & Qt::WindowFullScreen) && !(windowState & Qt::WindowFullScreen) && mResolutionChanged)
	{
		restoreResolution();
	}
	else if (!(current & Qt::WindowFullScreen) && (windowState & Qt::WindowFullScreen) && size() != mCurrentResolution)
	{
		setResolution(size(), -1, -1);
		mResolutionChanged = true;
	}
	QGLWidget::setWindowState(windowState);
}

void QtGameWidget::showFullScreen()
{
	if (size() != mCurrentResolution)
	{
		setResolution(size(), -1, -1);
		mResolutionChanged = true;
	}
	QGLWidget::setFixedSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
	QGLWidget::showFullScreen();
}

void QtGameWidget::showMaximized()
{
	if (QGLWidget::windowState() & Qt::WindowFullScreen && mResolutionChanged)
	{
		restoreResolution();
	}
	QGLWidget::showMaximized();
}

void QtGameWidget::showMinimized()
{
	if (QGLWidget::windowState() & Qt::WindowFullScreen && mResolutionChanged)
	{
		restoreResolution();
	}
	QGLWidget::showMinimized();
}

void QtGameWidget::showNormal()
{
	if (QGLWidget::windowState() & Qt::WindowFullScreen && mResolutionChanged)
	{
		restoreResolution();
	}
	QGLWidget::showNormal();
}

void QtGameWidget::updateResolutionList()
{
	int minWidth = mMinimumSize.width();
	int minHeight = mMinimumSize.height();
	mResolutions.clear();
#ifdef WZ_WS_X11
	XRRScreenConfiguration *config = XRRGetScreenInfo(QX11Info::display(), RootWindow(QX11Info::display(), x11Info().screen()));
	int sizeCount = 0;
	XRRScreenSize *sizes = XRRSizes(QX11Info::display(), 0, &sizeCount);
	for (int i = 0; i < sizeCount; i++)
	{
		QSize res(sizes[i].width, sizes[i].height);
		if (!mResolutions.contains(res) && sizes[i].width >= minWidth && sizes[i].height >= minHeight)
		{
			mResolutions += res;
		}
	}
	Rotation rotation;
	if (mOriginalResolution == QSize(0, 0))
	{
		SizeID sizeId = XRRConfigCurrentConfiguration(config, &rotation);
		mOriginalResolution = mCurrentResolution = mResolutions.at(sizeId);
		mOriginalRefreshRate = mCurrentRefreshRate = XRRConfigCurrentRate(config);
		mOriginalDepth = mCurrentDepth = -1;
	}
	XRRFreeScreenConfigInfo(config);
#elif WZ_WS_WIN32
	DEVMODE lpDevMode;
	memset(&lpDevMode, 0, sizeof(lpDevMode));
	lpDevMode.dmSize = sizeof(lpDevMode);
	lpDevMode.dmDriverExtra = 0;	// increase to receive private driver data
	if (mOriginalResolution == QSize(0, 0))
	{
		if (!EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &lpDevMode))
		{
			qWarning("Failed to enumerate display settings!");
			return;
		}
		mOriginalResolution = mCurrentResolution = QSize(lpDevMode.dmPelsWidth, lpDevMode.dmPelsHeight);
		mOriginalRefreshRate = mCurrentRefreshRate = lpDevMode.dmDisplayFrequency;
		mOriginalDepth = mCurrentDepth = lpDevMode.dmBitsPerPel;
	}
	for (int i = 0; EnumDisplaySettings(NULL, i, &lpDevMode); i++)
	{
		QSize res(lpDevMode.dmPelsWidth, lpDevMode.dmPelsHeight);
		// not changing depth or refresh rate
		if (!mResolutions.contains(res) && res.width() >= minWidth && res.height() >= minHeight
		    && lpDevMode.dmDisplayFrequency == mCurrentRefreshRate && lpDevMode.dmBitsPerPel == mCurrentDepth)
		{
			mResolutions += res;
		}
	}
#elif WZ_WS_MAC
	qWarning("Resolution query support for Mac not written yet");
#endif
}

QtGameWidget::QtGameWidget(QSize curResolution, const QGLFormat &format, QWidget *parent, Qt::WindowFlags f, const QGLWidget *shareWidget)
	: QGLWidget(format, parent, shareWidget, f), mOriginalResolution(0, 0), mMinimumSize(0, 0)
{
	mWantedSize = curResolution;
	mResolutionChanged = false;
	updateResolutionList();
}

bool QtGameWidget::setResolution(const QSize res, int rate, int depth)
{
#ifdef WZ_WS_X11
	Q_UNUSED(depth);
	Window root = RootWindow(QX11Info::display(), x11Info().screen());
	XRRScreenConfiguration *config = XRRGetScreenInfo(QX11Info::display(), root);
	int sizeCount, i;
	XRRScreenSize *sizes = XRRSizes(QX11Info::display(), 0, &sizeCount);
	for (i = 0; i < sizeCount && !(sizes[i].width == res.width() && sizes[i].height == res.height()); i++) ;
	if (i == sizeCount)
	{
		qWarning("Resolution (%d, %d) no longer found in list!", res.width(), res.height());
		return false;
	}
	Status error;
	if (rate == -1)
	{
		error = XRRSetScreenConfig(QX11Info::display(), config, root, i, RR_Rotate_0, CurrentTime);
	}
	else
	{
		error = XRRSetScreenConfigAndRate(QX11Info::display(), config, root, i, RR_Rotate_0, rate, CurrentTime);
	}
	XRRFreeScreenConfigInfo(config);
	if (error)
	{
		// XRandR does not work with NVidia binary blob driver and TwinView setting. Use XF86VidMode for this case instead?
		qWarning("Unable to change screen resolution using XRandR");
		return false;
	}
#elif WZ_WS_WIN32
	DEVMODE settings;

	memset(&settings, 0, sizeof(DEVMODE));
	settings.dmSize = sizeof(DEVMODE);
	settings.dmBitsPerPel = mCurrentDepth;
	settings.dmPelsWidth = res.width();
	settings.dmPelsHeight = res.height();
	settings.dmDisplayFrequency = mOriginalRefreshRate;
	settings.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY;
	switch (ChangeDisplaySettings(&settings, CDS_FULLSCREEN))
	{
	case DISP_CHANGE_SUCCESSFUL:
		break;
	case DISP_CHANGE_BADDUALVIEW:
		qWarning("Bad resolution change: The settings change was unsuccessful because the system is DualView capable.");
		return false;
	case DISP_CHANGE_BADFLAGS:
		qWarning("Bad resolution change: An invalid set of flags was passed in.");
		return false;
	case DISP_CHANGE_RESTART:
		qWarning("Bad resolution change: Restart required.");
		return false;
	default:
		qWarning("Bad resolution change: Unknown cause");
		return false;
	}
#elif WZ_WS_MAC
	QWarning("Resolution change support for Mac not written yet");
	return false;
#endif
	mCurrentResolution = res;
	QGLWidget::setFixedSize(res);
	return true;
}

void QtGameWidget::restoreResolution()
{
	setResolution(mOriginalResolution, mOriginalRefreshRate, mOriginalDepth);
	QGLWidget::setFixedSize(mWantedSize);
	mResolutionChanged = false;
}
