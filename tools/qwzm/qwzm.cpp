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
	QTimer *timer = new QTimer(this);

	setupUi(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(tick()));
	connect(actionQuit, SIGNAL(triggered()), qApp, SLOT(quit()));
	connect(actionSave_as, SIGNAL(triggered()), this, SLOT(saveAs()));
	connect(actionImport_3DS, SIGNAL(triggered()), this, SLOT(open3DS()));
	connect(actionOpenWZM, SIGNAL(triggered()), this, SLOT(openWZM()));
	connect(actionWireframe, SIGNAL(triggered()), this, SLOT(toggleWireframe()));
	connect(actionCulling, SIGNAL(triggered()), this, SLOT(toggleCulling()));
	connect(actionAnimation, SIGNAL(triggered()), this, SLOT(toggleAnimation()));
	connect(comboBoxTeam, SIGNAL(currentIndexChanged(int)), this, SLOT(toggleTeam(int)));

	// Set defaults
	toggleAnimation();

	timer->start(50);
}

QWzmViewer::~QWzmViewer()
{
}

void QWzmViewer::toggleTeam(int index)
{
	glView->setTeam(index);
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
	filename = QFileDialog::getOpenFileName(this, tr("Choose output file"), QString::null, QString::null);
}

void QWzmViewer::save()
{
}

void QWzmViewer::open3DS()
{
	QString model = QFileDialog::getOpenFileName(this, tr("Choose 3DS file"), QString::null, tr("3DS models (*.3ds)"));
	QString texture = QFileDialog::getOpenFileName(this, tr("Find texture"), QString::null, tr("PNG texture (*.png)"));
	if (model != "" && texture != "")
	{
		// TODO
	}
}

void QWzmViewer::openWZM()
{
	filename = QFileDialog::getOpenFileName(this, tr("Choose 3DS file"), QString::null, tr("WZM models (*.wzm)"));
	if (filename != "")
	{
		MODEL *psModel = readModel(filename.toAscii().constData(), 0);
		if (psModel)
		{
			QFileInfo texPath(psModel->texPath);

			// Try to find texture automatically
			if (!texPath.exists())
			{
				texPath.setFile(QString("../../data/base/texpages/"), psModel->texPath);
				if (!texPath.exists())
				{
					texPath.setFile(QFileDialog::getExistingDirectory(this, tr("Specify texture directory"), QString::null), psModel->texPath);
					if (!texPath.exists())
					{
						QMessageBox::critical(this, tr("Oops..."), "Could not find texture", QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
						return;
					}
				}
			}

			qWarning("Creating model from %s and texture from %s", filename.toAscii().constData(), texPath.absoluteFilePath().toAscii().constData());
			psModel->pixmap = readPixmap(texPath.absoluteFilePath().toAscii().constData());
			if (!psModel->pixmap)
			{
				QMessageBox::critical(this, tr("Oops..."), "Could not read texture", QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
			}
			comboBoxTeam->setCurrentIndex(0);
			glView->setModel(psModel);
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
