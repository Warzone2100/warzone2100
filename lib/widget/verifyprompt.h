// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project

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
 *  Definitions for verify prompt functions
 */

#ifndef __INCLUDED_LIB_WIDGET_VERIFY_PROMPT_H__
#define __INCLUDED_LIB_WIDGET_VERIFY_PROMPT_H__

#include "widget.h"
#include <string>
#include <functional>

#if (defined(WZ_OS_WIN) && defined(WZ_CC_MINGW))
#  if (defined(snprintf) && !defined(_GL_STDIO_H) && defined(_LIBINTL_H))
// On mingw / MXE builds, libintl's define of snprintf breaks json.hpp
// So undef it here and restore it later (HACK)
#    define _wz_restore_libintl_snprintf
#    undef snprintf
#  endif
#endif

#include <nlohmann/json.hpp>

#if defined(_wz_restore_libintl_snprintf)
#  undef _wz_restore_libintl_snprintf
#  undef snprintf
#  define snprintf libintl_snprintf
#endif


class W_LABEL;
class W_EDITBOX;
class W_SLIDER;

namespace WzVerifyPrompt {

enum class PromptType
{
	Captcha,
	Slide
};

class VerifyPromptData
{
public:
	virtual ~VerifyPromptData();
	virtual PromptType GetType() const = 0;
};

class IvImageProviderInterface
{
public:
	virtual ~IvImageProviderInterface();

	virtual optional<iV_Image> GetImage() const = 0;
};

class CaptchaVerifyPromptData : public VerifyPromptData
{
public:
	virtual PromptType GetType() const override { return PromptType::Captcha; }

public:
	std::shared_ptr<IvImageProviderInterface> image;
};

class SlideVerifyPromptData : public VerifyPromptData
{
public:
	virtual PromptType GetType() const override { return PromptType::Slide; }

public:
	std::shared_ptr<IvImageProviderInterface> image;
	std::shared_ptr<IvImageProviderInterface> tile;
	uint32_t tile_width = 0;
	uint32_t tile_height = 0;
	uint32_t tile_x = 0;
	uint32_t tile_y = 0;
};

// Make a new VerifyPrompt widget (captcha)
typedef std::function<void(WzString result)> CaptchaVerifyResponseFunc;
std::shared_ptr<WIDGET> makeCaptchaWidget(CaptchaVerifyPromptData&& promptData, const CaptchaVerifyResponseFunc& responseCallback);

// Make a new VerifyPrompt widget (slide)
typedef std::function<void(uint32_t pointX, uint32_t pointY)> SlideVerifyResponseFunc;
std::shared_ptr<WIDGET> makeSlideWidget(SlideVerifyPromptData&& promptData, bool dragDrop, const SlideVerifyResponseFunc& responseCallback);

} // namespace WzVerifyPrompt

#endif // __INCLUDED_LIB_WIDGET_VERIFY_PROMPT_H__
