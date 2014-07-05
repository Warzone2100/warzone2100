/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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

/** @file wzapp.cpp
 *  Qt-related functions.
 */

#include <QtWidgets/QImage>
#include <QtWidgets/QBitmap>
#include <QtWidgets/QPainter>
#include <QtWidgets/QMouseEvent>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QClipboard>

// Get platform defines before checking for them.
// Qt headers MUST come before platform specific stuff!
#include "wzapp_qt.h"

#if defined(WZ_CC_MSVC)
#include "wzapp.h.moc"		// this is generated on the pre-build event.
#endif

#include "lib/exceptionhandler/dumpinfo.h"
#include "lib/framework/file.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/pieclip.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/framework/wzapp.h"
#include "src/main.h"
#include "src/configuration.h"
#include "lib/gamelib/gtime.h"
#include <deque>

/* The possible states for keys */
typedef enum _key_state
{
	KEY_UP,
	KEY_PRESSED,
	KEY_DOWN,
	KEY_RELEASED,
	KEY_PRESSRELEASE,	///< When a key goes up and down in a frame
	KEY_DOUBLECLICK,	///< Only used by mouse keys
	KEY_DRAG,		///< Only used by mouse keys
} KEY_STATE;

typedef struct _input_state
{
	KEY_STATE state;	///< Last key/mouse state
	UDWORD lastdown;	///< last key/mouse button down timestamp
	Vector2i pressPos;      ///< Location of last mouse press event.
	Vector2i releasePos;    ///< Location of last mouse release event.
} INPUT_STATE;

/// constant for the interval between 2 singleclicks for doubleclick event in ms
#define DOUBLE_CLICK_INTERVAL 250

/** The current state of the keyboard */
static INPUT_STATE aKeyState[KEY_MAXSCAN];

/** How far the mouse has to move to start a drag */
#define DRAG_THRESHOLD	5

/** Which button is being used for a drag */
static MOUSE_KEY_CODE dragKey;

/** The start of a possible drag by the mouse */
static SDWORD dragX, dragY;

/** The current mouse button state */
static INPUT_STATE aMouseState[MOUSE_BAD];
static MousePresses mousePresses;

/** The input string buffer */
struct InputKey
{
	InputKey(UDWORD key_ = 0, utf_32_char unicode_ = 0) : key(key_), unicode(unicode_) {}

	UDWORD key;
	utf_32_char unicode;
};
static std::deque<InputKey> inputBuffer;

static QColor fontColor;
static uint16_t mouseXPos = 0, mouseYPos = 0;
static bool mouseInWindow = false;
static CURSOR lastCursor = CURSOR_ARROW;
static bool crashing = false;

unsigned screenWidth = 0;   // Declared in screen.h
unsigned screenHeight = 0;  // Declared in screen.h
static void inputAddBuffer(UDWORD key, utf_32_char unicode);
static int WZkeyToQtKey(int code);

void WzMainWindow::loadCursor(CURSOR cursor, char const *name)
{
	cursors[cursor] = new QCursor(QPixmap::fromImage(QImage(name, "PNG")));
}

WzMainWindow::WzMainWindow(QSize resolution, const QGLFormat &format, QWidget *parent) : QtGameWidget(resolution, format, parent)
{
	myself = this;
	notReadyToPaint = true;
	tickCount.start();
	for (int i = 0; i < CURSOR_MAX; cursors[i++] = NULL) ;
	setAutoFillBackground(false);
	setAutoBufferSwap(false);
	setMouseTracking(true);

	// Mac apps typically don't have window icons unless document-based.
#if !defined(WZ_OS_MAC)
	setWindowIcon(QIcon(QPixmap::fromImage(QImage("wz::images/warzone2100.png", "PNG"))));
#endif
	setWindowTitle(PACKAGE_NAME);

	loadCursor(CURSOR_EMBARK, "wz::images/intfac/image_cursor_embark.png");
	loadCursor(CURSOR_DEST, "wz::images/intfac/image_cursor_dest.png");
	loadCursor(CURSOR_DEFAULT, "wz::images/intfac/image_cursor_default.png");
	loadCursor(CURSOR_BUILD, "wz::images/intfac/image_cursor_build.png");
	loadCursor(CURSOR_SCOUT, "wz::images/intfac/image_cursor_scout.png");
	loadCursor(CURSOR_DISEMBARK, "wz::images/intfac/image_cursor_disembark.png");
	loadCursor(CURSOR_ATTACK, "wz::images/intfac/image_cursor_attack.png");
	loadCursor(CURSOR_GUARD, "wz::images/intfac/image_cursor_guard.png");
	loadCursor(CURSOR_FIX, "wz::images/intfac/image_cursor_fix.png");
	loadCursor(CURSOR_SELECT, "wz::images/intfac/image_cursor_select.png");
//	loadCursor(CURSOR_REPAIR, 64, 160, buffer);		// FIX ME: This IS in infac.img, but the define is MIA
	loadCursor(CURSOR_SEEKREPAIR, "wz::images/intfac/image_cursor_repair.png");  // FIX ME: This is NOT in infac.img!
	loadCursor(CURSOR_PICKUP, "wz::images/intfac/image_cursor_pickup.png");
	loadCursor(CURSOR_NOTPOSSIBLE, "wz::images/intfac/image_cursor_notpos.png");
	loadCursor(CURSOR_MOVE, "wz::images/intfac/image_cursor_move.png");
	loadCursor(CURSOR_LOCKON, "wz::images/intfac/image_cursor_lockon.png");
//	loadCursor(CURSOR_ECM, 224, 160, buffer);		// FIX ME: Not defined yet!
	loadCursor(CURSOR_JAM, "wz::images/intfac/image_cursor_ecm.png");  // FIX ME: This is NOT in infac.img, and is using IMAGE CURSOR ECM ?
	loadCursor(CURSOR_ATTACH, "wz::images/intfac/image_cursor_attach.png");
	loadCursor(CURSOR_BRIDGE, "wz::images/intfac/image_cursor_bridge.png");
	loadCursor(CURSOR_BOMB, "wz::images/intfac/image_cursor_bomb.png");

	// Reused (unused) cursors
	cursors[CURSOR_ARROW] = new QCursor(Qt::ArrowCursor);
	cursors[CURSOR_MENU] = new QCursor(Qt::ArrowCursor);
	cursors[CURSOR_BOMB] = new QCursor(Qt::ArrowCursor);
	cursors[CURSOR_EDGEOFMAP] = new QCursor(Qt::ArrowCursor);
	cursors[CURSOR_SIGHT] = new QCursor(Qt::ArrowCursor);
	cursors[CURSOR_TARGET] = new QCursor(Qt::ArrowCursor);
	cursors[CURSOR_UARROW] = new QCursor(Qt::SizeVerCursor);
	cursors[CURSOR_DARROW] = new QCursor(Qt::SizeVerCursor);
	cursors[CURSOR_LARROW] = new QCursor(Qt::SizeHorCursor);
	cursors[CURSOR_RARROW] = new QCursor(Qt::SizeHorCursor);

	// Fonts
	regularFont.setFamily("DejaVu Sans");
	regularFont.setPixelSize(12);
	boldFont.setFamily("DejaVu Sans");
	boldFont.setPixelSize(21);
	boldFont.setWeight(QFont::DemiBold);
	smallFont.setFamily("DejaVu Sans");
	smallFont.setPixelSize(9);

	// Want focusOutEvent messages.
	setFocusPolicy(Qt::StrongFocus);

	// set radix character (again) to the period (".")
	setlocale(LC_NUMERIC, "C");

#if !defined(WZ_OS_MAC)
	// Want áéíóú inputMethodEvent messages.
	setAttribute(Qt::WA_InputMethodEnabled, true);
#else
	// But not on the Mac (no ALT+H on US Extended Keyboards)
	setAttribute(Qt::WA_InputMethodEnabled, false);
#endif
}

