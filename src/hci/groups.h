#ifndef __INCLUDED_SRC_HCI_GROUPS_H__
#define __INCLUDED_SRC_HCI_GROUPS_H__

#include "../intdisplay.h"
#include "objects_stats.h"

#include "../objmem.h"
#include "../input/keyconfig.h"
#include "../keybind.h"
#include "lib/widget/label.h"
#include "../selection.h"

void setGroupButtonEnabled(bool bNewState);
bool getGroupButtonEnabled();

class GroupButton;

class GroupsForum: public IntFormAnimated
{
private:
	typedef IntFormAnimated BaseWidget;
public:
	void display(int xOffset, int yOffset);
	void initialize();
	static std::shared_ptr<GroupsForum> make()
	{
		class make_shared_enabler: public GroupsForum {};
		auto widget = std::make_shared<make_shared_enabler>();
		// widget->controller = controller;
		widget->initialize();
		return widget;
	}
	std::shared_ptr<GroupButton> makeGroupButton(size_t groupNumber);
	std::shared_ptr<IntListTabWidget> groupsList;
	void addTabList();
};

#endif // __INCLUDED_SRC_HCI_GROUPS_H__
