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

	You should have received a copy of the GNU Lessser General Public
	License along with this program. If not, see
	<http://www.gnu.org/licenses/>.
*/

#include "qwzm.h"

QConnectorViewer::QConnectorViewer(QWidget *parent)
           : QDialog(parent), Ui_ConnectorView()
{
	setupUi(this);
	connect(pushButtonClose, SIGNAL(pressed()), this, SLOT(hide()));
//	connect(pushButtonPrepend, SIGNAL(pressed()), parent, SLOT(prependConnector()));
//	connect(pushButtonAppend, SIGNAL(pressed()), parent, SLOT(appendConnector()));
//	connect(pushButtonRemove, SIGNAL(pressed()), parent, SLOT(removeConnector()));
}

void QConnectorViewer::setModel(QStandardItemModel *model)
{
	tableView->setModel(model);
	tableView->setSelectionMode(QAbstractItemView::SingleSelection);
	tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
}

QModelIndex QConnectorViewer::selectedIndex()
{
	return tableView->currentIndex();
}

void QConnectorViewer::setSelectedIndex(int idx)
{
	tableView->setCurrentIndex(tableView->model()->index(idx, 0));
}

void QConnectorViewer::updateModel()
{
	tableView->resizeColumnsToContents();
}

QConnectorViewer::~QConnectorViewer()
{
}

QAnimViewer::QAnimViewer(QWidget *parent)
           : QDialog(parent), Ui_AnimationView()
{
	setupUi(this);
	connect(pushButtonClose, SIGNAL(pressed()), this, SLOT(hide()));
	connect(pushButtonPrepend, SIGNAL(pressed()), parent, SLOT(prependFrame()));
	connect(pushButtonAppend, SIGNAL(pressed()), parent, SLOT(appendFrame()));
	connect(pushButtonRemove, SIGNAL(pressed()), parent, SLOT(removeFrame()));
}

void QAnimViewer::setModel(QStandardItemModel *model)
{
	tableViewAnimation->setModel(model);
	tableViewAnimation->setSelectionMode(QAbstractItemView::SingleSelection);
	tableViewAnimation->setSelectionBehavior(QAbstractItemView::SelectRows);
}

QModelIndex QAnimViewer::selectedIndex()
{
	return tableViewAnimation->currentIndex();
}

void QAnimViewer::setSelectedIndex(int idx)
{
	tableViewAnimation->setCurrentIndex(tableViewAnimation->model()->index(idx, 0));
}

void QAnimViewer::updateModel()
{
	tableViewAnimation->resizeColumnsToContents();
}

QAnimViewer::~QAnimViewer()
{
}

QWzmViewer::QWzmViewer(QWidget *parent)
           : QMainWindow(parent), Ui::QWZM()
{
	psModel = NULL;
	QTimer *timer = new QTimer(this);

	setupUi(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(tick()));
	connect(actionQuit, SIGNAL(triggered()), qApp, SLOT(quit()));
	connect(actionSave, SIGNAL(triggered()), this, SLOT(save()));
	connect(actionSaveAs, SIGNAL(triggered()), this, SLOT(saveAs()));
	connect(actionImport_3DS, SIGNAL(triggered()), this, SLOT(open3DS()));
	connect(actionOpenWZM, SIGNAL(triggered()), this, SLOT(openWZM()));
	connect(actionImport_PIE, SIGNAL(triggered()), this, SLOT(openPIE()));
	connect(actionWireframe, SIGNAL(triggered()), this, SLOT(toggleWireframe()));
	connect(actionHelp, SIGNAL(triggered()), glView, SLOT(help()));
	connect(actionAxis, SIGNAL(triggered()), glView, SLOT(toggleAxisIsDrawn()));
	connect(actionCulling, SIGNAL(triggered()), this, SLOT(toggleCulling()));
	connect(actionAnimation, SIGNAL(triggered()), this, SLOT(toggleAnimation()));
	connect(actionScaleModel, SIGNAL(triggered()), this, SLOT(toggleScale()));
	connect(comboBoxTeam, SIGNAL(currentIndexChanged(int)), this, SLOT(setTeam(int)));
	connect(comboBoxSelectedMesh, SIGNAL(currentIndexChanged(int)), this, SLOT(setMesh(int)));
	connect(actionSwapYZ, SIGNAL(triggered()), this, SLOT(toggleSwapYZ()));
	connect(actionReverseWinding, SIGNAL(triggered()), this, SLOT(toggleReverseWinding()));
	connect(actionFlipVerticalTexCoords, SIGNAL(triggered()), this, SLOT(toggleFlipVerticalTexCoords()));
	connect(actionEditFrames, SIGNAL(triggered()), this, SLOT(toggleEditAnimation()));
	connect(actionEditConnectors, SIGNAL(triggered()), this, SLOT(toggleEditConnectors()));

	// Set defaults
	toggleAnimation();
	actionSave->setEnabled(false);
	actionSaveAs->setEnabled(false);

	connectorView = new QConnectorViewer(this);
	connectors.setColumnCount(4);
	connectors.setHeaderData(0, Qt::Horizontal, QString("X"));
	connectors.setHeaderData(1, Qt::Horizontal, QString("Y"));
	connectors.setHeaderData(2, Qt::Horizontal, QString("Z"));
	connectors.setHeaderData(3, Qt::Horizontal, QString("Type"));
	connectorView->setModel(&connectors);

	animView = new QAnimViewer(this);
	anim.setColumnCount(8);
	anim.setHeaderData(0, Qt::Horizontal, QString("Time"));
	anim.setHeaderData(1, Qt::Horizontal, QString("Tex"));
	anim.setHeaderData(2, Qt::Horizontal, QString("Trs X"));
	anim.setHeaderData(3, Qt::Horizontal, QString("Trs Y"));
	anim.setHeaderData(4, Qt::Horizontal, QString("Trs Z"));
	anim.setHeaderData(5, Qt::Horizontal, QString("Rot X"));
	anim.setHeaderData(6, Qt::Horizontal, QString("Rot Y"));
	anim.setHeaderData(7, Qt::Horizontal, QString("Rot Z"));
	animView->setModel(&anim);

	timer->start(25);
}

