#ifndef __INCLUDED_SRC_HCI_COMMANDER_INTERFACE_H__
#define __INCLUDED_SRC_HCI_COMMANDER_INTERFACE_H__

#include "objects_stats.h"

class CommanderController: public BaseObjectsController, public std::enable_shared_from_this<CommanderController>
{
public:
	STRUCTURE_STATS *getObjectStatsAt(size_t objectIndex) const override;

	STRUCTURE *getAssignedFactoryAt(size_t objectIndex) const;

	size_t objectsSize() const override
	{
		return commanders.size();
	}

	DROID *getObjectAt(size_t index) const override
	{
		ASSERT_OR_RETURN(nullptr, index < commanders.size(), "Invalid object index (%zu); max: (%zu)", index, commanders.size());
		return commanders[index];
	}

	bool findObject(std::function<bool (BASE_OBJECT *)> iteration) const override
	{
		return BaseObjectsController::findObject(commanders, iteration);
	}

	void updateData();
	bool showInterface() override;
	void refresh() override;
	void clearData() override;
	void displayOrderForm();

	DROID *getHighlightedObject() const override
	{
		return highlightedCommander;
	}

	void setHighlightedObject(BASE_OBJECT *object) override;

private:
	void updateCommandersList();
	std::vector<DROID *> commanders;
	static DROID *highlightedCommander;
};

#endif // __INCLUDED_SRC_HCI_COMMANDER_INTERFACE_H__
