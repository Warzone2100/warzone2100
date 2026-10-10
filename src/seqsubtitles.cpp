// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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

#include "seqsubtitles.h"

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/widget/paragraph.h"
#include "text.h"
#include "wzjsonhelpers.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <vector>

#define SUBTITLES_TYPE "wz2100.subtitles.v1"
#define SUBTITLE_AREAS_TYPE "wz2100.subtitleareas.v1"

static const float MIN_FONT_POINTS = 9.f;	// the smallest text size used (font_small's)
static const iV_fonts VIDEO_FONTS[] = {font_video_1, font_video_2, font_video_3, font_video_4};

struct SubtitleArea
{
	// position, size and padding in fractions of the video's width and height
	float x = 0.f;
	float y = 0.f;
	float width = 0.f;
	float height = 0.f;
	float paddingX = 0.f;
	float paddingY = 0.f;
	float fontSize = 0.f;	// in fractions of the video's height
	bool centerVertically = false;
	bool clip = true;
	bool alwaysShow = false;
};

struct SubtitleLine
{
	double start = 0.;
	double end = 0.;
	std::string area;
	WzString text;	// translated, with the speaker
};

struct AreaLayout
{
	iV_fonts font = font_video_1;
	// the area inside its padding, in points on screen
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;
	int lineSize = 0;
};

struct LineLayout
{
	struct Fragment
	{
		WzText text;
		int x = 0;	// from the start of its line
		size_t line = 0;
	};
	std::vector<Fragment> fragments;
	std::vector<int> lineWidths;
	size_t shownLines = 0;
	std::unique_ptr<WidthLimitedWzText> clippedLine;	// the last shown line of clipped text, shortened for an ellipsis
};

struct SeqSubtitles
{
	std::string fileName;
	std::map<std::string, SubtitleArea> areas;
	std::vector<SubtitleLine> lines;

	// the layout, for the video rectangle it was made for
	bool laidOut = false;
	int layoutX = 0;
	int layoutY = 0;
	int layoutWidth = 0;
	int layoutHeight = 0;
	std::map<std::string, AreaLayout> areaLayouts;
	std::vector<LineLayout> lineLayouts;

	~SeqSubtitles();
};

// the subtitles laid out now (at most one: its text uses the video fonts)
static SeqSubtitles *videoFontsOwner = nullptr;

SeqSubtitles::~SeqSubtitles()
{
	if (videoFontsOwner == this)
	{
		videoFontsOwner = nullptr;
	}
}

static void freeLayout(SeqSubtitles &subtitles)
{
	subtitles.areaLayouts.clear();
	subtitles.lineLayouts.clear();
	subtitles.laidOut = false;
}

static bool checkType(const nlohmann::json &obj, const char *type, const std::string &fileName)
{
	auto it = obj.find("type");
	if (it == obj.end() || !it->is_string() || it->get<std::string>() != type)
	{
		debug(LOG_ERROR, "%s: expected \"type\": \"%s\"", fileName.c_str(), type);
		return false;
	}
	return true;
}

static bool validReferenceSize(double size)
{
	return size > 0.0 && size <= std::numeric_limits<float>::max();
}

static float toFraction(double number, float reference)
{
	return static_cast<float>(std::max(-2.0, std::min(2.0, number / reference)));
}

static bool readReference(const nlohmann::json &obj, const std::string &fileName, float &referenceWidth, float &referenceHeight)
{
	auto it = obj.find("reference");
	if (it == obj.end())
	{
		return false;
	}
	if (!it->is_array() || it->size() != 2 || !(*it)[0].is_number() || !(*it)[1].is_number()
		|| !validReferenceSize((*it)[0].get<double>()) || !validReferenceSize((*it)[1].get<double>()))
	{
		debug(LOG_ERROR, "%s: \"reference\" must be [width, height]", fileName.c_str());
		return false;
	}
	referenceWidth = (*it)[0].get<float>();
	referenceHeight = (*it)[1].get<float>();
	return true;
}

