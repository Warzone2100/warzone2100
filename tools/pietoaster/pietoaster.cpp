#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "SDL.h"
#include "AntTweakBar.h"

#ifdef _WIN32
	#include <windows.h>	// required by gl.h
#endif
#include <GL/gl.h>
#include <GL/glu.h>

#include "pie_types.h"
#include "imdloader.h"
#include "resmaster.h"
#include "physfs.h"

#include "screen.h"
#include "game_io.h"

static char mychars[255]; //temp char
static int mychars_index = 0; //temp char current index
static TwBar *textBar; //text box bar
static BOOL textboxUp = FALSE; //whether textbox is up or not

static char *text_pointer = NULL;

// the dumbest shift kmod char handling function on planet earth :)
static Uint16 shiftChar(Uint16 key)
{
	switch (key)
	{
	case '`':
		key = '~';
		break;
	case '1':
		key = '!';
		break;
	case '2':
		key = '@';
		break;
	case '3':
		key = '!';
		break;
	case '4':
		key = '$';
		break;
	case '5':
		key = '%';
		break;
	case '6':
		key = '^';
		break;
	case '7':
		key = '&';
		break;
	case '8':
		key = '*';
		break;
	case '9':
		key = '(';
		break;
	case '0':
		key = ')';
		break;
	case '-':
		key = '_';
		break;
	case '[':
		key = '{';
		break;
	case ']':
		key = '}';
		break;
	case ';':
		key = ';';
		break;
	case '\'':
		key = '\"';
		break;
	case ',':
		key = '<';
		break;
	case '.':
		key = '>';
		break;
	case '/':
		key = '?';
		break;
	default:
		break;
	}
	return key;
}

static void mychars_incr(Uint16 key)
{
	if (mychars_index < 255 - 1)
	{
		char str = (Uint8)key;

		strncpy(&mychars[mychars_index], &str, 1);
		mychars_index++;
	}
}

static void mychars_decr()
{
	if (mychars_index > 0)
	{
		mychars[mychars_index] = '\0';
		mychars_index--;
	}
}

static void TW_CALL addTextBox(void *clientData)
{
	char test[255] = " label='";
	//char test2[255] = "TextBox label='";
	char *string = (char*)clientData;
	int size = strlen(string);

	if (textboxUp)
	{
		return;
	}

	// Sets the pointer
	text_pointer = string;

	textBar = TwNewBar("TextBox");
	//_snprintf(&test2[15], size, "%s", string);
	//_snprintf(&test2[15 + size], 1, "\'");

	//TwDefine(&test2);
	TwDefine("TextBox position = '200 400'");
	TwDefine("TextBox size = '400 50'");

	_snprintf(&test[8], size, "%s", string);
	_snprintf(&test[8 + size], 1, "\'");
	//strcat(&test[0], attr);

	textboxUp = TRUE;

	strncpy(&mychars[0], string, size);
	mychars_index = size;

	TwAddVarRO(textBar, "textbox", TW_TYPE_INT32, &mychars_index, &test[0]);
}

static void deleteTextBox()
{
	/*copies chars from mychars to text_pointer location if both
	values are not null */
	if (text_pointer && mychars_index)
	{
		int size = mychars_index;
		strncpy(text_pointer, mychars, size);
		text_pointer = NULL;
	}

	TwDeleteBar(textBar);
	textboxUp = FALSE;
}

static void updateTextBox(void)
{
	char test[255] = " label='";

	TwRemoveVar(textBar, "textbox");

	_snprintf(&test[8], mychars_index, "%s", &mychars[0]);
	_snprintf(&test[8 + mychars_index], 1, "\'");

	TwAddVarRO(textBar, "textbox", TW_TYPE_INT32, &mychars_index, test);
}

int main(int argc, char *argv[])
{
	const SDL_VideoInfo* video = NULL;
	bool bQuit = false;
	char *path = "./";


	PHYSFS_setWriteDir(path);
	if (!PHYSFS_addToSearchPath( PHYSFS_getWriteDir(), 0 ))
	{
		fprintf(stderr, "unable to add path %s to physfs %s\n", PHYSFS_getLastError());
	}

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
	g_Screen.m_flags = SDL_OPENGL | SDL_HWSURFACE | SDL_RESIZABLE;
	g_Screen.m_width = 800;
	g_Screen.m_height = 600;


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

	g_pieVertexType = TwDefineStruct("VerticePie", vertice3fMembers, 4, sizeof(_vertice_list), NULL, NULL);

	TwStructMember vector2fMembers[] =
	{
		{ "X",	   TW_TYPE_FLOAT,	offsetof(Vector2f, x), "min=0 max=4096 step=1" },
		{ "Y",	   TW_TYPE_FLOAT,	offsetof(Vector2f, y), "min=0 max=4096 step=1" },
	};

	g_pieVector2fType = TwDefineStruct("Vector2fPie", vector2fMembers, 2, sizeof(Vector2f), NULL, NULL);

	// Tell the window size to AntTweakBar
	TwWindowSize(g_Screen.m_width, g_Screen.m_height);

	iIMDShape *testIMD = NULL;

	ResMaster = new CResMaster();

	ResMaster->readTextureList("pages.txt");

	ResMaster->loadTexPages();

	ResMaster->addGUI();

	//argc = 2;
	//argv[1] = "drmbod08.pie";

	if (argc < 2)
	{
		fprintf(stderr, "no file specified\n");
		return 0;
	}

	testIMD = iV_ProcessIMD(argv[1]);
	if (testIMD == NULL)
	{
		fprintf(stderr, "file %s doesn't exist\n", argv[1]);
		return 0;
	}
	ResMaster->addPie(testIMD, argv[1]);

	ResMaster->getPies()->ToFile("test13.pie");


	while( !bQuit )
	{
		SDL_Event event;
		int handled;

		//glClearColor(0.6f, 0.95f, 1.0f, 1);
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
			if (textboxUp)
			{
				if (event.type == SDL_KEYDOWN)
				{
					Uint16 key = event.key.keysym.sym;
					SDLMod modifier = event.key.keysym.mod;

					if (key == SDLK_BACKSPACE)
					{
						mychars_decr();
					}
					else if (key == SDLK_KP_ENTER)
					{
						//finish and save changes to csv data
						deleteTextBox();
						continue;
					}
					else if (key == SDLK_PAUSE ||
							key == SDLK_ESCAPE ||
							key == SDLK_SPACE)
					{
						//cancels current action and closes text box
						text_pointer = NULL;
						deleteTextBox();
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
						mychars_incr(key);
					}
					updateTextBox();
					//interrupts input when text box is up
					continue;
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
					bQuit = TRUE;
					break;
				case SDL_VIDEORESIZE:	// Window size has changed
					// Resize SDL video mode
 					g_Screen.m_width = event.resize.w;
					g_Screen.m_height = event.resize.h;
					if( !g_Screen.setVideoMode() )
						fprintf(stderr, "WARNING: Video mode set failed: %s", SDL_GetError());

					// Resize OpenGL viewport
					glViewport(0, 0, g_Screen.m_width, g_Screen.m_height);
					if (ResMaster->isTextureMapperUp())
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
					glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);

					// Reloads texture and bind
					ResMaster->freeTexPages();
					ResMaster->loadTexPages();
					// TwWindowSize has been called by TwEventSDL, so it is not necessary to call it again here.
					break;
				}
			}
		}

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glTranslatef(0.0f,0.0f,0.0f);

		glColor4ub(255, 255, 255, 255);

		ResMaster->draw();
		ResMaster->updateInput();

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

	delete ResMaster;

	return 1;
}
