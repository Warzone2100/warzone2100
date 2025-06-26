// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2020-2025  Warzone 2100 Project

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
#include "lib/widget/table.h"
#include "lib/widget/margin.h"
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
#define TL_SX				FRONTEND_SIDEX

#define TL_ENTRYW			(FRONTEND_BOTFORMW - 80)
#define TL_ENTRYH			(25)

#define TL_PREVIEWBOX_Y_SPACING 45
#define TL_PREVIEWBOX_H		84

class MusicManagerForm;

// MARK: - MusicManager_CDAudioEventSink declaration
class MusicManager_CDAudioEventSink : public CDAudioEventSink
{
public:
	MusicManager_CDAudioEventSink(const std::shared_ptr<MusicManagerForm>& musicManagerForm);
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
	void CDAudioEvent_UpdateCurrentTrack(const std::shared_ptr<const WZ_TRACK>& track);
private:
	std::weak_ptr<MusicManagerForm> weakMusicManagerForm;
	bool shouldUnregisterEventSink = false;
};


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

class TrackDetailsForm : public W_FORM
{
protected:
	TrackDetailsForm()
	: W_FORM()
	{ }
	void initialize();
public:
	static std::shared_ptr<TrackDetailsForm> make();

	virtual void display(int xOffset, int yOffset) override;
	virtual void geometryChanged() override;

	virtual ~TrackDetailsForm() override
	{
		if (pAlbumCoverTexture)
		{
			delete pAlbumCoverTexture;
			pAlbumCoverTexture = nullptr;
		}
	}

	void displayTrack(const std::shared_ptr<const WZ_TRACK>& track);

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

	std::shared_ptr<W_LABEL> psNowPlaying;
	std::shared_ptr<W_LABEL> psSelectedTrackName;
	std::shared_ptr<W_LABEL> psSelectedTrackAuthorName;
	std::shared_ptr<W_LABEL> psSelectedTrackAlbumName;
	std::shared_ptr<W_LABEL> psSelectedTrackAlbumDate;
	std::shared_ptr<ScrollableListWidget> descriptionContainer;

	int32_t outerHorizontalPadding = 20;
};

#define WZ_TRACKDETAILS_IMAGE_Y 12
#define WZ_TRACKDETAILS_IMAGE_SIZE 60

std::shared_ptr<TrackDetailsForm> TrackDetailsForm::make()
{
	class make_shared_enabler: public TrackDetailsForm { };
	auto widget = std::make_shared<make_shared_enabler>();
	widget->initialize();
	return widget;
}

void TrackDetailsForm::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int w = width();
	int h = height();

	pie_UniTransBoxFill(x0, y0, x0 + w, y0 + h, pal_RGBA(0,0,0,100));

	if (disableChildren) { return; }

	// now draw the album cover, if present
	int imageLeft = x0 + outerHorizontalPadding + psNowPlaying->width() + 10;
	int imageTop = y0 + WZ_TRACKDETAILS_IMAGE_Y;

	if (pAlbumCoverTexture)
	{
		iV_DrawImageAnisotropic(*pAlbumCoverTexture, Vector2i(imageLeft, imageTop), Vector2f(0,0), Vector2f(WZ_TRACKDETAILS_IMAGE_SIZE, WZ_TRACKDETAILS_IMAGE_SIZE), 0.f, WZCOL_WHITE);
	}
}

void TrackDetailsForm::geometryChanged()
{
	int w = width();
	int totalUsableTextWidth = w - (outerHorizontalPadding * 2) - 10 - WZ_TRACKDETAILS_IMAGE_SIZE - 10;
	int leftSideWidth = (totalUsableTextWidth / 2) - 10;

	psNowPlaying->setGeometry(outerHorizontalPadding, 14, leftSideWidth, 20);
	psSelectedTrackName->setGeometry(outerHorizontalPadding, 14 + 20, leftSideWidth, 20);
	psSelectedTrackAuthorName->setGeometry(outerHorizontalPadding, 14 + 20 + 17, leftSideWidth, 20);

	int albumInfoXPosStart = psNowPlaying->x() + psNowPlaying->width() + 10 + WZ_TRACKDETAILS_IMAGE_SIZE + 10;
	int maxWidthOfAlbumLabel = width() - albumInfoXPosStart - outerHorizontalPadding;

	psSelectedTrackAlbumName->setGeometry(albumInfoXPosStart, 14, maxWidthOfAlbumLabel, 20);
	psSelectedTrackAlbumDate->setGeometry(albumInfoXPosStart, 14 + 13, maxWidthOfAlbumLabel, 20);

	int descriptionContainerY0 = 12 + 13 + 18;
	int descriptionContainerHeight = height() - descriptionContainerY0 - 12;
	descriptionContainer->setGeometry(albumInfoXPosStart, descriptionContainerY0, maxWidthOfAlbumLabel, descriptionContainerHeight);
}

