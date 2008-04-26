/*
 *  PieToaster is an OpenGL application to edit 3D models in
 *  Warzone 2100's (an RTS game) PIE 3D model format, which is heavily
 *  inspired by PieSlicer created by stratadrake.
 *  Copyright (C) 2007  Carl Hee
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "SDL.h"
#include "AntTweakBar.h"

#include <SDL_opengl.h>

#include "pie_types.h"
#include "imdloader.h"
#include "resmaster.h"
#include "gui.h"

#include "screen.h"
#include "game_io.h"


int main(int argc, char *argv[])
{
	const SDL_VideoInfo* video = NULL;
	bool bQuit = false;

	if( g_Screen.initialize() < 0 )
	{
		fprintf(stderr, "Video initialization failed: %s\n", SDL_GetError());
		SDL_Quit();
        exit(1);
    }

    video = SDL_GetVideoInfo();
    if( !video )
	{
		fprintf(stderr, "Video query failed: %s\n", SDL_GetError());
		SDL_Quit();
        exit(1);
    }

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	g_Screen.m_bpp = video->vfmt->BitsPerPixel;

	fprintf(stderr, "BPP %d\n", video->vfmt->BitsPerPixel);
	if (g_Screen.m_bpp == 16)
	{
	    fprintf(stderr, "WARNING:Running at 16bit expecting decreased performance\nChanging Desktop depth to 32bit may correct this problem\n");
	}

	g_Screen.m_flags = SDL_OPENGL | SDL_HWSURFACE | SDL_RESIZABLE;
	g_Screen.m_width = 800;
	g_Screen.m_height = 600;
	g_Screen.m_useVBO = false;

	//flags |= SDL_FULLSCREEN;
	if( !g_Screen.setVideoMode() )
	{
        fprintf(stderr, "Video mode set failed: %s", SDL_GetError());
		SDL_Quit();
		exit(1);
	}
	SDL_WM_SetCaption("Pie Toaster powered by AntTweakBar+SDL", "WZ Stats Tools");
	// Enable SDL unicode and key-repeat
	SDL_EnableUNICODE(1);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	// Set OpenGL viewport and states
	glViewport(0, 0, g_Screen.m_width, g_Screen.m_height);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	//glEnable(GL_LIGHTING);
	//glEnable(GL_LIGHT0);	// use default light diffuse and position
	//glEnable(GL_NORMALIZE);
	//glEnable(GL_COLOR_MATERIAL);
	glDisable(GL_CULL_FACE);
	//glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);

	glShadeModel(GL_SMOOTH);

	// Initialize AntTweakBar
	TwInit(TW_OPENGL, NULL);

	/// pie vertex type
	TwStructMember vertice3fMembers[] =
	{
		{ "X",	   TW_TYPE_FLOAT,	offsetof(_vertice_list, vertice.x), "step=0.1" },
		{ "Y",	   TW_TYPE_FLOAT,	offsetof(_vertice_list, vertice.y), "step=0.1" },
		{ "Z",	   TW_TYPE_FLOAT,	offsetof(_vertice_list, vertice.z),	"step=0.1" },
		{ "Selected", TW_TYPE_BOOLCPP,	offsetof(_vertice_list, selected), "" },
	};

	g_tw_pieVertexType = TwDefineStruct("VerticePie", vertice3fMembers, 4, sizeof(_vertice_list), NULL, NULL);

	TwStructMember vector2fMembers[] =
	{
		{ "X",	   TW_TYPE_FLOAT,	offsetof(Vector2f, x), "min=0 max=4096 step=1" },
		{ "Y",	   TW_TYPE_FLOAT,	offsetof(Vector2f, y), "min=0 max=4096 step=1" },
	};

	g_tw_pieVector2fType = TwDefineStruct("Vector2fPie", vector2fMembers, 2, sizeof(Vector2f), NULL, NULL);

	// Tell the window size to AntTweakBar
	TwWindowSize(g_Screen.m_width, g_Screen.m_height);

	iIMDShape *testIMD = NULL;

	//ResMaster = new CResMaster;

	ResMaster.getOGLExtensionString();


	if (ResMaster.isOGLExtensionAvailable("GL_ARB_vertex_buffer_object"))
	{
		g_Screen.initializeVBOExtension();
	}

	ResMaster.cacheGridsVertices();

	ResMaster.readTextureList("pages.txt");

	ResMaster.loadTexPages();

	ResMaster.addGUI();

#ifdef SDL_TTF_TEST
	if (!(ResMaster.initFont() && ResMaster.loadFont("FreeMono.ttf", 12)))
	{
		return 1;
	}
#endif

	//argc = 2;
	//argv[1] = "building1b.pie";

	if (argc < 2)
	{
		fprintf(stderr, "NOTE:no file specified\n");
		//ResMaster.addPie(testIMD, "newpie"); //No need to add new pie for now
	}
	else
	{
		testIMD = iV_ProcessIMD(argv[1]);

		if (testIMD == NULL)
		{
			fprintf(stderr, "no file specified\n creating new one...\n");
			ResMaster.addPie(testIMD, "newpie");
		}
		else
		{
			ResMaster.addPie(testIMD, argv[1]);
		}
	}


	//ResMaster.getPieAt(0)->ToFile("test13.pie");

	OpenFileDialog.m_Up = false;

	while( !bQuit )
	{
		SDL_Event event;
		int handled;

		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		// Set OpenGL camera
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(30, (double)g_Screen.m_width/g_Screen.m_height, 1, 1000);
		gluLookAt(0,0,250, 0,0,0, 0,1,0);

		//updateBars();

		inputUpdate();

        // Process incoming events
		while( SDL_PollEvent(&event) )
		{
			if (OpenFileDialog.m_Up)
			{
				if (event.type == SDL_KEYDOWN)
				{
					Uint16 key = event.key.keysym.sym;
					SDLMod modifier = event.key.keysym.mod;

					if (key == SDLK_BACKSPACE)
					{
						OpenFileDialog.decrementChar();
					}
					else if (key == SDLK_KP_ENTER || key == SDLK_RETURN)
					{
						OpenFileDialog.doFunction();
						OpenFileDialog.deleteTextBox();
						continue;
					}
					else if (key == SDLK_PAUSE ||
							key == SDLK_ESCAPE)
					{
						//cancels current action and closes text box
						//OpenFileDialog.text_pointer = NULL;
						OpenFileDialog.deleteTextBox();
						continue;
					}
					else if (key > 12 && key < 128)
					{
						if (modifier & KMOD_CAPS ||
							modifier & KMOD_LSHIFT ||
							modifier & KMOD_RSHIFT)
						{
							if (key >= 'a' && key <= 'z')
							{
								key = toupper(key);
							}
							else
							{
								key = shiftChar(key);
							}
						}
						OpenFileDialog.incrementChar(key);
					}
					OpenFileDialog.updateTextBox();
					//interrupts input when text box is up
					continue;
				}
			}
			else if (AddSubModelDialog.m_Up)
			{
				if (event.type == SDL_KEYDOWN)
				{
					Uint16 key = event.key.keysym.sym;
					SDLMod modifier = event.key.keysym.mod;

					if (key == SDLK_BACKSPACE)
					{
						AddSubModelDialog.decrementChar();
					}
					else if (key == SDLK_KP_ENTER || key == SDLK_RETURN)
					{
						AddSubModelDialog.doFunction();
						AddSubModelDialog.deleteTextBox();
						continue;
					}
					else if (key == SDLK_PAUSE ||
							key == SDLK_ESCAPE)
					{
						//cancels current action and closes text box
						//OpenFileDialog.text_pointer = NULL;
						AddSubModelDialog.deleteTextBox();
						continue;
					}
					else if (key > 12 && key < 128)
					{
						if (modifier & KMOD_CAPS ||
							modifier & KMOD_LSHIFT ||
							modifier & KMOD_RSHIFT)
						{
							if (key >= 'a' && key <= 'z')
							{
								key = toupper(key);
							}
							else
							{
								key = shiftChar(key);
							}
						}
						AddSubModelDialog.incrementChar(key);
					}
					AddSubModelDialog.updateTextBox();
					//interrupts input when text box is up
					continue;
				}
			}
			else if (AddSubModelFileDialog.m_Up)
			{
				if (event.type == SDL_KEYDOWN)
				{
					Uint16 key = event.key.keysym.sym;
					SDLMod modifier = event.key.keysym.mod;

					if (key == SDLK_BACKSPACE)
					{
						AddSubModelFileDialog.decrementChar();
					}
					else if (key == SDLK_KP_ENTER || key == SDLK_RETURN)
					{
						AddSubModelFileDialog.doFunction();
						AddSubModelFileDialog.deleteTextBox();
						continue;
					}
					else if (key == SDLK_PAUSE ||
							key == SDLK_ESCAPE)
					{
						//cancels current action and closes text box
						//OpenFileDialog.text_pointer = NULL;
						AddSubModelFileDialog.deleteTextBox();
						continue;
					}
					else if (key > 12 && key < 128)
					{
						if (modifier & KMOD_CAPS ||
							modifier & KMOD_LSHIFT ||
							modifier & KMOD_RSHIFT)
						{
							if (key >= 'a' && key <= 'z')
							{
								key = toupper(key);
							}
							else
							{
								key = shiftChar(key);
							}
						}
						AddSubModelFileDialog.incrementChar(key);
					}
					AddSubModelFileDialog.updateTextBox();
					//interrupts input when text box is up
					continue;
				}
			}
			else if (ReadAnimFileDialog.m_Up)
			{
				if (event.type == SDL_KEYDOWN)
				{
					Uint16 key = event.key.keysym.sym;
					SDLMod modifier = event.key.keysym.mod;

					if (key == SDLK_BACKSPACE)
					{
						ReadAnimFileDialog.decrementChar();
					}
					else if (key == SDLK_KP_ENTER || key == SDLK_RETURN)
					{
						ReadAnimFileDialog.doFunction();
						ReadAnimFileDialog.deleteTextBox();
						continue;
					}
					else if (key == SDLK_PAUSE ||
							key == SDLK_ESCAPE)
					{
						//cancels current action and closes text box
						//OpenFileDialog.text_pointer = NULL;
						ReadAnimFileDialog.deleteTextBox();
						continue;
					}
					else if (key > 12 && key < 128)
					{
						if (modifier & KMOD_CAPS ||
							modifier & KMOD_LSHIFT ||
							modifier & KMOD_RSHIFT)
						{
							if (key >= 'a' && key <= 'z')
							{
								key = toupper(key);
							}
							else
							{
								key = shiftChar(key);
							}
						}
						ReadAnimFileDialog.incrementChar(key);
					}
					ReadAnimFileDialog.updateTextBox();
					//interrupts input when text box is up
					continue;
				}
			}
			else if (WriteAnimFileDialog.m_Up)
			{
				if (event.type == SDL_KEYDOWN)
				{
					Uint16 key = event.key.keysym.sym;
					SDLMod modifier = event.key.keysym.mod;

					if (key == SDLK_BACKSPACE)
					{
						WriteAnimFileDialog.decrementChar();
					}
					else if (key == SDLK_KP_ENTER || key == SDLK_RETURN)
					{
						WriteAnimFileDialog.doFunction();
						WriteAnimFileDialog.deleteTextBox();
						continue;
					}
					else if (key == SDLK_PAUSE ||
							key == SDLK_ESCAPE)
					{
						//cancels current action and closes text box
						//OpenFileDialog.text_pointer = NULL;
						WriteAnimFileDialog.deleteTextBox();
						continue;
					}
					else if (key > 12 && key < 128)
					{
						if (modifier & KMOD_CAPS ||
							modifier & KMOD_LSHIFT ||
							modifier & KMOD_RSHIFT)
						{
							if (key >= 'a' && key <= 'z')
							{
								key = toupper(key);
							}
							else
							{
								key = shiftChar(key);
							}
						}
						WriteAnimFileDialog.incrementChar(key);
					}
					WriteAnimFileDialog.updateTextBox();
					//interrupts input when text box is up
					continue;
				}
			}

			if (event.type == SDL_KEYDOWN)
			{
				Uint16 key = event.key.keysym.sym;
				SDLMod modifier = event.key.keysym.mod;

				if ((key == SDLK_KP_ENTER || key == SDLK_RETURN) && modifier & KMOD_LALT)
				{
					// Resize SDL video mode
 					g_Screen.m_flags ^= SDL_FULLSCREEN;
					if( !g_Screen.setVideoMode() )
						fprintf(stderr, "WARNING: Video mode set failed: %s", SDL_GetError());

					// Resize OpenGL viewport
					glViewport(0, 0, g_Screen.m_width, g_Screen.m_height);
					if (ResMaster.isTextureMapperUp())
					{
						glMatrixMode(GL_PROJECTION);
						glLoadIdentity();
						glOrtho(0, g_Screen.m_width, 0, g_Screen.m_height, -1, 1);
						glMatrixMode(GL_MODELVIEW);
						glLoadIdentity();
					}
					else
					{
						// Restore OpenGL states (SDL seems to lost them)
						glEnable(GL_DEPTH_TEST);
						glEnable(GL_TEXTURE_2D);
						gluPerspective(30, (double)g_Screen.m_width/g_Screen.m_height, 1, 1000);
						gluLookAt(0,0,250, 0,0,0, 0,1,0);
						//glEnable(GL_LIGHTING);
						//glEnable(GL_LIGHT0);
						//glEnable(GL_NORMALIZE);
						//glEnable(GL_COLOR_MATERIAL);
					}
					glDisable(GL_CULL_FACE);
					//glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);

					// Reloads texture and bind
					ResMaster.freeTexPages();
					ResMaster.loadTexPages();
					ResMaster.cacheGridsVertices();

					// Flush vbo id's
					Uint32	index;
					for (index = 0;index < ResMaster.m_pieCount;index++)
					{
						ResMaster.getPieAt(index)->flushVBOPolys();
					}

					SDL_Event newEvent;
					newEvent.type = SDL_VIDEORESIZE;
					newEvent.resize.w = g_Screen.m_width;
					newEvent.resize.h = g_Screen.m_height;
					SDL_PushEvent(&newEvent);
				}
			}

			// Send event to AntTweakBar
			handled = TwEventSDL(&event);

			// If event has not been handled by AntTweakBar, process it
			if( !handled )
			{
				switch( event.type )
				{
				case SDL_KEYUP:
				case SDL_KEYDOWN:
					inputKeyEvent(event.key, event.type);
					break;
				case SDL_MOUSEMOTION:
					inputMotionMouseEvent(event.motion);
					break;
				case SDL_MOUSEBUTTONUP:
				case SDL_MOUSEBUTTONDOWN:
					inputButtonMouseEvent(event.button, event.type);
					break;
				case SDL_QUIT:	// Window is closed
					bQuit = true;
					break;
				case SDL_VIDEORESIZE:	// Window size has changed
					// Resize SDL video mode
 					g_Screen.m_width = event.resize.w;
					g_Screen.m_height = event.resize.h;
					if( !g_Screen.setVideoMode() )
						fprintf(stderr, "WARNING: Video mode set failed: %s", SDL_GetError());

					// Resize OpenGL viewport
					glViewport(0, 0, g_Screen.m_width, g_Screen.m_height);
					if (ResMaster.isTextureMapperUp())
					{
						glMatrixMode(GL_PROJECTION);
						glLoadIdentity();
						glOrtho(0, g_Screen.m_width, 0, g_Screen.m_height, -1, 1);
						glMatrixMode(GL_MODELVIEW);
						glLoadIdentity();
					}
					else
					{
						// Restore OpenGL states (SDL seems to lost them)
						glEnable(GL_DEPTH_TEST);
						glEnable(GL_TEXTURE_2D);
						gluPerspective(30, (double)g_Screen.m_width/g_Screen.m_height, 1, 1000);
						gluLookAt(0,0,250, 0,0,0, 0,1,0);
						//glEnable(GL_LIGHTING);
						//glEnable(GL_LIGHT0);
						//glEnable(GL_NORMALIZE);
						//glEnable(GL_COLOR_MATERIAL);
					}
					glDisable(GL_CULL_FACE);
					//glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);

					// Reloads texture and bind
					ResMaster.freeTexPages();
					ResMaster.loadTexPages();
					ResMaster.cacheGridsVertices();

					// Flush vbo id's
					Uint32	index;
					for (index = 0;index < ResMaster.m_pieCount;index++)
					{
						ResMaster.getPieAt(index)->flushVBOPolys();
					}

					// TwWindowSize has been called by TwEventSDL, so it is not necessary to call it again here.
					break;
				}
			}
			else
			{
				// Resets all keys and buttons
				inputInitialize();
				// Input in TwBars rebuilds vbo's
				switch( event.type )
				{
					case SDL_KEYUP:
					case SDL_KEYDOWN:
					case SDL_MOUSEMOTION:
					case SDL_MOUSEBUTTONUP:
					case SDL_MOUSEBUTTONDOWN:
						// Flush vbo id's
						Uint32	index;
						for (index = 0;index < ResMaster.m_pieCount;index++)
						{
							CPieInternal	*temp = ResMaster.getPieAt(index);
							if (temp)
							{
								temp->flushVBOPolys();
							}
						}
						break;
					default:
						break;
				}
			}
		}

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glTranslatef(0.0f,0.0f,0.0f);

		glColor4ub(255, 255, 255, 255);

		ResMaster.logic();
		ResMaster.draw();
		ResMaster.updateInput();

		// Draw tweak bars
		TwDraw();

		// Present frame buffer
		SDL_GL_SwapBuffers();

		SDL_Delay(10);
	}

	// Remove TW bars
	//removeBars();

	// Terminate AntTweakBar
	TwTerminate();

	// Terminate SDL
	SDL_Quit();

	ResMaster.~CResMaster();

	return 1;
}
