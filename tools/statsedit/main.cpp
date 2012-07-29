#include <QtGui/QApplication>
#include "statseditor.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	StatsEditor w;
	w.show();

	return a.exec();
}
