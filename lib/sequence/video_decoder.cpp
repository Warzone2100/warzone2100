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

#include "lib/framework/frame.h"
#include "video_decoder.h"
#include "ogg_decoder.h"
#if defined(WZ_ENABLE_WEBM)
# include "webm_decoder.h"
#endif

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>

WZVideoDecoder::~WZVideoDecoder()
{ }

namespace
{

struct LanguageCodes
{
	const char *iso639_1;
	const char *iso639_2t;
	const char *iso639_2b;	// only set where it differs from the /T form
};

// Languages WZ ships translations for. Sufficient for canonicalizing both the
// game-language side (639-1 style locale codes) and the container side
// (Matroska uses 639-2, historically the /B forms).
constexpr LanguageCodes languageCodeTable[] = {
	{"bg", "bul", nullptr}, {"ca", "cat", nullptr}, {"cs", "ces", "cze"},
	{"da", "dan", nullptr}, {"de", "deu", "ger"},   {"el", "ell", "gre"},
	{"en", "eng", nullptr}, {"es", "spa", nullptr}, {"et", "est", nullptr},
	{"eu", "eus", "baq"},   {"fi", "fin", nullptr}, {"fr", "fra", "fre"},
	{"fy", "fry", nullptr}, {"ga", "gle", nullptr}, {"hr", "hrv", nullptr},
	{"hu", "hun", nullptr}, {"id", "ind", nullptr}, {"it", "ita", nullptr},
	{"ja", "jpn", nullptr}, {"ko", "kor", nullptr}, {"la", "lat", nullptr},
	{"lt", "lit", nullptr}, {"lv", "lav", nullptr}, {"nb", "nob", nullptr},
	{"nl", "nld", "dut"},   {"pl", "pol", nullptr}, {"pt", "por", nullptr},
	{"ro", "ron", "rum"},   {"ru", "rus", nullptr}, {"sk", "slk", "slo"},
	{"sl", "slv", nullptr}, {"sv", "swe", nullptr}, {"tr", "tur", nullptr},
	{"uk", "ukr", nullptr}, {"zh", "zho", "chi"},
};

// lowercase primary language subtag: "pt_BR" / "de-DE" / "GER" -> "pt" / "de" / "ger"
std::string normalizeLanguageCode(const WzString& code)
{
	std::string s = code.toUtf8();
	size_t delim = s.find_first_of("-_");
	if (delim != std::string::npos)
	{
		s.resize(delim);
	}
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return s;
}

// canonical ISO 639-1 form where known; otherwise the normalized input
std::string canonicalLanguageCode(const WzString& code)
{
	std::string s = normalizeLanguageCode(code);
	if (s.length() == 3)
	{
		for (const auto& entry : languageCodeTable)
		{
			if (s == entry.iso639_2t || (entry.iso639_2b && s == entry.iso639_2b))
			{
				return entry.iso639_1;
			}
		}
	}
	return s;
}

bool languageMatches(const WzString& a, const WzString& b)
{
	std::string ca = canonicalLanguageCode(a);
	return !ca.empty() && ca == canonicalLanguageCode(b);
}

} // anonymous namespace

size_t videoDecoderChooseAudioTrack(const std::vector<WZAudioTrackMetadata>& tracks, const WzString& preferredLanguage)
{
	if (tracks.empty())
	{
		return 0;
	}

	// 1. the preferred language, if we have it
	if (!preferredLanguage.isEmpty())
	{
		for (size_t i = 0; i < tracks.size(); ++i)
		{
			if (languageMatches(tracks[i].languageCode, preferredLanguage))
			{
				return i;
			}
		}
	}

	// 2. English - by language tag, not track order (untagged tracks are
	// reported as "eng": the Matroska default language is English)
	for (size_t i = 0; i < tracks.size(); ++i)
	{
		if (languageMatches(tracks[i].languageCode, "en"))
		{
			return i;
		}
	}

	// 3. the container default (or, failing that, the first) track, so a file
	// with no English track at all still plays something
	for (size_t i = 0; i < tracks.size(); ++i)
	{
		if (tracks[i].isDefault)
		{
			return i;
		}
	}
	return 0;
}

bool videoDecoderWebmSupported()
{
#if defined(WZ_ENABLE_WEBM)
	return true;
#else
	return false;
#endif
}

std::unique_ptr<WZVideoDecoder> videoDecoderOpen(std::shared_ptr<VideoProvider> provider)
{
	if (!provider)
	{
		return nullptr;
	}

	// sniff the container format from the first bytes (don't trust file extensions)
	uint8_t magic[4] = {0};
	if (!provider->seek(0) || provider->read(magic, sizeof(magic)) != sizeof(magic) || !provider->seek(0))
	{
		debug(LOG_ERROR, "Unable to read video file header: %s", provider->filename().toUtf8().c_str());
		return nullptr;
	}

	if (memcmp(magic, "OggS", 4) == 0)
	{
		return oggTheoraDecoderOpen(std::move(provider));
	}

	static const uint8_t ebmlMagic[4] = { 0x1A, 0x45, 0xDF, 0xA3 };
	if (memcmp(magic, ebmlMagic, 4) == 0)
	{
		// EBML magic: a WebM (Matroska) container
#if defined(WZ_ENABLE_WEBM)
		return webmVideoDecoderOpen(std::move(provider));
#else
		debug(LOG_ERROR, "This build does not include WebM video support: %s", provider->filename().toUtf8().c_str());
		return nullptr;
#endif
	}

	debug(LOG_ERROR, "Unrecognized video container format: %s", provider->filename().toUtf8().c_str());
	return nullptr;
}