// Applies an area definition's keys (in reference units) to an area
static void applyAreaKeys(SubtitleArea &area, const nlohmann::json &keys, float referenceWidth, float referenceHeight, const std::string &fileName, const std::string &name)
{
	for (auto it = keys.begin(); it != keys.end(); ++it)
	{
		const std::string &key = it.key();
		const nlohmann::json &value = it.value();
		if (key == "x" || key == "y" || key == "width" || key == "height" || key == "fontSize" || key == "padding")
		{
			if (!value.is_number())
			{
				debug(LOG_ERROR, "%s: area \"%s\": \"%s\" must be a number", fileName.c_str(), name.c_str(), key.c_str());
				continue;
			}
			const double number = value.get<double>();
			if (key == "x") { area.x = toFraction(number, referenceWidth); }
			else if (key == "y") { area.y = toFraction(number, referenceHeight); }
			else if (key == "width") { area.width = toFraction(number, referenceWidth); }
			else if (key == "height") { area.height = toFraction(number, referenceHeight); }
			else if (key == "fontSize") { area.fontSize = toFraction(number, referenceHeight); }
			else
			{
				area.paddingX = toFraction(number, referenceWidth);
				area.paddingY = toFraction(number, referenceHeight);
			}
		}
		else if (key == "align")
		{
			if (!value.is_string() || value.get<std::string>() != "center")
			{
				debug(LOG_ERROR, "%s: area \"%s\": only \"align\": \"center\" is supported", fileName.c_str(), name.c_str());
			}
		}
		else if (key == "valign")
		{
			if (value.is_string() && (value.get<std::string>() == "top" || value.get<std::string>() == "center"))
			{
				area.centerVertically = (value.get<std::string>() == "center");
			}
			else
			{
				debug(LOG_ERROR, "%s: area \"%s\": \"valign\" must be \"top\" or \"center\"", fileName.c_str(), name.c_str());
			}
		}
		else if (key == "overflow")
		{
			if (value.is_string() && value.get<std::string>() == "clip")
			{
				area.clip = true;
			}
			else if (value.is_string() && value.get<std::string>() == "overflow")
			{
				area.clip = false;
			}
			else
			{
				debug(LOG_ERROR, "%s: area \"%s\": \"overflow\" must be \"clip\" or \"overflow\"", fileName.c_str(), name.c_str());
			}
		}
		else if (key == "alwaysShow")
		{
			if (value.is_boolean())
			{
				area.alwaysShow = value.get<bool>();
			}
			else
			{
				debug(LOG_ERROR, "%s: area \"%s\": \"alwaysShow\" must be true or false", fileName.c_str(), name.c_str());
			}
		}
		else
		{
			debug(LOG_ERROR, "%s: area \"%s\": unknown key \"%s\"", fileName.c_str(), name.c_str(), key.c_str());
		}
	}
}

// Adds or updates the areas an "areas" object defines
static void readAreas(const nlohmann::json &obj, float referenceWidth, float referenceHeight, const std::string &fileName, std::map<std::string, SubtitleArea> &areas)
{
	auto it = obj.find("areas");
	if (it == obj.end())
	{
		return;
	}
	if (!it->is_object())
	{
		debug(LOG_ERROR, "%s: \"areas\" must be an object", fileName.c_str());
		return;
	}
	for (auto area = it->begin(); area != it->end(); ++area)
	{
		if (!area.value().is_object())
		{
			debug(LOG_ERROR, "%s: area \"%s\" must be an object", fileName.c_str(), area.key().c_str());
			continue;
		}
		applyAreaKeys(areas[area.key()], area.value(), referenceWidth, referenceHeight, fileName, area.key());
	}
}

