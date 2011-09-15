#ifndef QTGAME_MACOSX_SCREEN_RESOLUTIONS_H
#define QTGAME_MACOSX_SCREEN_RESOLUTIONS_H

#include "lib/framework/wzglobal.h"

#ifdef WZ_WS_MAC
#include <QtCore/QList>
#include <QtCore/QSize>

/*!
 * Appends available screen resolutions from every connected screen to
 * the given list that are larger than the given minimum size and match
 * a screen's current bit depth and refresh rate.
 *
 * \param resolutions A list to append resolution sizes to
 * \param minWidth
 * \param minHeight
 * \return true on success, false otherwise
 */
bool macosxAppendAvailableScreenResolutions(QList<QSize> &resolutions, int minWidth, int minHeight);

#endif // WZ_WS_MAC

#endif // QTGAME_MACOSX_SCREEN_RESOLUTIONS_H
