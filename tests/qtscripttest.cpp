#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <QtCore/QCoreApplication>
#include "lint.h"

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	char datapath[PATH_MAX], fullpath[PATH_MAX], filename[PATH_MAX];
	FILE *fp = fopen("jslist.txt", "r");

	if (!fp)
	{
		fprintf(stderr, "%s: Failed to open list file\n", argv[0]);
		return -1;
	}
	strcpy(datapath, getenv("srcdir"));
	strcat(datapath, "/../data/");

	while (!feof(fp))
	{
		fscanf(fp, "%256s\n", filename);
		printf("Testing script: %s\n", filename);
		strcpy(fullpath, datapath);
		strcat(fullpath, filename);
		testGlobalScript(fullpath);
	}
	fclose(fp);

	return 0;
}