static optional<SubtitleLine> readLine(const nlohmann::json &obj, const std::map<std::string, SubtitleArea> &areas, const std::string &fileName, size_t index)
{
	if (!obj.is_object())
	{
		debug(LOG_ERROR, "%s: line %zu must be an object", fileName.c_str(), index);
		return nullopt;
	}
	auto start = obj.find("start");
	auto end = obj.find("end");
	auto area = obj.find("area");
	if (start == obj.end() || !start->is_number() || end == obj.end() || !end->is_number() || start->get<double>() >= end->get<double>())
	{
		debug(LOG_ERROR, "%s: line %zu needs a \"start\" before its \"end\" (seconds)", fileName.c_str(), index);
		return nullopt;
	}
	if (area == obj.end() || !area->is_string() || areas.count(area->get<std::string>()) == 0)
	{
		debug(LOG_ERROR, "%s: line %zu needs the \"area\" of a defined area", fileName.c_str(), index);
		return nullopt;
	}
	auto text = obj.find("text");
	auto textId = obj.find("textId");
	if ((text == obj.end()) == (textId == obj.end()) || (text != obj.end() && !text->is_string()) || (textId != obj.end() && !textId->is_string()))
	{
		debug(LOG_ERROR, "%s: line %zu needs either a \"text\" or a \"textId\"", fileName.c_str(), index);
		return nullopt;
	}

	SubtitleLine line;
	line.start = start->get<double>();
	line.end = end->get<double>();
	line.area = area->get<std::string>();
	if (textId != obj.end())
	{
		const std::string id = textId->get<std::string>();
		const char *str = (psStringRes != nullptr) ? strresGetString(psStringRes, id.c_str()) : nullptr;
		if (str == nullptr)
		{
			debug(LOG_ERROR, "%s: no string \"%s\" is loaded", fileName.c_str(), id.c_str());
		}
		line.text = WzString::fromUtf8((str != nullptr) ? str : id.c_str());
	}
	else
	{
		line.text = WzString::fromUtf8(_(text->get<std::string>().c_str()));
	}
	auto speaker = obj.find("speaker");
	if (speaker != obj.end())
	{
		if (speaker->is_string())
		{
			line.text = WzString::fromUtf8(std::string("<") + PE_("speaker", speaker->get<std::string>().c_str()) + ">: ") + line.text;
		}
		else
		{
			debug(LOG_ERROR, "%s: line %zu: \"speaker\" must be a string", fileName.c_str(), index);
		}
	}
	return line;
}

std::shared_ptr<SeqSubtitles> seqSubtitles_Load(const WzString &fileName)
{
	auto subtitles = std::make_shared<SeqSubtitles>();
	subtitles->fileName = "sequenceaudio/" + fileName.toUtf8();

	auto json = wzLoadJsonObjectFromFile(subtitles->fileName);
	if (!json.has_value() || !checkType(json.value(), SUBTITLES_TYPE, subtitles->fileName))
	{
		return nullptr;
	}

	bool haveReference = false;
	float referenceWidth = 0.f;
	float referenceHeight = 0.f;
	auto areasFile = json->find("areasFile");
	if (areasFile != json->end())
	{
		if (areasFile->is_string())
		{
			const std::string areasFileName = "sequenceaudio/" + areasFile->get<std::string>();
			auto areasJson = wzLoadJsonObjectFromFile(areasFileName);
			if (!areasJson.has_value())
			{
				debug(LOG_ERROR, "%s: can't load \"areasFile\" %s", subtitles->fileName.c_str(), areasFileName.c_str());
			}
			else if (checkType(areasJson.value(), SUBTITLE_AREAS_TYPE, areasFileName))
			{
				haveReference = readReference(areasJson.value(), areasFileName, referenceWidth, referenceHeight);
				if (haveReference)
				{
					readAreas(areasJson.value(), referenceWidth, referenceHeight, areasFileName, subtitles->areas);
				}
				else
				{
					debug(LOG_ERROR, "%s: needs a \"reference\"", areasFileName.c_str());
				}
			}
		}
		else
		{
			debug(LOG_ERROR, "%s: \"areasFile\" must be a string", subtitles->fileName.c_str());
		}
	}
	haveReference = readReference(json.value(), subtitles->fileName, referenceWidth, referenceHeight) || haveReference;
	if (json->contains("areas"))
	{
		if (haveReference)
		{
			readAreas(json.value(), referenceWidth, referenceHeight, subtitles->fileName, subtitles->areas);
		}
		else
		{
			debug(LOG_ERROR, "%s: needs a \"reference\" for its areas", subtitles->fileName.c_str());
		}
	}
	for (auto it = subtitles->areas.begin(); it != subtitles->areas.end();)
	{
		const SubtitleArea &area = it->second;
		if (area.width <= 0.f || area.height <= 0.f || area.fontSize <= 0.f)
		{
			debug(LOG_ERROR, "%s: area \"%s\" needs a width, height and fontSize", subtitles->fileName.c_str(), it->first.c_str());
			it = subtitles->areas.erase(it);
			continue;
		}
		if (area.x < 0.f || area.x > 1.f || area.y < 0.f || area.y > 1.f || area.width > 1.f || area.height > 1.f || area.fontSize > 1.f
			|| area.paddingX < 0.f || area.paddingX > 0.5f || area.paddingY < 0.f || area.paddingY > 0.5f)
		{
			debug(LOG_ERROR, "%s: area \"%s\" is not inside the video", subtitles->fileName.c_str(), it->first.c_str());
			it = subtitles->areas.erase(it);
			continue;
		}
		++it;
	}

	auto lines = json->find("lines");
	if (lines == json->end() || !lines->is_array())
	{
		debug(LOG_ERROR, "%s: needs a \"lines\" array", subtitles->fileName.c_str());
		return nullptr;
	}
	for (size_t i = 0; i < lines->size(); ++i)
	{
		if (auto line = readLine((*lines)[i], subtitles->areas, subtitles->fileName, i))
		{
			subtitles->lines.push_back(std::move(line.value()));
		}
	}

	// one line per area at a time: an overlapping line replaces the earlier one
	std::map<std::string, std::vector<std::pair<double, double>>> times;
	for (const auto &line : subtitles->lines)
	{
		times[line.area].push_back({line.start, line.end});
	}
	for (auto &area : times)
	{
		std::sort(area.second.begin(), area.second.end());
		for (size_t i = 1; i < area.second.size(); ++i)
		{
			if (area.second[i].first < area.second[i - 1].second)
			{
				debug(LOG_ERROR, "%s: area \"%s\" has overlapping lines at %.2f s", subtitles->fileName.c_str(), area.first.c_str(), area.second[i].first);
			}
		}
	}
	return subtitles;
}

