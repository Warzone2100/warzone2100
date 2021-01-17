#ifndef __INCLUDED_SRC_HCI_BUILD_INTERFACE_H__
#define __INCLUDED_SRC_HCI_BUILD_INTERFACE_H__

#include <vector>
#include "../objectdef.h"
#include "../hci.h"
#include "objects_stats_interface.h"

class BuildInterfaceController: public BaseObjectsStatsController, public std::enable_shared_from_this<BuildInterfaceController>
{
public:
	STRUCTURE_STATS *getObjectStatsAt(size_t objectIndex) const override;
	STRUCTURE_STATS *getStatsAt(size_t statsIndex) const override
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
		return intGetShowFavorites();
	}

	void setShouldShowFavorite(bool value)
	{
		intSetShowFavorites(value);
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

	void updateData();
	void toggleFavorites(BASE_STATS *buildOption);
	void startBuildPosition(BASE_STATS *buildOption);
	bool showInterface();
	void closeInterface();
	void refresh() override;
	void updateSelected();
	void toggleBuilderSelection(DROID *droid);
	void displayStatsForm();

private:
	void updateBuildersList();
	void updateBuildOptionsList();
	std::vector<STRUCTURE_STATS *> stats;
	std::vector<DROID *> builders;
};

bool showBuildInterface();

#endif // __INCLUDED_SRC_HCI_BUILD_INTERFACE_H__