void TrackDetailsForm::initialize()
{
	psNowPlaying = std::make_shared<W_LABEL>();
	attach(psNowPlaying);
	psNowPlaying->setGeometry(outerHorizontalPadding, 14, 210, 20);
	psNowPlaying->setFont(font_regular, WZCOL_TEXT_MEDIUM);
	psNowPlaying->setTextAlignment(WLAB_ALIGNTOPLEFT);
	psNowPlaying->setTransparentToMouse(true);

	psSelectedTrackName = std::make_shared<W_LABEL>();
	attach(psSelectedTrackName);
	psSelectedTrackName->setGeometry(outerHorizontalPadding, 14 + 20, 210, 20);
	psSelectedTrackName->setFont(font_regular_bold, WZCOL_TEXT_BRIGHT);
	psSelectedTrackName->setTextAlignment(WLAB_ALIGNTOPLEFT);
	psSelectedTrackName->setCanTruncate(true);
	psSelectedTrackName->setTransparentToMouse(true);

	// Add Selected Track author details
	psSelectedTrackAuthorName = std::make_shared<W_LABEL>();
	attach(psSelectedTrackAuthorName);
	psSelectedTrackAuthorName->setGeometry(outerHorizontalPadding, 14 + 20 + 17, 210, 20);
	psSelectedTrackAuthorName->setFont(font_regular, WZCOL_TEXT_BRIGHT);
	psSelectedTrackAuthorName->setTextAlignment(WLAB_ALIGNTOPLEFT);
	psSelectedTrackAuthorName->setCanTruncate(true);
	psSelectedTrackAuthorName->setTransparentToMouse(true);

	// album info xPosStart
	int albumInfoXPosStart = psNowPlaying->x() + psNowPlaying->width() + 10 + WZ_TRACKDETAILS_IMAGE_SIZE + 10;
	int maxWidthOfAlbumLabel = TL_W - albumInfoXPosStart - outerHorizontalPadding;

	psSelectedTrackAlbumName = std::make_shared<W_LABEL>();
	attach(psSelectedTrackAlbumName);
	psSelectedTrackAlbumName->setGeometry(albumInfoXPosStart, 14, maxWidthOfAlbumLabel, 20);
	psSelectedTrackAlbumName->setFont(font_small, WZCOL_TEXT_BRIGHT);
	psSelectedTrackAlbumName->setTextAlignment(WLAB_ALIGNTOPLEFT);
	psSelectedTrackAlbumName->setCanTruncate(true);
	psSelectedTrackAlbumName->setTransparentToMouse(true);

	psSelectedTrackAlbumDate = std::make_shared<W_LABEL>();
	attach(psSelectedTrackAlbumDate);
	psSelectedTrackAlbumDate->setGeometry(albumInfoXPosStart, 14 + 13, maxWidthOfAlbumLabel, 20);
	psSelectedTrackAlbumDate->setFont(font_small, WZCOL_TEXT_BRIGHT);
	psSelectedTrackAlbumDate->setTextAlignment(WLAB_ALIGNTOPLEFT);
	psSelectedTrackAlbumDate->setCanTruncate(true);
	psSelectedTrackAlbumDate->setTransparentToMouse(true);

	descriptionContainer = ScrollableListWidget::make();
	attach(descriptionContainer);
	descriptionContainer->setGeometry(albumInfoXPosStart, 14 + 13 + 18, maxWidthOfAlbumLabel, 20);
}

void TrackDetailsForm::displayTrack(const std::shared_ptr<const WZ_TRACK>& track)
{
	psNowPlaying->setString((track) ? (WzString::fromUtf8(_("NOW PLAYING")) + ":") : "");
	psSelectedTrackName->setString((track) ? WzString::fromUtf8(track->title) : "");
	psSelectedTrackAuthorName->setString((track) ? WzString::fromUtf8(track->author) : "");

	std::shared_ptr<const WZ_ALBUM> pAlbum = (track) ? track->album.lock() : nullptr;

	psSelectedTrackAlbumName->setString((pAlbum) ? (WzString::fromUtf8(_("Album")) + ": " + WzString::fromUtf8(pAlbum->title)) : "");
	psSelectedTrackAlbumDate->setString((pAlbum) ? (WzString::fromUtf8(_("Date")) + ": " + WzString::fromUtf8(pAlbum->date)) : "");

	descriptionContainer->clear();
	if (pAlbum)
	{
		auto psSelectedTrackAlbumDescription = std::make_shared<Paragraph>();
		psSelectedTrackAlbumDescription->setGeometry(0, 0, descriptionContainer->width(), 20);
		psSelectedTrackAlbumDescription->setFont(font_small);
		psSelectedTrackAlbumDescription->setFontColour(WZCOL_TEXT_MEDIUM);
		psSelectedTrackAlbumDescription->setTransparentToMouse(true);
		psSelectedTrackAlbumDescription->addText(WzString::fromUtf8(pAlbum->description));
		descriptionContainer->addItem(psSelectedTrackAlbumDescription);
	}

	loadImage((pAlbum) ? pAlbum->album_cover_filename : "");
}

