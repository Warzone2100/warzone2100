AM_CPPFLAGS = $(SDL_CFLAGS) $(WZ_CPPFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS)

noinst_LIBRARIES = libwidget.a
noinst_HEADERS = \
	bar.h \
	button.h \
	editbox.h \
	form.h \
	label.h \
	scrap.h \
	slider.h \
	tip.h \
	widgbase.h \
	widget.h \
	widgint.h

libwidget_a_SOURCES = \
	bar.c \
	button.c \
	editbox.c \
	form.c \
	label.c \
	scrap.c \
	slider.c \
	tip.c \
	widget.c

libwidget_a_LIBADD = $(top_builddir)/lib/ivis_opengl/libivis_opengl.a \
	$(top_builddir)/lib/ivis_common/libivis_common.a $(top_builddir)/lib/framework/libframework.a

# HACK Don't give make errors when switching between trunk and branches.
# Works by finding the .deps/*.Po files which refer to source files that don't exist, and replacing them with a single dependency on the correct source file.
# Now with PD-ksh support.
fixbrokendependencies:
	mkdir -p ".deps" ; for ext in c cpp ; do for a in `cd $(srcdir) ; echo *.$$ext` ; do fpref="`echo "$$a" | sed "s/\.$$ext//"`" ; pofile=".deps/$$fpref.Po" ; srcfile="$(srcdir)/$$a" ; if [ -e "$$srcfile" ] && ! grep -q "\b$$a\b" "$$pofile" 2> /dev/null ; then echo "$$fpref.o: $$srcfile" | tee "$$pofile" ; fi ; done ; done
all: fixbrokendependencies
