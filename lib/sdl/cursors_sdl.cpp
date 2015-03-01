/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/** @file
 *  \brief Cursor definitions (32x32)
 */

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/tex.h"
#include "src/warzoneconfig.h"
#include "src/frontend.h"
#include "cursors_sdl.h"
#include <SDL2/SDL.h>

static CURSOR currentCursor = CURSOR_MAX;
static SDL_Cursor *aCursors[CURSOR_MAX];
static bool monoCursor;

/* TODO: do bridge and attach need swapping? */
static const char *cursor_arrow[] = {
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "X                               ",
  "XX                              ",
  "X.X                             ",
  "X..X                            ",
  "X...X                           ",
  "X....X                          ",
  "X.....X                         ",
  "X......X                        ",
  "X.......X                       ",
  "X........X                      ",
  "X.....XXXXX                     ",
  "X..X..X                         ",
  "X.X X..X                        ",
  "XX  X..X                        ",
  "X    X..X                       ",
  "     X..X                       ",
  "      X..X                      ",
  "      X..X                      ",
  "       XX                       ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "0,0"
};

static const char *cursor_dest[] = {
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "           ..                   ",
  "           ...X                 ",
  "           ..X.X                ",
  "           ..X .X               ",
  "           .X.X .X              ",
  "           .X.X  .X             ",
  "           .X .X  .X            ",
  "           .X .X   .......      ",
  "           .X  .X  .XX....X     ",
  "           .X  .X .XXX....X     ",
  "           .X   ..XXXX....X     ",
  "          ...X  ..XXXX....X     ",
  "         .....X ..........X     ",
  "         .....X ..........X     ",
  "         .....X .XX.XX.XX..     ",
  "          ...X .X..X..X..X..    ",
  "           XX  .X..X..X..XX.X   ",
  "               X...........XX   ",
  "                XXXXXXXXXXXX    ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "15,18"
};

static const char *cursor_sight[] = {
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                                ",
  "                                ",
  "            .......             ",
  "   ..     ...........     ..    ",
  "   ....  X..XXXXXXX..X  ....X   ",
  "    .....XXXX      XXX.....XX   ",
  "    .......X        .......X    ",
  "     ........     ........XX    ",
  "     .......XX     .......X     ",
  "    XX.....XX       .....XXX    ",
  "   ..X....XX         ....X..X   ",
  "   ..XX..XX           ..XX..X   ",
  "  ...X .XX             .X ...X  ",
  "  ..XX  X               X  ..X  ",
  "  ..X                      ..X  ",
  "  ..X                      ..X  ",
  "  ..X                      ..X  ",
  "  ..X                      ..X  ",
  "  ...  .               .X ...X  ",
  "   ..X ..             ..X ..XX  ",
  "   ..X....           ....X..X   ",
  "    XX.....         .....XXXX   ",
  "     .......       .......XX    ",
  "     ........     ........XX    ",
  "    .......XXX    XX.......X    ",
  "    .....XXX        XX.....X    ",
  "   ....XXX...     ...XXX....    ",
  "   ..XXX X...........XXX X..X   ",
  "    XX    XX.......XXX     XX   ",
  "            XXXXXXXX            ",
  "                                ",
  "                                ",
  "15,16"
};

static const char *cursor_target[] = {
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                                ",
  "                                ",
  "                                ",
  "               .                ",
  "               .X               ",
  "            ....X..             ",
  "          ......X....           ",
  "         ...XXX.XXX...          ",
  "        ..XX   .X   X..         ",
  "       ..X     .X     ..        ",
  "      ..X      XX      ..       ",
  "      ..X              ..X      ",
  "     ..X                ..X     ",
  "     ..X                ..X     ",
  "     ..X                ..X     ",
  "   ........X   .    ........X   ",
  "    XXXXXXXX   X     XXXXXXXX   ",
  "     ..X                ..X     ",
  "     ..X                ..X     ",
  "     ..X                ..X     ",
  "      ..                .X      ",
  "      ..X      .       ..X      ",
  "       ..      .X     ..X       ",
  "        ..     .X    ..X        ",
  "         ...   .X   ..X         ",
  "         X......X....X          ",
  "          XX....X..XX           ",
  "            XXX.XXX             ",
  "               .X               ",
  "               XX               ",
  "                                ",
  "                                ",
  "15,15"
};