QWzmViewer::~QWzmViewer()
{
}

void QWzmViewer::toggleEditAnimation()
{
	animView->show();
}

void QWzmViewer::toggleEditConnectors()
{
	connectorView->show();
}

void QWzmViewer::setTeam(int index)
{
	glView->setTeam(index);
}

void QWzmViewer::toggleScale()
{
	double result = QInputDialog::getDouble(this, tr("Choose scale factor"), tr("Factor:") );
	qWarning("TODO: %f", result);
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
	int currentMesh = comboBoxSelectedMesh->currentIndex();
	if (psModel)
	{
		MESH *psMesh = &psModel->mesh[currentMesh];

		psMesh->currentFrame = animView->selectedIndex().row();
	}
	glView->updateGL();
	if (psModel && actionAnimation->isChecked())
	{
		MESH *psMesh = &psModel->mesh[currentMesh];

		animView->setSelectedIndex(psMesh->currentFrame);
	}
}

void QWzmViewer::toggleCulling()
{
	if (actionCulling->isChecked())
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
	if (psModel && actionAnimation->isChecked())
	{
		animView->setSelectedIndex(0);
		for (int i = 0; i < psModel->meshes; i++)
		{
			MESH *psMesh = &psModel->mesh[i];

			psMesh->currentFrame = 0;
			psMesh->lastChange = 0;
		}
	}

	glView->setAnimation(actionAnimation->isChecked());
}

void QWzmViewer::toggleWireframe()
{
	if (actionWireframe->isChecked())
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}

void QWzmViewer::saveAs()
{
	if (psModel)
	{
		filename = QFileDialog::getSaveFileName(this, tr("Choose output file"), QString::null, QString::null);
		actionSave->setEnabled(true);
		save();
	}
}

