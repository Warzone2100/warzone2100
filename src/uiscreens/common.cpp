#include "common.h"

// ////////////////////////////////////////////////////////////////////////////
// Helper functions

// Returns true if escape key pressed.
//
bool CancelPressed()
{
	const bool cancel = keyPressed(KEY_ESC);
	if (cancel)
	{
		inputLoseFocus();	// clear the input buffer.
	}
	return cancel;
}

void moveToParentRightEdge(WIDGET* widget, int32_t right)
{
	if (auto parent = widget->parent())
	{
		widget->move(parent->width() - (widget->width() + right), widget->y());
	}
}

std::shared_ptr<WIDGET> addMargin(std::shared_ptr<WIDGET> widget)
{
	return Margin(0, 20).wrap(widget);
}

void addTextButton(UDWORD id, UDWORD PosX, UDWORD PosY, const std::string& txt, unsigned int style)
{
	auto button = makeTextButton(id, txt, style);
	if (style & WBUT_TXTCENTRE)
	{
		button->setGeometry(PosX, PosY, FRONTEND_BUTWIDTH, button->height());
	}
	else
	{
		button->move(PosX + 35, PosY);
	}

	widgGetFromID(psWScreen, FRONTEND_BOTFORM)->attach(button);
}

W_BUTTON* addSmallTextButton(UDWORD id, UDWORD PosX, UDWORD PosY, const char* txt, unsigned int style)
{
	W_BUTINIT sButInit;

	sButInit.formID = FRONTEND_BOTFORM;
	sButInit.id = id;
	sButInit.x = (short)PosX;
	sButInit.y = (short)PosY;

	// Align
	if (!(style & WBUT_TXTCENTRE))
	{
		sButInit.width = (uint16_t)(iV_GetTextWidth(txt, font_small)) + 4;
		sButInit.x += 35;
	}
	else
	{
		sButInit.style |= WBUT_TXTCENTRE;
		sButInit.width = FRONTEND_BUTWIDTH;
	}

	// Enable right clicks
	if (style & WBUT_SECONDARY)
	{
		sButInit.style |= WBUT_SECONDARY;
	}

	sButInit.UserData = (style & WBUT_DISABLE); // store disable state
	sButInit.pUserData = new DisplayTextOptionCache();
	sButInit.onDelete = [](WIDGET* psWidget) {
		assert(psWidget->pUserData != nullptr);
		delete static_cast<DisplayTextOptionCache*>(psWidget->pUserData);
		psWidget->pUserData = nullptr;
	};

	sButInit.height = FRONTEND_BUTHEIGHT;
	sButInit.pDisplay = displayTextOption;
	sButInit.FontID = font_small;
	sButInit.pText = txt;
	W_BUTTON* pButton = widgAddButton(psWScreen, &sButInit);

	// Disable button
	if (style & WBUT_DISABLE)
	{
		widgSetButtonState(psWScreen, id, WBUT_DISABLE);
	}
	return pButton;
}

// ////////////////////////////////////////////////////////////////////////////
std::shared_ptr<W_SLIDER> makeFESlider(UDWORD id, UDWORD parent, UDWORD stops, UDWORD pos)
{
	W_SLDINIT sSldInit;
	sSldInit.formID = parent;
	sSldInit.id = id;
	sSldInit.width = iV_GetImageWidth(IntImages, IMAGE_SLIDER_BIG);
	sSldInit.height = iV_GetImageHeight(IntImages, IMAGE_SLIDER_BIG);
	sSldInit.numStops = (UBYTE)stops;
	sSldInit.barSize = iV_GetImageHeight(IntImages, IMAGE_SLIDER_BIG);
	sSldInit.pos = (UBYTE)pos;
	sSldInit.pDisplay = displayBigSlider;
	sSldInit.pCallback = intUpdateQuantitySlider;

	auto slider = std::make_shared<W_SLIDER>(&sSldInit);
	return slider;
}

void addFESlider(UDWORD id, UDWORD parent, UDWORD x, UDWORD y, UDWORD stops, UDWORD pos)
{
	auto slider = makeFESlider(id, parent, stops, pos);
	slider->move(x, y);
	widgGetFromID(psWScreen, parent)->attach(slider);
}

// ////////////////////////////////////////////////////////////////////////////
// drawing functions