static const char *cursor_larrow[] = {
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                ..              ",
  "              ...X              ",
  "            ....X               ",
  "          ...............       ",
  "        .................X      ",
  "      ...................X      ",
  "    .....................X      ",
  "    XX...................X      ",
  "      XX.................X      ",
  "        XX...............X      ",
  "          XX....XXXXXXXXXX      ",
  "            XX...X              ",
  "              XX..X             ",
  "                XXX             ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "7,15"
};

static const char *cursor_rarrow[] = {
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "              ..                ",
  "               ...              ",
  "                ....            ",
  "       ...............          ",
  "       .................        ",
  "       ...................      ",
  "       .....................X   ",
  "       ...................XX    ",
  "       .................XX      ",
  "       ...............XX        ",
  "        XXXXXXXX....XX          ",
  "              X...XX            ",
  "              ..XX              ",
  "              XX                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "25,16"
};

static const char *cursor_darrow[] = {
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "            .......             ",
  "            .......X            ",
  "            .......X            ",
  "            .......X            ",
  "            .......X            ",
  "            .......X            ",
  "            .......X XX         ",
  "         .X .......XX.X         ",
  "         ..X.......X..X         ",
  "          ...........X          ",
  "          ...........X          ",
  "           .........X           ",
  "           .........X           ",
  "            .......X            ",
  "            .......X            ",
  "             .....X             ",
  "             .....X             ",
  "              ...X              ",
  "              ...X              ",
  "               .X               ",
  "               .X               ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "15,24"
};

static const char *cursor_uarrow[] = {
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "               .                ",
  "               .X               ",
  "              ...X              ",
  "              ...X              ",
  "             .....X             ",
  "             .....X             ",
  "            .......X            ",
  "            .......X            ",
  "           .........X           ",
  "           .........X           ",
  "          ...........X          ",
  "          ...........X          ",
  "         ..X.......X..X         ",
  "         .X .......XX.X         ",
  "            .......X XX         ",
  "            .......X            ",
  "            .......X            ",
  "            .......X            ",
  "            .......X            ",
  "            .......X            ",
  "            .......X            ",
  "             XXXXXXX            ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "15,8"
};

static const char *cursor_default[] = {
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "            ..XX                ",
  "            ....XX              ",
  "             .....XX            ",
  "             .......XX          ",
  "              ........XX        ",
  "              ..........XX      ",
  "               ...........X     ",
  "               ....XXXXXXXX     ",
  "                ...X            ",
  "                ...X            ",
  "                 ..X            ",
  "                 ..X            ",
  "                  .X            ",
  "                  .X            ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "12,12"
};

static const char *cursor_attach[] = {
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "     ...X  ....X  ....X  ...    ",
  "     XXX.X.XXXX.X.XXXX.X.XXX    ",
  "       .....  .....  .....      ",
  "        .X.X   .X.X   .X.X      ",
  "     ...X  ....X  ....X  ...    ",
  "     XXX   XXXX   XXXX   XXX    ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "17,14"
};

static const char *cursor_attack[] = {
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                                ",
  "                                ",
  "                                ",
  "               .                ",
  "               .X               ",
  "            ....X..             ",
  "          ......X....           ",
  "         ...XXX.XXX...          ",
  "        ..XX   .X   X..         ",
  "       ..X     .X     ..        ",
  "      ..X      XX      ..       ",
  "      ..X              ..X      ",
  "     ..X                ..X     ",
  "     ..X                ..X     ",
  "     ..X                ..X     ",
  "   ........X   .    ........X   ",
  "    XXXXXXXX   X     XXXXXXXX   ",
  "     ..X                ..X     ",
  "     ..X                ..X     ",
  "     ..X                ..X     ",
  "      ..                .X      ",
  "      ..X      .       ..X      ",
  "       ..      .X     ..X       ",
  "        ..     .X    ..X        ",
  "         ...   .X   ..X         ",
  "         X......X....X          ",
  "          XX....X..XX           ",
  "            XXX.XXX             ",
  "               .X               ",
  "               XX               ",
  "                                ",
  "                                ",
  "15,15"
};

