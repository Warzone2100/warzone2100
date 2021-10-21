#ifndef __INCLUDED_SRC_HCI_BUILD_INTERFACE_H__
#define __INCLUDED_SRC_HCI_BUILD_INTERFACE_H__

#include "objects_stats.h"

class BuildController: public BaseObjectsStatsController, public std::enable_shared_from_this<BuildController>
{
public:
	STRUCTURE_STATS *getObjectStatsAt(size_t objectIndex) const override;
	STRUCTURE_STATS *getStatsAt(size_t statsIndex) const override
	{
		ASSERT_OR_RETURN(nullptr, statsIndex < stats.size(), "Invalid stats index (%zu); max: (%zu)", statsIndex, stats.size());
		return stats[statsIndex];
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
		ASSERT_OR_RETURN(nullptr, index < builders.size(), "Invalid object index (%zu); max: (%zu)", index, builders.size());
		return builders[index];
	}

	bool findObject(std::function<bool (BASE_OBJECT *)> iteration) const override
	{
		return BaseObjectsController::findObject(builders, iteration);
	}

	void updateData();
	void toggleFavorites(STRUCTURE_STATS *buildOption);
	void startBuildPosition(STRUCTURE_STATS *buildOption);
	bool showInterface() override;
	void refresh() override;
	void clearData() override;
	void toggleBuilderSelection(DROID *droid);
	std::shared_ptr<StatsForm> makeStatsForm() override;

	static void resetShowFavorites()
	{
		BuildController::showFavorites = false;
	}

	DROID *getHighlightedObject() const override
	{
		return highlightedBuilder;
	}

	void setHighlightedObject(BASE_OBJECT *object) override;

private:
	void updateBuildersList();
	void updateBuildOptionsList();
	std::vector<STRUCTURE_STATS *> stats;
	std::vector<DROID *> builders;

	static bool showFavorites;
	static DROID *highlightedBuilder;
};

#endif // __INCLUDED_SRC_HCI_BUILD_INTERFACE_H__
