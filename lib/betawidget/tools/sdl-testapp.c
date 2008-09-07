#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <SDL.h>
#include <SDL_main.h>

#include <betawidget/widget.h>
#include <betawidget/hBox.h>
#include <betawidget/spacer.h>
#include <betawidget/textEntry.h>
#include <betawidget/window.h>
#include <betawidget/platform/sdl/event.h>
#include <betawidget/platform/sdl/init.h>
#include <betawidget/lua-wrap.h>
#include <lualib.h>
#include <lauxlib.h>

static lua_State* lua_open_betawidget(void)
{
	lua_State* L = lua_open();
	luaopen_base(L);
	luaopen_string(L);
	luaopen_betawidget(L);

	return L;
}

static void createGUI(lua_State* const L)
{
	const int top = lua_gettop(L); /* save stack */
#if (defined(LUA_VERSION_NUM) && (LUA_VERSION_NUM>=501))
	if (luaL_dofile(L, "sdl-testapp.lua") != 0)	/* looks like this is lua 5.1.X or later, good */
#else
	if (lua_dofile(L, "sdl-testapp.lua") != 0)	/* might be lua 5.0.x, using lua_dostring */
#endif
		fputs(lua_tostring(L, -1), stderr);

	lua_settop(L, top); /* restore the stack */
}

int main(int argc, char *argv[])
{
	SDL_Surface *screen;
	SDL_Event sdlEvent;
	bool quit = false;
	lua_State* L;

	// Make sure everything is cleaned up on quit
	atexit(SDL_Quit);

	// Init SDL
	if (SDL_Init(SDL_INIT_VIDEO) < -1)
	{
		fprintf(stderr, "SDL_init: %s\n", SDL_GetError());
		exit(1);
	}

	// Enable UNICODE support (for key events)
	SDL_EnableUNICODE(true);

	// 800x600 true colour
	screen = SDL_SetVideoMode(800, 600, 32, SDL_OPENGL);
	if (screen == NULL)
	{
		fprintf(stderr, "SDL_SetVideoMode: %s\n", SDL_GetError());
		exit(1);
	}

	SDL_WM_SetCaption("Warzone UI Simulator", NULL);

	// Init OpenGL
	glClearColor(0.8f, 0.8f, 0.8f, 0.0f);
	glCullFace(GL_BACK);
	glEnable(GL_BLEND);
	glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);

	glViewport(0, 0, 800, 600);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, 800, 600, 0.0, -1.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	/*
	 * Create the GUI
	 */
	widgetSDLInit();
	L = lua_open_betawidget();
	createGUI(L);

	// Main event loop
	while (!quit)
	{
		int i;

		while (SDL_PollEvent(&sdlEvent))
		{
			if (sdlEvent.type == SDL_QUIT)
			{
				quit = true;
			}
			else
			{
				widgetHandleSDLEvent(&sdlEvent);
			}
		}

		// Fire timer events
		widgetFireTimers();

		glClear(GL_COLOR_BUFFER_BIT);

		for (i = 0; i < vectorSize(windowGetWindowVector()); i++)
		{
			window *wnd = vectorAt(windowGetWindowVector(), i);

			widgetDraw(WIDGET(wnd));
			widgetComposite(WIDGET(wnd));
		}


		SDL_GL_SwapBuffers();
	}

	lua_close(L);
	widgetSDLQuit();
	SDL_Quit();

	return EXIT_SUCCESS;
}
