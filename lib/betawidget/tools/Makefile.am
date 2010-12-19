noinst_PROGRAMS = sdl-testapp

EXTRA_DIST = sdl-testapp.lua

sdl_testapp_SOURCES = \
	sdl-testapp.cpp

sdl_testapp_CPPFLAGS = $(LUA_CFLAGS) $(SDL_CFLAGS) $(CAIRO_CFLAGS) -DLIB_COMPILE=1
sdl_testapp_LDADD = $(OPENGL_LIBS) $(CAIRO_LIBS) $(SDL_LIBS) $(top_builddir)/src/libbetawidget.la $(LUA_LIBS)
