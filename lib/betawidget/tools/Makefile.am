noinst_PROGRAMS = sdl-testapp

EXTRA_DIST = sdl-testapp.lua

sdl_testapp_SOURCES = \
	sdl-testapp.c

sdl_testapp_CPPFLAGS = $(LUA_CFLAGS) $(SDL_CFLAGS)
sdl_testapp_LDADD = $(OPENGL_LIBS) $(SDL_LIBS) $(top_builddir)/src/libbetawidget.la $(LUA_LIBS)
