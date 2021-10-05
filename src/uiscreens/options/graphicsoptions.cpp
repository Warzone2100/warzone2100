#include "graphicsoptions.h"

#include "lib/ivis_opengl/piestate.h" // pie_GetFogEnabled
#include "lib/widget/gridlayout.h" // grid stuff
#include "lib/widget/scrollablelist.h" // scrollable list

#include "../../warzoneconfig.h" // war_Get*
#include "../../radar.h" // rotateRadar
#include "../../seqdisp.h" // seq_GetSubtitles

char const* graphicsOptionsFmvmodeString()
{
	switch (war_GetFMVmode())
	{
	case FMV_1X: return _("1×");
	case FMV_2X: return _("2×");
	case FMV_FULLSCREEN: return _("Fullscreen");
	default: return _("Unsupported");
	}
}

char const* graphicsOptionsScanlinesString()
{
	switch (war_getScanlineMode())
	{
	case SCANLINES_OFF: return _("Off");
	case SCANLINES_50: return _("50%");
	case SCANLINES_BLACK: return _("Black");
	default: return _("Unsupported");
	}
}

char const* graphicsOptionsScreenShakeString()
{
	return getShakeStatus() ? _("On") : _("Off");
}

char const* graphicsOptionsSubtitlesString()
{
	return seq_GetSubtitles() ? _("On") : _("Off");
}

char const* graphicsOptionsShadowsString()
{
	return getDrawShadows() ? _("On") : _("Off");
}

char const* graphicsOptionsFogString()
{
	return pie_GetFogEnabled() ? _("On") : _("Off");
}

char const* graphicsOptionsRadarString()
{
	return rotateRadar ? _("Rotating") : _("Fixed");
}

char const* graphicsOptionsRadarJumpString()
{
	return war_GetRadarJump() ? _("Instant") : _("Tracked");
}

// ////////////////////////////////////////////////////////////////////////////
// Graphics Options
void startGraphicsOptionsMenu()
{
	addBackdrop();
	addTopForm(false);
	addBottomForm();

	WIDGET* parent = widgGetFromID(psWScreen, FRONTEND_BOTFORM);

	auto grid = std::make_shared<GridLayout>();
	grid_allocation::slot row(0);

	////////////
	//FMV mode.
	grid->place({ 0 }, row, addMargin(makeTextButton(FRONTEND_FMVMODE, _("Video Playback"), WBUT_SECONDARY)));
	grid->place({ 1, 1, false }, row, addMargin(makeTextButton(FRONTEND_FMVMODE_R, graphicsOptionsFmvmodeString(), WBUT_SECONDARY)));
	row.start++;

	// Scanlines
	grid->place({ 0 }, row, addMargin(makeTextButton(FRONTEND_SCANLINES, _("Scanlines"), WBUT_SECONDARY)));
	grid->place({ 1, 1, false }, row, addMargin(makeTextButton(FRONTEND_SCANLINES_R, graphicsOptionsScanlinesString(), WBUT_SECONDARY)));
	row.start++;

	////////////
	//shadows
	grid->place({ 0 }, row, addMargin(makeTextButton(FRONTEND_SHADOWS, _("Shadows"), WBUT_SECONDARY)));
	grid->place({ 1, 1, false }, row, addMargin(makeTextButton(FRONTEND_SHADOWS_R, graphicsOptionsShadowsString(), WBUT_SECONDARY)));
	row.start++;

	// fog
	grid->place({ 0 }, row, addMargin(makeTextButton(FRONTEND_FOG, _("Fog"), WBUT_SECONDARY)));
	grid->place({ 1, 1, false }, row, addMargin(makeTextButton(FRONTEND_FOG_R, graphicsOptionsFogString(), WBUT_SECONDARY)));
	row.start++;

	// Radar
	grid->place({ 0 }, row, addMargin(makeTextButton(FRONTEND_RADAR, _("Radar"), WBUT_SECONDARY)));
	grid->place({ 1, 1, false }, row, addMargin(makeTextButton(FRONTEND_RADAR_R, graphicsOptionsRadarString(), WBUT_SECONDARY)));
	row.start++;

	// RadarJump
	grid->place({ 0 }, row, addMargin(makeTextButton(FRONTEND_RADAR_JUMP, _("Radar Jump"), WBUT_SECONDARY)));
	grid->place({ 1, 1, false }, row, addMargin(makeTextButton(FRONTEND_RADAR_JUMP_R, graphicsOptionsRadarJumpString(), WBUT_SECONDARY)));
	row.start++;

	// screenshake
	grid->place({ 0 }, row, addMargin(makeTextButton(FRONTEND_SSHAKE, _("Screen Shake"), WBUT_SECONDARY)));
	grid->place({ 1, 1, false }, row, addMargin(makeTextButton(FRONTEND_SSHAKE_R, graphicsOptionsScreenShakeString(), WBUT_SECONDARY)));
	row.start++;

	grid->setGeometry(0, 0, FRONTEND_BUTWIDTH, grid->idealHeight());

	auto scrollableList = ScrollableListWidget::make();
	scrollableList->setGeometry(0, FRONTEND_POS2Y, FRONTEND_BOTFORMW - 1, FRONTEND_BOTFORMH - FRONTEND_POS2Y - 1);
	scrollableList->addItem(grid);
	parent->attach(scrollableList);

	// Add some text down the side of the form
	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_SIDEY, _("GRAPHICS OPTIONS"));

	////////////
	// quit.
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);
}

