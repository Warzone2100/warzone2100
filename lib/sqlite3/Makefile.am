AM_CPPFLAGS = $(SDL_CFLAGS) $(WZ_CPPFLAGS)
AM_CFLAGS = $(WZ_CFLAGS) -Wno-missing-field-initializers

noinst_LIBRARIES = libsqlite3.a
noinst_HEADERS = physfs_vfs.h sqlite3ext.h sqlite3.h

libsqlite3_a_SOURCES = physfs_vfs.c sqlite3.c
