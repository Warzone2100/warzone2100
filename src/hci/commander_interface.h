#ifndef __INCLUDED_SRC_HCI_COMMANDER_INTERFACE_H__
#define __INCLUDED_SRC_HCI_COMMANDER_INTERFACE_H__

#include <vector>
#include "../objectdef.h"
#include "../hci.h"
#include "objects_stats_interface.h"

class CommanderInterfaceController: public BaseStatsController, public std::enable_shared_from_this<CommanderInterfaceController>
{
public:
	STRUCTURE_STATS *getObjectStatsAt(size_t objectIndex) const override;

	size_t statsSize() const override
	{
		return 0;
	}

	size_t objectsSize() const override
	{
		return commanders.size();
	}

	DROID *getObjectAt(size_t index) const override
	{
		return commanders.at(index);
	}

	bool findObject(std::function<bool (BASE_OBJECT *)> iteration) const override
	{
		return BaseObjectsController::findObject(commanders, iteration);
	}

	void updateData();
	bool showInterface() override;
	void refresh() override;
	void displayOrderForm();

private:
	void updateCommandersList();
	std::vector<DROID *> commanders;
};

#endif // __INCLUDED_SRC_HCI_COMMANDER_INTERFACE_H__