static const char *cursor_bomb[] = {
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                                ",
  "                                ",
  "                                ",
  "                   . .          ",
  "                  . . .         ",
  "                  XXX.          ",
  "               XXXX . .         ",
  "               XX  . .          ",
  "               XX               ",
  "              ....X             ",
  "              ....X             ",
  "            .XXXXXX.XX          ",
  "          ............X         ",
  "         ..............X        ",
  "        ................X       ",
  "       ..................X      ",
  "       ..................X      ",
  "      ....................X     ",
  "      ....................X     ",
  "      ....................X     ",
  "      ....................X     ",
  "      ....................X     ",
  "       ..................X      ",
  "       ..................X      ",
  "        ................X       ",
  "         ..............X        ",
  "          ............X         ",
  "            ........XX          ",
  "            XXXXXXXX            ",
  "                                ",
  "                                ",
  "                                ",
  "16,16"
};

static const char *cursor_bridge[] = {
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                                ",
  "                                ",
  "           ..                   ",
  "         ....                   ",
  "       .......                  ",
  "     .........                  ",
  "   ............                 ",
  "   ............             ..  ",
  "   X............          .. .  ",
  "   X............        ..XX.   ",
  "    X............     ..XXX.    ",
  "    X............   ..XXXX.     ",
  "     X..............XXXXX.      ",
  "   ..X............XXXXXX.       ",
  "  ....X..........XXXXX..        ",
  "  ....X.........XXXX......      ",
  "  .....X.......XXX.........     ",
  "  .....X......XX............    ",
  "  ......X....XXXXX..........X   ",
  "  ......X...XX..XXXX........X   ",
  " ........X. ..XXX    .......X   ",
  ".........X..XXX      .......X.. ",
  " XX......X XX        ..........X",
  "   XXX...X           .........XX",
  "      XXXX           .........X ",
  "                     ........XX ",
  "                   ..........X  ",
  "                    XX......XX  ",
  "                      XXX...X   ",
  "                         XXXX   ",
  "                                ",
  "                                ",
  "16,16"
};

static const char *cursor_build[] = {
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                ......          ",
  "              .....XX..X        ",
  "           .X.....X  XXX        ",
  "          .X......X             ",
  "          X.......X             ",
  "          .......XX             ",
  "         .......X               ",
  "         ......X.X              ",
  "       .......X...X             ",
  "      .......X ....X            ",
  "       ....XX   ....X           ",
  "        ...X     ....X          ",
  "         .X       ....X         ",
  "                   ....X        ",
  "                    ....X       ",
  "                     ...X       ",
  "                      ..X       ",
  "                       X        ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "15,15"
};

static const char *cursor_embark[] = {
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "           ...........X         ",
  "           ...........X         ",
  "           ...........X         ",
  "           ...........X         ",
  "        .................X      ",
  "         ...............XX      ",
  "          .............X        ",
  "           ...........X         ",
  "           X.........X          ",
  "        ...XX.......XX...X      ",
  "         ...XX.....XX...X       ",
  "          ...XX...XX...X        ",
  "           ...XX.XX...X         ",
  "            ...XXX...X          ",
  "             ...X...X           ",
  "              .....X            ",
  "               ...X             ",
  "                .X              ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "16,22"
};

static const char *cursor_disembark[] = {
	/* width height num_colors chars_per_pixel */
	"    32    32        3            1",
	/* colors */
	"X c #000000",
	". c #ffffff",
	"  c None",
	/* pixels */
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"           ...........X         ",
	"           ...........X         ",
	"           ...........X         ",
	"           ...........X         ",
	"        .................X      ",
	"         ...............XX      ",
	"          .............X        ",
	"           ...........X         ",
	"           X.........X          ",
	"        ...XX.......XX...X      ",
	"         ...XX.....XX...X       ",
	"          ...XX...XX...X        ",
	"           ...XX.XX...X         ",
	"            ...XXX...X          ",
	"             ...X...X           ",
	"              .....X            ",
	"               ...X             ",
	"                .X              ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"16,22"
};

