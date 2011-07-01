AM_CPPFLAGS = $(WZ_CPPFLAGS) $(QT4_CFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS) $(QT4_CFLAGS)

MOCHEADER = qtgame.h
MOCEDFILES = $(MOCHEADER:%.h=%_moc.cpp)

%_moc.cpp: %.h
	$(MOC4) -o $@ $<

BUILT_SOURCES = $(MOCEDFILES)
CLEANFILES = $(MOCEDFILES)

noinst_LIBRARIES = libqtgame.a
noinst_HEADERS = qtgame.h $(MOCHEADER)
libqtgame_a_SOURCES = qtgame.cpp qtgame_moc.cpp