WzMainWindow::~WzMainWindow()
{
	for (int i = 0; i < CURSOR_MAX; delete cursors[i++]) ;
}

WzMainWindow *WzMainWindow::instance()
{
	assert(myself != NULL);
	return myself;
}

void WzMainWindow::initializeGL()
{
	QtGameWidget::initializeGL();
}

#if 0
// Re-enable when Qt's font rendering is improved.
void WzMainWindow::drawPixmap(int XPos, int YPos, QPixmap *pix)
{
	QPainter painter(context()->device());
	painter.drawPixmap(XPos, YPos, *pix);
	painter.end();
	glEnable(GL_CULL_FACE);
	rendStatesRendModeHack();  // rendStates.rendMode = REND_ALPHA;
	pie_SetRendMode(REND_OPAQUE);		// beat state machinery into submission
}
#endif

void WzMainWindow::resizeGL(int width, int height)
{
	screenWidth = width;
	screenHeight = height;

	scaledFont.setFamily("DejaVu Sans");
	scaledFont.setPixelSize(12 * height / 480);

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, width, height, 0, 1, -1);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glCullFace(GL_FRONT);
	glEnable(GL_CULL_FACE);
}

void WzMainWindow::paintGL()
{
	if (notReadyToPaint)
	{
		return;
	}
	if (crashing)
	{
		glClear(GL_COLOR_BUFFER_BIT);
		swapBuffers();
		return;
	}

	mainLoop();

	// Tell the input system about the start of another frame
	inputNewFrame();
	update();	// add a new paint event for constant redraws
}

void WzMainWindow::setCursor(CURSOR index)
{
	QWidget::setCursor(*cursors[index]);
}

void WzMainWindow::setCursor(QCursor cursor)
{
	QWidget::setCursor(cursor);
}

WzMainWindow *WzMainWindow::myself = NULL;

void WzMainWindow::setFontType(enum iV_fonts fontID)
{
	switch (fontID)
	{
	case font_regular:
		setFont(regularFont);
		break;
	case font_large:
		setFont(boldFont);
		break;
	case font_small:
		setFont(smallFont);
		break;
	case font_scaled:
		setFont(scaledFont);
	default:
		break;
	}
}

void WzMainWindow::setFontSize(float size)
{
	QFont theFont = font();
	theFont.setPixelSize(size);
	setFont(theFont);
}

void WzMainWindow::mouseMoveEvent(QMouseEvent *event)
{
	mouseXPos = clip(event->x(), 0, width() - 1);
	mouseYPos = clip(event->y(), 0, height() - 1);
	mouseInWindow = mouseXPos == event->x() && mouseYPos == event->y();

	/* now see if a drag has started */
	if ((aMouseState[dragKey].state == KEY_PRESSED ||
	     aMouseState[dragKey].state == KEY_DOWN)
	    && (abs(dragX - mouseXPos) > DRAG_THRESHOLD ||
	        abs(dragY - mouseYPos) > DRAG_THRESHOLD))
	{
		aMouseState[dragKey].state = KEY_DRAG;
	}
}

MOUSE_KEY_CODE WzMainWindow::buttonToIdx(Qt::MouseButton button)
{
	MOUSE_KEY_CODE idx;

	switch (button)
	{
		case Qt::LeftButton:
			idx = MOUSE_LMB;
			debug(LOG_INPUT, "MOUSE_LMB clicked");
			break;
		case Qt::RightButton:
			idx = MOUSE_RMB;
			debug(LOG_INPUT, "MOUSE_RMB clicked");
			break;
		case Qt::MidButton:
			idx = MOUSE_MMB;
			debug(LOG_INPUT, "MOUSE_MMB clicked");
			break;
		case Qt::XButton1:
			idx = MOUSE_MMB;
			debug(LOG_INPUT, "MOUSE_MMB (Xbutton1) clicked");
			break;
		case Qt::XButton2:
			idx = MOUSE_MMB;
			debug(LOG_INPUT, "MOUSE_MMB (Xbutton2) clicked");
			break;
		default:
		case Qt::NoButton:
			idx = MOUSE_BAD;
			debug(LOG_INPUT, "NoButton (strange case ?");
			break;	// strange case
	}

	return idx;
}

