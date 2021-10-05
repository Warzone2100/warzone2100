
#include "autiozoomoptions.h"

#include "lib/sound/sounddefs.h"
#include "lib/sound/audio.h"
#include "lib/sound/track.h"
#include "lib/sound/tracklib.h"
#include "lib/sound/mixer.h"

#include "lib/widget/gridlayout.h"
#include "lib/widget/alignment.h"
#include "lib/widget/scrollablelist.h"

#include "graphicsoptions.h"

#include "../../warzoneconfig.h"
#include "../../seqdisp.h"
#include "../../keybind.h"
#include "../../radar.h"

static std::string audioAndZoomOptionsSoundHRTFMode()
{
	// retrieve whether the current "setting" is Auto or not
	// as sound_GetHRTFMode() returns the current actual status (and never auto, even if the request was "auto")
	bool isAutoSetting = war_GetHRTFMode() == HRTFMode::Auto;

	std::string currentMode;
	auto mode = sound_GetHRTFMode();
	switch (mode)
	{
	case HRTFMode::Unsupported:
		currentMode = _("Unsupported");
		return currentMode;
	case HRTFMode::Disabled:
		currentMode = _("Disabled");
		break;
	case HRTFMode::Enabled:
		currentMode = _("Enabled");
		break;
	case HRTFMode::Auto:
		// should not happen, but if it does, just return
		currentMode = _("Auto");
		return currentMode;
	}

	if (isAutoSetting)
	{
		return std::string(_("Auto")) + " (" + currentMode + ")";
	}
	return currentMode;
}

static std::string audioAndZoomOptionsMapZoomString()
{
	char mapZoom[20];
	ssprintf(mapZoom, "%d", war_GetMapZoom());
	return mapZoom;
}

static std::string audioAndZoomOptionsMapZoomRateString()
{
	char mapZoomRate[20];
	ssprintf(mapZoomRate, "%d", war_GetMapZoomRate());
	return mapZoomRate;
}

static std::string audioAndZoomOptionsRadarZoomString()
{
	char radarZoom[20];
	ssprintf(radarZoom, "%d", war_GetRadarZoom());
	return radarZoom;
}

