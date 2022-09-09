/*
	This file is part of Warzone 2100.
	Copyright (C) 2020  Warzone 2100 Project

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
/*
 * musicmanager.cpp
 *
 * This is the Music Manager screen.
 */

#include "musicmanager.h"

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "input/manager.h"
#include "intdisplay.h"
#include "hci.h"
#include "multiint.h"
#include "frontend.h"
#include "frend.h"
#include "ingameop.h"

#include "lib/sound/playlist.h"
#include "lib/widget/button.h"
#include "lib/widget/label.h"
#include "lib/widget/scrollablelist.h"
#include <chrono>
#include <memory>


#define W_TRACK_ROW_PADDING 5
#define W_TRACK_COL_PADDING 10
#define W_TRACK_CHECKBOX_SIZE 12

#define W_TRACK_COL_TITLE_X 	W_TRACK_COL_PADDING
#define W_TRACK_COL_TITLE_W		180
#define W_TRACK_COL_ALBUM_X		(W_TRACK_COL_TITLE_X + W_TRACK_COL_TITLE_W + W_TRACK_COL_PADDING)
#define W_TRACK_COL_ALBUM_W		130
#define W_TRACK_CHECKBOX_STARTINGPOS (W_TRACK_COL_ALBUM_X + W_TRACK_COL_ALBUM_W + W_TRACK_COL_PADDING + W_TRACK_COL_PADDING)

#define W_TRACK_HEADER_Y		20
#define W_TRACK_HEADER_HEIGHT	(20 + (W_TRACK_ROW_PADDING * 2))
#define W_TRACK_HEADER_COL_IMAGE_SIZE	16

#define TL_W				FRONTEND_BOTFORMW
#define TL_H				400
#define TL_X				FRONTEND_BOTFORMX
#define TL_Y				(W_TRACK_HEADER_Y + W_TRACK_HEADER_HEIGHT)
#define TL_SX				FRONTEND_SIDEX

#define TL_ENTRYW			(FRONTEND_BOTFORMW - 80)
#define TL_ENTRYH			(25)

#define TL_PREVIEWBOX_Y_SPACING 45
#define TL_PREVIEWBOX_H		80

static int GetTrackListHeight()
{
	if (pie_GetVideoBufferHeight() > (BASE_COORDS_Y + TL_PREVIEWBOX_Y_SPACING + 20 + TL_PREVIEWBOX_H))
	{
		return TL_H;
	}
	return pie_GetVideoBufferHeight() - TL_Y - 20 - TL_PREVIEWBOX_Y_SPACING - TL_PREVIEWBOX_H;
}

static int GetNumVisibleTracks()
{
	int maxTracks = static_cast<int>(std::floor(float(GetTrackListHeight() - TL_Y - W_TRACK_HEADER_Y) / float(TL_ENTRYH)));
	return maxTracks;
}

static int GetDetailsBoxStartYPos()
{
	return GetTrackListHeight() + TL_PREVIEWBOX_Y_SPACING;
}

static int GetTotalTrackAndDetailsBoxHeight()
{
	return GetTrackListHeight() + TL_PREVIEWBOX_Y_SPACING + TL_PREVIEWBOX_H;
}

static int GetTrackListStartXPos(int ingame)
{
	if (!ingame)
	{
		return TL_X;
	}
	return TL_X - 10;
}

// MARK: - MusicManager_CDAudioEventSink declaration
class MusicManager_CDAudioEventSink : public CDAudioEventSink
{
public:
	virtual ~MusicManager_CDAudioEventSink() override {};
	virtual void startedPlayingTrack(const std::shared_ptr<const WZ_TRACK>& track) override;
	virtual void trackEnded(const std::shared_ptr<const WZ_TRACK>& track) override;
	virtual void musicStopped() override;
	virtual void musicPaused(const std::shared_ptr<const WZ_TRACK>& track) override;
	virtual void musicResumed(const std::shared_ptr<const WZ_TRACK>& track) override;
	virtual bool unregisterEventSink() const override { return shouldUnregisterEventSink; }
public:
	void setUnregisterEventSink()
	{
		shouldUnregisterEventSink = true;
	}
private:
	bool shouldUnregisterEventSink = false;
};

// MARK: - Globals

struct TrackRowCache; // forward-declare
class W_TrackRow; // foward-declare
static std::unordered_map<W_TrackRow*, std::shared_ptr<TrackRowCache>> trackRowsCache;
static std::vector<std::shared_ptr<const WZ_TRACK>> trackList;
static std::shared_ptr<const WZ_TRACK> selectedTrack;
static std::shared_ptr<MusicManager_CDAudioEventSink> musicManagerAudioEventSink;

