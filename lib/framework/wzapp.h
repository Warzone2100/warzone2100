#ifndef WZAPP_H
#define WZAPP_H

#include <QtGui/QApplication>
#include <QtCore/QTimer>
#include <QtOpenGL/QGLWidget>
#include <QtCore/QBuffer>
#include <QtCore/QTime>
#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QSemaphore>

// Get platform defines before checking for them.
// Qt headers MUST come before platform specific stuff!
#include "lib/framework/frame.h"
#include "lib/framework/cursors.h"
#include "lib/ivis_common/textdraw.h"
#include "input.h"

class WzMainWindow : public QGLWidget
{
	Q_OBJECT

private:
	void loadCursor(CURSOR cursor, int x, int y, QBuffer &buffer);
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
	QFont regularFont, boldFont, smallFont;
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
