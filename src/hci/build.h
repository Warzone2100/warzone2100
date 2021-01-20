#ifndef __INCLUDED_SRC_HCI_BUILD_INTERFACE_H__
#define __INCLUDED_SRC_HCI_BUILD_INTERFACE_H__

#include <vector>
#include "../objectdef.h"
#include "../hci.h"
#include "objects_stats.h"

class BuildController: public BaseStatsController, public std::enable_shared_from_this<BuildController>
{
public:
	STRUCTURE_STATS *getObjectStatsAt(size_t objectIndex) const override;
	STRUCTURE_STATS *getStatsAt(size_t statsIndex) const
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
		updateBuildOptionsList();
	}

	bool shouldShowFavorites() const
	{
		return BuildController::showFavorites;
	}

	void setShouldShowFavorite(bool value)
	{
		BuildController::showFavorites = value;
		updateBuildOptionsList();
	}

	size_t objectsSize() const override
	{
		return builders.size();
	}

	DROID *getObjectAt(size_t index) const override
	{
		return builders.at(index);
	}

	bool findObject(std::function<bool (BASE_OBJECT *)> iteration) const override
	{
		return BaseObjectsController::findObject(builders, iteration);
	}

	void updateData();
	void toggleFavorites(BASE_STATS *buildOption);
	void startBuildPosition(BASE_STATS *buildOption);
	bool showInterface() override;
	void refresh() override;
	void toggleBuilderSelection(DROID *droid);
	void displayStatsForm();

	static void resetShowFavorites()
	{
		BuildController::showFavorites = false;
	}

private:
	void updateBuildersList();
	void updateBuildOptionsList();
	std::vector<STRUCTURE_STATS *> stats;
	std::vector<DROID *> builders;

	static bool showFavorites;
};

#endif // __INCLUDED_SRC_HCI_BUILD_INTERFACE_H__