// now-playing widgets
static std::shared_ptr<W_LABEL> psNowPlaying = nullptr;
static std::shared_ptr<W_LABEL> psSelectedTrackName = nullptr;
static std::shared_ptr<W_LABEL> psSelectedTrackAuthorName = nullptr;
static std::shared_ptr<W_LABEL> psSelectedTrackAlbumName = nullptr;
static std::shared_ptr<W_LABEL> psSelectedTrackAlbumDate = nullptr;
static std::shared_ptr<W_LABEL> psSelectedTrackAlbumDescription = nullptr;

// MARK: - W_MusicModeCheckboxButton

struct W_MusicModeCheckboxButton : public W_BUTTON
{
public:
	W_MusicModeCheckboxButton(MusicGameMode mode, bool isChecked)
		: W_BUTTON()
		, mode(mode)
		, isChecked(isChecked)
	{
		addOnClickHandler([](W_BUTTON& button) {
			W_MusicModeCheckboxButton& self = dynamic_cast<W_MusicModeCheckboxButton&>(button);
			if (!self.isEnabled()) { return; }
			self.isChecked = !self.isChecked;
		});
	}
	void display(int xOffset, int yOffset) override;
	void highlight(W_CONTEXT *psContext) override;
	void highlightLost() override;

	bool getIsChecked() const { return isChecked; }

	void setCheckboxSize(int size)
	{
		cbSize = size;
	}
	int checkboxSize() const { return cbSize; }

	MusicGameMode getMusicMode() { return mode; }
private:
	bool isEnabled() { return (getState() & WBUT_DISABLE) == 0; }
private:
	MusicGameMode mode;
	bool isChecked = false;
	int cbSize = 0;
};

