#ifndef __INCLUDED_SRC_HCI_RESEARCH_INTERFACE_H__
#define __INCLUDED_SRC_HCI_RESEARCH_INTERFACE_H__

#include <vector>
#include "../objectdef.h"
#include "../hci.h"
#include "objects_stats_interface.h"

class ResearchInterfaceController: public BaseObjectsStatsController, public std::enable_shared_from_this<ResearchInterfaceController>
{
public:
	RESEARCH *getObjectStatsAt(size_t objectIndex) const override;
	RESEARCH *getStatsAt(size_t statsIndex) const override
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

	nonstd::optional<size_t> getSelectedFacilityIndex();
	void updateData();
	bool showInterface();
	void closeInterface();
	void refresh() override;
	void updateSelected();
	void displayStatsForm();
	void startResearch(RESEARCH *research);
	void requestResearchCancellation(STRUCTURE *facility);

private:
	void updateFacilitiesList();
	void updateResearchOptionsList();
	std::vector<RESEARCH *> stats;
	std::vector<STRUCTURE *> facilities;
};

bool showBuildInterface();

#endif // __INCLUDED_SRC_HCI_RESEARCH_INTERFACE_H__
