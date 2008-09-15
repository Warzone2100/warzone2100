if !USE_SYSTEM_SQLITE
AM_CPPFLAGS = $(SDL_CFLAGS) $(patsubst -DDATADIR%,,$(WZ_CPPFLAGS))
AM_CFLAGS = $(patsubst -W%,,$(WZ_CFLAGS))

noinst_LIBRARIES = libsqlite3.a
noinst_HEADERS = sqlite3ext.h sqlite3.h

libsqlite3_a_SOURCES = sqlite3.c
endif
