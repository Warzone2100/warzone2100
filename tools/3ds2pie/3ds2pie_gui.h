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
#ifndef GUI3DS2PIE_H
#define GUI3DS2PIE_H

#include "ui_3ds2pie_gui.h"

/**
	@author Dennis Schridde <devurandom@gna.org>
*/
class Gui3ds2pie : public QDialog, private Ui::Gui3ds2pie
{
		Q_OBJECT
	public:
		Gui3ds2pie( QWidget *parent = 0 );

		~Gui3ds2pie();

	private slots:
		virtual void accept();

		void sliderChanged(int value);
		void spinboxChanged(double value);

		void browseInputFile();
		void browseOutputFile();

		void dragEnterEvent(QDragEnterEvent *event);
		void dropEvent(QDropEvent *event);
};

#endif
