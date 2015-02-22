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
 * @file qtgame.cpp
 *
 * Qt resolution switching
 */
#include "qtgame.h"

#include "lib/framework/wzglobal.h"
#include "swapinterval.h"

#if defined(WZ_CC_MSVC)
#include "qtgame.h.moc"		// this is generated on the pre-build event.
#endif

#ifdef WZ_WS_X11
#include <X11/extensions/Xrandr.h>
#include <QX11Info>
#endif

#ifdef WZ_WS_MAC
#include "macosx_screen_resolutions.h"
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
#elif defined(WZ_WS_WIN32)
	RECT lpRect;
	QRect qRect = QtGameWidget::geometry();

	lpRect.top = qRect.top();
	lpRect.left = qRect.left();
	lpRect.bottom = qRect.bottom();
	lpRect.right = qRect.right();

	ClipCursor(&lpRect);
#endif
	mCursorTrapped = true;
}

void QtGameWidget::freeMouse()
{
#ifdef WZ_WS_X11
	XUngrabPointer(QX11Info::display(), CurrentTime);
#elif defined(WZ_WS_WIN32)
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
#elif defined(WZ_WS_WIN32)
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
#elif defined(WZ_WS_MAC)
	macosxAppendAvailableScreenResolutions(mResolutions, QSize(minWidth, minHeight), pos());

	// OS X will restore the resolution itself upon process termination.
	mOriginalRefreshRate = 0;
	mOriginalDepth = 0;
#endif
	// TODO: Sorting would be nice.
}

QGLFormat QtGameWidget::adjustFormat(const QGLFormat &format)
{
	QGLFormat adjusted(format);
	mSwapInterval = adjusted.swapInterval();
	adjusted.setSwapInterval(0);
	return adjusted;
}

void QtGameWidget::initializeGL()
{
	setSwapInterval(mSwapInterval);
}

QtGameWidget::QtGameWidget(QSize curResolution, const QGLFormat &format, QWidget *parent, Qt::WindowFlags f, const QGLWidget *shareWidget)
	: QGLWidget(adjustFormat(format), parent, shareWidget, f), mOriginalResolution(0, 0), mMinimumSize(0, 0)
{
	QGLWidget::setFixedSize(curResolution);  // Don't know whether this needs to be done here, but if not, the window contents are displaced 2% of the time.
	mWantedSize = curResolution;
	mResolutionChanged = false;
	updateResolutionList();
}

void QtGameWidget::setSwapInterval(int interval)
{
	mSwapInterval = interval;
	makeCurrent();
	::setSwapInterval(*this, &mSwapInterval);
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
#elif defined(WZ_WS_WIN32)
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
#elif defined(WZ_WS_MAC)
	if (res != QSize(0,0)) {
		macosxSetScreenResolution(res, pos());
	}
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