void W_MusicModeCheckboxButton::display(int xOffset, int yOffset)
{
	int x0 = xOffset + x();
	int y0 = yOffset + y();

	bool down = (getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	bool isDisabled = (getState() & WBUT_DISABLE) != 0;

	// calculate checkbox dimensions
	Vector2i checkboxOffset{0, (height() - cbSize) / 2}; // left-align, center vertically
	Vector2i checkboxPos{x0 + checkboxOffset.x, y0 + checkboxOffset.y};

	// draw checkbox border
	PIELIGHT notifyBoxAddColor = WZCOL_NOTIFICATION_BOX;
	notifyBoxAddColor.byte.a = uint8_t(float(notifyBoxAddColor.byte.a) * ((!isDisabled) ? 0.7f : 0.2f));
	pie_UniTransBoxFill(checkboxPos.x, checkboxPos.y, checkboxPos.x + cbSize, checkboxPos.y + cbSize, notifyBoxAddColor);
	PIELIGHT checkBoxOutsideColor = WZCOL_TEXT_MEDIUM;
	if (isDisabled)
	{
		checkBoxOutsideColor.byte.a = 60;
	}
	iV_Box2(checkboxPos.x, checkboxPos.y, checkboxPos.x + cbSize, checkboxPos.y + cbSize, checkBoxOutsideColor, checkBoxOutsideColor);

	if (down || isChecked)
	{
		// draw checkbox "checked" inside
		#define CB_INNER_INSET 2
		PIELIGHT checkBoxInsideColor = WZCOL_TEXT_MEDIUM;
		checkBoxInsideColor.byte.a = (!isDisabled) ? 200 : 60;
		pie_UniTransBoxFill(checkboxPos.x + CB_INNER_INSET, checkboxPos.y + CB_INNER_INSET, checkboxPos.x + cbSize - (CB_INNER_INSET), checkboxPos.y + cbSize - (CB_INNER_INSET), checkBoxInsideColor);
	}
}

void W_MusicModeCheckboxButton::highlight(W_CONTEXT *psContext)
{
	if (!isEnabled()) return;
	W_BUTTON::highlight(psContext);
}

void W_MusicModeCheckboxButton::highlightLost()
{
	if (!isEnabled()) return;
	W_BUTTON::highlightLost();
}

// MARK: - W_TrackRow

struct TrackRowCache {
	WzText wzText_Title;
	WzText wzText_Album;
	UDWORD lastUsedFrameNumber;
};

class W_TrackRow : public W_BUTTON
{
protected:
	W_TrackRow(W_BUTINIT const *init, std::shared_ptr<const WZ_TRACK> track): W_BUTTON(init), track(std::move(track)) {}
	void initialize(bool ingame);

public:
	static std::shared_ptr<W_TrackRow> make(W_BUTINIT const *init, std::shared_ptr<const WZ_TRACK> const &track, bool ingame)
	{
		class make_shared_enabler: public W_TrackRow
		{
		public:
			make_shared_enabler(W_BUTINIT const *init, std::shared_ptr<const WZ_TRACK> const &track): W_TrackRow(init, track) {}
		};
		auto widget = std::make_shared<make_shared_enabler>(init, track);
		widget->initialize(ingame);
		return widget;
	}

	void display(int xOffset, int yOffset) override;

	std::shared_ptr<const WZ_TRACK> getTrack() const {
		return std::shared_ptr<const WZ_TRACK>(track);
	};

protected:
	void geometryChanged() override;

private:
	std::shared_ptr<const WZ_TRACK> track;
	std::string album_name;
	std::vector<std::shared_ptr<W_MusicModeCheckboxButton>> musicModeCheckboxes;
	MusicGameMode musicMode = MusicGameMode::MENUS;
};

void W_TrackRow::initialize(bool ingame)
{
	auto album = track->album.lock();
	album_name = album->title;

	// add music mode checkboxes
	musicMode = MusicGameMode::MENUS;
	if (ingame)
	{
		musicMode = PlayList_GetCurrentMusicMode();
	}

	for (int musicModeIdx = 0; musicModeIdx < NUM_MUSICGAMEMODES; musicModeIdx++)
	{
		auto pCheckBox = std::make_shared<W_MusicModeCheckboxButton>(static_cast<MusicGameMode>(musicModeIdx), PlayList_IsTrackEnabledForMusicMode(track, static_cast<MusicGameMode>(musicModeIdx)));
		attach(pCheckBox);
		auto captureTrack = track;
		pCheckBox->addOnClickHandler([captureTrack](W_BUTTON& button) {
			W_MusicModeCheckboxButton& self = dynamic_cast<W_MusicModeCheckboxButton&>(button);
			PlayList_SetTrackMusicMode(captureTrack, self.getMusicMode(), self.getIsChecked());
		});
		pCheckBox->setGeometry(W_TRACK_CHECKBOX_STARTINGPOS + ((W_TRACK_CHECKBOX_SIZE + W_TRACK_COL_PADDING) * musicModeIdx), W_TRACK_ROW_PADDING, W_TRACK_CHECKBOX_SIZE, W_TRACK_CHECKBOX_SIZE);
		pCheckBox->setCheckboxSize(W_TRACK_CHECKBOX_SIZE);
		if (musicMode != MusicGameMode::MENUS)
		{
			if (musicModeIdx != static_cast<int>(musicMode))
			{
				pCheckBox->setState(WBUT_DISABLE);
			}
		}

		musicModeCheckboxes.push_back(pCheckBox);
	}
}

void W_TrackRow::geometryChanged()
{
	for (size_t i = 0; i < musicModeCheckboxes.size(); i++)
	{
		auto pCB = musicModeCheckboxes[i];
		pCB->setGeometry(W_TRACK_CHECKBOX_STARTINGPOS + ((W_TRACK_CHECKBOX_SIZE + W_TRACK_COL_PADDING) * i), std::max(W_TRACK_ROW_PADDING, (height() - W_TRACK_CHECKBOX_SIZE) / 2), W_TRACK_CHECKBOX_SIZE, W_TRACK_CHECKBOX_SIZE);
	}
}

static WzString truncateTextToMaxWidth(WzString str, iV_fonts fontID, int maxWidth)
{
	if ((int)iV_GetTextWidth(str, fontID) > maxWidth)
	{
		while (!str.isEmpty() && (int)iV_GetTextWidth((str + "..."), fontID) > maxWidth)
		{
			if (!str.pop_back())  // Clip name.
			{
				ASSERT(false, "WzString::pop_back() failed??");
				break;
			}
		}
		str += "...";
	}
	return str;
}

void W_TrackRow::display(int xOffset, int yOffset)
{
	bool isSelectedTrack = (track == selectedTrack);

	// get track cache
	std::shared_ptr<TrackRowCache> pCache = nullptr;
	auto it = trackRowsCache.find(this);
	if (it != trackRowsCache.end())
	{
		pCache = it->second;
	}
	if (!pCache)
	{
		pCache = std::make_shared<TrackRowCache>();
		trackRowsCache[this] = pCache;

		// calculate max displayable length for title and album
		WzString title_truncated = truncateTextToMaxWidth(WzString::fromUtf8(track->title), font_regular, W_TRACK_COL_TITLE_W);
		WzString album_truncated = truncateTextToMaxWidth(WzString::fromUtf8(album_name), font_regular, W_TRACK_COL_ALBUM_W);
		pCache->wzText_Title.setText(title_truncated, font_regular);
		pCache->wzText_Album.setText(album_truncated, font_regular);
	}
	pCache->lastUsedFrameNumber = frameGetFrameNumber();

	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int x1 = x0 + width();
	int y1 = y0 + height();

	if (isSelectedTrack)
	{
		// draw selected background
		pie_UniTransBoxFill(x0, y0, x1, y1, WZCOL_KEYMAP_ACTIVE);
	}

	PIELIGHT textColor = WZCOL_TEXT_BRIGHT;
	if ((musicMode != MusicGameMode::MENUS) && !PlayList_IsTrackEnabledForMusicMode(track, musicMode))
	{
		textColor.byte.a = uint8_t(float(textColor.byte.a) * 0.6f);
	}

	// draw track title
	Vector2i textBoundingBoxOffset(0, 0);
	textBoundingBoxOffset.y = yOffset + y() + (height() - pCache->wzText_Title.lineSize()) / 2;
	float fy = float(textBoundingBoxOffset.y) /*+ float(W_TRACK_ROW_PADDING)*/ - float(pCache->wzText_Title.aboveBase());
	pCache->wzText_Title.render(W_TRACK_COL_TITLE_X + x0, static_cast<int>(fy), textColor);

	// draw album name
	pCache->wzText_Album.render(W_TRACK_COL_ALBUM_X + x0, static_cast<int>(fy), textColor);

	// Music mode checkboxes are child widgets
}

// MARK: - "Now Playing" Details Block

static gfx_api::texture* loadImageToTexture(const std::string& imagePath)
{
	if (imagePath.empty())
	{
		return nullptr;
	}

	const std::string& filename = imagePath;
	const char *extension = strrchr(filename.c_str(), '.'); // determine the filetype

	if (!extension || strcmp(extension, ".png") != 0)
	{
		debug(LOG_ERROR, "Bad image filename: %s", filename.c_str());
		return nullptr;
	}

	gfx_api::texture* pAlbumCoverTexture = gfx_api::context::get().loadTextureFromFile(imagePath.c_str(), gfx_api::texture_type::user_interface);
	return pAlbumCoverTexture;
}

class TrackDetailsForm : public IntFormAnimated
{
public:
	TrackDetailsForm()
	: IntFormAnimated(true)
	{ }
	virtual void display(int xOffset, int yOffset) override;
	virtual ~TrackDetailsForm() override
	{
		if (pAlbumCoverTexture)
		{
			delete pAlbumCoverTexture;
			pAlbumCoverTexture = nullptr;
		}
	}

	bool loadImage(const std::string& imagePath)
	{
		if (imagePath.empty())
		{
			if (pAlbumCoverTexture)
			{
				delete pAlbumCoverTexture;
				pAlbumCoverTexture = nullptr;
			}
			album_cover_path.clear();
			return true;
		}

		if (imagePath == album_cover_path)
		{
			// already loaded
			return true;
		}

		if (pAlbumCoverTexture)
		{
			delete pAlbumCoverTexture;
			pAlbumCoverTexture = nullptr;
		}
		pAlbumCoverTexture = loadImageToTexture(imagePath);
		if (!pAlbumCoverTexture)
		{
			album_cover_path.clear();
			return false;
		}
		album_cover_path = imagePath;
		return true;
	}

private:
	gfx_api::texture* pAlbumCoverTexture = nullptr;
	std::string album_cover_path;
};

#define WZ_TRACKDETAILS_IMAGE_X	(20 + 210 + 10)
#define WZ_TRACKDETAILS_IMAGE_Y 10
#define WZ_TRACKDETAILS_IMAGE_SIZE 60

void TrackDetailsForm::display(int xOffset, int yOffset)
{
	IntFormAnimated::display(xOffset, yOffset);

	if (disableChildren) { return; }

	// now draw the album cover, if present
	int imageLeft = x() + xOffset + WZ_TRACKDETAILS_IMAGE_X;
	int imageTop = y() + yOffset + WZ_TRACKDETAILS_IMAGE_Y;

	if (pAlbumCoverTexture)
	{
		iV_DrawImageAnisotropic(*pAlbumCoverTexture, Vector2i(imageLeft, imageTop), Vector2f(0,0), Vector2f(WZ_TRACKDETAILS_IMAGE_SIZE, WZ_TRACKDETAILS_IMAGE_SIZE), 0.f, WZCOL_WHITE);
	}
}

static void UpdateTrackDetailsBox(TrackDetailsForm *pTrackDetailsBox)
{
	ASSERT_OR_RETURN(, pTrackDetailsBox, "pTrackDetailsBox is null");

	// Add "Now Playing" label
	if (!psNowPlaying)
	{
		pTrackDetailsBox->attach(psNowPlaying = std::make_shared<W_LABEL>());
	}
	psNowPlaying->setGeometry(20, 12, 210, 20);
	psNowPlaying->setFont(font_regular, WZCOL_TEXT_MEDIUM);
	psNowPlaying->setString((selectedTrack) ? (WzString::fromUtf8(_("NOW PLAYING")) + ":") : "");
	psNowPlaying->setTextAlignment(WLAB_ALIGNTOPLEFT);
	// set a custom hit-testing function that ignores all mouse input / clicks
	psNowPlaying->setCustomHitTest([](WIDGET *psWidget, int x, int y) -> bool { return false; });

	// Add Selected Track name
	if (!psSelectedTrackName)
	{
		pTrackDetailsBox->attach(psSelectedTrackName = std::make_shared<W_LABEL>());
	}
	psSelectedTrackName->setGeometry(20, 12 + 20, 210, 20);
	psSelectedTrackName->setFont(font_regular_bold, WZCOL_TEXT_BRIGHT);
	psSelectedTrackName->setString((selectedTrack) ? WzString::fromUtf8(selectedTrack->title) : "");
	psSelectedTrackName->setTextAlignment(WLAB_ALIGNTOPLEFT);
	// set a custom hit-testing function that ignores all mouse input / clicks
	psSelectedTrackName->setCustomHitTest([](WIDGET *psWidget, int x, int y) -> bool { return false; });

	// Add Selected Track author details
	if (!psSelectedTrackAuthorName)
	{
		pTrackDetailsBox->attach(psSelectedTrackAuthorName = std::make_shared<W_LABEL>());
	}
	psSelectedTrackAuthorName->setGeometry(20, 12 + 20 + 17, 210, 20);
	psSelectedTrackAuthorName->setFont(font_regular, WZCOL_TEXT_BRIGHT);
	psSelectedTrackAuthorName->setString((selectedTrack) ? WzString::fromUtf8(selectedTrack->author) : "");
	psSelectedTrackAuthorName->setTextAlignment(WLAB_ALIGNTOPLEFT);
	// set a custom hit-testing function that ignores all mouse input / clicks
	psSelectedTrackAuthorName->setCustomHitTest([](WIDGET *psWidget, int x, int y) -> bool { return false; });

	// album info xPosStart
	int albumInfoXPosStart = psNowPlaying->x() + psNowPlaying->width() + 10 + WZ_TRACKDETAILS_IMAGE_SIZE + 10;
	int maxWidthOfAlbumLabel = TL_W - albumInfoXPosStart - 20;
	std::shared_ptr<const WZ_ALBUM> pAlbum = (selectedTrack) ? selectedTrack->album.lock() : nullptr;

	if (!psSelectedTrackAlbumName)
	{
		pTrackDetailsBox->attach(psSelectedTrackAlbumName = std::make_shared<W_LABEL>());
	}
	psSelectedTrackAlbumName->setGeometry(albumInfoXPosStart, 12, maxWidthOfAlbumLabel, 20);
	psSelectedTrackAlbumName->setFont(font_small, WZCOL_TEXT_BRIGHT);
	psSelectedTrackAlbumName->setString((pAlbum) ? (WzString::fromUtf8(_("Album")) + ": " + WzString::fromUtf8(pAlbum->title)) : "");
	psSelectedTrackAlbumName->setTextAlignment(WLAB_ALIGNTOPLEFT);
	// set a custom hit-testing function that ignores all mouse input / clicks
	psSelectedTrackAlbumName->setCustomHitTest([](WIDGET *psWidget, int x, int y) -> bool { return false; });

	if (!psSelectedTrackAlbumDate)
	{
		pTrackDetailsBox->attach(psSelectedTrackAlbumDate = std::make_shared<W_LABEL>());
	}
	psSelectedTrackAlbumDate->setGeometry(albumInfoXPosStart, 12 + 13, maxWidthOfAlbumLabel, 20);
	psSelectedTrackAlbumDate->setFont(font_small, WZCOL_TEXT_BRIGHT);
	psSelectedTrackAlbumDate->setString((pAlbum) ? (WzString::fromUtf8(_("Date")) + ": " + WzString::fromUtf8(pAlbum->date)) : "");
	psSelectedTrackAlbumDate->setTextAlignment(WLAB_ALIGNTOPLEFT);
	// set a custom hit-testing function that ignores all mouse input / clicks
	psSelectedTrackAlbumDate->setCustomHitTest([](WIDGET *psWidget, int x, int y) -> bool { return false; });

	if (!psSelectedTrackAlbumDescription)
	{
		pTrackDetailsBox->attach(psSelectedTrackAlbumDescription = std::make_shared<W_LABEL>());
	}
	psSelectedTrackAlbumDescription->setGeometry(albumInfoXPosStart, 12 + 13 + 15, maxWidthOfAlbumLabel, 20);
	psSelectedTrackAlbumDescription->setFont(font_small, WZCOL_TEXT_BRIGHT);
	int heightOfTitleLabel =  psSelectedTrackAlbumDescription->setFormattedString((pAlbum) ? WzString::fromUtf8(pAlbum->description) : "", maxWidthOfAlbumLabel, font_small);
	psSelectedTrackAlbumDescription->setGeometry(albumInfoXPosStart, 12 + 13 + 15, maxWidthOfAlbumLabel, heightOfTitleLabel);
	psSelectedTrackAlbumDescription->setTextAlignment(WLAB_ALIGNTOPLEFT);
	// set a custom hit-testing function that ignores all mouse input / clicks
	psSelectedTrackAlbumDescription->setCustomHitTest([](WIDGET *psWidget, int x, int y) -> bool { return false; });

	pTrackDetailsBox->loadImage((pAlbum) ? pAlbum->album_cover_filename : "");
}

static void addTrackDetailsBox(WIDGET *parent, bool ingame)
{
	if (widgGetFromID(psWScreen, MULTIOP_CONSOLEBOX))
	{
		return;
	}

	auto pTrackDetailsBox = std::make_shared<TrackDetailsForm>();
	parent->attach(pTrackDetailsBox);
	pTrackDetailsBox->id = MULTIOP_CONSOLEBOX;
	if (!ingame)
	{
		pTrackDetailsBox->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			psWidget->setGeometry(TL_X, GetDetailsBoxStartYPos(), TL_W, TL_PREVIEWBOX_H);
		}));
	}
	else
	{
		pTrackDetailsBox->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			psWidget->setGeometry(((300-(TL_W/2))+D_W), ((240-(GetTotalTrackAndDetailsBoxHeight()/2))+D_H) + GetDetailsBoxStartYPos(), TL_W, TL_PREVIEWBOX_H);
		}));
	}

	UpdateTrackDetailsBox(pTrackDetailsBox.get());
}