// TODO consider using QWidget::mouseDoubleClickEvent() for double-click
void WzMainWindow::mousePressEvent(QMouseEvent *event)
{
	mouseXPos = event->x();
	mouseYPos = event->y();

	Qt::MouseButtons presses = event->buttons();	// full state info for all buttons
	MOUSE_KEY_CODE idx = buttonToIdx(event->button());			// index of button that caused event

	if (idx == MOUSE_BAD)
	{
		debug(LOG_ERROR, "bad mouse idx");	// FIXME remove
		return; // not recognized mouse button
	}

	MousePress mousePress;
	mousePress.action = MousePress::Press;
	mousePress.key = idx;
	mousePress.pos = Vector2i(mouseXPos, mouseYPos);
	mousePresses.push_back(mousePress);

	aMouseState[idx].pressPos.x = mouseXPos;
	aMouseState[idx].pressPos.y = mouseYPos;

	if (aMouseState[idx].state == KEY_UP
		|| aMouseState[idx].state == KEY_RELEASED
		|| aMouseState[idx].state == KEY_PRESSRELEASE)
	{
		if (!presses.testFlag(Qt::MidButton)) //skip doubleclick check for wheel
		{
			// whether double click or not
			if (realTime - aMouseState[idx].lastdown < DOUBLE_CLICK_INTERVAL)
			{
				aMouseState[idx].state = KEY_DOUBLECLICK;
				aMouseState[idx].lastdown = 0;
			}
			else
			{
				aMouseState[idx].state = KEY_PRESSED;
				aMouseState[idx].lastdown = realTime;
			}
		}
		else	//mouse wheel up/down was used, so notify. FIXME.
		{
			aMouseState[idx].state = KEY_PRESSED;
			aMouseState[idx].lastdown = 0;
		}

		if (idx == MOUSE_LMB || idx == MOUSE_RMB || idx == MOUSE_MMB)
		{
			dragKey = idx;
			dragX = mouseX();
			dragY = mouseY();
		}
	}
}

void WzMainWindow::wheelEvent(QWheelEvent *event)
{
	int direction = event->delta();

	if (direction > 0)
	{
		aMouseState[MOUSE_WUP].state = KEY_PRESSED;
		aMouseState[MOUSE_WUP].lastdown = realTime;
	}
	else
	{
		aMouseState[MOUSE_WDN].state = KEY_PRESSED;
		aMouseState[MOUSE_WDN].lastdown = realTime;
	}
}

void WzMainWindow::mouseReleaseEvent(QMouseEvent *event)
{
	mouseXPos = event->x();
	mouseYPos = event->y();

	MOUSE_KEY_CODE idx = buttonToIdx(event->button());

	if (idx == MOUSE_BAD)
	{
		return; // not recognized mouse button
	}

	MousePress mousePress;
	mousePress.action = MousePress::Release;
	mousePress.key = idx;
	mousePress.pos = Vector2i(mouseXPos, mouseYPos);
	mousePresses.push_back(mousePress);

	aMouseState[idx].releasePos.x = mouseXPos;
	aMouseState[idx].releasePos.y = mouseYPos;

	if (aMouseState[idx].state == KEY_PRESSED)
	{
		aMouseState[idx].state = KEY_PRESSRELEASE;
	}
	else if (aMouseState[idx].state == KEY_DOWN
			|| aMouseState[idx].state == KEY_DRAG
			|| aMouseState[idx].state == KEY_DOUBLECLICK)
	{
		aMouseState[idx].state = KEY_RELEASED;
	}
}

static unsigned int setKey(int code, bool pressed)
{
	if (pressed)
	{
		if (aKeyState[code].state == KEY_UP ||
			aKeyState[code].state == KEY_RELEASED ||
			aKeyState[code].state == KEY_PRESSRELEASE)
		{
			// whether double key press or not
			aKeyState[code].state = KEY_PRESSED;
			aKeyState[code].lastdown = 0;
		}
	}
	else
	{
		if (aKeyState[code].state == KEY_PRESSED)
		{
			aKeyState[code].state = KEY_PRESSRELEASE;
		}
		else if (aKeyState[code].state == KEY_DOWN)
		{
			aKeyState[code].state = KEY_RELEASED;
		}
	}
	return code;
}

