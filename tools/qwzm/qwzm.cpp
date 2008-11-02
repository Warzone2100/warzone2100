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
	connect(actionCulling, SIGNAL(triggered()), this, SLOT(toggleCulling()));
	connect(actionAnimation, SIGNAL(triggered()), this, SLOT(toggleAnimation()));
	connect(actionScaleModel, SIGNAL(triggered()), this, SLOT(toggleScale()));
	connect(comboBoxTeam, SIGNAL(currentIndexChanged(int)), this, SLOT(toggleTeam(int)));
	connect(actionSwapYZ, SIGNAL(triggered()), this, SLOT(toggleSwapYZ()));
	connect(actionReverseWinding, SIGNAL(triggered()), this, SLOT(toggleReverseWinding()));
	connect(actionFlipVerticalTexCoords, SIGNAL(triggered()), this, SLOT(toggleFlipVerticalTexCoords()));

	// Set defaults
	toggleAnimation();
	actionSave->setEnabled(false);
	actionSaveAs->setEnabled(false);

	timer->start(25);
}

QWzmViewer::~QWzmViewer()
{
}

void QWzmViewer::toggleTeam(int index)
{
	glView->setTeam(index);
}

void QWzmViewer::toggleScale()
{
	double result = QInputDialog::getDouble(this, tr("Choose scale factor"), tr("Factor:") );
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
	glView->updateGL();
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

void QWzmViewer::setModel(QFileInfo &texPath)
{
	psModel->pixmap = readPixmap(texPath.absoluteFilePath().toAscii().constData());
	if (!psModel->pixmap)
	{
		QMessageBox::critical(this, tr("Oops..."), "Could not read texture", QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
		psModel = NULL;
		return;
	}
	comboBoxTeam->setCurrentIndex(0);
	glView->setModel(psModel);
	actionSave->setEnabled(false);
	actionSaveAs->setEnabled(true);
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
