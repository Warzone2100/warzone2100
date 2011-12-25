/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2011  Warzone 2100 Project

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
/*! \file input.h
 * \brief Prototypes for the keyboard and mouse input funcitons.
 */
#ifndef _input_h
#define _input_h

/* Check the header files have been included from frame.h if they
 * are used outside of the framework library.
 */
#if !defined(_frame_h) && !defined(FRAME_LIB_INCLUDE)
#error Framework header files MUST be included from Frame.h ONLY.
#endif

#include "types.h"
#include "lib/framework/utf.h"
#include "vector.h"

#ifdef BACKEND_QT
/** Defines for all the key codes used. */
enum KEY_CODE
{
	KEY_ESC         = 27,
	KEY_1           = '1',
	KEY_2           = '2',
	KEY_3           = '3',
	KEY_4           = '4',
	KEY_5           = '5',
	KEY_6           = '6',
	KEY_7           = '7',
	KEY_8           = '8',
	KEY_9           = '9',
	KEY_0           = '0',
	KEY_MINUS       = '-',
	KEY_EQUALS      = '=',
	KEY_BACKSPACE   = '\b',
	KEY_TAB         = '\t',
	KEY_Q           = 'q',
	KEY_W           = 'w',
	KEY_E           = 'e',
	KEY_R           = 'r',
	KEY_T           = 't',
	KEY_Y           = 'y',
	KEY_U           = 'u',
	KEY_I           = 'i',
	KEY_O           = 'o',
	KEY_P           = 'p',
	KEY_LBRACE      = '[',
	KEY_RBRACE      = ']',
	KEY_RETURN      = '\r',  // Comes from SDL, don't know why not '\n'.
	KEY_LCTRL       = 306,
	KEY_A           = 'a',
	KEY_S           = 's',
	KEY_D           = 'd',
	KEY_F           = 'f',
	KEY_G           = 'g',
	KEY_H           = 'h',
	KEY_J           = 'j',
	KEY_K           = 'k',
	KEY_L           = 'l',
	KEY_SEMICOLON   = ';',
	KEY_QUOTE       = '\'',
	KEY_BACKQUOTE   = '`',
	KEY_LSHIFT      = 304,
	KEY_LMETA       = 309,  // TODO Added after Qt branched.
	KEY_LSUPER      = 310,  // TODO Added after Qt branched.
	KEY_BACKSLASH   = '\\',
	KEY_Z           = 'z',
	KEY_X           = 'x',
	KEY_C           = 'c',
	KEY_V           = 'v',
	KEY_B           = 'b',
	KEY_N           = 'n',
	KEY_M           = 'm',
	KEY_COMMA       = ',',
	KEY_FULLSTOP    = '.',
	KEY_FORWARDSLASH= '/',
	KEY_RSHIFT      = 303,
	KEY_RMETA       = 311,  // TODO Added after Qt branched.
	KEY_RSUPER      = 312,  // TODO Added after Qt branched.
	KEY_KP_STAR     = 268,
	KEY_LALT        = 308,
	KEY_SPACE       = ' ',
	KEY_CAPSLOCK    = 301,
	KEY_F1          = 282,
	KEY_F2          = 283,
	KEY_F3          = 284,
	KEY_F4          = 285,
	KEY_F5          = 286,
	KEY_F6          = 287,
	KEY_F7          = 288,
	KEY_F8          = 289,
	KEY_F9          = 290,
	KEY_F10         = 291,
	KEY_NUMLOCK     = 300,
	KEY_SCROLLLOCK  = 302,
	KEY_KP_7        = 263,
	KEY_KP_8        = 264,
	KEY_KP_9        = 265,
	KEY_KP_MINUS    = 269,
	KEY_KP_4        = 260,
	KEY_KP_5        = 261,
	KEY_KP_6        = 262,
	KEY_KP_PLUS     = 270,
	KEY_KP_1        = 257,
	KEY_KP_2        = 258,
	KEY_KP_3        = 259,
	KEY_KP_0        = 256,
	KEY_KP_FULLSTOP = 266,
	KEY_F11         = 292,
	KEY_F12         = 293,
	KEY_RCTRL       = 305,
	KEY_KP_BACKSLASH= 267,  ///< Most keypads just have a forward slash.
	KEY_RALT        = 307,
	KEY_HOME        = 278,
	KEY_UPARROW     = 273,
	KEY_PAGEUP      = 280,
	KEY_LEFTARROW   = 276,
	KEY_RIGHTARROW  = 275,
	KEY_END         = 279,
	KEY_DOWNARROW   = 274,
	KEY_PAGEDOWN    = 281,
	KEY_INSERT      = 277,
	KEY_DELETE      = 127,
	KEY_KPENTER     = 271,

	KEY_MAXSCAN     = 323,  ///< The largest possible scan code.