// MARK: - Track List

class MusicListHeader : public W_FORM
{
public:
	MusicListHeader();
	virtual void display(int xOffset, int yOffset);
};

MusicListHeader::MusicListHeader()
	: W_FORM()
{ }

void MusicListHeader::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int x1 = x0 + width();
	int y1 = y0 + height() - 1;
	iV_TransBoxFill(x0, y0, x1, y1);
	iV_Line(x0, y1, x1, y1, WZCOL_MENU_SEPARATOR);

	// column lines
	iV_Line(x0 + W_TRACK_COL_TITLE_X + W_TRACK_COL_TITLE_W, y0 + 5, x0 + W_TRACK_COL_TITLE_X + W_TRACK_COL_TITLE_W, y1 - 5, WZCOL_MENU_SEPARATOR);
	iV_Line(x0 + W_TRACK_COL_ALBUM_X + W_TRACK_COL_ALBUM_W, y0 + 5, x0 + W_TRACK_COL_ALBUM_X + W_TRACK_COL_ALBUM_W, y1 - 5, WZCOL_MENU_SEPARATOR);
}

class W_MusicListHeaderColImage : public W_BUTTON
{
public:
	W_MusicListHeaderColImage()
	: W_BUTTON()
	{
		AudioCallback = nullptr;
	}
	virtual ~W_MusicListHeaderColImage() override
	{
		if (pAlbumCoverTexture)
		{
			delete pAlbumCoverTexture;
			pAlbumCoverTexture = nullptr;
		}
	}

