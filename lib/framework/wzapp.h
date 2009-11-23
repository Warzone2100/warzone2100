#ifndef WZAPP_H
#define WZAPP_H

#include <QApplication>
#include <QTimer>
#include <QGLWidget>
#include <QBuffer>
#include <QTime>

#ifdef __cplusplus
extern "C" {
#endif
#include "lib/framework/cursors.h"
#include "lib/ivis_common/textdraw.h"
#include "input.h"
#ifdef __cplusplus
}
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
	QFont regular, bold;
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
	int ticks() { return tickCount.elapsed(); }

public slots:
	void tick();
	void close();
};
         
#endif
