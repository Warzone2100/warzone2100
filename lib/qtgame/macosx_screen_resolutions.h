#ifndef QTGAME_MACOSX_SCREEN_RESOLUTIONS_H
#define QTGAME_MACOSX_SCREEN_RESOLUTIONS_H

#include "lib/framework/wzglobal.h"

#ifdef WZ_WS_MAC
#include <QtCore/QList>
#include <QtCore/QSize>
#include <QtCore/QPoint>

/*!
 * Appends available screen resolutions for the main window's screen
 * to the given list. Resolutions that are smaller than the given
 * minimum size or are not available for the current bit depth and
 * refresh rate are omitted.
 *
 * \param resolutions A list to append resolution sizes to
 * \param minSize Filter out resolutions smaller than this size
 * \return true on success, false otherwise
 */
bool macosxAppendAvailableScreenResolutions(QList<QSize> &resolutions, QSize minSize, QPoint screenPoint);

/*!
 * Sets the resolution of the main window's screen.
 *
 * \param resolution The target resolution size
 * \return true on success, false otherwise
 */
bool macosxSetScreenResolution(QSize resolution, QPoint screenPoint);

#endif // WZ_WS_MAC

#endif // QTGAME_MACOSX_SCREEN_RESOLUTIONS_H