// ////////////////////////////////////////////////////////////////////////////
// Audio and Zoom Options Menu
void startAudioAndZoomOptionsMenu()
{
	addBackdrop();
	addTopForm(false);
	addBottomForm();

	WIDGET* parent = widgGetFromID(psWScreen, FRONTEND_BOTFORM);

	auto addSliderWrap = [](std::shared_ptr<WIDGET> widget) {
		Alignment sliderAlignment(Alignment::Vertical::Center, Alignment::Horizontal::Left);
		return sliderAlignment.wrap(addMargin(widget));
	};

	auto grid = std::make_shared<GridLayout>();
	grid_allocation::slot row(0);

	// 2d audio
	grid->place({ 0 }, row, addMargin(makeTextButton(FRONTEND_FX, _("Voice Volume"), WBUT_SECONDARY)));
	grid->place({ 1, 1, false }, row, addSliderWrap(makeFESlider(FRONTEND_FX_SL, FRONTEND_BOTFORM, AUDIO_VOL_MAX, static_cast<UDWORD>(sound_GetUIVolume() * 100.0f))));
	row.start++;

	// 3d audio
	grid->place({ 0 }, row, addMargin(makeTextButton(FRONTEND_3D_FX, _("FX Volume"), WBUT_SECONDARY)));
	grid->place({ 1, 1, false }, row, addSliderWrap(makeFESlider(FRONTEND_3D_FX_SL, FRONTEND_BOTFORM, AUDIO_VOL_MAX, static_cast<UDWORD>(sound_GetEffectsVolume() * 100.0f))));
	row.start++;

	// cd audio
	grid->place({ 0 }, row, addMargin(makeTextButton(FRONTEND_MUSIC, _("Music Volume"), WBUT_SECONDARY)));
	grid->place({ 1, 1, false }, row, addSliderWrap(makeFESlider(FRONTEND_MUSIC_SL, FRONTEND_BOTFORM, AUDIO_VOL_MAX, static_cast<UDWORD>(sound_GetMusicVolume() * 100.0f))));
	row.start++;

	////////////
	//subtitle mode.
	grid->place({ 0 }, row, addMargin(makeTextButton(FRONTEND_SUBTITLES, _("Subtitles"), WBUT_SECONDARY)));
	grid->place({ 1, 1, false }, row, addMargin(makeTextButton(FRONTEND_SUBTITLES_R, graphicsOptionsSubtitlesString(), WBUT_SECONDARY)));
	row.start++;

	// HRTF
	grid->place({ 0 }, row, addMargin(makeTextButton(FRONTEND_SOUND_HRTF, _("HRTF"), WBUT_SECONDARY)));
	grid->place({ 1, 1, false }, row, addMargin(makeTextButton(FRONTEND_SOUND_HRTF_R, audioAndZoomOptionsSoundHRTFMode(), WBUT_SECONDARY)));
	row.start++;
	if (sound_GetHRTFMode() == HRTFMode::Unsupported)
	{
		widgSetButtonState(psWScreen, FRONTEND_SOUND_HRTF, WBUT_DISABLE);
		widgSetTip(psWScreen, FRONTEND_SOUND_HRTF, _("HRTF is not supported on your device / system / OpenAL library"));
		widgSetButtonState(psWScreen, FRONTEND_SOUND_HRTF_R, WBUT_DISABLE);
		widgSetTip(psWScreen, FRONTEND_SOUND_HRTF_R, _("HRTF is not supported on your device / system / OpenAL library"));
	}

	// map zoom
	grid->place({ 0 }, row, addMargin(makeTextButton(FRONTEND_MAP_ZOOM, _("Map Zoom"), WBUT_SECONDARY)));
	grid->place({ 1, 1, false }, row, addMargin(makeTextButton(FRONTEND_MAP_ZOOM_R, audioAndZoomOptionsMapZoomString(), WBUT_SECONDARY)));
	row.start++;

	// map zoom rate
	grid->place({ 0 }, row, addMargin(makeTextButton(FRONTEND_MAP_ZOOM_RATE, _("Map Zoom Rate"), WBUT_SECONDARY)));
	grid->place({ 1, 1, false }, row, addMargin(makeTextButton(FRONTEND_MAP_ZOOM_RATE_R, audioAndZoomOptionsMapZoomRateString(), WBUT_SECONDARY)));
	row.start++;

	// radar zoom
	grid->place({ 0 }, row, addMargin(makeTextButton(FRONTEND_RADAR_ZOOM, _("Radar Zoom"), WBUT_SECONDARY)));
	grid->place({ 1, 1, false }, row, addMargin(makeTextButton(FRONTEND_RADAR_ZOOM_R, audioAndZoomOptionsRadarZoomString(), WBUT_SECONDARY)));
	row.start++;

	grid->setGeometry(0, 0, FRONTEND_BUTWIDTH, grid->idealHeight());

	auto scrollableList = ScrollableListWidget::make();
	scrollableList->setGeometry(0, FRONTEND_POS2Y, FRONTEND_BOTFORMW - 1, FRONTEND_BOTFORMH - FRONTEND_POS2Y - 1);
	scrollableList->addItem(grid);
	parent->attach(scrollableList);

	// quit.
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);

	//add some text down the side of the form
	// TRANSLATORS: "AUDIO" options determine the volume of game sounds.
	// "OPTIONS" means "SETTINGS".
	// To break this message into two lines, you can use the delimiter "\n",
	// e.g. "AUDIO / ZOOM\nOPTIONS" would show "OPTIONS" in a second line.
	WzString messageString = WzString::fromUtf8(_("AUDIO / ZOOM OPTIONS"));
	std::vector<WzString> messageStringLines = messageString.split("\n");
	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_SIDEY, messageStringLines[0].toUtf8().c_str());
	// show a second sidetext line if the translation requires it
	if (messageStringLines.size() > 1)
	{
		messageString.remove(0, messageStringLines[0].length() + 1);
		addSideText(FRONTEND_MULTILINE_SIDETEXT, FRONTEND_SIDEX + 22, \
			FRONTEND_SIDEY, messageString.toUtf8().c_str());
	}
}