void seqFMVmode()
{
	war_SetFMVmode(seqCycle(war_GetFMVmode(), FMV_FULLSCREEN, 1, FMV_MODE(FMV_MAX - 1)));
}

void seqScanlineMode()
{
	war_setScanlineMode(seqCycle(war_getScanlineMode(), SCANLINES_OFF, 1, SCANLINES_BLACK));
}

bool runGraphicsOptionsMenu()
{
	WidgetTriggers const& triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	switch (id)
	{
	case FRONTEND_QUIT:
		changeTitleMode(OPTIONS);
		break;

	case FRONTEND_SHADOWS:
	case FRONTEND_SHADOWS_R:
		setDrawShadows(!getDrawShadows());
		widgSetString(psWScreen, FRONTEND_SHADOWS_R, graphicsOptionsShadowsString());
		break;

	case FRONTEND_FOG:
	case FRONTEND_FOG_R:
		if (pie_GetFogEnabled())
		{
			pie_SetFogStatus(false);
			pie_EnableFog(false);
		}
		else
		{
			pie_EnableFog(true);
		}
		widgSetString(psWScreen, FRONTEND_FOG_R, graphicsOptionsFogString());
		break;

	case FRONTEND_FMVMODE:
	case FRONTEND_FMVMODE_R:
		seqFMVmode();
		widgSetString(psWScreen, FRONTEND_FMVMODE_R, graphicsOptionsFmvmodeString());
		break;

	case FRONTEND_SCANLINES:
	case FRONTEND_SCANLINES_R:
		seqScanlineMode();
		widgSetString(psWScreen, FRONTEND_SCANLINES_R, graphicsOptionsScanlinesString());
		break;

	case FRONTEND_RADAR:
	case FRONTEND_RADAR_R:
		rotateRadar = !rotateRadar;
		widgSetString(psWScreen, FRONTEND_RADAR_R, graphicsOptionsRadarString());
		break;

	case FRONTEND_RADAR_JUMP:
	case FRONTEND_RADAR_JUMP_R:
		war_SetRadarJump(!war_GetRadarJump());
		widgSetString(psWScreen, FRONTEND_RADAR_JUMP_R, graphicsOptionsRadarJumpString());
		break;

	case FRONTEND_SSHAKE:
	case FRONTEND_SSHAKE_R:
		setShakeStatus(!getShakeStatus());
		widgSetString(psWScreen, FRONTEND_SSHAKE_R, graphicsOptionsScreenShakeString());
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
