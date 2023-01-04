#ifndef __INCLUDED_TOOLS_MAPLIB_H__
#define __INCLUDED_TOOLS_MAPLIB_H__

#include <QtCore/qglobal.h>

// framework
#include "lib/framework/wzglobal.h"
#include "lib/framework/types.h"
#include "lib/framework/physfs_ext.h" // Also includes physfs.h

#include <cstdlib>
#include <stdio.h>
#include <string.h>

// framework/debug.h
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define debug(z, ...) do { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } while (0)



void physfs_init(const char* binpath);
void physfs_shutdown();

char* physfs_addmappath(char *path);

void physfs_printSearchPath();

#endif // #ifndef __INCLUDED_TOOLS_MAPLIB_H__
