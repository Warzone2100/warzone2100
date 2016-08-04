#include <QtGui/QApplication>
#include <QFileInfo>
#include "statseditor.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	const char *path;
	if (argc <= 1)
	{
		// No args, look for default paths
		// TODO!
		qFatal("Need path to data directory");
	}
	else
	{
		path = argv[1];
	}
	// Verify path
	QFileInfo v(path);
	if (!v.exists() || !v.isDir())
	{
		qFatal("No such directory");
		// TODO, more testing!
	}
	StatsEditor w(path);
	w.show();
	return a.exec();
}