// show a background picture (currently used for version and mods labels)
void displayTitleBitmap(WZ_DECL_UNUSED WIDGET* psWidget, WZ_DECL_UNUSED UDWORD xOffset, WZ_DECL_UNUSED UDWORD yOffset)
{
	char modListText[MAX_STR_LENGTH] = "";

	if (!getModList().empty())
	{
		sstrcat(modListText, _("Mod: "));
		sstrcat(modListText, getModList().c_str());
	}

	assert(psWidget->pUserData != nullptr);
	TitleBitmapCache& cache = *static_cast<TitleBitmapCache*>(psWidget->pUserData);

	cache.formattedVersionString.setText(version_getFormattedVersionString(), font_regular);
	cache.modListText.setText(modListText, font_regular);
	cache.gfxBackend.setText(gfx_api::context::get().getFormattedRendererInfoString(), font_small);

	cache.formattedVersionString.render(pie_GetVideoBufferWidth() - 9, pie_GetVideoBufferHeight() - 14, WZCOL_GREY, 270.f);

	if (!getModList().empty())
	{
		cache.modListText.render(9, 14, WZCOL_GREY);
	}

	cache.gfxBackend.render(9, pie_GetVideoBufferHeight() - 10, WZCOL_GREY);

	cache.formattedVersionString.render(pie_GetVideoBufferWidth() - 10, pie_GetVideoBufferHeight() - 15, WZCOL_TEXT_BRIGHT, 270.f);

	if (!getModList().empty())
	{
		cache.modListText.render(10, 15, WZCOL_TEXT_BRIGHT);
	}

	cache.gfxBackend.render(10, pie_GetVideoBufferHeight() - 11, WZCOL_TEXT_BRIGHT);
}

// ////////////////////////////////////////////////////////////////////////////
// show warzone logo
void displayLogo(WIDGET* psWidget, UDWORD xOffset, UDWORD yOffset)
{
	iV_DrawImage2(WzString::fromUtf8("image_fe_logo.png"), xOffset + psWidget->x(), yOffset + psWidget->y(), psWidget->width(), psWidget->height());
}


// ////////////////////////////////////////////////////////////////////////////
// show, well have a guess..
void displayBigSlider(WIDGET* psWidget, UDWORD xOffset, UDWORD yOffset)
{
	W_SLIDER* Slider = (W_SLIDER*)psWidget;
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();

	iV_DrawImage(IntImages, IMAGE_SLIDER_BIG, x + STAT_SLD_OX, y + STAT_SLD_OY);			// draw bdrop

	int sx = (Slider->width() - 3 - Slider->barSize) * Slider->pos / Slider->numStops;  // determine pos.
	iV_DrawImage(IntImages, IMAGE_SLIDER_BIGBUT, x + 3 + sx, y + 3);								//draw amount
}

// ////////////////////////////////////////////////////////////////////////////
// show text written on its side.
void displayTextAt270(WIDGET* psWidget, UDWORD xOffset, UDWORD yOffset)
{
	SDWORD		fx, fy;
	W_LABEL* psLab;

	psLab = (W_LABEL*)psWidget;

	// Any widget using displayTextAt270 must have its pUserData initialized to a (DisplayTextOptionCache*)
	assert(psWidget->pUserData != nullptr);
	DisplayTextOptionCache& cache = *static_cast<DisplayTextOptionCache*>(psWidget->pUserData);

	// TODO: Only works for single-line (not "formatted text") labels
	cache.wzText.setText(psLab->getString().toUtf8(), font_large);

	fx = xOffset + psWidget->x();
	fy = yOffset + psWidget->y() + cache.wzText.width();

	cache.wzText.render(fx + 2, fy + 2, WZCOL_GREY, 270.f);
	cache.wzText.render(fx, fy, WZCOL_TEXT_BRIGHT, 270.f);
}

// ////////////////////////////////////////////////////////////////////////////
// show a text option.
void displayTextOption(WIDGET* psWidget, UDWORD xOffset, UDWORD yOffset)
{
	SDWORD			fx, fy, fw;
	W_BUTTON* psBut = (W_BUTTON*)psWidget;
	bool			hilight = false;
	bool			greyOut = psWidget->UserData || (psBut->getState() & WBUT_DISABLE); // if option is unavailable.

	// Any widget using displayTextOption must have its pUserData initialized to a (DisplayTextOptionCache*)
	assert(psWidget->pUserData != nullptr);
	DisplayTextOptionCache& cache = *static_cast<DisplayTextOptionCache*>(psWidget->pUserData);

	cache.wzText.setText(psBut->pText.toUtf8(), psBut->FontID);

	if (psBut->isMouseOverWidget()) // if mouse is over text then hilight.
	{
		hilight = true;
	}

	fw = cache.wzText.width();
	fy = yOffset + psWidget->y() + (psWidget->height() - cache.wzText.lineSize()) / 2 - cache.wzText.aboveBase();

	if (psWidget->style & WBUT_TXTCENTRE)							//check for centering, calculate offset.
	{
		fx = xOffset + psWidget->x() + ((psWidget->width() - fw) / 2);
	}
	else
	{
		fx = xOffset + psWidget->x();
	}

	PIELIGHT colour;

	if (greyOut)														// unavailable
	{
		colour = WZCOL_TEXT_DARK;
	}
	else															// available
	{
		if (hilight)													// hilight
		{
			colour = WZCOL_TEXT_BRIGHT;
		}
		else if (psWidget->id == FRONTEND_HYPERLINK || psWidget->id == FRONTEND_DONATELINK || psWidget->id == FRONTEND_CHATLINK) // special case for our hyperlink
		{
			colour = WZCOL_YELLOW;
		}
		else														// don't highlight
		{
			colour = WZCOL_TEXT_MEDIUM;
		}
	}

	cache.wzText.render(fx, fy, colour);

	return;
}