void WzMainWindow::realHandleKeyEvent(QKeyEvent *event, bool pressed)
{
	unsigned int lastKey;
	bool isKeypad = event->modifiers() & Qt::KeypadModifier;

	switch (event->text().size())
	{
		case 0:
			debug(LOG_INPUT, "Key%s 0x%04X, isKeypad=%d", pressed ? "Down" : "Up  ", event->key(), isKeypad);
		case 1:
			debug(LOG_INPUT, "Key%s 0x%04X, isKeypad=%d, 0x%04X", pressed ? "Down" : "Up  ", event->key(), isKeypad, event->text().unicode()[0].unicode());
			break;
		case 2:
			debug(LOG_INPUT, "Key%s 0x%04X, isKeypad=%d, 0x%04X, 0x%04X", pressed ? "Down" : "Up  ", event->key(), isKeypad, event->text().unicode()[0].unicode(), event->text().unicode()[1].unicode());
			break;
		case 3:
			debug(LOG_INPUT, "Key%s 0x%04X, isKeypad=%d, 0x%04X, 0x%04X, ...", pressed ? "Down" : "Up  ", event->key(), isKeypad, event->text().unicode()[0].unicode(), event->text().unicode()[1].unicode());
			break;
	}

	if (!isKeypad)
	{
		switch (event->key())
		{
			case Qt::Key_Shift              :	lastKey = setKey(KEY_LSHIFT, pressed); break;

			case Qt::Key_Control            :	lastKey = setKey(KEY_LCTRL, pressed); break;

			case Qt::Key_Alt                :	lastKey = setKey(KEY_LALT, pressed); break;
			case Qt::Key_AltGr              :	lastKey = setKey(KEY_RALT, pressed); break;

			case Qt::Key_Meta               :	lastKey = setKey(KEY_LMETA, pressed); break;

			case Qt::Key_Escape             :	lastKey = setKey(KEY_ESC, pressed); break;
			case Qt::Key_Backspace          :	lastKey = setKey(KEY_BACKSPACE, pressed); break;
			case Qt::Key_QuoteLeft          :	lastKey = setKey(KEY_BACKQUOTE, pressed); break;
			case Qt::Key_1                  :	lastKey = setKey(KEY_1, pressed); break;
			case Qt::Key_2                  :	lastKey = setKey(KEY_2, pressed); break;
			case Qt::Key_3                  :	lastKey = setKey(KEY_3, pressed); break;
			case Qt::Key_4                  :	lastKey = setKey(KEY_4, pressed); break;
			case Qt::Key_5                  :	lastKey = setKey(KEY_5, pressed); break;
			case Qt::Key_6                  :	lastKey = setKey(KEY_6, pressed); break;
			case Qt::Key_7                  :	lastKey = setKey(KEY_7, pressed); break;
			case Qt::Key_8                  :	lastKey = setKey(KEY_8, pressed); break;
			case Qt::Key_9                  :	lastKey = setKey(KEY_9, pressed); break;
			case Qt::Key_0                  :	lastKey = setKey(KEY_0, pressed); break;
			case Qt::Key_Minus              :	lastKey = setKey(KEY_MINUS, pressed); break;
			case Qt::Key_Equal              :	lastKey = setKey(KEY_EQUALS, pressed); break;
			case Qt::Key_Backtab:
			case Qt::Key_Tab                :	lastKey = setKey(KEY_TAB, pressed); break;
			case Qt::Key_Q                  :	lastKey = setKey(KEY_Q, pressed); break;
			case Qt::Key_W                  :	lastKey = setKey(KEY_W, pressed); break;
			case Qt::Key_E                  :	lastKey = setKey(KEY_E, pressed); break;
			case Qt::Key_R                  :	lastKey = setKey(KEY_R, pressed); break;
			case Qt::Key_T                  :	lastKey = setKey(KEY_T, pressed); break;
			case Qt::Key_Y                  :	lastKey = setKey(KEY_Y, pressed); break;
			case Qt::Key_U                  :	lastKey = setKey(KEY_U, pressed); break;
			case Qt::Key_I                  :	lastKey = setKey(KEY_I, pressed); break;
			case Qt::Key_O                  :	lastKey = setKey(KEY_O, pressed); break;
			case Qt::Key_P                  :	lastKey = setKey(KEY_P, pressed); break;
			case Qt::Key_BracketLeft        :	lastKey = setKey(KEY_LBRACE, pressed); break;
			case Qt::Key_BracketRight       :	lastKey = setKey(KEY_RBRACE, pressed); break;
			case Qt::Key_Enter              :	lastKey = setKey(KEY_RETURN, pressed); break;
			case Qt::Key_Return             :	lastKey = setKey(KEY_RETURN, pressed); break;
			case Qt::Key_A                  :	lastKey = setKey(KEY_A, pressed); break;
			case Qt::Key_S                  :	lastKey = setKey(KEY_S, pressed); break;
			case Qt::Key_D                  :	lastKey = setKey(KEY_D, pressed); break;
			case Qt::Key_F                  :	lastKey = setKey(KEY_F, pressed); break;
			case Qt::Key_G                  :	lastKey = setKey(KEY_G, pressed); break;
			case Qt::Key_H                  :	lastKey = setKey(KEY_H, pressed); break;
			case Qt::Key_J                  :	lastKey = setKey(KEY_J, pressed); break;
			case Qt::Key_K                  :	lastKey = setKey(KEY_K, pressed); break;
			case Qt::Key_L                  :	lastKey = setKey(KEY_L, pressed); break;
			case Qt::Key_Semicolon          :	lastKey = setKey(KEY_SEMICOLON, pressed); break;
			case Qt::Key_QuoteDbl           :	lastKey = setKey(KEY_QUOTE, pressed); break;		// ?
			case Qt::Key_Apostrophe         :	lastKey = setKey(KEY_BACKQUOTE, pressed); break;	// ?
			case Qt::Key_Backslash          :	lastKey = setKey(KEY_BACKSLASH, pressed); break;
			case Qt::Key_Z                  :	lastKey = setKey(KEY_Z, pressed); break;
			case Qt::Key_X                  :	lastKey = setKey(KEY_X, pressed); break;
			case Qt::Key_C                  :	lastKey = setKey(KEY_C, pressed); break;
			case Qt::Key_V                  :	lastKey = setKey(KEY_V, pressed); break;
			case Qt::Key_B                  :	lastKey = setKey(KEY_B, pressed); break;
			case Qt::Key_N                  :	lastKey = setKey(KEY_N, pressed); break;
			case Qt::Key_M                  :	lastKey = setKey(KEY_M, pressed); break;
			case Qt::Key_Comma              :	lastKey = setKey(KEY_COMMA, pressed); break;
			case Qt::Key_Period             :	lastKey = setKey(KEY_FULLSTOP, pressed); break;
			case Qt::Key_Slash              :	lastKey = setKey(KEY_FORWARDSLASH, pressed); break;
			case Qt::Key_Space              :	lastKey = setKey(KEY_SPACE, pressed); break;
			case Qt::Key_CapsLock           :	lastKey = setKey(KEY_CAPSLOCK, pressed); break;
			case Qt::Key_F1                 :	lastKey = setKey(KEY_F1, pressed); break;
			case Qt::Key_F2                 :	lastKey = setKey(KEY_F2, pressed); break;
			case Qt::Key_F3                 :	lastKey = setKey(KEY_F3, pressed); break;
			case Qt::Key_F4                 :	lastKey = setKey(KEY_F4, pressed); break;
			case Qt::Key_F5                 :	lastKey = setKey(KEY_F5, pressed); break;
			case Qt::Key_F6                 :	lastKey = setKey(KEY_F6, pressed); break;
			case Qt::Key_F7                 :	lastKey = setKey(KEY_F7, pressed); break;
			case Qt::Key_F8                 :	lastKey = setKey(KEY_F8, pressed); break;
			case Qt::Key_F9                 :	lastKey = setKey(KEY_F9, pressed); break;
			case Qt::Key_F10                :	lastKey = setKey(KEY_F10, pressed); break;
			case Qt::Key_NumLock            :	lastKey = setKey(KEY_NUMLOCK, pressed); break;
			case Qt::Key_ScrollLock         :	lastKey = setKey(KEY_SCROLLLOCK, pressed); break;
			case Qt::Key_F11                :	lastKey = setKey(KEY_F11, pressed); break;
			case Qt::Key_F12                :	lastKey = setKey(KEY_F12, pressed); break;
			case Qt::Key_Home               :	lastKey = setKey(KEY_HOME, pressed); break;
			case Qt::Key_Up                 :	lastKey = setKey(KEY_UPARROW, pressed); break;
			case Qt::Key_PageUp             :	lastKey = setKey(KEY_PAGEUP, pressed); break;
			case Qt::Key_Left               :	lastKey = setKey(KEY_LEFTARROW, pressed); break;
			case Qt::Key_Right              :	lastKey = setKey(KEY_RIGHTARROW, pressed); break;
			case Qt::Key_End                :	lastKey = setKey(KEY_END, pressed); break;
			case Qt::Key_Down               :	lastKey = setKey(KEY_DOWNARROW, pressed); break;
			case Qt::Key_PageDown           :	lastKey = setKey(KEY_PAGEDOWN, pressed); break;
			case Qt::Key_Insert             :	lastKey = setKey(KEY_INSERT, pressed); break;
			case Qt::Key_Delete             :	lastKey = setKey(KEY_DELETE, pressed); break;
			default:
				debug(LOG_WARNING, "Ignoring normal key %d.\n", event->key());
				lastKey = 0;
				event->ignore();
				break;
		}
	}
	else  // Is keypad.
	{
		switch (event->key())
		{
			case Qt::Key_Asterisk           :	lastKey = setKey(KEY_KP_STAR, pressed); break;
			case Qt::Key_0                  :	lastKey = setKey(KEY_KP_0, pressed); break;
			case Qt::Key_1                  :	lastKey = setKey(KEY_KP_1, pressed); break;
			case Qt::Key_2                  :	lastKey = setKey(KEY_KP_2, pressed); break;
			case Qt::Key_3                  :	lastKey = setKey(KEY_KP_3, pressed); break;
			case Qt::Key_4                  :	lastKey = setKey(KEY_KP_4, pressed); break;
			case Qt::Key_5                  :	lastKey = setKey(KEY_KP_5, pressed); break;
			case Qt::Key_6                  :	lastKey = setKey(KEY_KP_6, pressed); break;
			case Qt::Key_7                  :	lastKey = setKey(KEY_KP_7, pressed); break;
			case Qt::Key_8                  :	lastKey = setKey(KEY_KP_8, pressed); break;
			case Qt::Key_9                  :	lastKey = setKey(KEY_KP_9, pressed); break;
#ifndef WZ_OS_MAC  // Anything except mac
			case Qt::Key_Home               :	lastKey = setKey(KEY_KP_7, pressed); break;
			case Qt::Key_Up                 :	lastKey = setKey(KEY_KP_8, pressed); break;
			case Qt::Key_PageUp             :	lastKey = setKey(KEY_KP_9, pressed); break;
			case Qt::Key_Left               :	lastKey = setKey(KEY_KP_4, pressed); break;
			case Qt::Key_Clear              :	lastKey = setKey(KEY_KP_5, pressed); break;
			case Qt::Key_Right              :	lastKey = setKey(KEY_KP_6, pressed); break;
			case Qt::Key_End                :	lastKey = setKey(KEY_KP_1, pressed); break;
			case Qt::Key_Down               :	lastKey = setKey(KEY_KP_2, pressed); break;
			case Qt::Key_PageDown           :	lastKey = setKey(KEY_KP_3, pressed); break;
			case Qt::Key_Insert             :	lastKey = setKey(KEY_KP_0, pressed); break;
#else  // Mac-specific, arrow keys are numpad on the mac.
			case Qt::Key_Home               :	lastKey = setKey(KEY_HOME, pressed); break;
			case Qt::Key_Up                 :	lastKey = setKey(KEY_UPARROW, pressed); break;
			case Qt::Key_PageUp             :	lastKey = setKey(KEY_PAGEUP, pressed); break;
			case Qt::Key_Left               :	lastKey = setKey(KEY_LEFTARROW, pressed); break;
			case Qt::Key_Clear              :	lastKey = setKey(KEY_DELETE, pressed); break;
			case Qt::Key_Right              :	lastKey = setKey(KEY_RIGHTARROW, pressed); break;
			case Qt::Key_End                :	lastKey = setKey(KEY_END, pressed); break;
			case Qt::Key_Down               :	lastKey = setKey(KEY_DOWNARROW, pressed); break;
			case Qt::Key_PageDown           :	lastKey = setKey(KEY_PAGEDOWN, pressed); break;
			case Qt::Key_Insert             :	lastKey = setKey(KEY_INSERT, pressed); break;
#endif
			case Qt::Key_Delete             :	lastKey = setKey(KEY_KP_FULLSTOP, pressed); break;
			case Qt::Key_Plus               :	lastKey = setKey(KEY_KP_PLUS, pressed); break;
			case Qt::Key_Minus              :	lastKey = setKey(KEY_KP_MINUS, pressed); break;
			case Qt::Key_Period             :	lastKey = setKey(KEY_KP_FULLSTOP, pressed); break;
			case Qt::Key_Slash              :	lastKey = setKey(KEY_KP_BACKSLASH, pressed); break;  // HACK Shouldn't there be a KEY_KP_SLASH? Most keypads only have a forwardslash key.
			case Qt::Key_Enter              :	lastKey = setKey(KEY_KPENTER, pressed); break;
			case Qt::Key_Return             :	lastKey = setKey(KEY_KPENTER, pressed); break;
			default:
				debug(LOG_WARNING, "Ignoring keypad key %d.\n", event->key());
				lastKey = 0;
				event->ignore();
				break;
		}
	}

	if (pressed)
	{
		inputAddBuffer(lastKey, event->text().unicode()->unicode());
	}

	event->accept();
}

