noinst_PROGRAMS = sdl-testapp

sdl_testapp_SOURCES = \
	sdl-testapp.c

sdl_testapp_CPPFLAGS = -I$(top_srcdir)/.. $(LUA_CFLAGS) $(SDL_CFLAGS)
sdl_testapp_LDADD = $(OPENGL_LIBS) $(SDL_LIBS) $(top_builddir)/libbetawidget.la $(LUA_LIBTOOL)
