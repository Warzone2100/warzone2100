#include "../hci.h"
#include "groups.h"
#include "objects_stats.h"

class GroupButton : public DynamicIntFancyButton
{
private:
	typedef DynamicIntFancyButton BaseWidget;
	std::shared_ptr<W_LABEL> groupNumberLabel;
	std::shared_ptr<W_LABEL> groupCountLabel;
public:
	size_t groupNumber;
	static std::shared_ptr<GroupButton> make(size_t groupNumber)
	{
		class make_shared_enabler: public GroupButton {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->groupNumber = groupNumber;
		widget->initialize();
		return widget;
	}
	void initialize()
	{
		attach(groupNumberLabel = std::make_shared<W_LABEL>());
		groupNumberLabel->setGeometry(OBJ_TEXTX, OBJ_B1TEXTY - 5, 16, 16);
		groupNumberLabel->setString(WzString::fromUtf8(astringf("%u", groupNumber)));

		attach(groupCountLabel = std::make_shared<W_LABEL>());
		groupCountLabel->setGeometry(OBJ_TEXTX + 40, OBJ_B1TEXTY + 20, 16, 16);
		groupCountLabel->setString("");
	}
	GroupButton() { }

	void clickPrimary() override
	{
		// select the group
		kf_SelectGrouping(groupNumber);

	}
	void clickSecondary() override
	{
		assignDroidsToGroup(selectedPlayer, groupNumber, true);

	}
protected:
	void display(int xOffset, int yOffset) override
	{
		DROID	*psDroid, *displayDroid = NULL;
		size_t numberInGroup = 0;
		std::map<std::vector<uint32_t>, size_t> unitcounter;
		size_t most_droids_of_same_type_in_group = 0;

		for (psDroid = apsDroidLists[selectedPlayer]; psDroid != nullptr; psDroid = psDroid->psNext)
		{
			// display whatever unit occurs the most in this group
			if (psDroid->group == groupNumber)
			{
				// find the identifier for this droid
				std::vector<uint32_t> components = buildComponentsFromDroid(psDroid);
				if (++unitcounter[components] > most_droids_of_same_type_in_group)
				{
					most_droids_of_same_type_in_group = unitcounter[components];
					displayDroid = psDroid;
				}
				numberInGroup++;
			}
		}
		if (!numberInGroup)
		{
			groupCountLabel->setString("");
			displayBlank(xOffset, yOffset);
		} else
		{
			displayIMD(AtlasImage(), ImdObject::Droid(displayDroid), xOffset, yOffset);
			groupCountLabel->setString(WzString::fromUtf8(astringf("%u", numberInGroup)));
		}
	}
	std::string getTip() override
	{
		return "Select / Assign Group Number: " + std::to_string(groupNumber);
	}
	bool isHighlighted() const override
	{
		return false;
	}
};

void GroupsForum::display(int xOffset, int yOffset)
{
	// draw the background
	BaseWidget::display(xOffset, yOffset);
}

void GroupsForum::initialize()
{
	// the layout should be like this when the build menu is open
	id = IDOBJ_GROUP;
	setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(OBJ_BACKX, OBJ_BACKY - 80, OBJ_BACKWIDTH, OBJ_BACKHEIGHT - 40);
	}));
	addTabList();
	// create the 11 buttons for each group
	for (size_t i = 1; i <= 10; i++)
	{
		// check if the 10th group works
		auto buttonHolder = std::make_shared<WIDGET>();
		groupsList->addWidgetToLayout(buttonHolder);
		auto groupButton = makeGroupButton(i % 10);
		buttonHolder->attach(groupButton);
		groupButton->setGeometry(0, 0, OBJ_BUTWIDTH, OBJ_BUTHEIGHT);
	}
}

void GroupsForum::addTabList()
{
	attach(groupsList = IntListTabWidget::make());
	groupsList->id = IDOBJ_GROUP;
	groupsList->setChildSize(OBJ_BUTWIDTH, OBJ_BUTHEIGHT * 2);
	groupsList->setChildSpacing(OBJ_GAP, OBJ_GAP);
	int groupListWidth = OBJ_BUTWIDTH * 5 + STAT_GAP * 4;
	groupsList->setGeometry((OBJ_BACKWIDTH - groupListWidth) / 2, OBJ_TABY, groupListWidth, OBJ_BACKHEIGHT - OBJ_TABY);
}

std::shared_ptr<GroupButton> GroupsForum::makeGroupButton(size_t groupNumber)
{
	return GroupButton::make(groupNumber);
}



