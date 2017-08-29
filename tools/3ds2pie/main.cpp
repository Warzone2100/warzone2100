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

#include <QApplication>
#include <QTranslator>
#include <QLocale>

#include "3ds2pie_gui.h"

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	QTranslator appTranslator;
	appTranslator.load("3ds2pie_gui_" + QLocale::system().name());
	app.installTranslator(&appTranslator);

	Gui3ds2pie * dialog = new Gui3ds2pie();
	dialog->show();
	return app.exec();
}