	bool loadImage(const std::string& imagePath)
	{
		if (pAlbumCoverTexture)
		{
			delete pAlbumCoverTexture;
			pAlbumCoverTexture = nullptr;
		}
		pAlbumCoverTexture = loadImageToTexture(imagePath);
		return pAlbumCoverTexture != nullptr;
	}

	void display(int xOffset, int yOffset) override
	{
		if (pAlbumCoverTexture)
		{
			int imageLeft = x() + xOffset;
			int imageTop = y() + yOffset;

			iV_DrawImageAnisotropic(*pAlbumCoverTexture, Vector2i(imageLeft, imageTop), Vector2f(0,0), Vector2f(W_TRACK_HEADER_COL_IMAGE_SIZE, W_TRACK_HEADER_COL_IMAGE_SIZE), 0.f, WZCOL_WHITE);
		}
	}
private:
	gfx_api::texture* pAlbumCoverTexture = nullptr;
};

static const std::string music_mode_col_header_images[] = {
	"images/frontend/image_music_campaign.png",
	"images/frontend/image_music_challenges.png",
	"images/frontend/image_music_skirmish.png",
	"images/frontend/image_music_multiplayer.png"
};

static void addTrackList(WIDGET *parent, bool ingame)
{
	auto pHeader = std::make_shared<MusicListHeader>();
	parent->attach(pHeader);
	pHeader->setGeometry(GetTrackListStartXPos(ingame), W_TRACK_HEADER_Y, TL_ENTRYW, W_TRACK_HEADER_HEIGHT);

	const int headerColY = W_TRACK_ROW_PADDING + (W_TRACK_ROW_PADDING / 2);

	auto title_header = std::make_shared<W_LABEL>();
	pHeader->attach(title_header);
	title_header->setGeometry(W_TRACK_COL_TITLE_X, headerColY, W_TRACK_COL_TITLE_W, W_TRACK_HEADER_HEIGHT);
	title_header->setFontColour(WZCOL_TEXT_MEDIUM);
	title_header->setString(_("Title"));
	title_header->setTextAlignment(WLAB_ALIGNTOPLEFT);
	// set a custom hit-testing function that ignores all mouse input / clicks
	title_header->setCustomHitTest([](WIDGET *psWidget, int x, int y) -> bool { return false; });

	auto album_header = std::make_shared<W_LABEL>();
	pHeader->attach(album_header);
	album_header->setGeometry(W_TRACK_COL_ALBUM_X, headerColY, W_TRACK_COL_ALBUM_W, W_TRACK_HEADER_HEIGHT);
	album_header->setFontColour(WZCOL_TEXT_MEDIUM);
	album_header->setString(_("Album"));
	album_header->setTextAlignment(WLAB_ALIGNTOPLEFT);
	// set a custom hit-testing function that ignores all mouse input / clicks
	album_header->setCustomHitTest([](WIDGET *psWidget, int x, int y) -> bool { return false; });

	for (int musicModeIdx = 0; musicModeIdx < NUM_MUSICGAMEMODES; musicModeIdx++)
	{
		const std::string& headerImage = music_mode_col_header_images[musicModeIdx];
		auto musicModeHeader = std::make_shared<W_MusicListHeaderColImage>();
		pHeader->attach(musicModeHeader);
		musicModeHeader->loadImage(headerImage);
		musicModeHeader->setGeometry(W_TRACK_CHECKBOX_STARTINGPOS - 2 + ((W_TRACK_CHECKBOX_SIZE + W_TRACK_COL_PADDING) * musicModeIdx), headerColY, W_TRACK_HEADER_COL_IMAGE_SIZE, W_TRACK_HEADER_HEIGHT);
		std::string musicGameModeStr = to_string(static_cast<MusicGameMode>(musicModeIdx));
		musicModeHeader->setTip(musicGameModeStr);
	}

	auto pTracksScrollableList = ScrollableListWidget::make();
	parent->attach(pTracksScrollableList);
	pTracksScrollableList->setBackgroundColor(WZCOL_TRANSPARENT_BOX);
	pTracksScrollableList->setCalcLayout([ingame](WIDGET *psWidget) {
		psWidget->setGeometry(GetTrackListStartXPos(ingame), TL_Y, TL_ENTRYW, GetNumVisibleTracks() * TL_ENTRYH);
	});

	size_t tracksCount = trackList.size();
	W_BUTINIT emptyInit;
	for (size_t i = 0; i < tracksCount; i++)
	{
		auto pTrackRow = W_TrackRow::make(&emptyInit, trackList[i], ingame);
		pTrackRow->setGeometry(0, 0, TL_ENTRYW, TL_ENTRYH);
		pTrackRow->addOnClickHandler([](W_BUTTON& clickedButton) {
			W_TrackRow& pTrackRow = dynamic_cast<W_TrackRow&>(clickedButton);
			if (selectedTrack == pTrackRow.getTrack()) { return; }
			cdAudio_PlaySpecificTrack(pTrackRow.getTrack());
		});
		pTracksScrollableList->addItem(pTrackRow);
	}
}

