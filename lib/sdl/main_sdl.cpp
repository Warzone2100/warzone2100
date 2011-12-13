#include <QtCore/QCoreApplication>

extern void mainLoop();

void wzMain(int &argc, char **argv)
{
	QCoreApplication app(argc, argv);  // For Qt-script.
}

bool wzMain2()
{
	return true;
}

void wzMain3()
{
	mainLoop();
}
