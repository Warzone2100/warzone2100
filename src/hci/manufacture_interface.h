#ifndef __INCLUDED_SRC_HCI_MANUFACTURE_INTERFACE_H__
#define __INCLUDED_SRC_HCI_MANUFACTURE_INTERFACE_H__

#include <vector>
#include "../objectdef.h"
#include "../hci.h"
#include "objects_stats_interface.h"

class ManufactureInterfaceController: public BaseObjectsStatsController, public std::enable_shared_from_this<ManufactureInterfaceController>
{
public:
	DROID_TEMPLATE *getObjectStatsAt(size_t objectIndex) const override;
	DROID_TEMPLATE *getStatsAt(size_t statsIndex) const override
	{
		return stats.at(statsIndex);
	}

	size_t statsSize() const override
	{
		return stats.size();
	}

	bool shouldShowRedundantDesign() const
	{
		return intGetShouldShowRedundantDesign();
	}

	void setShouldShowRedundantDesign(bool value)
	{
		intSetShouldShowRedundantDesign(value);
		updateManufactureOptionsList();
	}

	size_t objectsSize() const override
	{
		return factories.size();
	}

	STRUCTURE *getObjectAt(size_t index) const override
	{
		return factories.at(index);
	}

	void updateData();
	void adjustFactoryProduction(DROID_TEMPLATE *manufactureOption, bool add);
	void adjustFactoryLoop(bool add);
	void startDeliveryPointPosition();
	bool showInterface();
	void closeInterface();
	void refresh() override;
	void updateSelected();
	void displayStatsForm();

private:
	void updateFactoriesList();
	void updateManufactureOptionsList();
	std::vector<DROID_TEMPLATE *> stats;
	std::vector<STRUCTURE *> factories;
};

bool showManufactureInterface();

#endif // __INCLUDED_SRC_HCI_MANUFACTURE_INTERFACE_H__
