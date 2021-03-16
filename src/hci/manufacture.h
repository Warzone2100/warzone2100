#ifndef __INCLUDED_SRC_HCI_MANUFACTURE_INTERFACE_H__
#define __INCLUDED_SRC_HCI_MANUFACTURE_INTERFACE_H__

#include "objects_stats.h"

class ManufactureController: public BaseObjectsStatsController, public std::enable_shared_from_this<ManufactureController>
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
		ASSERT_OR_RETURN(nullptr, index < factories.size(), "Invalid object index (%zu); max: (%zu)", index, factories.size());
		return factories[index];
	}

	bool findObject(std::function<bool (BASE_OBJECT *)> iteration) const override
	{
		return BaseObjectsController::findObject(factories, iteration);
	}

	void updateData();
	void adjustFactoryProduction(DROID_TEMPLATE *manufactureOption, bool add);
	void adjustFactoryLoop(bool add);
	void releaseFactoryProduction(STRUCTURE *structure);
	void cancelFactoryProduction(STRUCTURE *structure);
	void startDeliveryPointPosition();
	bool showInterface() override;
	void refresh() override;
	std::shared_ptr<StatsForm> makeStatsForm() override;

	STRUCTURE *getHighlightedObject() const override
	{
		return highlightedFactory;
	}

	void setHighlightedObject(BASE_OBJECT *object) override;

private:
	void updateFactoriesList();
	void updateManufactureOptionsList();
	std::vector<DROID_TEMPLATE *> stats;
	std::vector<STRUCTURE *> factories;
	static STRUCTURE *highlightedFactory;
};

#endif // __INCLUDED_SRC_HCI_MANUFACTURE_INTERFACE_H__
