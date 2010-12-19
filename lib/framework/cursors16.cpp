/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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
 *  \brief Cursor definition (16x16)
 */

#include "frame.h"
#include "cursors.h"

/* TODO: do bridge and attach need swapping? */
static const char *cursor_arrow[] = {
  /* width height num_colors chars_per_pixel */
  "    16    16        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "X               ",
  ".X              ",
  "..X             ",
  "...X            ",
  "....X           ",
  ".X.             ",
  "X .X            ",
  "  X.            ",
  "   .X           ",
  "                ",
  "                ",
  "                ",
  "                ",
  "                ",
  "                ",
  "                ",
  "0,0"
};

static const char *cursor_dest[] = {
  /* width height num_colors chars_per_pixel */
  "    16    16        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                ",
  "                ",
  "                ",
  "                ",
  "     .          ",
  "     .XX        ",
  "     .. X       ",
  "     . X X      ",
  "     . . .X..   ",
  "     .  .XX..   ",
  "    ... .....   ",
  "    ... X.XX.   ",
  "     X ..X..X.  ",
  "        XXXXXX  ",
  "                ",
  "                ",
  "7,9"
};

static const char *cursor_sight[] = {
  /* width height num_colors chars_per_pixel */
  "    16    16        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                ",
  " .   .....   .  ",
  "  ..XX   XX..X  ",
  "  ....   ....X  ",
  "  X..X    ..XX  ",
  " .X.X      .X.  ",
  " .X          .X ",
  " .           .X ",
  " .           .X ",
  " .X.       . .X ",
  "  X..     ..XX  ",
  "  ....   ....X  ",
  "  ..XX    X..X  ",
  " .XXX.....XXX.  ",
  "      XXXX      ",
  "                ",
  "7,8"
};

static const char *cursor_target[] = {
  /* width height num_colors chars_per_pixel */
  "    16    16        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                ",
  "       .        ",
  "      ...       ",
  "    ..X.X..     ",
  "   .X  .   .    ",
  "   .       .X   ",
  "  .X        .   ",
  " ....X .  ....  ",
  "  .X        .   ",
  "  .X        .   ",
  "   .   .   .X   ",
  "    .  .  .X    ",
  "    X.....X     ",
  "      X.X       ",
  "       X        ",
  "                ",
  "7,7"
};

static const char *cursor_larrow[] = {
  /* width height num_colors chars_per_pixel */
  "    16    16        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                ",
  "                ",
  "                ",
  "                ",
  "        .       ",
  "      ..        ",
  "    ........X   ",
  "  ..........X   ",
  "   X........X   ",
  "     X..XXXXX   ",
  "       X.       ",
  "                ",
  "                ",
  "                ",
  "                ",
  "                ",
  "3,7"
};

static const char *cursor_rarrow[] = {
  /* width height num_colors chars_per_pixel */
  "    16    16        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                ",
  "                ",
  "                ",
  "                ",
  "                ",
  "       ..       ",
  "   ........     ",
  "   ..........   ",
  "   ..........X  ",
  "   ........X    ",
  "       ..X      ",
  "       X        ",
  "                ",
  "                ",
  "                ",
  "                ",
  "12,8"
};

static const char *cursor_darrow[] = {
  /* width height num_colors chars_per_pixel */
  "    16    16        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                ",
  "                ",
  "                ",
  "      ...       ",
  "      ...X      ",
  "      ...X      ",
  "      ...XX     ",
  "    .X...X.     ",
  "     .....X     ",
  "     .....      ",
  "      ...X      ",
  "      ...       ",
  "       .X       ",
  "       .        ",
  "                ",
  "                ",
  "7,12"
};

static const char *cursor_uarrow[] = {
  /* width height num_colors chars_per_pixel */
  "    16    16        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                ",
  "                ",
  "       .        ",
  "       .X       ",
  "      ...       ",
  "      ...X      ",
  "     .....      ",
  "     .....X     ",
  "    . ...X.     ",
  "      ...X      ",
  "      ...X      ",
  "      ...X      ",
  "      XXXX      ",
  "                ",
  "                ",
  "                ",
  "7,4"
};

static const char *cursor_default[] = {
  /* width height num_colors chars_per_pixel */
  "    16    16        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                ",
  "                ",
  "                ",
  "                ",
  "                ",
  "      X         ",
  "      ..X       ",
  "      ....X     ",
  "       .....X   ",
  "       ..XXXX   ",
  "        .X      ",
  "        .X      ",
  "         X      ",
  "                ",
  "                ",
  "                ",
  "5,6"
};

