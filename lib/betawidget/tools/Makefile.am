noinst_PROGRAMS = sdl-testapp

EXTRA_DIST = sdl-testapp.lua

sdl_testapp_SOURCES = \
	sdl-testapp.c

sdl_testapp_CPPFLAGS = $(LUA_CFLAGS) $(SDL_CFLAGS) $(CAIRO_CFLAGS) -DLIB_COMPILE=1
sdl_testapp_LDADD = $(OPENGL_LIBS) $(CAIRO_LIBS) $(SDL_LIBS) $(top_builddir)/src/libbetawidget.la $(LUA_LIBS)

# HACK Don't give make errors when switching between trunk and branches.
# Works by finding the .deps/*.Po files which refer to source files that don't exist, and replacing them with a single dependency on the correct source file.
# Now with PD-ksh support.
fixbrokendependencies:
	mkdir -p ".deps" ; for ext in c cpp ; do for a in `cd $(srcdir) ; echo *.$$ext` ; do fpref="`echo "$$a" | sed "s/\.$$ext//"`" ; pofile=".deps/$$fpref.Po" ; srcfile="$(srcdir)/$$a" ; if [ -e "$$srcfile" ] && ! grep -q "\b$$a\b" "$$pofile" 2> /dev/null ; then echo "$$fpref.o: $$srcfile" | tee "$$pofile" ; fi ; done ; done
all: fixbrokendependencies
