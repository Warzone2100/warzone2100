/*
	Copyright (C) 2008 by Warzone Resurrection Team

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this program. If not, see
	<http://www.gnu.org/licenses/>.
*/

#ifndef QWZM_H
#define QWZM_H

#include "ui_qwzm.h"

extern "C"
{
#include "wzmutils.h"
}

#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>

#include <QtCore/QFileInfo>
#include <QtGui/QComboBox>
#include <QtGui/QLabel>
#include <QtGui/QSlider>
#include <QtGui/QStandardItemModel>

class QWzmViewer : public QMainWindow
{
	Q_OBJECT

public:
	QWzmViewer(QWidget *parent = 0);
	~QWzmViewer();

protected:
	void closeEvent(QCloseEvent *event);
	MODEL *load3DS(QString input);
	MODEL *loadPIE(QString input);
	int savePIE(const char *filename, const MODEL *psModel, int pieVersion, int type);
	void setModel(const QFileInfo &texPath);

protected slots:
	void actionOpen();
	void actionSaveAs();
	void actionSave();
	void open3DS();
	void actionAboutApplication();
	void toggleWireframe();
	void toggleCulling();
	void setTeam(int index);
	void tick();
	void toggleAnimation();
	void toggleScale();
	void toggleSwapYZ();
	void toggleReverseWinding();
	void toggleFlipVerticalTexCoords();
	void setMesh(int index);
	void toggleEditAnimation(bool show);
	void toggleEditConnectors(bool show);
	void animLock();
	void animUnlock();
	void setVisibleMesh(int index);

	void rowsChanged(const QModelIndex &parent, int start, int end);
	void dataChanged(const QModelIndex &first, const QModelIndex &last);
	void reloadFrames();

	void prependFrame();
	void appendFrame();
	void removeFrame();

private:
	Ui::MainWindow *ui;
	MODEL *psModel;
	QString filename;
	QStandardItemModel animationModel;
	QStandardItemModel connectorsModel;
	QComboBox *selectedMeshComboBox;
	QComboBox *visibleMeshComboBox;
	QComboBox *teamComboBox;
	QSlider *scaleSlider;
	QLabel *fileNameLabel;
	QTimer *timer;
};
#endif