static const char *cursor_fix[] = {
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "           ...                  ",
  "             ..X                ",
  "              ..X               ",
  "              ..X               ",
  "       .      ..X               ",
  "       .X     ..X               ",
  "       .X    X..X               ",
  "       ..XXXX...X               ",
  "        .........X              ",
  "         .........X             ",
  "              .....X            ",
  "               .....XXXXX       ",
  "                .........X      ",
  "                 .........X     ",
  "                  ...X   ..X    ",
  "                  ..X     .X    ",
  "                  ..X     .X    ",
  "                  ..X           ",
  "                  ..X           ",
  "                   ..XXX        ",
  "                    ...X        ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "16,16"
};

static const char *cursor_guard[] = {
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "       .....X      .....        ",
  "       .XXX.........XXX.X       ",
  "       .X..XXXXXXXXX..X.X       ",
  "       .X.............X.X       ",
  "       .X.............X.X       ",
  "       .X.............X.X       ",
  "       .X............X.X        ",
  "        .X...........X.X        ",
  "        .X...........X.X        ",
  "        .XX.........XX.X        ",
  "         .X.........X.X         ",
  "         .XX.......XX.X         ",
  "          .XX.....XX.X          ",
  "           .XX...XX.X           ",
  "            ..XXX..X            ",
  "             .....X             ",
  "               .XX              ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "16,17"
};

static const char *cursor_jam[] = {
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "             .....X             ",
  "           ...XXX...X           ",
  "          ..XX   XX..X          ",
  "         ..X       X..X         ",
  "         .X   ...X  X.X         ",
  "        ..X  .X  .X  ..X        ",
  "        .X  .X    .X  .X        ",
  "        .X  .X .X .X  .X        ",
  "        .X  .XX.XX.X  .X        ",
  "        ..X  X...XX  ..X        ",
  "         .X   ...X   .X         ",
  "         ..X .....X ..X         ",
  "          .XX.....XX.X          ",
  "           X.......XX           ",
  "            .......X            ",
  "           .........X           ",
  "           .........X           ",
  "            XXXXXXXXX           ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "15,15"
};

static const char *cursor_lockon[] = {
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "             .....X             ",
  "           ...XXX...X           ",
  "          ..XX   XX..X          ",
  "         ..X       X..X         ",
  "         .X   ...X  X.X         ",
  "        ..X  .X  .X  ..X        ",
  "        .X  .X    .X  .X        ",
  "        .X  .X .X .X  .X        ",
  "        .X  .XX.XX.X  .X        ",
  "        ..X  X...XX  ..X        ",
  "         .X   ...X   .X         ",
  "         ..X .....X ..X         ",
  "          .XX.....XX.X          ",
  "           X.......XX           ",
  "            .......X            ",
  "           .........X           ",
  "           .........X           ",
  "            XXXXXXXXX           ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "15,15"
};

static const char *cursor_scout[] = {
	/* width height num_colors chars_per_pixel */
	"    32    32        3            1",
	/* colors */
	"X c #000000",
	". c #ffffff",
	"  c None",
	/* pixels */
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"             .....X             ",
	"           ...XXX...X           ",
	"          ..XX   XX..X          ",
	"         ..X       X..X         ",
	"         .X   ...X  X.X         ",
	"        ..X  .X  .X  ..X        ",
	"        .X  .X    .X  .X        ",
	"        .X  .X .X .X  .X        ",
	"        .X  .XX.XX.X  .X        ",
	"        ..X  X...XX  ..X        ",
	"         .X   ...X   .X         ",
	"         ..X .....X ..X         ",
	"          .XX.....XX.X          ",
	"           X.......XX           ",
	"            .......X            ",
	"           .........X           ",
	"           .........X           ",
	"            XXXXXXXXX           ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"                                ",
	"15,15"
};

static const char *cursor_menu[] = {
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "          ..XX                  ",
  "          ....XX                ",
  "           .....XX              ",
  "           .......XX            ",
  "            ........XX          ",
  "            ..........XX        ",
  "             ........XX         ",
  "             .....XX            ",
  "              ...XX             ",
  "              ...XX             ",
  "               ..XX             ",
  "               . X              ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "11,11"
};