// MARK: - Track List

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

	int32_t idealWidth() override
	{
		return 16;
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

// MARK: - MusicManagerForm

class MusicManagerForm : public W_FORM
{
protected:
	explicit MusicManagerForm(bool ingame)
	: W_FORM()
	, ingame(ingame) {}

	void initialize()
	{
		// get track list
		trackList = PlayList_GetFullTrackList();
		selectedTrack = cdAudio_GetCurrentTrack();

		addTrackList(this);
		addTrackDetailsBox();

		updateActiveTrackImpl(selectedTrack);

		// register for cd audio events
		musicManagerAudioEventSink = std::make_shared<MusicManager_CDAudioEventSink>(std::static_pointer_cast<MusicManagerForm>(shared_from_this()));
		cdAudio_RegisterForEvents(std::static_pointer_cast<CDAudioEventSink>(musicManagerAudioEventSink));
	}

	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;
	virtual void geometryChanged() override;
	virtual void display(int xOffset, int yOffset) override;

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

	virtual ~MusicManagerForm();

	void updateActiveTrack(const std::shared_ptr<const WZ_TRACK>& track);

private:
	void addTrackList(WIDGET* parent);
	void addTrackDetailsBox();
	void updateActiveTrackImpl(const std::shared_ptr<const WZ_TRACK>& track);
	std::shared_ptr<WIDGET> createColumnHeaderLabel(const char* text, int minLeftPadding);
	std::shared_ptr<TableRow> createTrackRow(const std::shared_ptr<const WZ_TRACK>& track);
private:
	std::vector<std::shared_ptr<const WZ_TRACK>> trackList;
	std::shared_ptr<const WZ_TRACK> selectedTrack;
	std::shared_ptr<ScrollableTableWidget> table;
	std::shared_ptr<TrackDetailsForm> trackDetailsBox;
	std::shared_ptr<MusicManager_CDAudioEventSink> musicManagerAudioEventSink;
	int32_t minDesiredTableWidth = 0;
	bool ingame;
	const int32_t tablePaddingX = 6;
};

MusicManagerForm::~MusicManagerForm()
{
	if (musicManagerAudioEventSink)
	{
		musicManagerAudioEventSink->setUnregisterEventSink();
		musicManagerAudioEventSink.reset();
	}
	if (!ingame)
	{
		cdAudio_PlayTrack(cdAudio_CurrentSongContext());
	}
}

void MusicManagerForm::geometryChanged()
{
	int w = width();
	int h = height();

	if (w == 0 || h == 0)
	{
		return;
	}

	int trackDetailsBoxHeight = TL_PREVIEWBOX_H;
	int trackDetailsBoxY0 = h - trackDetailsBoxHeight;
	trackDetailsBox->setGeometry(0, trackDetailsBoxY0, w, trackDetailsBoxHeight);

	int tracksTableX0 = tablePaddingX;
	int tracksTableY0 = 0;
	int tracksTableHeight = trackDetailsBoxY0 - tracksTableY0 - 5;
	int tracksTableWidth = w - (tablePaddingX * 2);
	table->setGeometry(tracksTableX0, tracksTableY0, tracksTableWidth, tracksTableHeight);

	table->setMinimumColumnWidths(table->getMinimumColumnWidths());
}

int32_t MusicManagerForm::idealWidth()
{
	return std::max<int32_t>(table->idealWidth(), (minDesiredTableWidth + (tablePaddingX * 2) + 100));
}

int32_t MusicManagerForm::idealHeight()
{
	return trackDetailsBox->idealHeight() + 400;
}

void MusicManagerForm::display(int xOffset, int yOffset)
{
	// currently, no-op
}

void MusicManagerForm::updateActiveTrack(const std::shared_ptr<const WZ_TRACK>& track)
{
	if (track == selectedTrack)
	{
		return;
	}
	updateActiveTrackImpl(track);
}

void MusicManagerForm::updateActiveTrackImpl(const std::shared_ptr<const WZ_TRACK>& track)
{
	selectedTrack = track;

	trackDetailsBox->displayTrack(track);

	auto it = std::find(trackList.begin(), trackList.end(), track);
	if (it == trackList.end())
	{
		return;
	}
	auto trackRowIndex = std::distance(trackList.begin(), it);
	const auto& tableRows = table->getRows();
	ASSERT(trackRowIndex < tableRows.size(), "Invalid row index: %zu", trackRowIndex);
	for (size_t i = 0; i < tableRows.size(); ++i)
	{
		if (i == trackRowIndex)
		{
			tableRows[i]->setBackgroundColor(WZCOL_KEYMAP_ACTIVE);
			tableRows[i]->setHighlightsOnMouseOver(false);
			continue;
		}
		tableRows[i]->setBackgroundColor(nullopt);
		tableRows[i]->setHighlightsOnMouseOver(true);
	}
}

std::shared_ptr<WIDGET> MusicManagerForm::createColumnHeaderLabel(const char* text, int minLeftPadding)
{
	auto label = std::make_shared<W_LABEL>();
	label->setTextAlignment(WLAB_ALIGNTOPLEFT);
	label->setFontColour(WZCOL_TEXT_MEDIUM);
	label->setString(text);
	label->setGeometry(0, 0, label->getMaxLineWidth(), 0);
	label->setCacheNeverExpires(true);
	label->setCanTruncate(true);
	return Margin(W_TRACK_ROW_PADDING + (W_TRACK_ROW_PADDING / 2), 0, 0, minLeftPadding).wrap(label);
}

void MusicManagerForm::addTrackList(WIDGET* parent)
{
	// Create column headers for tracks listing
	std::vector<TableColumn> columns;
	columns.push_back({createColumnHeaderLabel(_("Title"), 0), TableColumn::ResizeBehavior::RESIZABLE_AUTOEXPAND});
	columns.push_back({createColumnHeaderLabel(_("Album"), 0), TableColumn::ResizeBehavior::RESIZABLE_AUTOEXPAND});

	for (int musicModeIdx = 0; musicModeIdx < NUM_MUSICGAMEMODES; musicModeIdx++)
	{
		const std::string& headerImage = music_mode_col_header_images[musicModeIdx];
		auto musicModeHeader = std::make_shared<W_MusicListHeaderColImage>();
		musicModeHeader->loadImage(headerImage);
		musicModeHeader->setGeometry(0, 0, W_TRACK_HEADER_COL_IMAGE_SIZE, W_TRACK_HEADER_HEIGHT);
		std::string musicGameModeStr = to_string(static_cast<MusicGameMode>(musicModeIdx));
		musicModeHeader->setTip(musicGameModeStr);
		columns.push_back({Margin(W_TRACK_ROW_PADDING + (W_TRACK_ROW_PADDING / 2), 0, 0, 0).wrap(musicModeHeader), TableColumn::ResizeBehavior::FIXED_WIDTH});
	}

	int maxIdealColumnHeight = W_TRACK_HEADER_HEIGHT;
	std::vector<size_t> minimumColumnWidths = { W_TRACK_COL_TITLE_W, W_TRACK_COL_ALBUM_W };
	for (size_t i = 2; i < columns.size(); ++i)
	{
		minimumColumnWidths.push_back(static_cast<size_t>(std::max<int>(columns[i].columnWidget->idealWidth(), 0)));
	}
	minDesiredTableWidth = std::accumulate(minimumColumnWidths.begin(), minimumColumnWidths.end(), 0);
	minDesiredTableWidth += (W_TRACK_COL_PADDING * 2) + (W_TRACK_COL_PADDING * 2 * (int)minimumColumnWidths.size());

	// Create table
	table = ScrollableTableWidget::make(columns, maxIdealColumnHeight);
	table->setColumnPadding(Vector2i(W_TRACK_COL_PADDING, 0));
	parent->attach(table);
	table->setBackgroundColor(pal_RGBA(0,0,0,0));
	table->setMinimumColumnWidths(minimumColumnWidths);
	table->setDrawColumnLines(false);

	// Add tracks
	auto weakSelf = std::weak_ptr<MusicManagerForm>(std::static_pointer_cast<MusicManagerForm>(shared_from_this()));
	size_t tracksCount = trackList.size();
	W_BUTINIT emptyInit;
	for (size_t trackIdx = 0; trackIdx < tracksCount; trackIdx++)
	{
		auto trackRow = createTrackRow(trackList[trackIdx]);
		trackRow->setHighlightsOnMouseOver(true);
		trackRow->setGeometry(0, 0, TL_ENTRYW, TL_ENTRYH);
		trackRow->addOnClickHandler([weakSelf, trackIdx](W_BUTTON& clickedButton) {
			auto strongSelf = weakSelf.lock();
			ASSERT_OR_RETURN(, strongSelf != nullptr, "Widget gone?");
			if (strongSelf->selectedTrack == strongSelf->trackList[trackIdx]) { return; }
			cdAudio_PlaySpecificTrack(strongSelf->trackList[trackIdx]);
		});
		table->addRow(trackRow);
	}
}

std::shared_ptr<TableRow> MusicManagerForm::createTrackRow(const std::shared_ptr<const WZ_TRACK>& track)
{
	std::vector<std::shared_ptr<WIDGET>> columnWidgets;

	auto album = track->album.lock();
	auto album_name = album->title;

	// Track Title
	auto trackTitleLabel = std::make_shared<W_LABEL>();
	trackTitleLabel->setFont(font_regular, WZCOL_TEXT_BRIGHT);
	trackTitleLabel->setString(WzString::fromUtf8(track->title));
	trackTitleLabel->setGeometry(0, 0, trackTitleLabel->getMaxLineWidth(), trackTitleLabel->requiredHeight());
	trackTitleLabel->setTransparentToMouse(true);
	trackTitleLabel->setCanTruncate(true);
	columnWidgets.push_back(trackTitleLabel);

	// Album Title
	auto albumTitleLabel = std::make_shared<W_LABEL>();
	albumTitleLabel->setFont(font_regular, WZCOL_TEXT_BRIGHT);
	albumTitleLabel->setString(WzString::fromUtf8(album_name));
	albumTitleLabel->setGeometry(0, 0, albumTitleLabel->getMaxLineWidth(), albumTitleLabel->requiredHeight());
	albumTitleLabel->setTransparentToMouse(true);
	albumTitleLabel->setCanTruncate(true);
	columnWidgets.push_back(albumTitleLabel);

	// add music mode checkboxes
	auto musicMode = MusicGameMode::MENUS;
	if (ingame)
	{
		musicMode = PlayList_GetCurrentMusicMode();
	}

	for (int musicModeIdx = 0; musicModeIdx < NUM_MUSICGAMEMODES; musicModeIdx++)
	{
		auto pCheckBox = std::make_shared<W_MusicModeCheckboxButton>(static_cast<MusicGameMode>(musicModeIdx), PlayList_IsTrackEnabledForMusicMode(track, static_cast<MusicGameMode>(musicModeIdx)));
		auto captureTrack = track;
		pCheckBox->addOnClickHandler([captureTrack](W_BUTTON& button) {
			W_MusicModeCheckboxButton& self = dynamic_cast<W_MusicModeCheckboxButton&>(button);
			PlayList_SetTrackMusicMode(captureTrack, self.getMusicMode(), self.getIsChecked());
		});
		pCheckBox->setGeometry(0, 0, W_TRACK_CHECKBOX_SIZE, W_TRACK_CHECKBOX_SIZE);
		pCheckBox->setCheckboxSize(W_TRACK_CHECKBOX_SIZE);
		if (musicMode != MusicGameMode::MENUS)
		{
			if (musicModeIdx != static_cast<int>(musicMode))
			{
				pCheckBox->setState(WBUT_DISABLE);
			}
		}
		columnWidgets.push_back(pCheckBox);
	}

	int rowHeight = TL_ENTRYH;

	auto row = TableRow::make(columnWidgets, rowHeight);
	row->setDisabledColor(WZCOL_FORM_DARK);
	return row;
}

void MusicManagerForm::addTrackDetailsBox()
{
	trackDetailsBox = TrackDetailsForm::make();
	attach(trackDetailsBox);
}

std::shared_ptr<WIDGET> makeMusicManagerForm(bool ingame)
{
	return MusicManagerForm::make(ingame);
}

// MARK: - CD Audio Event Sink Implementation

void MusicManager_CDAudioEventSink::CDAudioEvent_UpdateCurrentTrack(const std::shared_ptr<const WZ_TRACK>& track)
{
	auto psMusicManagerForm = weakMusicManagerForm.lock();
	if (psMusicManagerForm)
	{
		psMusicManagerForm->updateActiveTrack(track);
	}
}

MusicManager_CDAudioEventSink::MusicManager_CDAudioEventSink(const std::shared_ptr<MusicManagerForm>& musicManagerForm)
: weakMusicManagerForm(musicManagerForm)
{ }

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
