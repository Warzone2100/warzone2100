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

#include "qwzm.h"
#include "conversion.h"

#include <QtCore/QSettings>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>


QWzmViewer::QWzmViewer(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow()), psModel(NULL), timer(new QTimer(this))
{
	QSettings settings;
	ui->setupUi(this);

	connectorsModel.setColumnCount(4);
	connectorsModel.setHeaderData(0, Qt::Horizontal, QString("X"));
	connectorsModel.setHeaderData(1, Qt::Horizontal, QString("Y"));
	connectorsModel.setHeaderData(2, Qt::Horizontal, QString("Z"));
	connectorsModel.setHeaderData(3, Qt::Horizontal, QString(tr("Type")));

	animationModel.setColumnCount(8);
	animationModel.setHeaderData(0, Qt::Horizontal, QString(tr("Time")));
	animationModel.setHeaderData(1, Qt::Horizontal, QString("Tex"));
	animationModel.setHeaderData(2, Qt::Horizontal, QString("Trs X"));
	animationModel.setHeaderData(3, Qt::Horizontal, QString("Trs Y"));
	animationModel.setHeaderData(4, Qt::Horizontal, QString("Trs Z"));
	animationModel.setHeaderData(5, Qt::Horizontal, QString("Rot X"));
	animationModel.setHeaderData(6, Qt::Horizontal, QString("Rot Y"));
	animationModel.setHeaderData(7, Qt::Horizontal, QString("Rot Z"));

	fileNameLabel = new QLabel(tr("No file loaded"), ui->statusBar);

	selectedMeshComboBox = new QComboBox(ui->statusBar);
	selectedMeshComboBox->setToolTip(tr("Selected Mesh"));
	selectedMeshComboBox->setEnabled(false);

	visibleMeshComboBox = new QComboBox(ui->statusBar);
	visibleMeshComboBox->setEnabled(false);
	visibleMeshComboBox->setToolTip(tr("Visible Mesh"));
	visibleMeshComboBox->addItem(tr("All"));

	teamComboBox = new QComboBox(ui->statusBar);
	teamComboBox->setToolTip(tr("Team Colour"));
	teamComboBox->addItem(tr("Green"));
	teamComboBox->addItem(tr("Yellow"));
	teamComboBox->addItem(tr("Grey"));
	teamComboBox->addItem(tr("Black"));
	teamComboBox->addItem(tr("Red"));
	teamComboBox->addItem(tr("Blue"));
	teamComboBox->addItem(tr("Pink"));
	teamComboBox->addItem(tr("Cyan"));

	scaleSlider = new QSlider(ui->statusBar);
	scaleSlider->setToolTip(tr("Scale"));
	scaleSlider->setRange(1, 1000);
	scaleSlider->setValue(100);
	scaleSlider->setOrientation(Qt::Horizontal);
	scaleSlider->setEnabled(false);

	ui->statusBar->addPermanentWidget(fileNameLabel);
	ui->statusBar->addPermanentWidget(selectedMeshComboBox);
	ui->statusBar->addPermanentWidget(visibleMeshComboBox);
	ui->statusBar->addPermanentWidget(teamComboBox);
	ui->statusBar->addPermanentWidget(scaleSlider);

	ui->connectorTableView->setModel(&connectorsModel);
	ui->animationTableView->setModel(&animationModel);

	ui->animationDockWidget->hide();
	ui->connectorDockWidget->hide();

	connect(timer, SIGNAL(timeout()), this, SLOT(tick()));
	connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(actionOpen()));
	connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(actionSave()));
	connect(ui->actionSaveAs, SIGNAL(triggered()), this, SLOT(actionSaveAs()));
	connect(ui->actionQuit, SIGNAL(triggered()), QApplication::instance(), SLOT(quit()));
	connect(ui->actionImport_3DS, SIGNAL(triggered()), this, SLOT(open3DS()));
	connect(ui->actionWireframe, SIGNAL(triggered()), this, SLOT(toggleWireframe()));
	connect(ui->actionHelpContents, SIGNAL(triggered()), ui->glView, SLOT(help()));
	connect(ui->actionAboutQt, SIGNAL(triggered()), QApplication::instance(), SLOT(aboutQt()));
	connect(ui->actionAboutApplication, SIGNAL(triggered()), this, SLOT(actionAboutApplication()));
	connect(ui->actionAxis, SIGNAL(triggered()), ui->glView, SLOT(toggleAxisIsDrawn()));
	connect(ui->actionCulling, SIGNAL(triggered()), this, SLOT(toggleCulling()));
	connect(ui->actionAnimation, SIGNAL(triggered()), this, SLOT(toggleAnimation()));
	connect(ui->actionScaleModel, SIGNAL(triggered()), this, SLOT(toggleScale()));
	connect(selectedMeshComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setMesh(int)));
	connect(visibleMeshComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setVisibleMesh(int)));
	connect(teamComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setTeam(int)));
	connect(ui->actionSwapYZ, SIGNAL(triggered()), this, SLOT(toggleSwapYZ()));
	connect(ui->actionReverseWinding, SIGNAL(triggered()), this, SLOT(toggleReverseWinding()));
	connect(ui->actionFlipVerticalTexCoords, SIGNAL(triggered()), this, SLOT(toggleFlipVerticalTexCoords()));
	connect(ui->actionFramesEditor, SIGNAL(toggled(bool)), this, SLOT(toggleEditAnimation(bool)));
	connect(ui->actionConnectorsEditor, SIGNAL(toggled(bool)), this, SLOT(toggleEditConnectors(bool)));
	connect(ui->animationDockWidget, SIGNAL(visibilityChanged(bool)), ui->actionFramesEditor, SLOT(setChecked(bool)));
	connect(ui->connectorDockWidget, SIGNAL(visibilityChanged(bool)), ui->actionConnectorsEditor, SLOT(setChecked(bool)));
	connect(ui->prependFrameButton, SIGNAL(pressed()), this, SLOT(prependFrame()));
	connect(ui->appendFrameButton, SIGNAL(pressed()), this, SLOT(appendFrame()));
	connect(ui->removeFrameButton, SIGNAL(pressed()), this, SLOT(removeFrame()));

	// Set defaults
	toggleAnimation();

	ui->actionSave->setEnabled(false);
	ui->actionSaveAs->setEnabled(false);

	restoreGeometry(settings.value("geometry").toByteArray());
	restoreState(settings.value("windowState").toByteArray());

	timer->start(25);
}

