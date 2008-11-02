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

#include <lib3ds/file.h>
#include <lib3ds/mesh.h>
#include <lib3ds/vector.h>
#include <lib3ds/material.h>

#include "qwzm.h"

void QWzmViewer::load3DS(QString input)
{
	const bool swapYZ = true;
	const bool reverseWinding = true;
	const bool invertUV = true;
	const float scaleFactor = 1.0f;
	Lib3dsFile *f = lib3ds_file_load(input.toAscii().constData());
	Lib3dsMaterial *material = f->materials;
	int meshes = 0;
	Lib3dsMesh *m;
	int meshIdx;

	if (!f)
	{
		qWarning("Loading 3DS file %s failed", input.toAscii().constData());
		return;
	}

	for (meshes = 0, m = f->meshes; m; m = m->next, meshes++) ;	// count the meshes
	if (psModel)
	{
		freeModel(psModel);
		psModel = NULL;
	}
	psModel = createModel(meshes, 0);
	if (!psModel)
	{
		qFatal("Out of memory");
	}

	// Grab texture name
	for (int j = 0; material; material = material->next, j++)
	{
		Lib3dsTextureMap *texture = &material->texture1_map;
		QString texName(texture->name);

		if (j > 0)
		{
			qWarning("Texture %d %s: More than one texture currently not supported!", j, texture->name);
			continue;
		}
		strcpy(psModel->texPath, texName.toLower().toAscii().constData());
	}

	// Grab each mesh
	for (meshIdx = 0, m = f->meshes; m; m = m->next, meshIdx++)
	{
		MESH *psMesh = &psModel->mesh[meshIdx];

		psMesh->vertices = m->points;
		psMesh->faces = m->faces;
		psMesh->vertexArray = (GLfloat*)malloc(sizeof(GLfloat) * psMesh->vertices * 3);
		psMesh->indexArray = (GLuint*)malloc(sizeof(GLuint) * psMesh->faces * 3);
		psMesh->textureArrays = 1;	// only one supported from 3DS
		psMesh->textureArray[0] = (GLfloat*)malloc(sizeof(GLfloat) * psMesh->vertices * 2);

		for (unsigned int i = 0; i < m->points; i++)
		{
			Lib3dsVector pos;

			lib3ds_vector_copy(pos, m->pointL[i].pos);

			if (swapYZ)
			{
				psMesh->vertexArray[i * 3 + 0] = pos[0] * scaleFactor;
				psMesh->vertexArray[i * 3 + 1] = pos[2] * scaleFactor;
				psMesh->vertexArray[i * 3 + 2] = pos[1] * scaleFactor;
			}
			else
			{
				psMesh->vertexArray[i * 3 + 0] = pos[0] * scaleFactor;
				psMesh->vertexArray[i * 3 + 1] = pos[1] * scaleFactor;
				psMesh->vertexArray[i * 3 + 2] = pos[2] * scaleFactor;
			}
		}

		for (unsigned int i = 0; i < m->points; i++)
		{
			GLfloat *v = &psMesh->textureArray[0][i * 2];

			v[0] = m->texelL[i][0];
			if (invertUV)
			{
				v[1] = 1.0f - m->texelL[i][1];
			}
			else
			{
				v[1] = m->texelL[i][1];
			}
		}

		for (unsigned int i = 0; i < m->faces; ++i)
		{
			Lib3dsFace *face = &m->faceL[i];
			GLuint *v = &psMesh->indexArray[i * 3];

			if (reverseWinding)
			{
				v[0] = face->points[2];
				v[1] = face->points[1];
				v[2] = face->points[0];
			}
			else
			{
				v[0] = face->points[0];
				v[1] = face->points[1];
				v[2] = face->points[2];
			}
		}
	}

	lib3ds_file_free(f);
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
	connect(actionWireframe, SIGNAL(triggered()), this, SLOT(toggleWireframe()));
	connect(actionCulling, SIGNAL(triggered()), this, SLOT(toggleCulling()));
	connect(actionAnimation, SIGNAL(triggered()), this, SLOT(toggleAnimation()));
	connect(actionScaleModel, SIGNAL(triggered()), this, SLOT(toggleScale()));
	connect(comboBoxTeam, SIGNAL(currentIndexChanged(int)), this, SLOT(toggleTeam(int)));

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
	// TODO
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
			psModel->pixmap = readPixmap(texPath.absoluteFilePath().toAscii().constData());
			qWarning("Loading %s with texture %s", model.toAscii().constData(), texPath.absoluteFilePath().toAscii().constData());
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
						return;
					}
				}
			}

			qWarning("Creating model from %s and texture from %s", filename.toAscii().constData(), texPath.absoluteFilePath().toAscii().constData());
			tmpModel->pixmap = readPixmap(texPath.absoluteFilePath().toAscii().constData());
			if (!tmpModel->pixmap)
			{
				QMessageBox::critical(this, tr("Oops..."), "Could not read texture", QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
			}
			else
			{
				comboBoxTeam->setCurrentIndex(0);
				glView->setModel(tmpModel);
				actionSaveAs->setEnabled(true);
				actionSave->setEnabled(true);
				psModel = tmpModel;
			}
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
