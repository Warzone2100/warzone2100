noinst_PROGRAMS = auto_revision
auto_revision_SOURCES = autorevision.cpp

INCLUDES = -I$(top_srcdir)/src/include

auto_revision_LDADD = ../../base/tinyxml/libtinyxml.la

all-local: auto_revision
	./auto_revision +int +wx +t $(top_srcdir) $(top_srcdir)/src/include/autorevision.h
