#include "titlemenu.h"

#include "physfs.h"
#include "../frontend.h"
#include "../game.h"
#include "../urlhelpers.h"
#include "../loadsave.h"
#include "../frend.h"
#include "../mission.h"
#include "../multiint.h"


// ////////////////////////////////////////////////////////////////////////////
// Title Screen
void startTitleMenu()
{
	intRemoveReticule();

	addBackdrop();
	addTopForm(false);
	addBottomForm();

	addTextButton(FRONTEND_SINGLEPLAYER, FRONTEND_POS2X, FRONTEND_POS2Y, _("Single Player"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_MULTIPLAYER, FRONTEND_POS3X, FRONTEND_POS3Y, _("Multi Player"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_TUTORIAL, FRONTEND_POS4X, FRONTEND_POS4Y, _("Tutorial"), WBUT_TXTCENTRE);
	addTextButton(FRONTEND_OPTIONS, FRONTEND_POS5X, FRONTEND_POS5Y, _("Options"), WBUT_TXTCENTRE);

	// check whether video sequences are installed
	if (PHYSFS_exists("sequences/devastation.ogg"))
	{
		addTextButton(FRONTEND_PLAYINTRO, FRONTEND_POS6X, FRONTEND_POS6Y, _("View Intro"), WBUT_TXTCENTRE);
	}
	else
	{
		addTextButton(FRONTEND_PLAYINTRO, FRONTEND_POS6X, FRONTEND_POS6Y, _("View Intro"), WBUT_TXTCENTRE | WBUT_DISABLE);
		widgSetTip(psWScreen, FRONTEND_PLAYINTRO, _("Videos are missing, download them from http://wz2100.net"));
	}

	if (findLastSave())
	{
		addTextButton(FRONTEND_CONTINUE, FRONTEND_POS7X, FRONTEND_POS7Y, _("Continue Last Save"), WBUT_TXTCENTRE);
	}
	else
	{
		addTextButton(FRONTEND_CONTINUE, FRONTEND_POS7X, FRONTEND_POS7Y, _("Continue Last Save"), WBUT_TXTCENTRE | WBUT_DISABLE);
		widgSetTip(psWScreen, FRONTEND_CONTINUE, _("No last save available"));
	}
	addTextButton(FRONTEND_QUIT, FRONTEND_POS8X, FRONTEND_POS8Y, _("Quit Game"), WBUT_TXTCENTRE);
	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_SIDEY, _("MAIN MENU"));

	addSmallTextButton(FRONTEND_HYPERLINK, FRONTEND_POS9X, FRONTEND_POS9Y, _("Official site: http://wz2100.net/"), 0);
	widgSetTip(psWScreen, FRONTEND_HYPERLINK, _("Come visit the forums and all Warzone 2100 news! Click this link."));
	W_BUTTON* pRightAlignedButton = addSmallTextButton(FRONTEND_DONATELINK, FRONTEND_POS9X + 360, FRONTEND_POS9Y, _("Donate: http://donations.wz2100.net/"), 0);
	moveToParentRightEdge(pRightAlignedButton, 1);
	widgSetTip(psWScreen, FRONTEND_DONATELINK, _("Help support the project with our server costs, Click this link."));
	pRightAlignedButton = addSmallTextButton(FRONTEND_CHATLINK, FRONTEND_POS9X + 360, 0, _("Chat with players on Discord or IRC"), 0);
	moveToParentRightEdge(pRightAlignedButton, 6);
	widgSetTip(psWScreen, FRONTEND_CHATLINK, _("Connect to Discord or Freenode through webchat by clicking this link."));
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_UPGRDLINK, 7, 7, MULTIOP_BUTW, MULTIOP_BUTH, _("Check for a newer version"), IMAGE_GAMEVERSION, IMAGE_GAMEVERSION_HI, true);
}

void runUpgrdHyperlink()
{
	std::string link = "https://wz2100.net/versioncheck/?ver=";
	std::string version = version_getVersionString();
	std::string versionStr;
	for (char ch : version)
	{
		versionStr += (ch == ' ') ? '_' : ch;
	}
	link += urlEncode(versionStr.c_str());
	openURLInBrowser(link.c_str());
}

void runHyperlink()
{
	openURLInBrowser("https://wz2100.net/");
}

void rundonatelink()
{
	openURLInBrowser("http://donations.wz2100.net/");
}

void runchatlink()
{
	openURLInBrowser("https://wz2100.net/webchat/");
}

void runContinue()
{
	SPinit(lastSaveMP ? LEVEL_TYPE::SKIRMISH : LEVEL_TYPE::CAMPAIGN);
	sstrcpy(saveGameName, lastSavePath);
	bMultiPlayer = lastSaveMP;
	setCampaignNumber(getCampaign(lastSavePath));
}

bool runTitleMenu()
{
	WidgetTriggers const& triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	switch (id)
	{
	case FRONTEND_QUIT:
		changeTitleMode(QUIT);
		break;
	case FRONTEND_MULTIPLAYER:
		changeTitleMode(MULTI);
		break;
	case FRONTEND_SINGLEPLAYER:
		changeTitleMode(SINGLE);
		break;
	case FRONTEND_OPTIONS:
		changeTitleMode(OPTIONS);
		break;
	case FRONTEND_TUTORIAL:
		changeTitleMode(TUTORIAL);
		break;
	case FRONTEND_PLAYINTRO:
		changeTitleMode(SHOWINTRO);
		break;
	case FRONTEND_HYPERLINK:
		runHyperlink();
		break;
	case FRONTEND_UPGRDLINK:
		runUpgrdHyperlink();
		break;
	case FRONTEND_DONATELINK:
		rundonatelink();
		break;
	case FRONTEND_CHATLINK:
		runchatlink();
		break;
	case FRONTEND_CONTINUE:
		runContinue();
		changeTitleMode(LOADSAVEGAME);
		break;

	default:
		break;
	}

	widgDisplayScreen(psWScreen); // show the widgets currently running

	return true;
}
