#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QString>
#include "lint.h"

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <path to script file>\n", argv[0]);
		exit(-1);
	}
	testGlobalScript(argv[1]);
	return 0;
}