static const char *cursor_move[] = {
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "           .  .....X .          ",
  "          ..X.XXXXX.X..         ",
  "         ...XXX    XX...        ",
  "        ....X.X    .X....       ",
  "       .....X.X    .X.....      ",
  "        XXXXX.X    .XXXXX       ",
  "        .X....X    ....X.X      ",
  "       .XXXXXXX          .X     ",
  "       .X                .X     ",
  "       .X                .X     ",
  "       .X                .X     ",
  "       .X                .X     ",
  "        .X....     ....X.X      ",
  "        XXXXX.X    .XXXXX       ",
  "       .....X.X    .X.....      ",
  "        ....X.X    .X....X      ",
  "         ...XXX    XX...X       ",
  "          ..X.X    .X..X        ",
  "           .X .....X .X         ",
  "              XXXXX             ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "15,15"
};

static const char *cursor_notpossible[] = {
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "        .X            .X        ",
  "       ...X          ...X       ",
  "      .....X        .....X      ",
  "     .......X      .......X     ",
  "      .......X    .......X      ",
  "       .......X  .......X       ",
  "        .......X.......X        ",
  "         .............X         ",
  "          ...........X          ",
  "           .........X           ",
  "            .......X            ",
  "           .........X           ",
  "          ...........X          ",
  "         .............X         ",
  "        .......X.......X        ",
  "       .......X  .......X       ",
  "      .......X    .......X      ",
  "     .......X      .......X     ",
  "      .....X        .....X      ",
  "       ...X          ...X       ",
  "        .X            .X        ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "15,17"
};

static const char *cursor_pickup[] = {
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                                ",
  "                                ",
  "                                ",
  "             .X  .X             ",
  "             .X  .X             ",
  "             .X.X.X             ",
  "              ...X              ",
  "               .X               ",
  "              ...X              ",
  "             .X.X.X             ",
  "             .X  .X             ",
  "             .X.X.X             ",
  "              ...X              ",
  "              X.XX              ",
  "             .X.X.X             ",
  "            ..X.X..X            ",
  "           ...X.X...X           ",
  "          ....X.X....X          ",
  "         .....XXX.....X         ",
  "         ...XXXXXXX...X         ",
  "         .XXX      XX.X         ",
  "         .X          .X         ",
  "         .X          .X         ",
  "         .X          .X         ",
  "         .X          .X         ",
  "         ..X        ..X         ",
  "          ..X      ..X          ",
  "           ..X    ..X           ",
  "            .X    .X            ",
  "                                ",
  "                                ",
  "                                ",
  "15,20"
};

static const char *cursor_seekrepair[] = {
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "      ...                       ",
  "        ..X             X       ",
  "         ..X            .X      ",
  "         ..X            ..X     ",
  "  .      ..X        XXXX...X    ",
  "  .X     ..X        ........X   ",
  "  .X    X..X        .........X  ",
  "  ..XXXX...X        ..........  ",
  "   .........X       .........   ",
  "    .........X      ........    ",
  "         .....X         ...     ",
  "          .....XXXXX    ..      ",
  "           .........X   .       ",
  "            .........X          ",
  "             ...X   ..X         ",
  "             ..X     .X         ",
  "             ..X     .X         ",
  "             ..X                ",
  "             ..X                ",
  "              ..XXX             ",
  "               ...X             ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "16,15"
};

static const char *cursor_select[] = {
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "    .....X            .....X    ",
  "    .XXXXX             XXX.X    ",
  "    .X                    .X    ",
  "    .X                    .X    ",
  "    .X                    .X    ",
  "    XX                    XX    ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "               .X               ",
  "               XX               ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "    .                     .     ",
  "    .X                    .X    ",
  "    .X                    .X    ",
  "    .X                    .X    ",
  "    .....X            .....X    ",
  "    XXXXXX            XXXXXX    ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "15,15"
};