bool runAudioAndZoomOptionsMenu()
{
	WidgetTriggers const& triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	switch (id)
	{
	case FRONTEND_FX:
	case FRONTEND_3D_FX:
	case FRONTEND_MUSIC:
		break;

	case FRONTEND_FX_SL:
		sound_SetUIVolume((float)widgGetSliderPos(psWScreen, FRONTEND_FX_SL) / 100.0f);
		break;

	case FRONTEND_3D_FX_SL:
		sound_SetEffectsVolume((float)widgGetSliderPos(psWScreen, FRONTEND_3D_FX_SL) / 100.0f);
		break;

	case FRONTEND_MUSIC_SL:
		sound_SetMusicVolume((float)widgGetSliderPos(psWScreen, FRONTEND_MUSIC_SL) / 100.0f);
		break;

	case FRONTEND_SUBTITLES:
	case FRONTEND_SUBTITLES_R:
		seq_SetSubtitles(!seq_GetSubtitles());
		widgSetString(psWScreen, FRONTEND_SUBTITLES_R, graphicsOptionsSubtitlesString());
		break;

	case FRONTEND_SOUND_HRTF:
	case FRONTEND_SOUND_HRTF_R:
	{
		std::vector<HRTFMode> modesToCycle = { HRTFMode::Disabled, HRTFMode::Enabled, HRTFMode::Auto };
		auto current = std::find(modesToCycle.begin(), modesToCycle.end(), war_GetHRTFMode());
		if (current == modesToCycle.end())
		{
			current = modesToCycle.begin();
		}
		auto startingPoint = current;
		bool successfulChange = false;
		do {
			current = seqCycle(current, modesToCycle.begin(), 1, modesToCycle.end() - 1);
		} while ((current != startingPoint) && ((successfulChange = sound_SetHRTFMode(*current)) == false));
		if (successfulChange)
		{
			war_SetHRTFMode(*current);
			widgSetString(psWScreen, FRONTEND_SOUND_HRTF_R, audioAndZoomOptionsSoundHRTFMode().c_str());
		}
		break;
	}

	case FRONTEND_MAP_ZOOM:
	case FRONTEND_MAP_ZOOM_R:
	{
		war_SetMapZoom(seqCycle(war_GetMapZoom(), MINDISTANCE_CONFIG, MAP_ZOOM_CONFIG_STEP, MAXDISTANCE));
		widgSetString(psWScreen, FRONTEND_MAP_ZOOM_R, audioAndZoomOptionsMapZoomString().c_str());
		break;
	}

	case FRONTEND_MAP_ZOOM_RATE:
	case FRONTEND_MAP_ZOOM_RATE_R:
	{
		war_SetMapZoomRate(seqCycle(war_GetMapZoomRate(), MAP_ZOOM_RATE_MIN, MAP_ZOOM_RATE_STEP, MAP_ZOOM_RATE_MAX));
		widgSetString(psWScreen, FRONTEND_MAP_ZOOM_RATE_R, audioAndZoomOptionsMapZoomRateString().c_str());
		break;
	}

	case FRONTEND_RADAR_ZOOM:
	case FRONTEND_RADAR_ZOOM_R:
	{
		war_SetRadarZoom(seqCycle(war_GetRadarZoom(), MIN_RADARZOOM, RADARZOOM_STEP, MAX_RADARZOOM));
		widgSetString(psWScreen, FRONTEND_RADAR_ZOOM_R, audioAndZoomOptionsRadarZoomString().c_str());
		break;
	}

	case FRONTEND_QUIT:
		changeTitleMode(OPTIONS);
		break;

	default:
		break;
	}

	// If close button pressed then return from this menu.
	if (CancelPressed())
	{
		changeTitleMode(OPTIONS);
	}

	widgDisplayScreen(psWScreen);						// show the widgets currently running

	return true;
}