static const char *cursor_attach[] = {
  /* width height num_colors chars_per_pixel */
  "    16    16        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                ",
  "                ",
  "                ",
  "                ",
  "                ",
  "                ",
  "  .. ..X .. ..  ",
  "   ... .. ...   ",
  "  .. ..X .. ..  ",
  "                ",
  "                ",
  "                ",
  "                ",
  "                ",
  "                ",
  "                ",
  "8,7"
};

static const char *cursor_attack[] = {
  /* width height num_colors chars_per_pixel */
  "    16    16        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                ",
  "       .        ",
  "      ...       ",
  "    ..X.X..     ",
  "   .X  .   .    ",
  "   .       .X   ",
  "  .X        .   ",
  " ....X .  ....  ",
  "  .X        .   ",
  "  .X        .   ",
  "   .   .   .X   ",
  "    .  .  .X    ",
  "    X.....X     ",
  "      X.X       ",
  "       X        ",
  "                ",
  "7,7"
};

static const char *cursor_bomb[] = {
  /* width height num_colors chars_per_pixel */
  "    16    16        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                ",
  "         ..     ",
  "         X.     ",
  "       X ..     ",
  "       ..       ",
  "      XXX.X     ",
  "    .......X    ",
  "   .........X   ",
  "   ..........   ",
  "   ..........   ",
  "   ..........   ",
  "   .........X   ",
  "    .......X    ",
  "      ....X     ",
  "                ",
  "                ",
  "8,8"
};

static const char *cursor_bridge[] = {
  /* width height num_colors chars_per_pixel */
  "    16    16        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                ",
  "    ..          ",
  "  .....         ",
  " ......       . ",
  " X......    .X  ",
  "  ......  .XX   ",
  " .X......XXX    ",
  " .......XX...   ",
  " ..X...X......  ",
  " .....X.XX....  ",
  "....X.X   ..... ",
  " XX.X     .....X",
  "          ....X ",
  "          X...X ",
  "            XX  ",
  "                ",
  "8,8"
};

static const char *cursor_build[] = {
  /* width height num_colors chars_per_pixel */
  "    16    16        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                ",
  "                ",
  "                ",
  "                ",
  "       ..X.X    ",
  "     X...       ",
  "     ...X       ",
  "    ...XX       ",
  "   ...X..X      ",
  "    .X  ..X     ",
  "         ..X    ",
  "          ..    ",
  "           X    ",
  "                ",
  "                ",
  "                ",
  "7,7"
};

static const char *cursor_embark[] = {
  /* width height num_colors chars_per_pixel */
  "    16    16        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                ",
  "                ",
  "     ......     ",
  "     ......     ",
  "    ........X   ",
  "     ......X    ",
  "     X....X     ",
  "    ..X..X..    ",
  "     ..XX..     ",
  "      ....      ",
  "       ..       ",
  "                ",
  "                ",
  "                ",
  "                ",
  "                ",
  "8,11"
};

static const char *cursor_disembark[] = {
	/* width height num_colors chars_per_pixel */
	"    16    16        3            1",
	/* colors */
	"X c #000000",
	". c #ffffff",
	"  c None",
	/* pixels */
	"                ",
	"                ",
	"     ......     ",
	"     ......     ",
	"    ........X   ",
	"     ......X    ",
	"     X....X     ",
	"    ..X..X..    ",
	"     ..XX..     ",
	"      ....      ",
	"       ..       ",
	"                ",
	"                ",
	"                ",
	"                ",
	"                ",
	"8,11"
};

static const char *cursor_fix[] = {
  /* width height num_colors chars_per_pixel */
  "    16    16        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                ",
  "                ",
  "                ",
  "      .X        ",
  "       .        ",
  "   .   .        ",
  "   .XX..        ",
  "    .....       ",
  "       ...XX    ",
  "        .....   ",
  "         .   X  ",
  "         .      ",
  "         .XX    ",
  "                ",
  "                ",
  "                ",
  "8,8"
};

static const char *cursor_guard[] = {
  /* width height num_colors chars_per_pixel */
  "    16    16        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                ",
  "                ",
  "                ",
  "                ",
  "   ...   ...    ",
  "   ..XXXXX..    ",
  "   .........    ",
  "   .......XX    ",
  "    X.....XX    ",
  "    .......     ",
  "     X...XX     ",
  "      .X.X      ",
  "       .X       ",
  "                ",
  "                ",
  "                ",
  "8,8"
};

static const char *cursor_jam[] = {
  /* width height num_colors chars_per_pixel */
  "    16    16        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                ",
  "                ",
  "                ",
  "     ..X..      ",
  "    .X   X.     ",
  "    . . . .X    ",
  "    X X. X X    ",
  "    . X.X .X    ",
  "    .X... .     ",
  "     X...X      ",
  "     .....      ",
  "      XXXX      ",
  "                ",
  "                ",
  "                ",
  "                ",
  "7,7"
};