	KEY_IGNORE      = 5190
};
#else // BACKEND_QT
#include <SDL.h>
enum KEY_CODE
{
	KEY_ESC			=SDLK_ESCAPE,
	KEY_1			=SDLK_1,
	KEY_2			=SDLK_2,
	KEY_3			=SDLK_3,
	KEY_4			=SDLK_4,
	KEY_5			=SDLK_5,
	KEY_6			=SDLK_6,
	KEY_7			=SDLK_7,
	KEY_8			=SDLK_8,
	KEY_9			=SDLK_9,
	KEY_0			=SDLK_0,
	KEY_MINUS		=SDLK_MINUS,
	KEY_EQUALS		=SDLK_EQUALS,
	KEY_BACKSPACE		=SDLK_BACKSPACE,
	KEY_TAB			=SDLK_TAB,
	KEY_Q			=SDLK_q,
	KEY_W			=SDLK_w,
	KEY_E			=SDLK_e,
	KEY_R			=SDLK_r,
	KEY_T			=SDLK_t,
	KEY_Y			=SDLK_y,
	KEY_U			=SDLK_u,
	KEY_I			=SDLK_i,
	KEY_O			=SDLK_o,
	KEY_P			=SDLK_p,
	KEY_LBRACE		=SDLK_LEFTBRACKET,
	KEY_RBRACE		=SDLK_RIGHTBRACKET,
	KEY_RETURN		=SDLK_RETURN,
	KEY_LCTRL		=SDLK_LCTRL,
	KEY_A			=SDLK_a,
	KEY_S			=SDLK_s,
	KEY_D			=SDLK_d,
	KEY_F			=SDLK_f,
	KEY_G			=SDLK_g,
	KEY_H			=SDLK_h,
	KEY_J			=SDLK_j,
	KEY_K			=SDLK_k,
	KEY_L			=SDLK_l,
	KEY_SEMICOLON		=SDLK_SEMICOLON,
	KEY_QUOTE		=SDLK_QUOTE,
	KEY_BACKQUOTE		=SDLK_BACKQUOTE,
	KEY_LSHIFT		=SDLK_LSHIFT,
	KEY_LMETA		=SDLK_LMETA,
	KEY_LSUPER		=SDLK_LSUPER,
	KEY_BACKSLASH		=SDLK_BACKSLASH,
	KEY_Z			=SDLK_z,
	KEY_X			=SDLK_x,
	KEY_C			=SDLK_c,
	KEY_V			=SDLK_v,
	KEY_B			=SDLK_b,
	KEY_N			=SDLK_n,
	KEY_M			=SDLK_m,
	KEY_COMMA		=SDLK_COMMA,
	KEY_FULLSTOP		=SDLK_PERIOD,
	KEY_FORWARDSLASH	=SDLK_SLASH,
	KEY_RSHIFT		=SDLK_RSHIFT,
	KEY_RMETA		=SDLK_RMETA,
	KEY_RSUPER		=SDLK_RSUPER,
	KEY_KP_STAR		=SDLK_KP_MULTIPLY,
	KEY_LALT		=SDLK_LALT,
	KEY_SPACE		=SDLK_SPACE,
	KEY_CAPSLOCK		=SDLK_CAPSLOCK,
	KEY_F1			=SDLK_F1,
	KEY_F2			=SDLK_F2,
	KEY_F3			=SDLK_F3,
	KEY_F4			=SDLK_F4,
	KEY_F5			=SDLK_F5,
	KEY_F6			=SDLK_F6,
	KEY_F7			=SDLK_F7,
	KEY_F8			=SDLK_F8,
	KEY_F9			=SDLK_F9,
	KEY_F10			=SDLK_F10,
	KEY_NUMLOCK		=SDLK_NUMLOCK,
	KEY_SCROLLLOCK		=SDLK_SCROLLOCK,
	KEY_KP_7		=SDLK_KP7,
	KEY_KP_8		=SDLK_KP8,
	KEY_KP_9		=SDLK_KP9,
	KEY_KP_MINUS		=SDLK_KP_MINUS,
	KEY_KP_4		=SDLK_KP4,
	KEY_KP_5		=SDLK_KP5,
	KEY_KP_6		=SDLK_KP6,
	KEY_KP_PLUS		=SDLK_KP_PLUS,
	KEY_KP_1		=SDLK_KP1,
	KEY_KP_2		=SDLK_KP2,
	KEY_KP_3		=SDLK_KP3,
	KEY_KP_0		=SDLK_KP0,
	KEY_KP_FULLSTOP		=SDLK_KP_PERIOD,
	KEY_F11			=SDLK_F11,
	KEY_F12			=SDLK_F12,
	KEY_RCTRL		=SDLK_RCTRL,
	KEY_KP_BACKSLASH	=SDLK_KP_DIVIDE,
	KEY_RALT		=SDLK_RALT,
	KEY_HOME		=SDLK_HOME,
	KEY_UPARROW		=SDLK_UP,
	KEY_PAGEUP		=SDLK_PAGEUP,
	KEY_LEFTARROW		=SDLK_LEFT,
	KEY_RIGHTARROW		=SDLK_RIGHT,
	KEY_END			=SDLK_END,
	KEY_DOWNARROW		=SDLK_DOWN,
	KEY_PAGEDOWN		=SDLK_PAGEDOWN,
	KEY_INSERT		=SDLK_INSERT,
	KEY_DELETE		=SDLK_DELETE,
	KEY_KPENTER		=SDLK_KP_ENTER,
	KEY_IGNORE		=5190
};
/** The largest possible scan code. */
#define KEY_MAXSCAN SDLK_LAST
#endif // BACKEND_QT
enum MOUSE_KEY_CODE
{
	MOUSE_LMB = 1,
	MOUSE_MMB,
	MOUSE_RMB,
	MOUSE_WUP,
	MOUSE_WDN,

