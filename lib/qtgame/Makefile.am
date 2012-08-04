AM_CPPFLAGS = $(WZ_CPPFLAGS) $(QT4_CFLAGS) $(GLEW_CFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS) $(QT4_CFLAGS)

MOCHEADER = \
	qtgame.h \
	wzapp_qt.h
MOCEDFILES = $(MOCHEADER:%.h=%_moc.cpp)

%_moc.cpp: %.h
	$(MOC4) -o $@ $<

CLEANFILES = $(MOCEDFILES)

EXTRA_DIST = \
	swapinterval.mm \
	macosx_screen_resolutions.h macosx_screen_resolutions.cpp

noinst_LIBRARIES = libqtgame.a
noinst_HEADERS = \
	$(MOCHEADER) \
	qtgame.h \
	swapinterval.h

nodist_libqtgame_a_SOURCES = \
	qtgame_moc.cpp \
	wzapp_qt_moc.cpp

libqtgame_a_SOURCES = \
	main_qt.cpp \
	qtgame.cpp \
	swapinterval.cpp \
	wzapp_qt.cpp
