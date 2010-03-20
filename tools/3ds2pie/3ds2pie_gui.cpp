/*
	Copyright (C) 2008-2009 by Dennis Schridde

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
#include "3ds2pie.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QUrl>

// For dump_pie_file
#include <lib3ds/file.h>


Gui3ds2pie::Gui3ds2pie( QWidget *parent )
		: QDialog(parent), Ui::Gui3ds2pie()
{
	setupUi(this);

	setAcceptDrops(true);

	connect(inputFile_browse, SIGNAL(clicked()), this, SLOT(browseInputFile()));
	connect(outputFile_browse, SIGNAL(clicked()), this, SLOT(browseOutputFile()));
	connect(scale_slider, SIGNAL(valueChanged(int)), this, SLOT(sliderChanged(int)));
	connect(scale_spinbox, SIGNAL(valueChanged(double)), this, SLOT(spinboxChanged(double)));
}


Gui3ds2pie::~Gui3ds2pie()
{
}


void Gui3ds2pie::sliderChanged(int value)
{
	scale_spinbox->setValue(value/100.0);
}


void Gui3ds2pie::spinboxChanged(double value)
{
	scale_slider->setValue(value*100.0);
}


void Gui3ds2pie::browseInputFile()
{
	QString path = QFileDialog::getOpenFileName(this, tr("Choose input file"), QString::null, "*.3ds");
	inputFile_edit->setText(path);
}


void Gui3ds2pie::browseOutputFile()
{
	QString path = QFileDialog::getSaveFileName(this, tr("Choose output file"), QString::null, "*.pie");
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

	PIE_OPTIONS options;
	options.twoSidedPolys = twoSidedPolys->isChecked();
	options.exportPIE3 = exportPIE3->isChecked();
	options.invertUV = invertUV->isChecked();
	options.page = strdup(texturePage.toAscii().data());
	options.reverseWinding = reverseWinding->isChecked();
	options.scaleFactor = scale_spinbox->value();
	options.swapYZ = swapYZ->isChecked();
	options.useTCMask = useTCMask->isChecked();

	WZ_PIE_LEVEL pie;

	dump_3ds_to_pie(f, &pie, options);
	dump_pie_file(&pie, o, options);

	fclose(o);
	lib3ds_file_free(f);

	if (options.page)
		free(options.page);

	QDialog::accept();
}


