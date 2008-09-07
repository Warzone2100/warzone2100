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

static bool keyEvent(widget *self, const event *evt, int handlerId, void *userData)
{
	if (evt->type == EVT_KEY_DOWN)
	{
		puts("Key down");
	}
	else if (evt->type == EVT_DESTRUCT)
	{
		puts("Destruct");
	}
	
	return true;
}

static bool clickEvent(widget *self, const event *evt, int handlerId, void *userData)
{
	puts("Click");
	
	widgetReposition(self, 0, 0);
	
	return true;
}

static void createGUI()
{
	window *wnd;
	wnd = malloc(sizeof(window));
	windowInit(wnd, "myWindow", 400, 400);
	widgetReposition(WIDGET(wnd), 400, 50);
	widgetShow(WIDGET(wnd));
	
	widgetAddEventHandler(WIDGET(wnd), EVT_MOUSE_CLICK, clickEvent, NULL, NULL);
	widgetAddEventHandler(WIDGET(wnd), EVT_KEY_DOWN, keyEvent, keyEvent, NULL);
	
	window *wnd2;
	wnd2 = malloc(sizeof(window	));
	windowInit(wnd2, "myOtherWindow", 100, 100);
	windowRepositionFromAnchor(wnd2, wnd, CENTRE, 0, MIDDLE, 0);
	printf("It is: %f %f\n", WIDGET(wnd2)->offset.x, WIDGET(wnd2)->offset.y);
	widgetShow(WIDGET(wnd2));
}

int main(int argc, char *argv[])
{
	SDL_Surface *screen;
	SDL_Event sdlEvent;
	bool quit = false;
	
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
	createGUI();
	
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

	widgetSDLQuit();
	
	return EXIT_SUCCESS;
}
