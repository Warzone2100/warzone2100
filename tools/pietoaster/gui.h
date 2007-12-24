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
#ifndef _gui_h
#define _gui_h

#include <AntTweakBar.h>

#include "wzglobal.h"
#include "base_types.h"

extern TwType g_tw_pieVertexType;
extern TwType g_tw_pieVector2fType;

///AntTweakBar version of hacky, abused dialog...
class CAntTweakBarTextField {
public:
	bool	m_Up; //whether textbox is up or not

	CAntTweakBarTextField() : m_Up(false) {}

	virtual void doFunction();

	void	incrementChar(uint16_t key);
	void	decrementChar(void);

	void	addTextBox(const char * name);

	void	deleteTextBox();

	void	updateTextBox(void);

	char	*getText(void) {return m_name;};

	void	addUserInfo(void *userInfo) {m_userInfo = userInfo;};
protected:
	char	m_name[255]; //name
	char	m_text[255]; //temp char
	int32_t	m_textIndex; //temp char current index
	TwBar	*m_textBar; //text box bar
	void	*m_userInfo;	//pointer to user object
	//char	*m_textPointer = NULL;
};

// Instead of mixing AntTweakBar with another GUI that supports GFX button, I decided to write my own(crappy) GUI :)

class CToasterButton {
public:
	static const uint32_t MAX_STR_LENGTH = 128;

	CToasterButton(const char *name, const char *text, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t alignment, bool isImage, void *image);

	enum {
		ALIGN_LEFT = 1,
		ALIGN_CENTER,
		ALIGN_RIGHT
	};

	virtual void draw(void);
	virtual void OnClick(void);
	virtual void OnDBClick(void);
private:
	bool	m_IsImageButton;	///whether image button or not
	uint16_t	m_Alignment;		///Alignment of text/image
	uint16_t	m_X;	///offset to GUI x(0)
	uint16_t	m_Y;	///offset to GUI y(0)
	uint16_t	m_Width;	///Width of button
	uint16_t	m_Height;	///Height of button

	char	m_Name[MAX_STR_LENGTH];		///Name
	char	m_Text[MAX_STR_LENGTH];		///Text
	SDL_Surface	*m_Image;	///Pointer to image surface
};

class CToasterWidget {
public:
	CToasterWidget(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
	~CToasterWidget();

	void	addButton(const char *name, const char *text,uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t alignment, bool isImage, void *image);

	void	draw(void);
private:
	uint16_t	m_ScreenX;	///offset to screen x(0)
	uint16_t	m_ScreenY;	///offset to screen y(0)
	uint16_t	m_Width;	///Width of widget
	uint16_t	m_Height;	///Height of widget
	CToasterButton	m_Buttons[10];	///buttons
	uint16_t	m_numButtons;	///number of buttons
};


class CAddSubModelDialog : public CAntTweakBarTextField {
public:
	void	doFunction();
private:
};

class CAddSubModelFileDialog : public CAntTweakBarTextField {
public:
	void	doFunction();
private:
};

class COpenFileDialog : public CAntTweakBarTextField {
public:
	void	doFunction();
private:
};

class CReadAnimFileDialog : public CAntTweakBarTextField {
public:
	void	doFunction();
private:
};
class CWriteAnimFileDialog : public CAntTweakBarTextField {
public:
	void	doFunction();
private:
};

extern CAddSubModelDialog AddSubModelDialog;
extern CAddSubModelFileDialog AddSubModelFileDialog;
extern COpenFileDialog OpenFileDialog;
extern CReadAnimFileDialog ReadAnimFileDialog;
extern CWriteAnimFileDialog WriteAnimFileDialog;

#endif