QWzmViewer::~QWzmViewer()
{
	delete ui;
}

void QWzmViewer::closeEvent(QCloseEvent *event)
{
	QSettings settings;

	settings.setValue("geometry", saveGeometry());
	settings.setValue("windowState", saveState());

	QMainWindow::closeEvent(event);
}

void QWzmViewer::actionOpen()
{
	static QString lastDir; // Convenience HACK to remember last successful directory a model was loaded from.
	filename = QFileDialog::getOpenFileName(this, tr("Choose a PIE or WZM file"), lastDir, tr("All Compatible (*.wzm *.pie);;WZM models (*.wzm);;PIE models (*.pie)"));
	if (!filename.isEmpty())
	{
		QFileInfo theFile(filename);
		MODEL *tmpModel = NULL;

		if (theFile.completeSuffix().compare(QString("wzm"), Qt::CaseInsensitive) == 0)
		{
			tmpModel = readModel(filename.toAscii().constData(), 0);
		}
		else if (theFile.completeSuffix().compare(QString("pie"), Qt::CaseInsensitive) == 0)
		{
			tmpModel = loadPIE(filename);
		}
		else
		{
			return;
		}

		if (tmpModel)
		{

			QFileInfo texPath(theFile.absoluteDir(), tmpModel->texPath);

			// Try to find texture automatically
			if (!texPath.exists())
			{
				texPath.setFile(QString("../../data/base/texpages/"), tmpModel->texPath);
				if (!texPath.exists())
				{
					texPath.setFile(QFileDialog::getExistingDirectory(this, tr("Specify texture directory"), QString::null), tmpModel->texPath);
					if (!texPath.exists())
					{
						QMessageBox::warning(this, tr("Oops..."), "Could not find texture", QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
						freeModel(tmpModel);
						return;
					}
				}
			}
			if (psModel)
			{
				freeModel(psModel);
			}
			psModel = tmpModel;
			setModel(texPath);
			ui->actionSave->setEnabled(true);
			lastDir = theFile.absolutePath();
			fileNameLabel->setText(theFile.fileName());
			fileNameLabel->setToolTip(filename);
		}
		else
		{
			qWarning("Failed to create model!");

			ui->statusBar->showMessage(tr("Failed to create model!"), 3000);
		}
	}
}

void QWzmViewer::actionSaveAs()
{
	if (psModel)
	{
		filename = QFileDialog::getSaveFileName(this, tr("Choose output file"), QString::null, tr("All Compatible (*.wzm *.pie);;WZM (*.wzm);;PIE (*.pie)"));
		ui->actionSave->setEnabled(true);
		fileNameLabel->setText(QFileInfo(filename).fileName());
		fileNameLabel->setToolTip(filename);
		actionSave();
	}
}

void QWzmViewer::actionSave()
{
	int retVal = -1;
	if (!filename.isEmpty() && psModel)
	{
		QFileInfo fileInfo(filename);
		if (fileInfo.completeSuffix().compare(QString("wzm"), Qt::CaseInsensitive) == 0)
		{
			retVal = saveModel(filename.toAscii().constData(), psModel);
		}
		else if (fileInfo.completeSuffix().compare(QString("pie"), Qt::CaseInsensitive) == 0)
		{
			QPieExportDialog dialog;
			dialog.exec();
			if (dialog.result())
			{
				retVal = savePIE(filename.toAscii().constData(), psModel, dialog.getPieVersion(), dialog.getFlags());
			}
			else
			{
				retVal = 0;
			}
		}

		if (retVal != 0)
		{
			QMessageBox::warning(this, tr("Oops..."), "Could not save model", QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
		}
	}
}

void QWzmViewer::actionAboutApplication()
{
	QMessageBox::about(this, tr("About QWZM"), QString(tr("<b>QWZM %1</b><br />Model viewer and editor for Warzone 2100.").arg(QApplication::instance()->applicationVersion())));
}

void QWzmViewer::toggleEditAnimation(bool show)
{
	ui->animationDockWidget->setVisible(show);
}

void QWzmViewer::toggleEditConnectors(bool show)
{
	ui->connectorDockWidget->setVisible(show);
}

void QWzmViewer::setTeam(int index)
{
	ui->glView->setTeam(index);
}

void QWzmViewer::toggleScale()
{
//  double result = QInputDialog::getDouble(this, tr("Choose scale factor"), tr("Factor:") );
	qWarning("TODO");
}

void QWzmViewer::toggleSwapYZ()
{
	qWarning("TODO");
}

void QWzmViewer::toggleReverseWinding()
{
	qWarning("TODO");
}

void QWzmViewer::toggleFlipVerticalTexCoords()
{
	qWarning("TODO");
}

void QWzmViewer::tick()
{
	int currentMesh = selectedMeshComboBox->currentIndex();
	if (psModel)
	{
		MESH *psMesh = &psModel->mesh[currentMesh];

		psMesh->currentFrame = ui->animationTableView->currentIndex().row();

		if(psMesh->frames <= 0)
		{
			psMesh->currentFrame = -1;
		}
		else if(psMesh->currentFrame >= psMesh->frames || psMesh->currentFrame < 0)
		{
			psMesh->currentFrame = 0;
		}
	}
	ui->glView->updateGL();
	if (psModel && ui->actionAnimation->isChecked())
	{
		MESH *psMesh = &psModel->mesh[currentMesh];

		ui->animationTableView->setCurrentIndex(animationModel.index(psMesh->currentFrame, 0));
	}
}

void QWzmViewer::toggleCulling()
{
	if (ui->actionCulling->isChecked())
	{
		glEnable(GL_CULL_FACE);
	}
	else
	{
		glDisable(GL_CULL_FACE);
	}
}

void QWzmViewer::toggleAnimation()
{
	// Reset animation on start because it might be out of sync
	if (psModel && ui->actionAnimation->isChecked())
	{
		ui->animationTableView->setCurrentIndex(ui->animationTableView->model()->index(0, 0));

		for (int i = 0; i < psModel->meshes; i++)
		{
			MESH *psMesh = &psModel->mesh[i];

			psMesh->currentFrame = 0;
			psMesh->lastChange = 0;
		}
	}

	ui->glView->setAnimation(ui->actionAnimation->isChecked());
}

void QWzmViewer::toggleWireframe()
{
	if (ui->actionWireframe->isChecked())
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}

void QWzmViewer::rowsChanged(const QModelIndex&, int, int)
{
	reloadFrames();
}

void QWzmViewer::dataChanged(const QModelIndex&, const QModelIndex&)
{
	reloadFrames();
}

// Load animation frames from UI model into WZM drawing model
void QWzmViewer::reloadFrames()
{
	MESH *psMesh = &psModel->mesh[selectedMeshComboBox->currentIndex()];

	// Reallocate frames
	psMesh->frames = animationModel.rowCount();
	free(psMesh->frameArray);
	psMesh->frameArray = (FRAME *)malloc(sizeof(FRAME) * psMesh->frames);

	for (int i = 0; i < psMesh->frames; i++)
	{
		FRAME *psFrame = &psMesh->frameArray[i];

		psFrame->timeSlice = animationModel.data(animationModel.index(i, 0, QModelIndex())).toDouble();
		psFrame->textureArray = animationModel.data(animationModel.index(i, 1, QModelIndex())).toInt();
		psFrame->translation.x = animationModel.data(animationModel.index(i, 2, QModelIndex())).toDouble();
		psFrame->translation.y = animationModel.data(animationModel.index(i, 3, QModelIndex())).toDouble();
		psFrame->translation.z = animationModel.data(animationModel.index(i, 4, QModelIndex())).toDouble();
		psFrame->rotation.x = animationModel.data(animationModel.index(i, 5, QModelIndex())).toDouble();
		psFrame->rotation.y = animationModel.data(animationModel.index(i, 6, QModelIndex())).toDouble();
		psFrame->rotation.z = animationModel.data(animationModel.index(i, 7, QModelIndex())).toDouble();
	}

	if(psMesh->frames <= 0)
	{
		psMesh->currentFrame = -1;
	}
	else if(psMesh->currentFrame >= psMesh->frames || psMesh->currentFrame < 0)
	{
		psMesh->currentFrame = 0;
	}
}

void QWzmViewer::prependFrame()
{
	QModelIndex index = ui->animationTableView->currentIndex();

	animLock();
	animationModel.insertRow(index.row());
	animationModel.setData(animationModel.index(index.row(), 0, QModelIndex()), QString::number(0.1));
	animationModel.setData(animationModel.index(index.row(), 1, QModelIndex()), QString::number(0));
	animationModel.setData(animationModel.index(index.row(), 2, QModelIndex()), QString::number(0.0));
	animationModel.setData(animationModel.index(index.row(), 3, QModelIndex()), QString::number(0.0));
	animationModel.setData(animationModel.index(index.row(), 4, QModelIndex()), QString::number(0.0));
	animationModel.setData(animationModel.index(index.row(), 5, QModelIndex()), QString::number(0.0));
	animationModel.setData(animationModel.index(index.row(), 6, QModelIndex()), QString::number(0.0));
	animUnlock();	// act on last change only
	animationModel.setData(animationModel.index(index.row(), 7, QModelIndex()), QString::number(0.0));
}

void QWzmViewer::appendFrame()
{
	QModelIndex index = ui->animationTableView->currentIndex();
	int idx = index.row() + 1;

	animLock();
	animationModel.insertRow(idx);
	animationModel.setData(animationModel.index(idx, 0, QModelIndex()), QString::number(0.1));
	animationModel.setData(animationModel.index(idx, 1, QModelIndex()), QString::number(0));
	animationModel.setData(animationModel.index(idx, 2, QModelIndex()), QString::number(0.0));
	animationModel.setData(animationModel.index(idx, 3, QModelIndex()), QString::number(0.0));
	animationModel.setData(animationModel.index(idx, 4, QModelIndex()), QString::number(0.0));
	animationModel.setData(animationModel.index(idx, 5, QModelIndex()), QString::number(0.0));
	animationModel.setData(animationModel.index(idx, 6, QModelIndex()), QString::number(0.0));
	animUnlock();	// act on last change only
	animationModel.setData(animationModel.index(idx, 7, QModelIndex()), QString::number(0.0));
}

void QWzmViewer::removeFrame()
{
	QModelIndex index = ui->animationTableView->currentIndex();
	animationModel.removeRow(index.row());
}

void QWzmViewer::animLock()
{
	// Prevent backscatter
	disconnect(&animationModel, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(dataChanged(const QModelIndex &, const QModelIndex &)));
	disconnect(&animationModel, SIGNAL(rowsRemoved(const QModelIndex &, int, int)), this, SLOT(rowsChanged(const QModelIndex &, int, int)));
	disconnect(&animationModel, SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(rowsChanged(const QModelIndex &, int, int)));
}

void QWzmViewer::animUnlock()
{
	connect(&animationModel, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(dataChanged(const QModelIndex &, const QModelIndex &)));
	connect(&animationModel, SIGNAL(rowsRemoved(const QModelIndex &, int, int)), this, SLOT(rowsChanged(const QModelIndex &, int, int)));
	connect(&animationModel, SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(rowsChanged(const QModelIndex &, int, int)));
}

void QWzmViewer::setMesh(int index)
{
	if (index < 0)
	{
		return;
	}
	MESH *psMesh = &psModel->mesh[index];

	// Refresh frame view
	animLock();
	animationModel.setRowCount(psMesh->frames);
	for (int i = 0; i < psMesh->frames; i++)
	{
		FRAME *psFrame = &psMesh->frameArray[i];

		animationModel.setData(animationModel.index(i, 0, QModelIndex()), QString::number(psFrame->timeSlice));
		animationModel.setData(animationModel.index(i, 1, QModelIndex()), QString::number(psFrame->textureArray));
		animationModel.setData(animationModel.index(i, 2, QModelIndex()), QString::number(psFrame->translation.x));
		animationModel.setData(animationModel.index(i, 3, QModelIndex()), QString::number(psFrame->translation.y));
		animationModel.setData(animationModel.index(i, 4, QModelIndex()), QString::number(psFrame->translation.z));
		animationModel.setData(animationModel.index(i, 5, QModelIndex()), QString::number(psFrame->rotation.x));
		animationModel.setData(animationModel.index(i, 6, QModelIndex()), QString::number(psFrame->rotation.y));
		animationModel.setData(animationModel.index(i, 7, QModelIndex()), QString::number(psFrame->rotation.z));
	}

	ui->animationTableView->resizeColumnsToContents();
	ui->animationTableView->horizontalHeader()->setStretchLastSection(true);
	ui->animationTableView->setCurrentIndex(animationModel.index(psMesh->currentFrame, 0));

	animUnlock();

	// Refresh connector view
	connectorsModel.setRowCount(psMesh->connectors);
	for (int i = 0; i < psMesh->connectors; i++)
	{
		connectorsModel.setData(connectorsModel.index(i, 0, QModelIndex()), QString::number(psMesh->connectorArray[i].pos.x));
		connectorsModel.setData(connectorsModel.index(i, 1, QModelIndex()), QString::number(psMesh->connectorArray[i].pos.y));
		connectorsModel.setData(connectorsModel.index(i, 2, QModelIndex()), QString::number(psMesh->connectorArray[i].pos.z));
		connectorsModel.setData(connectorsModel.index(i, 4, QModelIndex()), QString::number(psMesh->connectorArray[i].type)); // TODO, dropdown box
	}
}

void QWzmViewer::setModel(const QFileInfo &texPath)
{
	psModel->pixmap = readPixmap(texPath.absoluteFilePath().toAscii().constData());

	if (!psModel->pixmap)
	{
		QMessageBox::warning(this, tr("Oops..."), "Could not read texture", QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
		freeModel(psModel);
		psModel = NULL;
		return;
	}

	ui->glView->setModel(psModel);
	teamComboBox->setCurrentIndex(0);
	ui->actionSave->setEnabled(false);
	ui->actionSaveAs->setEnabled(true);
	selectedMeshComboBox->setEnabled(true);
	selectedMeshComboBox->setMaxCount(0);	// delete previous
	selectedMeshComboBox->setMaxCount(psModel->meshes);
	visibleMeshComboBox->setEnabled(true);
	visibleMeshComboBox->setMaxCount(1);
	visibleMeshComboBox->setMaxCount(psModel->meshes + 1);
	ui->animationDockWidgetContents->setEnabled(true);

	for (int i = 0; i < psModel->meshes; i++)
	{
		selectedMeshComboBox->insertItem(i, QString::number(i));
		visibleMeshComboBox->insertItem(i + 1, QString::number(i));
	}

	selectedMeshComboBox->setCurrentIndex(0);
	setMesh(0);
}

void QWzmViewer::setVisibleMesh(int index)
{
	ui->glView->selectMesh(index);
}

void QWzmViewer::open3DS()
{
	QString model = QFileDialog::getOpenFileName(this, tr("Choose 3DS file"), QString::null, tr("3DS models (*.3ds)"));

	if (!model.isEmpty())
	{
		MODEL *tmpModel = load3DS(model);

		if (tmpModel)
		{
			QFileInfo texPath(tmpModel->texPath);

			if (!texPath.exists())
			{
				texPath.setFile(QFileDialog::getOpenFileName(this, tr("Find texture"), QString::null, tr("PNG texture (*.png)")));
				strcpy(tmpModel->texPath, texPath.fileName().toAscii().constData());
				if (!texPath.exists())
				{
					QMessageBox::warning(this, tr("Oops..."), "Could not open texture", QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
					freeModel(tmpModel);
					return;
				}
			}
			if (psModel)
			{
				freeModel(psModel);
			}
			psModel = tmpModel;
			ui->actionSave->setEnabled(true);
			setModel(texPath);
		}
		else
		{
			qWarning("Failed to create model!");

			ui->statusBar->showMessage(tr("Failed to create model!"), 3000);
		}
	}
}

int main(int argc, char *argv[])
{
	QApplication application(argc, argv);
	application.setApplicationName("QWZM");
	application.setApplicationVersion("Pre-Alpha");
	application.setOrganizationName("Warzone2100");
	application.setOrganizationDomain("wz2100.net");

	QWzmViewer *wzm = new QWzmViewer();

	wzm->show();
	return application.exec();
}