void QWzmViewer::save()
{
	if (filename != "" && psModel)
	{
		if (saveModel(filename.toAscii().constData(), psModel) != 0)
		{
			QMessageBox::critical(this, tr("Oops..."), "Could not save model", QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
		}
	}
}

void QWzmViewer::rowsChanged(const QModelIndex &parent, int start, int end)
{
	reloadFrames();
}

void QWzmViewer::dataChanged(const QModelIndex &first, const QModelIndex &last)
{
	reloadFrames();
}

// Load animation frames from UI model into WZM drawing model
void QWzmViewer::reloadFrames()
{
	MESH *psMesh = &psModel->mesh[comboBoxSelectedMesh->currentIndex()];

	// Reallocate frames
	psMesh->frames = anim.rowCount();
	free(psMesh->frameArray);
	psMesh->frameArray = (FRAME *)malloc(sizeof(FRAME) * psMesh->frames);

	for (int i = 0; i < psMesh->frames; i++)
	{
		FRAME *psFrame = &psMesh->frameArray[i];

		psFrame->timeSlice = anim.data(anim.index(i, 0, QModelIndex())).toDouble();
		psFrame->textureArray = anim.data(anim.index(i, 1, QModelIndex())).toInt();
		psFrame->translation.x = anim.data(anim.index(i, 2, QModelIndex())).toDouble();
		psFrame->translation.y = anim.data(anim.index(i, 3, QModelIndex())).toDouble();
		psFrame->translation.z = anim.data(anim.index(i, 4, QModelIndex())).toDouble();
		psFrame->rotation.x = anim.data(anim.index(i, 5, QModelIndex())).toDouble();
		psFrame->rotation.y = anim.data(anim.index(i, 6, QModelIndex())).toDouble();
		psFrame->rotation.z = anim.data(anim.index(i, 7, QModelIndex())).toDouble();
	}
}

void QWzmViewer::prependFrame()
{
	QModelIndex index = animView->selectedIndex();

	animLock();
	anim.insertRow(index.row());
	anim.setData(anim.index(index.row(), 0, QModelIndex()), QString::number(0.1));
	anim.setData(anim.index(index.row(), 1, QModelIndex()), QString::number(0));
	anim.setData(anim.index(index.row(), 2, QModelIndex()), QString::number(0.0));
	anim.setData(anim.index(index.row(), 3, QModelIndex()), QString::number(0.0));
	anim.setData(anim.index(index.row(), 4, QModelIndex()), QString::number(0.0));
	anim.setData(anim.index(index.row(), 5, QModelIndex()), QString::number(0.0));
	anim.setData(anim.index(index.row(), 6, QModelIndex()), QString::number(0.0));
	animUnlock();	// act on last change only
	anim.setData(anim.index(index.row(), 7, QModelIndex()), QString::number(0.0));
}

void QWzmViewer::appendFrame()
{
	QModelIndex index = animView->selectedIndex();
	int idx = index.row() + 1;

	animLock();
	anim.insertRow(idx);
	anim.setData(anim.index(idx, 0, QModelIndex()), QString::number(0.1));
	anim.setData(anim.index(idx, 1, QModelIndex()), QString::number(0));
	anim.setData(anim.index(idx, 2, QModelIndex()), QString::number(0.0));
	anim.setData(anim.index(idx, 3, QModelIndex()), QString::number(0.0));
	anim.setData(anim.index(idx, 4, QModelIndex()), QString::number(0.0));
	anim.setData(anim.index(idx, 5, QModelIndex()), QString::number(0.0));
	anim.setData(anim.index(idx, 6, QModelIndex()), QString::number(0.0));
	animUnlock();	// act on last change only
	anim.setData(anim.index(idx, 7, QModelIndex()), QString::number(0.0));
}

void QWzmViewer::removeFrame()
{
	QModelIndex index = animView->selectedIndex();
	anim.removeRow(index.row());
}

void QWzmViewer::animLock()
{
	// Prevent backscatter
	disconnect(&anim, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(dataChanged(const QModelIndex &, const QModelIndex &)));
	disconnect(&anim, SIGNAL(rowsRemoved(const QModelIndex &, int, int)), this, SLOT(rowsChanged(const QModelIndex &, int, int)));
	disconnect(&anim, SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(rowsChanged(const QModelIndex &, int, int)));
}

void QWzmViewer::animUnlock()
{
	connect(&anim, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(dataChanged(const QModelIndex &, const QModelIndex &)));
	connect(&anim, SIGNAL(rowsRemoved(const QModelIndex &, int, int)), this, SLOT(rowsChanged(const QModelIndex &, int, int)));
	connect(&anim, SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(rowsChanged(const QModelIndex &, int, int)));
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
	anim.setRowCount(psMesh->frames);
	for (int i = 0; i < psMesh->frames; i++)
	{
		FRAME *psFrame = &psMesh->frameArray[i];

		anim.setData(anim.index(i, 0, QModelIndex()), QString::number(psFrame->timeSlice));
		anim.setData(anim.index(i, 1, QModelIndex()), QString::number(psFrame->textureArray));
		anim.setData(anim.index(i, 2, QModelIndex()), QString::number(psFrame->translation.x));
		anim.setData(anim.index(i, 3, QModelIndex()), QString::number(psFrame->translation.y));
		anim.setData(anim.index(i, 4, QModelIndex()), QString::number(psFrame->translation.z));
		anim.setData(anim.index(i, 5, QModelIndex()), QString::number(psFrame->rotation.x));
		anim.setData(anim.index(i, 6, QModelIndex()), QString::number(psFrame->rotation.y));
		anim.setData(anim.index(i, 7, QModelIndex()), QString::number(psFrame->rotation.z));
	}
	animView->updateModel();
	animView->setSelectedIndex(psMesh->currentFrame);
	animUnlock();

	// Refresh connector view
	connectors.setRowCount(psMesh->connectors);
	for (int i = 0; i < psMesh->connectors; i++)
	{
		connectors.setData(connectors.index(i, 0, QModelIndex()), QString::number(psMesh->connectorArray[i].pos.x));
		connectors.setData(connectors.index(i, 1, QModelIndex()), QString::number(psMesh->connectorArray[i].pos.y));
		connectors.setData(connectors.index(i, 2, QModelIndex()), QString::number(psMesh->connectorArray[i].pos.z));
		connectors.setData(connectors.index(i, 4, QModelIndex()), QString::number(psMesh->connectorArray[i].type)); // TODO, dropdown box
	}
}

void QWzmViewer::setModel(QFileInfo &texPath)
{
	psModel->pixmap = readPixmap(texPath.absoluteFilePath().toAscii().constData());
	if (!psModel->pixmap)
	{
		QMessageBox::critical(this, tr("Oops..."), "Could not read texture", QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
		psModel = NULL;
		return;
	}
	glView->setModel(psModel);
	comboBoxTeam->setCurrentIndex(0);
	actionSave->setEnabled(false);
	actionSaveAs->setEnabled(true);
	comboBoxSelectedMesh->setMaxCount(0);	// delete previous
	comboBoxSelectedMesh->setMaxCount(psModel->meshes);
	for (int i = 0; i < psModel->meshes; i++)
	{
		comboBoxSelectedMesh->insertItem(i, QString::number(i));
	}
	comboBoxSelectedMesh->setCurrentIndex(0);
	setMesh(0);
}

void QWzmViewer::open3DS()
{
	QString model = QFileDialog::getOpenFileName(this, tr("Choose 3DS file"), QString::null, tr("3DS models (*.3ds)"));
	if (model != "")
	{
		load3DS(model);
		if (psModel)
		{
			QFileInfo texPath(psModel->texPath);

			if (!texPath.exists())
			{
				texPath.setFile(QString("../../data/base/texpages/"), psModel->texPath);
				if (!texPath.exists())
				{
					texPath.setFile(QFileDialog::getOpenFileName(this, tr("Find texture"), QString::null, tr("PNG texture (*.png)")));
					strcpy(psModel->texPath, texPath.fileName().toAscii().constData());
					if (!texPath.exists())
					{
						QMessageBox::critical(this, tr("Oops..."), "Could not open texture", QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
						psModel = NULL;
						return;
					}
				}
			}
			setModel(texPath);
		}
		else
		{
			qWarning("Failed to create model");
		}
	}
}

void QWzmViewer::openPIE()
{
	QString model = QFileDialog::getOpenFileName(this, tr("Choose PIE file"), QString::null, tr("PIE models (*.pie)"));
	if (model != "")
	{
		loadPIE(model);
		if (psModel)
		{
			QFileInfo texPath(psModel->texPath);

			if (!texPath.exists())
			{
				texPath.setFile(QString("../../data/base/texpages/"), psModel->texPath);
				if (!texPath.exists())
				{
					texPath.setFile(QFileDialog::getOpenFileName(this, tr("Find texture"), QString::null, tr("PNG texture (*.png)")));
					strcpy(psModel->texPath, texPath.fileName().toAscii().constData());
					if (!texPath.exists())
					{
						QMessageBox::critical(this, tr("Oops..."), "Could not open texture", QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
						psModel = NULL;
						return;
					}
				}
			}
			setModel(texPath);
		}
		else
		{
			qWarning("Failed to create model");
		}
	}
}

void QWzmViewer::openWZM()
{
	filename = QFileDialog::getOpenFileName(this, tr("Choose 3DS file"), QString::null, tr("WZM models (*.wzm)"));
	if (filename != "")
	{
		MODEL *tmpModel = readModel(filename.toAscii().constData(), 0);

		if (tmpModel)
		{
			QFileInfo texPath(tmpModel->texPath);

			// Try to find texture automatically
			if (!texPath.exists())
			{
				texPath.setFile(QString("../../data/base/texpages/"), tmpModel->texPath);
				if (!texPath.exists())
				{
					texPath.setFile(QFileDialog::getExistingDirectory(this, tr("Specify texture directory"), QString::null), tmpModel->texPath);
					if (!texPath.exists())
					{
						QMessageBox::critical(this, tr("Oops..."), "Could not find texture", QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
						free(tmpModel);
						return;
					}
				}
			}
			psModel = tmpModel;
			setModel(texPath);
			actionSave->setEnabled(true);
		}
		else
		{
			qWarning("Failed to create model!");
		}
	}
}

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	QWzmViewer *wzm = new QWzmViewer();

	wzm->show();
	return app.exec();
}
