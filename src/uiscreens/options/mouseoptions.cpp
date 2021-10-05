#include "mouseoptions.h"

#include "lib/framework/wzapp.h"

#include "lib/widget/gridlayout.h"
#include "lib/widget/scrollablelist.h"

#include "../../warzoneconfig.h" // war_Get*

char const* mouseOptionsMflipString()
{
	return getInvertMouseStatus() ? _("On") : _("Off");
}

char const* mouseOptionsTrapString()
{
	return war_GetTrapCursor() ? _("On") : _("Off");
}

char const* mouseOptionsMbuttonsString()
{
	return getRightClickOrders() ? _("On") : _("Off");
}

char const* mouseOptionsMmrotateString()
{
	return getMiddleClickRotate() ? _("Middle Mouse") : _("Right Mouse");
}

char const* mouseOptionsCursorModeString()
{
	return war_GetColouredCursor() ? _("On") : _("Off");
}

// ////////////////////////////////////////////////////////////////////////////
// Mouse Options
void startMouseOptionsMenu()
{
	addBackdrop();
	addTopForm(false);
	addBottomForm();

	WIDGET* parent = widgGetFromID(psWScreen, FRONTEND_BOTFORM);

	auto grid = std::make_shared<GridLayout>();
	grid_allocation::slot row(0);

	////////////
	// mouseflip
	grid->place({ 0 }, row, addMargin(makeTextButton(FRONTEND_MFLIP, _("Reverse Rotation"), WBUT_SECONDARY)));
	grid->place({ 1, 1, false }, row, addMargin(makeTextButton(FRONTEND_MFLIP_R, mouseOptionsMflipString(), WBUT_SECONDARY)));
	row.start++;

	// Cursor trapping
	grid->place({ 0 }, row, addMargin(makeTextButton(FRONTEND_TRAP, _("Trap Cursor"), WBUT_SECONDARY)));
	grid->place({ 1, 1, false }, row, addMargin(makeTextButton(FRONTEND_TRAP_R, mouseOptionsTrapString(), WBUT_SECONDARY)));
	row.start++;

	////////////
	// left-click orders
	grid->place({ 0 }, row, addMargin(makeTextButton(FRONTEND_MBUTTONS, _("Switch Mouse Buttons"), WBUT_SECONDARY)));
	grid->place({ 1, 1, false }, row, addMargin(makeTextButton(FRONTEND_MBUTTONS_R, mouseOptionsMbuttonsString(), WBUT_SECONDARY)));
	row.start++;

	////////////
	// middle-click rotate
	grid->place({ 0 }, row, addMargin(makeTextButton(FRONTEND_MMROTATE, _("Rotate Screen"), WBUT_SECONDARY)));
	grid->place({ 1, 1, false }, row, addMargin(makeTextButton(FRONTEND_MMROTATE_R, mouseOptionsMmrotateString(), WBUT_SECONDARY)));
	row.start++;

	// Hardware / software cursor toggle
	grid->place({ 0 }, row, addMargin(makeTextButton(FRONTEND_CURSORMODE, _("Colored Cursors"), WBUT_SECONDARY)));
	grid->place({ 1, 1, false }, row, addMargin(makeTextButton(FRONTEND_CURSORMODE_R, mouseOptionsCursorModeString(), WBUT_SECONDARY)));
	row.start++;

	grid->setGeometry(0, 0, FRONTEND_BUTWIDTH, grid->idealHeight());

	auto scrollableList = ScrollableListWidget::make();
	scrollableList->setGeometry(0, FRONTEND_POS2Y, FRONTEND_BOTFORMW - 1, FRONTEND_BOTFORMH - FRONTEND_POS2Y - 1);
	scrollableList->addItem(grid);
	parent->attach(scrollableList);

	// Add some text down the side of the form
	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_SIDEY, _("MOUSE OPTIONS"));

	// Quit/return
	addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, P_("menu", "Return"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);
}

bool runMouseOptionsMenu()
{
	WidgetTriggers const& triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id; // Just use first click here, since the next click could be on another menu.

	switch (id)
	{
	case FRONTEND_MFLIP:
	case FRONTEND_MFLIP_R:
		setInvertMouseStatus(!getInvertMouseStatus());
		widgSetString(psWScreen, FRONTEND_MFLIP_R, mouseOptionsMflipString());
		break;
	case FRONTEND_TRAP:
	case FRONTEND_TRAP_R:
		war_SetTrapCursor(!war_GetTrapCursor());
		widgSetString(psWScreen, FRONTEND_TRAP_R, mouseOptionsTrapString());
		break;

	case FRONTEND_MBUTTONS:
	case FRONTEND_MBUTTONS_R:
		setRightClickOrders(!getRightClickOrders());
		widgSetString(psWScreen, FRONTEND_MBUTTONS_R, mouseOptionsMbuttonsString());
		break;

	case FRONTEND_MMROTATE:
	case FRONTEND_MMROTATE_R:
		setMiddleClickRotate(!getMiddleClickRotate());
		widgSetString(psWScreen, FRONTEND_MMROTATE_R, mouseOptionsMmrotateString());
		break;

	case FRONTEND_CURSORMODE:
	case FRONTEND_CURSORMODE_R:
		war_SetColouredCursor(!war_GetColouredCursor());
		widgSetString(psWScreen, FRONTEND_CURSORMODE_R, mouseOptionsCursorModeString());
		wzSetCursor(CURSOR_DEFAULT);
		break;

	case FRONTEND_QUIT:
		changeTitleMode(OPTIONS);
		break;

	default:
		break;
	}

	if (CancelPressed())
	{
		changeTitleMode(OPTIONS);
	}

	widgDisplayScreen(psWScreen);

	return true;
}