	MOUSE_BAD
};

/** Tell the input system that we have lost the focus. */
extern void inputLoseFocus(void);

/** Converts the key code into an ascii string. */
extern void keyScanToString(KEY_CODE code, char *ascii, UDWORD maxStringSize);

/** Initialise the input module. */
extern void inputInitialise(void);

/** This returns true if the key is currently depressed. */
extern bool keyDown(KEY_CODE code);

/** This returns true if the key went from being up to being down this frame. */
extern bool keyPressed(KEY_CODE code);

/** This returns true if the key went from being down to being up this frame. */
extern bool keyReleased(KEY_CODE code);

/** Return the current X position of the mouse. */
extern uint16_t mouseX(void) WZ_DECL_PURE;

/** Return the current Y position of the mouse. */
extern uint16_t mouseY(void) WZ_DECL_PURE;

/// Return the position of the mouse where it was clicked last.
Vector2i mousePressPos(MOUSE_KEY_CODE code) WZ_DECL_PURE;
/// Return the position of the mouse where it was released last.
Vector2i mouseReleasePos(MOUSE_KEY_CODE code) WZ_DECL_PURE;

/** This returns true if the mouse key is currently depressed. */
extern bool mouseDown(MOUSE_KEY_CODE code);

/** This returns true if the mouse key was double clicked. */
extern bool mouseDClicked(MOUSE_KEY_CODE code);

/** This returns true if the mouse key went from being up to being down this frame. */
extern bool mousePressed(MOUSE_KEY_CODE code);

/** This returns true if the mouse key went from being down to being up this frame. */
extern bool mouseReleased(MOUSE_KEY_CODE code);

/** Check for a mouse drag, return the drag start coords if dragging. */
extern bool mouseDrag(MOUSE_KEY_CODE code, UDWORD *px, UDWORD *py);

/** Warps the mouse to the given position. */
extern void setMousePos(uint16_t x, uint16_t y);

extern void setMouseWarp(bool value);
extern bool getMouseWarp();

/* The input buffer can contain normal character codes and these control codes */
#define INPBUF_LEFT		KEY_LEFTARROW
#define INPBUF_RIGHT	KEY_RIGHTARROW
#define INPBUF_UP		KEY_UPARROW
#define INPBUF_DOWN		KEY_DOWNARROW
#define INPBUF_HOME		KEY_HOME
#define INPBUF_END		KEY_END
#define INPBUF_INS		KEY_INSERT
#define INPBUF_DEL		KEY_DELETE
#define INPBUF_PGUP		KEY_PAGEUP
#define INPBUF_PGDN		KEY_PAGEDOWN

/* Some defines for keys that map into the normal character space */
#define INPBUF_BKSPACE	KEY_BACKSPACE
#define INPBUF_TAB		KEY_TAB
#define INPBUF_CR		KEY_RETURN
#define INPBUF_ESC		KEY_ESC

/** Return the next key press or 0 if no key in the buffer.
 * The key returned will have been remapped to the correct ascii code for the
 * US layout (approximately) key map.
 * All key presses are buffered up (including auto repeat).
 * @param unicode is filled (unless NULL) with the unicode character corresponding
 * to the key press (using the user's native layout).
 */
extern UDWORD inputGetKey(utf_32_char *unicode);

/** Clear the input buffer. */
extern void inputClearBuffer(void);

/* This is called once a frame so that the system can tell
 * whether a key was pressed this turn or held down from the last frame.
 */
extern void inputNewFrame(void);

static inline bool specialOrderKeyDown()
{
	return keyDown(KEY_LALT) || keyDown(KEY_RALT) || keyDown(KEY_LMETA) || keyDown(KEY_RMETA);
}

#endif // _input_h
