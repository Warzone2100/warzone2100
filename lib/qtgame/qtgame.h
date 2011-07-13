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
	~QtGameWidget() { if (mResolutionChanged) restoreResolution(); }
	QList<QSize> availableResolutions() const { return mResolutions; }
	bool isMouseTrapped() { return mCursorTrapped; }

	int swapInterval() const { return mSwapInterval; }
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
	void resize(int w, int h) { resize(QSize(w, h)); }
	void setFixedSize(QSize size) { resize(size); }
	void setFixedSize(int w, int h) { resize(w, h); }
	void setFixedHeight(int h) { resize(size().width(), h); }
	void setFixedWidth(int w) { resize(w, size().height()); }
};

#endif