static const struct
{
	const char** image;
	CURSOR cursor_num;
} cursors[CURSOR_MAX] =
{
	{ cursor_arrow,         CURSOR_ARROW },
	{ cursor_dest,          CURSOR_DEST },
	{ cursor_sight,         CURSOR_SIGHT },
	{ cursor_target,        CURSOR_TARGET },
	{ cursor_larrow,        CURSOR_LARROW },
	{ cursor_rarrow,        CURSOR_RARROW },
	{ cursor_darrow,        CURSOR_DARROW },
	{ cursor_uarrow,        CURSOR_UARROW },
	{ cursor_default,       CURSOR_DEFAULT },
	{ cursor_default,       CURSOR_EDGEOFMAP },
	{ cursor_attach,        CURSOR_ATTACH },
	{ cursor_attack,        CURSOR_ATTACK },
	{ cursor_bomb,          CURSOR_BOMB },
	{ cursor_bridge,        CURSOR_BRIDGE },
	{ cursor_build,         CURSOR_BUILD },
	{ cursor_embark,        CURSOR_EMBARK },
	{ cursor_disembark,     CURSOR_DISEMBARK },
	{ cursor_fix,           CURSOR_FIX },
	{ cursor_guard,         CURSOR_GUARD },
	{ cursor_jam,           CURSOR_JAM },
	{ cursor_lockon,        CURSOR_LOCKON },
	{ cursor_scout,         CURSOR_SCOUT },
	{ cursor_menu,          CURSOR_MENU },
	{ cursor_move,          CURSOR_MOVE },
	{ cursor_notpossible,   CURSOR_NOTPOSSIBLE },
	{ cursor_pickup,        CURSOR_PICKUP },
	{ cursor_seekrepair,    CURSOR_SEEKREPAIR },
	{ cursor_select,        CURSOR_SELECT },
};

/**
	init_system_ColorCursor()-- Create a colored mouse cursor image
 */
SDL_Cursor* init_system_ColorCursor(CURSOR cur, const char *fileName)
{

	iV_Image *psSprite = (iV_Image *)malloc(sizeof(iV_Image));
	if (!psSprite)
	{
		debug(LOG_FATAL, "Could not allocate memory for cursor sprite. Exiting.");
		exit(-1);
	}

	if (!iV_loadImage_PNG(fileName, psSprite))
	{
		debug(LOG_FATAL, "Could not load cursor sprite. Exiting.");
		exit(-1);
	}

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	uint32_t rmask = 0xff000000;
	uint32_t gmask = 0x00ff0000;
	uint32_t bmask = 0x0000ff00;
	uint32_t amask = 0x000000ff;
#else
	uint32_t rmask = 0x000000ff;
	uint32_t gmask = 0x0000ff00;
	uint32_t bmask = 0x00ff0000;
	uint32_t amask = 0xff000000;
#endif

	SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(psSprite->bmp, psSprite->width, psSprite->height, psSprite->depth *8, psSprite->width * 4,rmask,gmask,bmask,amask);
	SDL_Cursor *pointer = SDL_CreateColorCursor(surface, psSprite->width/2, psSprite->height/2);	// We center the hotspot for all (FIXME ?)
	if (!pointer)
	{
		debug(LOG_FATAL, "Could not create cursor because %s", SDL_GetError());
		exit(-1);
	}

	// free up image & surface data
	free(psSprite->bmp);
	SDL_FreeSurface(surface);

	return pointer;
}

/**
	init_system_cursor32()-- Create a monochrome mouse cursor image
 */
SDL_Cursor* init_system_cursor32(CURSOR cur)
{
	int i, row, col;
	uint8_t data[4 * 32];
	uint8_t mask[4 * 32];
	int hot_x, hot_y;
	const char** image;
	ASSERT(cur < CURSOR_MAX, "Attempting to load non-existent cursor: %u", (unsigned int)cur);
	ASSERT(cursors[cur].cursor_num == cur, "Bad cursor mapping");
	image = cursors[cur].image;

	i = -1;
	for (row = 0; row < 32; ++row)
	{
		for (col = 0; col < 32; ++col)
		{
			if (col % 8)
			{
				data[i] <<= 1;
				mask[i] <<= 1;
			}
			else
			{
				++i;
				data[i] = mask[i] = 0;
			}
			switch (image[4 + row][col])
			{
				case 'X':
					data[i] |= 0x01;
					mask[i] |= 0x01;
					break;

				case '.':
					mask[i] |= 0x01;
					break;

				case ' ':
					break;
			}
		}
	}

	sscanf(image[4 + row], "%d,%d", &hot_x, &hot_y);
	return SDL_CreateCursor(data, mask, 32, 32, hot_x, hot_y);
}

/**
	wzSetCursor()-- Set the current cursor
 */
