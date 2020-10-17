#include "cliprect.h"
#include "lib/ivis_opengl/pieblitfunc.h"

void ClipRectWidget::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;

	iV_TransBoxFill(x0, y0, x0 + width(), y0 + height());
}

void ClipRectWidget::run(W_CONTEXT *psContext)
{
	W_CONTEXT newContext;
	newContext.xOffset = psContext->xOffset + x();
	newContext.yOffset = psContext->yOffset + y();
	newContext.mx = psContext->mx - x();
	newContext.my = psContext->my - y() + offset.y;

	runRecursive(&newContext);
}

bool ClipRectWidget::processClickRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed)
{
	W_CONTEXT newContext;
	newContext.xOffset = psContext->xOffset;
	newContext.yOffset = psContext->yOffset;
	newContext.mx = psContext->mx;
	newContext.my = psContext->my + offset.y;
	return WIDGET::processClickRecursive(&newContext, key, wasPressed);
}

void ClipRectWidget::displayRecursive(int xOffset, int yOffset)
{
	display(xOffset, yOffset);

	auto visibleRect = WzRect(offset.x, offset.y, width(), height());
	auto childrenXOffset = xOffset + x();
	auto childrenYOffset = yOffset + y() - offset.y;

	for (auto child : children())
	{
		if (child->visible() && visibleRect.contains(child->geometry()))
		{
			child->displayRecursive(childrenXOffset, childrenYOffset);
		}
	}
}

void ClipRectWidget::setTopOffset(uint16_t value)
{
	offset.y = value;
}

void ClipRectWidget::setLeftOffset(uint16_t value)
{
	offset.x = value;
}