// MARK: - "Now Playing" Details Block

static void closeMusicManager(bool ingame)
{
	trackList.clear();
	trackRowsCache.clear();
	selectedTrack.reset();
	if (musicManagerAudioEventSink)
	{
		musicManagerAudioEventSink->setUnregisterEventSink();
		musicManagerAudioEventSink.reset();
	}
	if (!ingame)
	{
		cdAudio_PlayTrack(cdAudio_CurrentSongContext());
	}

	psNowPlaying = nullptr;
	psSelectedTrackName = nullptr;
	psSelectedTrackAuthorName = nullptr;
	psSelectedTrackAlbumName = nullptr;
	psSelectedTrackAlbumDate = nullptr;
	psSelectedTrackAlbumDescription = nullptr;
}

class MusicManagerForm : public IntFormTransparent
{
protected:
	explicit MusicManagerForm(bool ingame)
	: IntFormTransparent()
	, ingame(ingame) {}

	void initialize()
	{
		this->id = MM_FORM;
		this->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			psWidget->setGeometry(0, 0, pie_GetVideoBufferWidth(), pie_GetVideoBufferHeight());
		}));

		// draws the background of the form
		auto botForm = std::make_shared<IntFormAnimated>();
		this->attach(botForm);
		botForm->id = MM_FORM + 1;

		if (!ingame)
		{
			botForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
				psWidget->setGeometry(TL_X, 20, TL_W, GetTrackListHeight());
			}));

			// cancel
			addMultiBut(*botForm, MM_RETURN, 10, 5, MULTIOP_OKW, MULTIOP_OKH, _("Return To Previous Screen"),
					IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);
		}
		else
		{
			// Text versions for in-game where image resources are not available
			botForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
				psWidget->setGeometry(((300-(TL_W/2))+D_W), ((240-(GetTotalTrackAndDetailsBoxHeight()/2))+D_H), TL_W, GetTrackListHeight() + 20);
			}));

			addButton(botForm, _("Go Back"), MM_GO_BACK, GetTrackListHeight() - 20);
			addButton(botForm, _("Resume Game"), MM_RETURN, GetTrackListHeight());
		}

		// get track list
		trackList = PlayList_GetFullTrackList();
		selectedTrack = cdAudio_GetCurrentTrack();

		// register for cd audio events
		musicManagerAudioEventSink = std::make_shared<MusicManager_CDAudioEventSink>();
		cdAudio_RegisterForEvents(std::static_pointer_cast<CDAudioEventSink>(musicManagerAudioEventSink));

		addTrackList(botForm.get(), ingame);
		addTrackDetailsBox(this, ingame);
	}

	static void addButton(const std::shared_ptr<IntFormAnimated> &botForm, const char *text, int id, int y)
	{
		W_BUTINIT sButInit;

		sButInit.formID		= botForm->id;
		sButInit.style		= WBUT_PLAIN | WBUT_TXTCENTRE;
		sButInit.width		= TL_W;
		sButInit.FontID		= font_regular;
		sButInit.x			= 0;
		sButInit.height		= 10;
		sButInit.pDisplay	= displayTextOption;
		sButInit.initPUserDataFunc = []() -> void * { return new DisplayTextOptionCache(); };
		sButInit.onDelete = [](WIDGET *psWidget) {
			assert(psWidget->pUserData != nullptr);
			delete static_cast<DisplayTextOptionCache *>(psWidget->pUserData);
			psWidget->pUserData = nullptr;
		};

		sButInit.id			= id;
		sButInit.y			= y;
		sButInit.pText		= text;
		sButInit.calcLayout = [y] (WIDGET *psWidget) {
			psWidget->move(0, y);
		};

		botForm->attach(std::make_shared<W_BUTTON>(&sButInit));
	}