void wzSetCursor(CURSOR cur)
{
	ASSERT(cur < CURSOR_MAX, "Specified cursor(%d) is over our limit of (%d)!", (int)cur,(int)CURSOR_MAX );
	// we reset mouse cursors on the fly...(only in the mouse options screen!)
	if ((~(war_GetColouredCursor() ^ monoCursor)) && (titleMode == MOUSE_OPTIONS))
	{
		sdlFreeCursors();
		war_GetColouredCursor() ? sdlInitColoredCursors() : sdlInitCursors();
		SDL_SetCursor(aCursors[cur]);
	}
	// If we are already using this cursor then  return
	if (cur != currentCursor)
        {
			SDL_SetCursor(aCursors[cur]);
			currentCursor = cur;
        }
}

/**
  sdlInitColoredCursors() --Setup the mouse cursor with bitmaps
*/
void sdlInitColoredCursors()
{
	monoCursor = false;
	aCursors[CURSOR_ARROW]       = init_system_cursor32(CURSOR_ARROW);
	aCursors[CURSOR_DEST]        = init_system_ColorCursor(CURSOR_DEST, "images/intfac/image_cursor_dest.png");
	aCursors[CURSOR_SIGHT]       = init_system_cursor32(CURSOR_SIGHT);
	aCursors[CURSOR_TARGET]      = init_system_cursor32(CURSOR_TARGET);
	aCursors[CURSOR_LARROW]      = init_system_ColorCursor(CURSOR_LARROW, "images/intfac/image_cursor_larrow.png");
	aCursors[CURSOR_RARROW]      = init_system_ColorCursor(CURSOR_RARROW, "images/intfac/image_cursor_rarrow.png");
	aCursors[CURSOR_DARROW]      = init_system_ColorCursor(CURSOR_DARROW, "images/intfac/image_cursor_darrow.png");
	aCursors[CURSOR_UARROW]      = init_system_ColorCursor(CURSOR_UARROW, "images/intfac/image_cursor_uarrow.png");
	aCursors[CURSOR_DEFAULT]     = init_system_ColorCursor(CURSOR_DEFAULT, "images/intfac/image_cursor_default.png");
	aCursors[CURSOR_EDGEOFMAP]   = init_system_cursor32(CURSOR_EDGEOFMAP);
	aCursors[CURSOR_ATTACH]      = init_system_ColorCursor(CURSOR_ATTACH, "images/intfac/image_cursor_attach.png");
	aCursors[CURSOR_ATTACK]      = init_system_ColorCursor(CURSOR_ATTACK, "images/intfac/image_cursor_attack.png");
	aCursors[CURSOR_BOMB]        = init_system_ColorCursor(CURSOR_BOMB, "images/intfac/image_cursor_bomb.png");
	aCursors[CURSOR_BRIDGE]      = init_system_ColorCursor(CURSOR_BRIDGE, "images/intfac/image_cursor_bridge.png");
	aCursors[CURSOR_BUILD]       = init_system_ColorCursor(CURSOR_BUILD, "images/intfac/image_cursor_build.png");
	aCursors[CURSOR_EMBARK]      = init_system_ColorCursor(CURSOR_EMBARK, "images/intfac/image_cursor_embark.png");
	aCursors[CURSOR_DISEMBARK]   = init_system_ColorCursor(CURSOR_DISEMBARK, "images/intfac/image_cursor_disembark.png");
	aCursors[CURSOR_FIX]         = init_system_ColorCursor(CURSOR_FIX, "images/intfac/image_cursor_fix.png");
	aCursors[CURSOR_GUARD]       = init_system_ColorCursor(CURSOR_GUARD, "images/intfac/image_cursor_guard.png");
	aCursors[CURSOR_JAM]         = init_system_ColorCursor(CURSOR_JAM, "images/intfac/image_cursor_ecm.png");
	aCursors[CURSOR_LOCKON]      = init_system_ColorCursor(CURSOR_LOCKON, "images/intfac/image_cursor_lockon.png");
	aCursors[CURSOR_SCOUT]       = init_system_ColorCursor(CURSOR_SCOUT, "images/intfac/image_cursor_scout.png");
	aCursors[CURSOR_MENU]        = init_system_cursor32(CURSOR_MENU);
	aCursors[CURSOR_MOVE]        = init_system_ColorCursor(CURSOR_MOVE, "images/intfac/image_cursor_move.png");
	aCursors[CURSOR_NOTPOSSIBLE] = init_system_ColorCursor(CURSOR_NOTPOSSIBLE, "images/intfac/image_cursor_notpos.png");
	aCursors[CURSOR_PICKUP]      = init_system_ColorCursor(CURSOR_PICKUP, "images/intfac/image_cursor_pickup.png");
	aCursors[CURSOR_SEEKREPAIR]  = init_system_ColorCursor(CURSOR_SEEKREPAIR, "images/intfac/image_cursor_repair.png");
	aCursors[CURSOR_SELECT]      = init_system_ColorCursor(CURSOR_SELECT, "images/intfac/image_cursor_select.png");
}

