noinst_PROGRAMS = sdl-testapp

sdl_testapp_SOURCES = \
	sdl-testapp.c

sdl_testapp_CPPFLAGS = $(LUA_CFLAGS) $(SDL_CFLAGS)
sdl_testapp_LDADD = $(OPENGL_LIBS) $(SDL_LIBS) $(top_builddir)/src/libbetawidget.la $(LUA_LIBTOOL)