static const char *cursor_lockon[] = {
  /* width height num_colors chars_per_pixel */
  "    16    16        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                ",
  "                ",
  "                ",
  "     ..X..      ",
  "    .X   X.     ",
  "    . . . .X    ",
  "    X X. X X    ",
  "    . X.X .X    ",
  "    .X... .     ",
  "     X...X      ",
  "     .....      ",
  "      XXXX      ",
  "                ",
  "                ",
  "                ",
  "                ",
  "7,7"
};

static const char *cursor_scout[] = {
	/* width height num_colors chars_per_pixel */
	"    16    16        3            1",
	/* colors */
	"X c #000000",
	". c #ffffff",
	"  c None",
	/* pixels */
	"                ",
	"                ",
	"                ",
	"     ..X..      ",
	"    .X   X.     ",
	"    . . . .X    ",
	"    X X. X X    ",
	"    . X.X .X    ",
	"    .X... .     ",
	"     X...X      ",
	"     .....      ",
	"      XXXX      ",
	"                ",
	"                ",
	"                ",
	"                ",
	"7,7"
};

static const char *cursor_menu[] = {
  /* width height num_colors chars_per_pixel */
  "    16    16        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                ",
  "                ",
  "                ",
  "                ",
  "                ",
  "     .X         ",
  "     ...X       ",
  "      ....X     ",
  "      ....X     ",
  "       .X       ",
  "       .X       ",
  "                ",
  "                ",
  "                ",
  "                ",
  "                ",
  "5,5"
};

static const char *cursor_move[] = {
  /* width height num_colors chars_per_pixel */
  "    16    16        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                ",
  "                ",
  "                ",
  "     . ..X.     ",
  "    ..X  X..    ",
  "   ....  ....   ",
  "    X..  ..XX   ",
  "   .        .   ",
  "   .        .   ",
  "    X..  ..XX   ",
  "   ....  ....   ",
  "    ..X  X..    ",
  "     . ..X.     ",
  "                ",
  "                ",
  "                ",
  "7,7"
};

static const char *cursor_notpossible[] = {
  /* width height num_colors chars_per_pixel */
  "    16    16        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                ",
  "                ",
  "                ",
  "    X      X    ",
  "   ..X    ..X   ",
  "   ...X  ...X   ",
  "    ...X...X    ",
  "     .....X     ",
  "      ...X      ",
  "     .....X     ",
  "    ...X...X    ",
  "   ...X  ...X   ",
  "   ..X    ..X   ",
  "    X      X    ",
  "                ",
  "                ",
  "7,8"
};

static const char *cursor_pickup[] = {
  /* width height num_colors chars_per_pixel */
  "    16    16        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                ",
  "      . .       ",
  "      ...       ",
  "       .        ",
  "      ...       ",
  "      ...       ",
  "       .X       ",
  "      ...X      ",
  "     .....X     ",
  "    ..XXX..     ",
  "    .     .     ",
  "    .     .     ",
  "    .X    .     ",
  "     .X  .      ",
  "                ",
  "                ",
  "7,10"
};

static const char *cursor_seekrepair[] = {
  /* width height num_colors chars_per_pixel */
  "    16    16        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                ",
  "                ",
  "                ",
  "    .           ",
  "    .X      .   ",
  " X  .X    ....  ",
  " .XX.X    ..... ",
  "  ....X   ....  ",
  "     ..XXX  .   ",
  "      ....X     ",
  "      .X  .     ",
  "      .X        ",
  "       .X       ",
  "                ",
  "                ",
  "                ",
  "8,7"
};

static const char *cursor_select[] = {
  /* width height num_colors chars_per_pixel */
  "    16    16        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                ",
  " ....      .... ",
  " .XXX      XXX. ",
  " .X          X. ",
  " .X          X. ",
  "                ",
  "                ",
  "       .        ",
  "                ",
  "                ",
  "                ",
  " .X          X. ",
  " .X          X. ",
  " .XXX      XXX. ",
  " ....      .... ",
  "                ",
  "7,7"
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
	{ cursor_disembark,     CURSOR_EMBARK },
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

SDL_Cursor* init_system_cursor16(CURSOR cur)
{
	int i, row, col;
	uint8_t data[4 * 16];
	uint8_t mask[4 * 16];
	int hot_x, hot_y;
	const char** image;
	ASSERT(cur < CURSOR_MAX, "Attempting to load non-existent cursor: %u", (unsigned int)cur);
	ASSERT(cursors[cur].cursor_num == cur, "Bad cursor mapping");
	image = cursors[cur].image;

	i = -1;
	for (row = 0; row < 16; ++row)
	{
		for (col = 0; col < 16; ++col)
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
	return SDL_CreateCursor(data, mask, 16, 16, hot_x, hot_y);
}
