#ifndef __INCLUDED_SRC_HCI_RESEARCH_INTERFACE_H__
#define __INCLUDED_SRC_HCI_RESEARCH_INTERFACE_H__

#include <vector>
#include "../objectdef.h"
#include "../hci.h"
#include "objects_stats.h"

class ResearchController: public BaseStatsController, public std::enable_shared_from_this<ResearchController>
{
public:
	RESEARCH *getObjectStatsAt(size_t objectIndex) const override;
	RESEARCH *getStatsAt(size_t statsIndex) const
	{
		return stats.at(statsIndex);
	}

	size_t statsSize() const override
	{
		return stats.size();
	}

	size_t objectsSize() const override
	{
		return facilities.size();
	}

	STRUCTURE *getObjectAt(size_t index) const override
	{
		return facilities.at(index);
	}

	bool findObject(std::function<bool (BASE_OBJECT *)> iteration) const override
	{
		return BaseObjectsController::findObject(facilities, iteration);
	}

	nonstd::optional<size_t> getSelectedFacilityIndex();
	void updateData();
	bool showInterface() override;
	void refresh() override;
	std::shared_ptr<StatsForm> makeStatsForm() override;
	void startResearch(RESEARCH *research);
	void requestResearchCancellation(STRUCTURE *facility);

	STRUCTURE *getHighlightedObject() const override
	{
		return highlightedFacility;
	}

	void setHighlightedObject(BASE_OBJECT *object) override;

private:
	void updateFacilitiesList();
	void updateResearchOptionsList();
	std::vector<RESEARCH *> stats;
	std::vector<STRUCTURE *> facilities;
	static STRUCTURE *highlightedFacility;
};

#endif // __INCLUDED_SRC_HCI_RESEARCH_INTERFACE_H__