public:
	static std::shared_ptr<MusicManagerForm> make(bool ingame)
	{
		class make_shared_enabler: public MusicManagerForm
		{
		public:
			explicit make_shared_enabler(bool ingame): MusicManagerForm(ingame) {}
		};
		auto widget = std::make_shared<make_shared_enabler>(ingame);
		widget->initialize();
		return widget;
	}

	~MusicManagerForm() override
	{
		closeMusicManager(ingame);
	}
private:
	bool ingame;
};

static bool musicManager(WIDGET *parent, bool ingame)
{
	parent->attach(MusicManagerForm::make(ingame));
	return true;
}

bool startInGameMusicManager(InputManager& inputManager)
{
	inputManager.contexts().pushState();
	inputManager.contexts().makeAllInactive();
	return musicManager(psWScreen->psForm.get(), true);
}

bool startMusicManager()
{
	addBackdrop();	//background image
	addSideText(FRONTEND_SIDETEXT, TL_SX, 20, _("MUSIC MANAGER"));
	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BACKDROP);
	return musicManager(parent, false);
}

static void perFrameCleanup()
{
	UDWORD currentFrameNum = frameGetFrameNumber();
	for (auto i = trackRowsCache.begin(), last = trackRowsCache.end(); i != last; )
	{
		if (currentFrameNum - i->second->lastUsedFrameNumber > 1)
		{
			i = trackRowsCache.erase(i);
		}
		else
		{
			++i;
		}
	}
}