void WzMainWindow::keyReleaseEvent(QKeyEvent *event)
{
	realHandleKeyEvent(event, false);
}

void WzMainWindow::keyPressEvent(QKeyEvent *event)
{
	realHandleKeyEvent(event, true);
}

void WzMainWindow::inputMethodEvent(QInputMethodEvent *event)
{
	// Foward all "committed" characters. Should be more advanced than that, but better than nothing.
	for (int i = 0; i < event->commitString().size(); ++i)
	{
		inputAddBuffer(' ', event->commitString()[i].unicode());
	}
	QWidget::inputMethodEvent(event);
}

void WzMainWindow::focusOutEvent(QFocusEvent *event)
{
	debug(LOG_INPUT, "Main window lost focus.");
	inputLoseFocus();
}

void WzMainWindow::close()
{
	qApp->quit();
}


/************************************/
/***                              ***/
/***          C interface         ***/
/***                              ***/
/************************************/

void wzQuit()
{
	WzMainWindow::instance()->close();
}

void wzScreenFlip()
{
	WzMainWindow::instance()->swapBuffers();
}

int wzGetTicks()
{
	return WzMainWindow::instance()->ticks();
}

/****************************************/
/***     Mouse and keyboard support   ***/
/****************************************/

void wzShowMouse(bool visible)
{
	if (!visible)
	{
		WzMainWindow::instance()->setCursor(QCursor(Qt::BlankCursor));
	}
	else
	{
		WzMainWindow::instance()->setCursor(lastCursor);
	}
}

