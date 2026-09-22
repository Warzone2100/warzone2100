/*
	This file is part of Warzone 2100.
	Copyright (C) 2021-2023  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "lib/widget/button.h"
#include "src/hci/objects_stats.h"
#include "src/structuredef.h"
#include "repair_st.h"
#include "../qtscript.h"
#include "../display.h"


STRUCTURE *RepairStationController::highlightedStation = nullptr;

void RepairStationController::startDeliveryPointPosition()
{
	auto rep_station = getHighlightedObject();
	ASSERT_NOT_NULLPTR_OR_RETURN(, rep_station);

	auto psFlag = ((REPAIR_FACILITY*) rep_station->pFunctionality)->psDeliveryPoint;
	if (psFlag)
	{
		startDeliveryPosition(psFlag);
	}
}

void RepairStationController::setHighlightedObject(BASE_OBJECT *object, bool jumpToHighlightedStatsObject)
{
	if (object == nullptr)
	{
		highlightedStation = nullptr;
		return;
	}

	auto rep_station = castStructure(object);
	if (rep_station == nullptr)
	{
		highlightedStation = nullptr;
		return;
	}

	highlightedStation = rep_station;
}

class RepairStationForm: public ObjectStatsForm
{
private:
	typedef ObjectStatsForm BaseWidget;
	using BaseWidget::BaseWidget;

public:
	static std::shared_ptr<RepairStationForm> make(const std::shared_ptr<RepairStationController> &controller)
	{
		class make_shared_enabler: public RepairStationForm {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->controller = controller;
		widget->initialize();
		return widget;
	}

	std::shared_ptr<StatsFormButton> makeOptionButton(size_t buttonIndex) const override
	{
		return nullptr;
	}

protected:
	RepairStationController &getController() const override
	{
		return *controller.get();
	}

private:
	void initialize() override
	{
		BaseWidget::initialize();
		attach(std::make_shared<DeliveryPointButton>(controller));
	}

	std::shared_ptr<RepairStationController> controller;

	class DeliveryPointButton: public W_BUTTON
	{
	private:
		typedef W_BUTTON BaseWidget;

	public:
		DeliveryPointButton(const std::shared_ptr<RepairStationController> &controller): BaseWidget(), controller(controller)
		{
			style |= WBUT_SECONDARY;
			move(4, STAT_SLDY);
			setTip(_("Repair Station Rally Point"));

			auto weakController = std::weak_ptr<RepairStationController>(controller);
			addOnClickHandler([weakController](W_BUTTON &) {
				auto controller = weakController.lock();
				ASSERT_NOT_NULLPTR_OR_RETURN(, controller);
				controller->startDeliveryPointPosition();
			});
		}

	protected:
		void display(int xOffset, int yOffset)
		{
			updateLayout();
			BaseWidget::display(xOffset, yOffset);
		}

	private:
		void updateLayout()
		{
			setImages(AtlasImage(IntImages, IMAGE_FDP_UP), AtlasImage(IntImages, IMAGE_FDP_DOWN), AtlasImage(IntImages, IMAGE_FDP_HI));
		}

		std::shared_ptr<RepairStationController> controller;
	};
};

bool RepairStationController::showInterface()
{
	updateHighlighted();
	auto objectsForm = RepairStationForm::make(shared_from_this());
	psWScreen->psForm->attach(objectsForm);
	displayStatsForm();
	return true;
}

std::shared_ptr<StatsForm> RepairStationController::makeStatsForm()
{
	return RepairStationForm::make(shared_from_this());
}