bool runInGameMusicManager(unsigned id, InputManager& inputManager)
{
	if (id == MM_RETURN)			// return
	{
		inputManager.contexts().popState();
		widgDelete(psWScreen, MM_FORM);
		inputLoseFocus();
		return true;
	}
	else if (id == MM_GO_BACK)
	{
		inputManager.contexts().popState();
		widgDelete(psWScreen, MM_FORM);
		intReopenMenuWithoutUnPausing();
	}

	perFrameCleanup();

	return false;
}

bool runMusicManager()
{
	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	if (id == MM_RETURN)			// return
	{
		changeTitleMode(OPTIONS);
	}

	widgDisplayScreen(psWScreen);				// show the widgets currently running

	perFrameCleanup();

	if (CancelPressed())
	{
		changeTitleMode(OPTIONS);
	}

	return true;
}

// MARK: - CD Audio Event Sink Implementation

static void CDAudioEvent_UpdateCurrentTrack(const std::shared_ptr<const WZ_TRACK>& track)
{
	if (selectedTrack == track)
	{
		return;
	}
	selectedTrack = track;
	if (!psWScreen) { return; }
	auto psTrackDetailsForm = dynamic_cast<TrackDetailsForm *>(widgGetFromID(psWScreen, MULTIOP_CONSOLEBOX));
	if (psTrackDetailsForm)
	{
		UpdateTrackDetailsBox(psTrackDetailsForm);
	}
}

void MusicManager_CDAudioEventSink::startedPlayingTrack(const std::shared_ptr<const WZ_TRACK>& track)
{
	CDAudioEvent_UpdateCurrentTrack(track);
}
void MusicManager_CDAudioEventSink::trackEnded(const std::shared_ptr<const WZ_TRACK>& track)
{
	// currently a no-op
}

void MusicManager_CDAudioEventSink::musicStopped()
{
	CDAudioEvent_UpdateCurrentTrack(nullptr);
}

void MusicManager_CDAudioEventSink::musicPaused(const std::shared_ptr<const WZ_TRACK>& track)
{
	CDAudioEvent_UpdateCurrentTrack(track);
}

void MusicManager_CDAudioEventSink::musicResumed(const std::shared_ptr<const WZ_TRACK>& track)
{
	CDAudioEvent_UpdateCurrentTrack(track);
}