void wzSetCursor(CURSOR index)
{
	ASSERT(index < CURSOR_MAX, "Attempting to load non-existent cursor: %u", (unsigned int)index);

	if (lastCursor != index)
	{
		WzMainWindow::instance()->setCursor(index);
		lastCursor = index;
	}
}

void wzGrabMouse()
{
	WzMainWindow::instance()->trapMouse();
}

void wzReleaseMouse()
{
	WzMainWindow::instance()->freeMouse();
}

void wzDelay(unsigned int delay)
{
	//SDL_Delay(delay);
}

bool wzActiveWindow()
{
	return WzMainWindow::instance()->underMouse();
}

uint16_t mouseX()
{
	return mouseXPos;
}

uint16_t mouseY()
{
	return mouseYPos;
}

bool wzMouseInWindow()
{
	return mouseInWindow;
}

Vector2i mousePressPos_DEPRECATED(MOUSE_KEY_CODE code)
{
	return aMouseState[code].pressPos;
}

Vector2i mouseReleasePos_DEPRECATED(MOUSE_KEY_CODE code)
{
	return aMouseState[code].releasePos;
}

void setMousePos(uint16_t x, uint16_t y)
{
	if (getMouseWarp())
	{
		WzMainWindow::instance()->cursor().setPos(x, y);
	}
}

/* This returns true if the mouse key is currently depressed */
bool mouseDown(MOUSE_KEY_CODE code)
{
	return (aMouseState[code].state != KEY_UP);
}

/* This returns true if the mouse key was double clicked */
bool mouseDClicked(MOUSE_KEY_CODE code)
{
	return (aMouseState[code].state == KEY_DOUBLECLICK);
}

/* This returns true if the mouse key went from being up to being down this frame */
bool mousePressed(MOUSE_KEY_CODE code)
{
	return ((aMouseState[code].state == KEY_PRESSED) ||
			(aMouseState[code].state == KEY_DOUBLECLICK) ||
			(aMouseState[code].state == KEY_PRESSRELEASE));
}

/* This returns true if the mouse key went from being down to being up this frame */
bool mouseReleased(MOUSE_KEY_CODE code)
{
	return ((aMouseState[code].state == KEY_RELEASED) ||
			(aMouseState[code].state == KEY_DOUBLECLICK) ||
			(aMouseState[code].state == KEY_PRESSRELEASE));
}

/* Check for a mouse drag, return the drag start coords if dragging */
bool mouseDrag(MOUSE_KEY_CODE code, UDWORD *px, UDWORD *py)
{
	if (aMouseState[code].state == KEY_DRAG)
	{
		*px = dragX;
		*py = dragY;
		return true;
	}

	return false;
}

void keyScanToString(KEY_CODE code, char *ascii, UDWORD maxStringSize)
{
	QString ourString;
	QByteArray ba;
	const QKeySequence ks = WZkeyToQtKey(code);			// convert key to something Qt understands

	ourString = ks.toString(QKeySequence::NativeText);	// and convert it to a QtString
	ba = ourString.toLatin1();							// convert that to a byte array
	snprintf(ascii, maxStringSize, "%s", ba.data());	// and use that data as the text for the key

#if defined(WZ_OS_MAC)	// Yet another mac hack, since these are normally shown as symbols, NOT text.
	if ( (code == KEY_LALT) || (code == KEY_RALT) )
		snprintf(ascii, maxStringSize, "Alt +");
	else if ( (code == KEY_LCTRL) || (code == KEY_RCTRL) )	// apparently, ctrl = cmd and cmd = ctrl on macs.
		snprintf(ascii, maxStringSize, "Cmd +");
	else if ( code == KEY_ESC )
		snprintf(ascii, maxStringSize, "Esc");
#endif

}

