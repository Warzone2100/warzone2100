#include "scrollablelist.h"
#include "lib/framework/input.h"
#include "lib/ivis_opengl/pieblitfunc.h"

static const auto SCROLLBAR_WIDTH = 15;

ScrollableListWidget::ScrollableListWidget(WIDGET *parent) : WIDGET(parent), scrollBar(this), listView(this)
{
	scrollBar.show(false);
}

void ScrollableListWidget::geometryChanged()
{
	scrollBar.setGeometry(width() - SCROLLBAR_WIDTH, 0, SCROLLBAR_WIDTH, height());
	scrollBar.setViewSize(height());
	layoutDirty = true;
}

void ScrollableListWidget::run(W_CONTEXT *psContext)
{
	updateLayout();
	listView.setTopOffset(snapOffset ? snappedOffset() : scrollBar.position());
}

/**
 * Snap offset to first visible child.
 *
 * This wouldn't be necessary if it were possible to clip the rendering.
 */
uint16_t ScrollableListWidget::snappedOffset()
{
	for (auto child : listView.children())
	{
		if (child->y() + child->height() / 2 > scrollBar.position())
		{
			return child->y();
		}
	}

	return 0;
}

void ScrollableListWidget::addItem(WIDGET *item)
{
	listView.attach(item);
	layoutDirty = true;
}

void ScrollableListWidget::updateLayout()
{
	if (!layoutDirty) {
		return;
	}

	layoutDirty = false;

	scrollableHeight = 0;
	for (auto child : listView.children())
	{
		scrollableHeight += child->height();
	}

	scrollBar.show(scrollableHeight > height());
	scrollBar.setScrollableSize(scrollableHeight);
	listView.setGeometry(0, 0, width() - (scrollBar.visible() ? scrollBar.width() + 1 : 0), height());

	auto currentTop = 0;
	for (auto child : listView.children())
	{
		child->setGeometry(0, currentTop, listView.width(), child->height());
		currentTop += child->height();
	}
}

bool ScrollableListWidget::processClickRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed)
{
	scrollBar.incrementPosition(-getMouseWheelSpeed().y * 20);
	return WIDGET::processClickRecursive(psContext, key, wasPressed);
}
