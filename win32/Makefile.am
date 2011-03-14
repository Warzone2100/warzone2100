.rc.o:
	$(WINDRES) -DVERSION="\"$(VERSION)\"" -o$@ $<

dist_noinst_DATA = warzone2100.rc

EXTRA_DIST = __BUILD_SCRIPT \
	     download.pl \
	     rules.mk \
	     Toolchain-mingw32.cmake \
	     libs/Makefile \
	     libs/dejavu/Makefile \
	     libs/devpkg/Makefile \
	     libs/expat/Makefile \
	     libs/fontconfig/Makefile \
	     libs/freetype2/Makefile \
	     libs/gettext/Makefile \
	     libs/iconv/Makefile \
	     libs/ogg/Makefile \
	     libs/png/Makefile \
	     libs/physfs/Makefile \
	     libs/quesoglc/Makefile \
	     libs/quesoglc/patches \
	     libs/sdl/Makefile \
	     libs/sdl/patches \
	     libs/theora/Makefile \
	     libs/vorbis/Makefile \
	     libs/zlib/Makefile

if MINGW32
nodist_noinst_DATA = \
		warzone2100.o
CLEANFILES = $(nodist_noinst_DATA)
endif
