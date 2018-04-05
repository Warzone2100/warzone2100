# for 3rdparty projects that don't support automake, provide support here

AM_CPPFLAGS = $(WZ_CPPFLAGS)
AM_CFLAGS = $(WZ_CFLAGS) $(WZ_C99FLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS)

# utf8proc

utf8proc_libutf8proc_a_CFLAGS = $(WZ_CFLAGS) -fPIC -std=c99 -Wall -pedantic -I$(srcdir)/utf8proc -DUTF8PROC_EXPORTS $(UTF8PROC_DEFINES)

noinst_LIBRARIES = utf8proc/libutf8proc.a
noinst_HEADERS = \
	utf8proc/utf8proc.h

utf8proc_libutf8proc_a_SOURCES = \
	utf8proc/utf8proc.c

EXTRA_DIST = \
	utf8proc