/* Initialise the input module */
void inputInitialise(void)
{
	unsigned int i;

	for (i = 0; i < KEY_MAXSCAN; i++)
	{
		aKeyState[i].state = KEY_UP;
	}

	for (i = 0; i < MOUSE_BAD; i++)
	{
		aMouseState[i].state = KEY_UP;
	}

	inputBuffer.clear();

	dragX = screenWidth / 2;
	dragY = screenHeight / 2;
	dragKey = MOUSE_LMB;
}

/* add count copies of the characater code to the input buffer */
static void inputAddBuffer(UDWORD key, utf_32_char unicode)
{
	inputBuffer.push_back(InputKey(key, unicode));
}

/* Clear the input buffer */
void inputClearBuffer(void)
{
	inputBuffer.clear();
}

/* Return the next key press or 0 if no key in the buffer.
 * The key returned will have been remaped to the correct ascii code for the
 * windows key map.
 * All key presses are buffered up (including windows auto repeat).
 */
UDWORD inputGetKey(utf_32_char *unicode)
{
	if (inputBuffer.empty())
	{
		return 0;
	}

	UDWORD retVal = inputBuffer.front().key;
	if (retVal == 0)
	{
		retVal = ' ';  // Don't return 0 if we got a virtual key, since that's interpreted as no input.
	}

	if (unicode != NULL)
	{
		*unicode = inputBuffer.front().unicode;
	}

	inputBuffer.pop_front();

	return retVal;
}

MousePresses const &inputGetClicks()
{
	return mousePresses;
}

/*!
 * This is called once a frame so that the system can tell
 * whether a key was pressed this turn or held down from the last frame.
 */
void inputNewFrame(void)
{
	unsigned int i;

	/* Do the keyboard */
	for (i = 0; i < KEY_MAXSCAN; i++)
	{
		if (aKeyState[i].state == KEY_PRESSED)
		{
			aKeyState[i].state = KEY_DOWN;
		}
		else if (aKeyState[i].state == KEY_RELEASED ||
				 aKeyState[i].state == KEY_PRESSRELEASE)
		{
			aKeyState[i].state = KEY_UP;
		}
	}

	/* Do the mouse */
	for (i = 0; i < MOUSE_BAD; i++)
	{
		if (aMouseState[i].state == KEY_PRESSED)
		{
			aMouseState[i].state = KEY_DOWN;
		}
		else if (aMouseState[i].state == KEY_RELEASED
				 || aMouseState[i].state == KEY_DOUBLECLICK
				 || aMouseState[i].state == KEY_PRESSRELEASE)
		{
			aMouseState[i].state = KEY_UP;
		}
	}

	mousePresses.clear();
}

/*!
 * Release all keys (and buttons) when we lose focus
 */
// FIXME This seems to be totally ignored! (Try switching focus while the dragbox is open)
void inputLoseFocus()
{
	unsigned int i;

	/* Lost the window focus, have to take this as a global key up */
	for(i = 0; i < KEY_MAXSCAN; i++)
	{
		aKeyState[i].state = KEY_UP;
	}
	for (i = 0; i < MOUSE_BAD; i++)
	{
		aMouseState[i].state = KEY_UP;
	}
}

/* This returns true if the key is currently depressed */
bool keyDown(KEY_CODE code)
{
	return (aKeyState[code].state != KEY_UP);
}

/* This returns true if the key went from being up to being down this frame */
bool keyPressed(KEY_CODE code)
{
	return ((aKeyState[code].state == KEY_PRESSED) || (aKeyState[code].state == KEY_PRESSRELEASE));
}

/* This returns true if the key went from being down to being up this frame */
bool keyReleased(KEY_CODE code)
{
	return ((aKeyState[code].state == KEY_RELEASED) || (aKeyState[code].state == KEY_PRESSRELEASE));
}

/**************************/
/***    Thread support  ***/
/**************************/

WZ_THREAD *wzThreadCreate(int (*threadFunc)(void *), void *data)
{
	return new WZ_THREAD(threadFunc, data);
}

int wzThreadJoin(WZ_THREAD *thread)
{
	thread->wait();
	int ret = thread->ret;
	delete thread;
	return ret;
}

void wzThreadStart(WZ_THREAD *thread)
{
	thread->start();
}

void wzYieldCurrentThread()
{
#if QT_VERSION >= 0x040500
	QThread::yieldCurrentThread();
#endif
}

WZ_MUTEX *wzMutexCreate()
{
	return new WZ_MUTEX;
}

void wzMutexDestroy(WZ_MUTEX *mutex)
{
	delete mutex;
}

void wzMutexLock(WZ_MUTEX *mutex)
{
	mutex->lock();
}

void wzMutexUnlock(WZ_MUTEX *mutex)
{
	mutex->unlock();
}

WZ_SEMAPHORE *wzSemaphoreCreate(int startValue)
{
	return new WZ_SEMAPHORE(startValue);
}

void wzSemaphoreDestroy(WZ_SEMAPHORE *semaphore)
{
	delete semaphore;
}

void wzSemaphoreWait(WZ_SEMAPHORE *semaphore)
{
	semaphore->acquire();
}

void wzSemaphorePost(WZ_SEMAPHORE *semaphore)
{
	semaphore->release();
}

/**************************/
/***     Font support   ***/
/**************************/

// Font and text drawing support based on Qt postponed until we can get it working properly on
// all platforms, and with decent speed, and without clobbering existing OpenGL states.
#if 0
void iV_SetFont(enum iV_fonts FontID)
{
	WzMainWindow::instance()->setFontType(FontID);
}

void iV_TextInit()
{
}

void iV_TextShutdown()
{
}

void iV_font(const char *fontName, const char *fontFace, const char *fontFaceBold)
{
	// TODO
}