/**
  sdlInitCursors() --Setup the mouse cursor with monochrome data
*/
void sdlInitCursors()
{
	monoCursor = true;
	aCursors[CURSOR_ARROW]       = init_system_cursor32(CURSOR_ARROW);
	aCursors[CURSOR_DEST]        = init_system_cursor32(CURSOR_DEST);
	aCursors[CURSOR_SIGHT]       = init_system_cursor32(CURSOR_SIGHT);
	aCursors[CURSOR_TARGET]      = init_system_cursor32(CURSOR_TARGET);
	aCursors[CURSOR_LARROW]      = init_system_cursor32(CURSOR_LARROW);
	aCursors[CURSOR_RARROW]      = init_system_cursor32(CURSOR_RARROW);
	aCursors[CURSOR_DARROW]      = init_system_cursor32(CURSOR_DARROW);
	aCursors[CURSOR_UARROW]      = init_system_cursor32(CURSOR_UARROW);
	aCursors[CURSOR_DEFAULT]     = init_system_cursor32(CURSOR_DEFAULT);
	aCursors[CURSOR_EDGEOFMAP]   = init_system_cursor32(CURSOR_EDGEOFMAP);
	aCursors[CURSOR_ATTACH]      = init_system_cursor32(CURSOR_ATTACH);
	aCursors[CURSOR_ATTACK]      = init_system_cursor32(CURSOR_ATTACK);
	aCursors[CURSOR_BOMB]        = init_system_cursor32(CURSOR_BOMB);
	aCursors[CURSOR_BRIDGE]      = init_system_cursor32(CURSOR_BRIDGE);
	aCursors[CURSOR_BUILD]       = init_system_cursor32(CURSOR_BUILD);
	aCursors[CURSOR_EMBARK]      = init_system_cursor32(CURSOR_EMBARK);
	aCursors[CURSOR_DISEMBARK]   = init_system_cursor32(CURSOR_DISEMBARK);
	aCursors[CURSOR_FIX]         = init_system_cursor32(CURSOR_FIX);
	aCursors[CURSOR_GUARD]       = init_system_cursor32(CURSOR_GUARD);
	aCursors[CURSOR_JAM]         = init_system_cursor32(CURSOR_JAM);
	aCursors[CURSOR_LOCKON]      = init_system_cursor32(CURSOR_LOCKON);
	aCursors[CURSOR_SCOUT]       = init_system_cursor32(CURSOR_SCOUT);
	aCursors[CURSOR_MENU]        = init_system_cursor32(CURSOR_MENU);
	aCursors[CURSOR_MOVE]        = init_system_cursor32(CURSOR_MOVE);
	aCursors[CURSOR_NOTPOSSIBLE] = init_system_cursor32(CURSOR_NOTPOSSIBLE);
	aCursors[CURSOR_PICKUP]      = init_system_cursor32(CURSOR_PICKUP);
	aCursors[CURSOR_SEEKREPAIR]  = init_system_cursor32(CURSOR_SEEKREPAIR);
	aCursors[CURSOR_SELECT]      = init_system_cursor32(CURSOR_SELECT);
}

void sdlFreeCursors()
{
	unsigned int i;
	for(i = 0 ; i < ARRAY_SIZE(aCursors); ++i)
	{
		SDL_FreeCursor(aCursors[i]);
	}
}
