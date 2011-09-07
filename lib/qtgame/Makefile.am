AM_CPPFLAGS = $(WZ_CPPFLAGS) $(QT4_CFLAGS) $(GLEW_CFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS) $(QT4_CFLAGS)

MOCHEADER = qtgame.h
MOCEDFILES = $(MOCHEADER:%.h=%_moc.cpp)

%_moc.cpp: %.h
	$(MOC4) -o $@ $<

CLEANFILES = $(MOCEDFILES)

noinst_LIBRARIES = libqtgame.a
noinst_HEADERS = qtgame.h swapinterval.h $(MOCHEADER)

nodist_libqtgame_a_SOURCES = \
	qtgame_moc.cpp

libqtgame_a_SOURCES = \
	qtgame.cpp \
	swapinterval.cpp
