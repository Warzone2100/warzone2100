#include <memory>
#include "scrollbar.h"
#include "lib/ivis_opengl/pieblitfunc.h"

static void displayScrollBar(WIDGET *widget, UDWORD xOffset, UDWORD yOffset)
{
	auto slider = (W_SLIDER *)widget;

	int x0 = slider->x() + xOffset;
	int y0 = slider->y() + yOffset;

	iV_TransBoxFill(x0, y0, x0 + slider->width(), y0 + slider->height());

	auto sliderY = (slider->height() - slider->barSize) * slider->pos / slider->numStops;
	pie_UniTransBoxFill(x0, y0 + sliderY, x0 + slider->width(), y0 + sliderY + slider->barSize, WZCOL_LBLUE);
}

ScrollBarWidget::ScrollBarWidget(WIDGET *parent) : WIDGET(parent)
{
	W_SLDINIT sliderInit;
	slider = new W_SLIDER(&sliderInit);
	slider->numStops = 1;
	slider->barSize = 1;
	slider->orientation = WSLD_TOP;
	slider->displayFunction = displayScrollBar;
	attach(slider);
}

void ScrollBarWidget::geometryChanged()
{
	slider->setGeometry(0, 0, width(), height());
}

uint16_t ScrollBarWidget::position() const
{
	return slider->pos;
}

void ScrollBarWidget::setScrollableSize(uint16_t value)
{
	scrollableSize = value;
	updateSlider();
}

void ScrollBarWidget::setViewSize(uint16_t value)
{
	viewSize = value;
	updateSlider();
}

void ScrollBarWidget::updateSlider()
{
	slider->barSize = viewSize * height() / scrollableSize;
	slider->numStops = scrollableSize - viewSize;
}