static bool textsFit(const std::vector<const WzString *> &texts, iV_fonts font, int width, int height)
{
	const int lineSize = iV_GetTextLineSize(font);
	for (const WzString *text : texts)
	{
		if (static_cast<int>(wzLayoutTextLines(*text, font, static_cast<unsigned int>(width)).size()) * lineSize > height)
		{
			return false;
		}
	}
	return true;
}

// The largest text size (in points), up to the target, at which all the texts fit, but no smaller than MIN_FONT_POINTS
static float fitFontSize(float target, const std::vector<const WzString *> &texts, int width, int height)
{
	if (target <= MIN_FONT_POINTS)
	{
		return MIN_FONT_POINTS;
	}
	const iV_fonts scratchFont = VIDEO_FONTS[0];
	iV_BindVideoFont(scratchFont, target);
	if (textsFit(texts, scratchFont, width, height))
	{
		return target;
	}
	float fits = MIN_FONT_POINTS;
	float fails = target;
	for (int i = 0; i < 6; ++i)
	{
		const float size = (fits + fails) / 2.f;
		iV_BindVideoFont(scratchFont, size);
		if (textsFit(texts, scratchFont, width, height))
		{
			fits = size;
		}
		else
		{
			fails = size;
		}
	}
	return fits;
}

static void layOut(SeqSubtitles &subtitles, int videoX, int videoY, int videoWidth, int videoHeight)
{
	if (videoFontsOwner != nullptr)
	{
		freeLayout(*videoFontsOwner);
	}

	// each used area's text size: the same for all of its lines
	std::map<std::string, float> sizes;
	for (const auto &entry : subtitles.areas)
	{
		const SubtitleArea &area = entry.second;
		std::vector<const WzString *> texts;
		for (const auto &line : subtitles.lines)
		{
			if (line.area == entry.first)
			{
				texts.push_back(&line.text);
			}
		}
		if (texts.empty())
		{
			continue;
		}
		AreaLayout areaLayout;
		areaLayout.x = videoX + static_cast<int>(std::lround((area.x + area.paddingX) * videoWidth));
		areaLayout.y = videoY + static_cast<int>(std::lround((area.y + area.paddingY) * videoHeight));
		areaLayout.width = std::max(1, static_cast<int>(std::lround((area.width - 2.f * area.paddingX) * videoWidth)));
		areaLayout.height = std::max(1, static_cast<int>(std::lround((area.height - 2.f * area.paddingY) * videoHeight)));
		sizes[entry.first] = fitFontSize(area.fontSize * videoHeight, texts, areaLayout.width, areaLayout.height);
		subtitles.areaLayouts[entry.first] = areaLayout;
	}

	// a video font per distinct size (areas beyond the fonts available share the nearest size)
	std::vector<float> fontSizes;
	for (const auto &entry : sizes)
	{
		auto sameSize = std::find_if(fontSizes.begin(), fontSizes.end(), [&](float size) { return std::fabs(size - entry.second) < 1.f / 64.f; });
		size_t index = static_cast<size_t>(sameSize - fontSizes.begin());
		if (sameSize == fontSizes.end())
		{
			if (fontSizes.size() < ARRAY_SIZE(VIDEO_FONTS))
			{
				fontSizes.push_back(entry.second);
			}
			else
			{
				debug(LOG_ERROR, "%s: more than %zu text sizes, area \"%s\" uses the nearest", subtitles.fileName.c_str(), ARRAY_SIZE(VIDEO_FONTS), entry.first.c_str());
				auto nearest = std::min_element(fontSizes.begin(), fontSizes.end(), [&](float a, float b) { return std::fabs(a - entry.second) < std::fabs(b - entry.second); });
				index = static_cast<size_t>(nearest - fontSizes.begin());
			}
		}
		subtitles.areaLayouts[entry.first].font = VIDEO_FONTS[index];
	}
	for (size_t i = 0; i < fontSizes.size(); ++i)
	{
		iV_BindVideoFont(VIDEO_FONTS[i], fontSizes[i]);
		debug(LOG_VIDEO, "%s: video text size %zu: %.2f points", subtitles.fileName.c_str(), i + 1, fontSizes[i]);
	}
	for (auto &entry : subtitles.areaLayouts)
	{
		entry.second.lineSize = std::max(1, iV_GetTextLineSize(entry.second.font));
	}

	subtitles.lineLayouts.resize(subtitles.lines.size());
	for (size_t i = 0; i < subtitles.lines.size(); ++i)
	{
		const SubtitleLine &line = subtitles.lines[i];
		const AreaLayout &areaLayout = subtitles.areaLayouts.at(line.area);
		LineLayout &lineLayout = subtitles.lineLayouts[i];
		auto wrapped = wzLayoutTextLines(line.text, areaLayout.font, static_cast<unsigned int>(areaLayout.width));
		lineLayout.shownLines = wrapped.size();
		if (subtitles.areas.at(line.area).clip)
		{
			lineLayout.shownLines = std::min(wrapped.size(), static_cast<size_t>(std::max(1, areaLayout.height / areaLayout.lineSize)));
		}
		const bool clipped = lineLayout.shownLines < wrapped.size();
		for (size_t lineIndex = 0; lineIndex < wrapped.size(); ++lineIndex)
		{
			const auto &fragments = wrapped[lineIndex];
			lineLayout.lineWidths.push_back(fragments.empty() ? 0 : static_cast<int>(fragments.back().offset + fragments.back().width));
			// (a clipped text's last shown line is drawn from clippedLine instead)
			if (lineIndex < lineLayout.shownLines && !(clipped && lineIndex + 1 == lineLayout.shownLines))
			{
				for (const auto &fragment : fragments)
				{
					lineLayout.fragments.push_back({WzText(fragment.text, areaLayout.font), static_cast<int>(fragment.offset), lineIndex});
				}
			}
		}
		if (clipped)
		{
			// the last shown line, shortened to leave room for an ellipsis
			const size_t last = lineLayout.shownLines - 1;
			WzString lastLine;
			unsigned int lastEnd = 0;
			for (const auto &fragment : wrapped[last])
			{
				if (!lastLine.isEmpty() && fragment.offset > lastEnd)
				{
					lastLine.append(" ");
				}
				lastLine.append(fragment.text);
				lastEnd = fragment.offset + fragment.width;
			}
			const int ellipsisWidth = iV_GetEllipsisWidth(areaLayout.font);
			lineLayout.clippedLine = std::make_unique<WidthLimitedWzText>();
			lineLayout.clippedLine->setTruncatableText(lastLine, areaLayout.font, static_cast<size_t>(std::max(0, areaLayout.width - ellipsisWidth)));
			lineLayout.lineWidths[last] = lineLayout.clippedLine->width() + ellipsisWidth;
		}
	}

	subtitles.laidOut = true;
	subtitles.layoutX = videoX;
	subtitles.layoutY = videoY;
	subtitles.layoutWidth = videoWidth;
	subtitles.layoutHeight = videoHeight;
	videoFontsOwner = &subtitles;
}

