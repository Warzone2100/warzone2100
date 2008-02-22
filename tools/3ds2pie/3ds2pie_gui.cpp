/*
	Copyright (C) 2008 by Dennis Schridde

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
#include "3ds2pie_gui.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QUrl>

// For dump_pie_file
#include <lib3ds/file.h>


extern "C" void dump_pie_file(Lib3dsFile *f, FILE *o, const char *page, bool swapYZ, bool inverseUV, bool reverseWinding, int baseTexFlags, float scaleFactor);


Gui3ds2pie::Gui3ds2pie( QWidget *parent )
		: QDialog(parent), Ui::Gui3ds2pie()
{
	setupUi(this);

	setAcceptDrops(true);

	connect(inputFile_browse, SIGNAL(clicked()), this, SLOT(browseInputFile()));
	connect(outputFile_browse, SIGNAL(clicked()), this, SLOT(browseOutputFile()));
}


Gui3ds2pie::~Gui3ds2pie()
{
}


void Gui3ds2pie::browseInputFile()
{
	QString path = QFileDialog::getOpenFileName(this, tr("Choose input file"), QString::null, QString::null);
	inputFile_edit->setText(path);
}


void Gui3ds2pie::browseOutputFile()
{
	QString path = QFileDialog::getSaveFileName(this, tr("Choose output file"), QString::null, QString::null);
	outputFile_edit->setText(path);
}


void Gui3ds2pie::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasUrls())
		foreach(const QUrl &url, event->mimeData()->urls())
			if(url.path().endsWith(".3ds", Qt::CaseInsensitive))
			{
				event->acceptProposedAction();
				break;
			}
}


void Gui3ds2pie::dropEvent(QDropEvent *event)
{
	foreach(const QUrl &url, event->mimeData()->urls())
		if(url.path().endsWith(".3ds", Qt::CaseInsensitive))
		{
			QString path = url.path();
			inputFile_edit->setText(path);
			outputFile_edit->setText( path.replace( path.lastIndexOf(".3ds", -1, Qt::CaseInsensitive), 4, ".pie" ) );
			break;
		}
}


void Gui3ds2pie::accept()
{
	QString inputFile = inputFile_edit->text();
	QString outputFile = outputFile_edit->text();
	QString texturePage = texturePage_edit->text();
	unsigned int baseTexFlags = (twoSidedPolys->isChecked() ? 2200 : 200 );

	if (inputFile.isEmpty())
	{
		QMessageBox::critical(this, tr("Error"), tr("Please specify an input file"));
		return;
	}
	if (outputFile.isEmpty())
	{
		QMessageBox::critical(this, tr("Error"), tr("Please specify an output file"));
		return;
	}

	Lib3dsFile *f = lib3ds_file_load(inputFile.toAscii().data());
	if (!f)
	{
		QMessageBox::critical(this, tr("Error"), tr("Loading file %1 failed").arg(inputFile));
		QDialog::reject();
		return;
	}

	FILE *o = fopen(outputFile.toAscii().data(), "w+");
	if (!o)
	{
		lib3ds_file_free(f);

		QMessageBox::critical(this, tr("Error"), tr("Can't open %1 for writing").arg(outputFile));
		QDialog::reject();
		return;
	}

	dump_pie_file(f, o, texturePage.toAscii().data(), swapYZ->isChecked(), invertUV->isChecked(), reverseWinding->isChecked(), baseTexFlags, 1.0f);

	fclose(o);
	lib3ds_file_free(f);

	QDialog::accept();
}


