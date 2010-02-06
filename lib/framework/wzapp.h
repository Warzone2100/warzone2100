#ifndef WZAPP_H
#define WZAPP_H

#include "lib/framework/cursors.h"
#include "lib/ivis_common/textdraw.h"
#include "input.h"

#if defined(__MACOSX__)
#include <QtGui/QApplication>
#include <QtCore/QTimer>
#include <QtOpenGL/QGLWidget>
#include <QtCore/QBuffer>
#include <QtCore/QTime>
#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QSemaphore>
#else
#include <QApplication>
#include <QTimer>
#include <QGLWidget>
#include <QBuffer>
#include <QTime>
#include <QThread>
#include <QMutex>
#include <QSemaphore>
#endif

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
	void realHandleKeyEvent(QKeyEvent *event, bool pressed);
	MOUSE_KEY_CODE buttonToIdx(Qt::MouseButton button);

	QCursor *cursors[CURSOR_MAX];
	QTimer *timer;
	QTime tickCount;
	QFont regular, bold, small;
	enum iV_fonts fontID;
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