static void drawOutlined(WzText &text, int x, int y)
{
	text.render(x - 1, y - 1, WZCOL_GREY);
	text.render(x - 1, y + 1, WZCOL_GREY);
	text.render(x + 1, y - 1, WZCOL_GREY);
	text.render(x + 1, y + 1, WZCOL_GREY);
	text.render(x, y, WZCOL_WHITE);
}

void seqSubtitles_Draw(SeqSubtitles &subtitles, double time, bool showSubtitles, int videoX, int videoY, int videoWidth, int videoHeight)
{
	if (videoWidth <= 0 || videoHeight <= 0)
	{
		return;
	}
	if (!subtitles.laidOut || videoFontsOwner != &subtitles || subtitles.layoutX != videoX || subtitles.layoutY != videoY
		|| subtitles.layoutWidth != videoWidth || subtitles.layoutHeight != videoHeight)
	{
		layOut(subtitles, videoX, videoY, videoWidth, videoHeight);
	}

	// each area's shown line: the latest started
	std::map<std::string, size_t> shown;
	for (size_t i = 0; i < subtitles.lines.size(); ++i)
	{
		const SubtitleLine &line = subtitles.lines[i];
		if (time < line.start || time >= line.end)
		{
			continue;
		}
		auto it = shown.find(line.area);
		if (it == shown.end() || subtitles.lines[it->second].start <= line.start)
		{
			shown[line.area] = i;
		}
	}

	for (const auto &entry : shown)
	{
		if (!showSubtitles && !subtitles.areas.at(entry.first).alwaysShow)
		{
			continue;
		}
		const AreaLayout &areaLayout = subtitles.areaLayouts.at(entry.first);
		LineLayout &lineLayout = subtitles.lineLayouts[entry.second];
		int top = areaLayout.y;
		if (subtitles.areas.at(entry.first).centerVertically)
		{
			top += std::max(0, areaLayout.height - static_cast<int>(lineLayout.shownLines) * areaLayout.lineSize) / 2;
		}
		for (auto &fragment : lineLayout.fragments)
		{
			const int x = areaLayout.x + (areaLayout.width - lineLayout.lineWidths[fragment.line]) / 2 + fragment.x;
			const int y = top + static_cast<int>(fragment.line) * areaLayout.lineSize - fragment.text.aboveBase();
			drawOutlined(fragment.text, x, y);
		}
		if (lineLayout.clippedLine)
		{
			const size_t last = lineLayout.shownLines - 1;
			const int x = areaLayout.x + (areaLayout.width - lineLayout.lineWidths[last]) / 2;
			const int y = top + static_cast<int>(last) * areaLayout.lineSize - lineLayout.clippedLine->aboveBase();
			drawOutlined(*lineLayout.clippedLine, x, y);
			iV_DrawEllipsis(areaLayout.font, Vector2f(x + lineLayout.clippedLine->width(), y), WZCOL_WHITE);
		}
	}
}

void seqSubtitles_ReleaseLayout()
{
	if (videoFontsOwner != nullptr)
	{
		freeLayout(*videoFontsOwner);
		videoFontsOwner = nullptr;
	}
	iV_UnbindVideoFonts();
}
