/*
 * Input.h
 *
 * Prototypes for the keyboard and mouse input funcitons.
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

/* The defines for all the key codes */
typedef enum _key_code
{
	KEY_ESC         =0x01,
	KEY_1           =0x02,
	KEY_2           =0x03,
	KEY_3           =0x04,
	KEY_4           =0x05,
	KEY_5           =0x06,
	KEY_6           =0x07,
	KEY_7           =0x08,
	KEY_8           =0x09,
	KEY_9           =0x0A,
	KEY_0           =0x0B,
	KEY_MINUS       =0x0C,
	KEY_EQUALS      =0x0D,
	KEY_BACKSPACE   =0x0E,
	KEY_TAB         =0x0F,
	KEY_Q           =0x10,
	KEY_W           =0x11,
	KEY_E           =0x12,
	KEY_R           =0x13,
	KEY_T           =0x14,
	KEY_Y           =0x15,
	KEY_U           =0x16,
	KEY_I           =0x17,
	KEY_O           =0x18,
	KEY_P           =0x19,
	KEY_LBRACE      =0x1A,
	KEY_RBRACE      =0x1B,
	KEY_RETURN      =0x1C,
	KEY_LCTRL       =0x1D,
	KEY_A           =0x1E,
	KEY_S           =0x1F,
	KEY_D           =0x20,
	KEY_F           =0x21,
	KEY_G           =0x22,
	KEY_H           =0x23,
	KEY_J           =0x24,
	KEY_K           =0x25,
	KEY_L           =0x26,
	KEY_SEMICOLON   =0x27,
	KEY_QUOTE       =0x28,
	KEY_BACKQUOTE   =0x29,
	KEY_LSHIFT      =0x2A,
	KEY_BACKSLASH   =0x2B,
	KEY_Z           =0x2C,
	KEY_X           =0x2D,
	KEY_C           =0x2E,
	KEY_V           =0x2F,
	KEY_B           =0x30,
	KEY_N           =0x31,
	KEY_M           =0x32,
	KEY_COMMA       =0x33,
	KEY_FULLSTOP    =0x34,
	KEY_FORWARDSLASH=0x35,
	KEY_RSHIFT      =0x36,
	KEY_KP_STAR     =0x37,
	KEY_LALT        =0x38,
	KEY_SPACE       =0x39,
	KEY_CAPSLOCK    =0x3A,
	KEY_F1          =0x3B,
	KEY_F2          =0x3C,
	KEY_F3          =0x3D,
	KEY_F4          =0x3E,
	KEY_F5          =0x3F,
	KEY_F6          =0x40,
	KEY_F7          =0x41,
	KEY_F8          =0x42,
	KEY_F9          =0x43,
	KEY_F10         =0x44,
	KEY_NUMLOCK     =0x45,
	KEY_SCROLLLOCK  =0x46,
	KEY_KP_7        =0x47,
	KEY_KP_8        =0x48,
	KEY_KP_9        =0x49,
	KEY_KP_MINUS    =0x4A,
	KEY_KP_4        =0x4B,
	KEY_KP_5        =0x4C,
	KEY_KP_6        =0x4D,
	KEY_KP_PLUS     =0x4E,
	KEY_KP_1        =0x4F,
	KEY_KP_2        =0x50,
	KEY_KP_3        =0x51,
	KEY_KP_0        =0x52,
	KEY_KP_FULLSTOP =0x53,
	KEY_F11         =0x57,
	KEY_F12         =0x58,
#ifdef WIN32
	KEY_RCTRL           =0x11D,
	KEY_KP_BACKSLASH    =0x135,
	KEY_RALT            =0x138,
	KEY_HOME            =0x147,
	KEY_UPARROW         =0x148,
	KEY_PAGEUP          =0x149,
	KEY_LEFTARROW       =0x14B,
	KEY_RIGHTARROW      =0x14D,
	KEY_END             =0x14F,
	KEY_DOWNARROW       =0x150,
	KEY_PAGEDOWN        =0x151,
	KEY_INSERT          =0x152,
	KEY_DELETE          =0x153,
	KEY_KPENTER         =0x11C
#else
// Playstation specific key maps so that we don't have to use f.loads of memory for the f.ing key code stuff
	KEY_RCTRL			=0x59,	// smells of wee
	KEY_RALT			=0x5a,	
	KEY_DELETE          =0x5b,

#endif
/*	KEY_PAUSE           =E1 10 45,
	KEY_PRINTSCR        =E0 2A E037*/
 
} KEY_CODE;

/* The largest possible scan code (probably a lot less than this but ...) */
//      but ...    it's not as if it's got to fit into 2meg of mem or anything is it ...
#ifdef WIN32
#define KEY_MAXSCAN  512
#else
#define KEY_MAXSCAN  (0x5b)		// see input.h for the max value
#endif


/* Converts the key code into an ascii string */
extern void keyScanToString(KEY_CODE code, STRING *ascii, UDWORD maxStringSize);

/* Initialise the input module */
extern void inputInitialise(void);

/* Add a key press to the key buffer */
extern void inputAddBuffer(UDWORD code, UDWORD count);

/* This returns true if the key is currently depressed */
extern BOOL keyDown(KEY_CODE code);

/* This returns true if the key went from being up to being down this frame */
extern BOOL keyPressed(KEY_CODE code);

/* This returns true if the key went from being down to being up this frame */
extern BOOL keyReleased(KEY_CODE code);


typedef enum _mouse_key_code
{
	MOUSE_LMB,
	MOUSE_MMB,
	MOUSE_RMB
} MOUSE_KEY_CODE;

/* These two functions return the current position of the mouse */
extern SDWORD mouseX(void);
extern SDWORD mouseY(void);

extern	BOOL	mouseWheelForward( void);
extern	BOOL	mouseWheelBackwards( void);
extern	BOOL	mouseWheelStatic( void);


/* This returns true if the mouse key is currently depressed */
extern BOOL mouseDown(MOUSE_KEY_CODE code);

/* This returns true if the mouse key was double clicked */
extern BOOL mouseDClicked(MOUSE_KEY_CODE code);

/* This returns true if the mouse key went from being up to being down this frame */
extern BOOL mousePressed(MOUSE_KEY_CODE code);

/* This returns true if the mouse key went from being down to being up this frame */
extern BOOL mouseReleased(MOUSE_KEY_CODE code);

/* Check for a mouse drag, return the drag start coords if dragging */
extern BOOL mouseDrag(MOUSE_KEY_CODE code, UDWORD *px, UDWORD *py);

/* The input buffer can contain normal character codes and these control codes */
#define INPBUF_LEFT		0x010000
#define INPBUF_RIGHT	0x020000
#define INPBUF_UP		0x030000
#define INPBUF_DOWN		0x040000
#define INPBUF_HOME		0x050000
#define INPBUF_END		0x060000
#define INPBUF_INS		0x070000
#define INPBUF_DEL		0x080000
#define INPBUF_PGUP		0x090000
#define INPBUF_PGDN		0x0a0000

/* Some defines for keys that map into the normal character space */
#define INPBUF_BKSPACE	0x000008
#define INPBUF_TAB		0x000009
#define INPBUF_CR		0x00000D
#define INPBUF_ESC		0x00001b

/* Return the next key press or 0 if no key in the buffer.
 * The key returned will have been remaped to the correct ascii code for the
 * windows key map.
 * All key presses are buffered up (including windows auto repeat).
 */
extern UDWORD inputGetKey(void);

/* Clear the input buffer */
extern void inputClearBuffer(void);

#endif