unsigned int iV_GetTextWidth(const char* string)
{
	return WzMainWindow::instance()->fontMetrics().width(QString::fromUtf8(string), -1);
}

unsigned int iV_GetCountedTextWidth(const char* string, size_t string_length)
{
	return WzMainWindow::instance()->fontMetrics().width(QString::fromUtf8(string), string_length);
}

unsigned int iV_GetTextHeight(const char* string)
{
	return WzMainWindow::instance()->fontMetrics().boundingRect(QString::fromUtf8(string)).height();
}

unsigned int iV_GetCharWidth(uint32_t charCode)
{
	return WzMainWindow::instance()->fontMetrics().width(QChar(charCode));
}

int iV_GetTextLineSize()
{
	return WzMainWindow::instance()->fontMetrics().lineSpacing();
}

int iV_GetTextAboveBase(void)
{
	return -WzMainWindow::instance()->fontMetrics().ascent();
}

int iV_GetTextBelowBase(void)
{
	return -WzMainWindow::instance()->fontMetrics().descent();
}

void iV_SetTextColour(PIELIGHT colour)
{
	fontColor = QColor(colour.byte.r, colour.byte.g, colour.byte.b, colour.byte.a);
}

void iV_DrawTextRotated(const char* string, float XPos, float YPos, float rotation)
{
	pie_SetTexturePage(TEXPAGE_EXTERN);
	glDisable(GL_CULL_FACE);
	QPainter painter(WzMainWindow::instance()->context()->device());
	painter.setPen(fontColor);
	if (rotation != 0.f)
	{
		painter.translate(XPos, YPos);
		painter.rotate(rotation);
		painter.drawText(0, 0, QString::fromUtf8(string));
	}
	else
	{
		painter.drawText(XPos, YPos, QString::fromUtf8(string));
	}
	painter.end();
	glEnable(GL_CULL_FACE);
	rendStatesRendModeHack();  // rendStates.rendMode = REND_ALPHA;
	pie_SetRendMode(REND_OPAQUE);		// beat state machinery into submission
}


void iV_SetTextSize(float size)
{
	WzMainWindow::instance()->setFontSize(size);
}
#endif

QString wzGetSelection()
{
	QString aText;
	QClipboard *clipboard = QApplication::clipboard();
	aText = clipboard->text(QClipboard::Selection);                           // try X11 specific buffer first
	if (aText.isEmpty()) aText = clipboard->text(QClipboard::Clipboard);      // if not, try generic clipboard
	return aText;
}

void wzFatalDialog(const char *text)
{
	crashing = true;
	QMessageBox::critical(NULL, "Fatal error", text);
}

static int WZkeyToQtKey(int code)
{
	if (code >= Qt::Key_0  && code  <= Qt::Key_AsciiTilde)
		return code;	// maps 1:1
	else if (code >= KEY_F1 && code <= KEY_F12)
	{
		return (Qt::Key_F1 + ( code - KEY_F1 ));
	}
	else if (code >= KEY_KP_0 && code <= KEY_KP_9)
	{
		return (Qt::Key_0 + ( code - KEY_KP_0 ));
	}
	else if (code >= Qt::Key_Exclam && code <= Qt::Key_Slash)
		return code;	// maps 1:1
	else if (code == KEY_ESC)
		return Qt::Key_Escape;
	else if (code == KEY_TAB)
		return Qt::Key_Tab;
	//Qt::Key_Backtab
	else if (code == KEY_BACKSPACE)
		return Qt::Key_Backspace;
	else if (code == KEY_RETURN)
		return Qt::Key_Return;
	else if (code == KEY_KPENTER)
		return 	Qt::Key_Enter;
	else if (code == KEY_INSERT)
		return Qt::Key_Insert;
	else if (code == KEY_DELETE)
		return Qt::Key_Delete;
	//else if (code ==
	//	return Qt::Key_Pause
	//else if (code ==
	//	return Qt::Key_Print
	//else if (code ==
	//	return Qt::Key_SysReq
	//else if (code ==
	//	return Qt::Key_Clear
	else if (code == KEY_KP_STAR)
		return Qt::Key_Asterisk;
	else if (code == KEY_KP_PLUS)
		return Qt::Key_Plus;
	else if (code == KEY_KP_MINUS)
		return Qt::Key_Minus;
	else if (code == KEY_KP_BACKSLASH)
		return Qt::Key_Backslash;
	else if (code == KEY_HOME)
		return Qt::Key_Home;
	else if (code == KEY_END)
		return Qt::Key_End;
	else if (code == KEY_LEFTARROW)
		return Qt::Key_Left;
	else if (code == KEY_UPARROW)
		return Qt::Key_Up;
	else if (code == KEY_RIGHTARROW)
		return Qt::Key_Right;
	else if (code == KEY_DOWNARROW)
		return Qt::Key_Down;
	else if (code == KEY_PAGEUP)
		return Qt::Key_PageUp;
	else if (code == KEY_PAGEDOWN)
		return Qt::Key_PageDown;
	else if (code == KEY_RSHIFT || code == KEY_LSHIFT)
		return Qt::SHIFT; //Qt::Key_Shift;
	else if (code == KEY_LCTRL || code == KEY_RCTRL)
		return Qt::CTRL; //Qt::Key_Control;
	else if (code == KEY_LMETA || code == KEY_RMETA)
		return Qt::META; //Qt::Key_Meta;
	else if (code == KEY_LALT || code == KEY_RALT)
		return Qt::ALT; //Qt::Key_Alt;
	else if (code == KEY_MAXSCAN)
	{
		// this just means that there is no key defined, so we print a "?" instead
		return Qt::Key_Question;
	}
	else if (code == KEY_SPACE)
		return Qt::Key_Space;

	ASSERT(false, "We missed mapping a key, code is %d, map it to input.h, then qnamespace.h", code);

	return 0;	// nothing found (should never happen)
}
