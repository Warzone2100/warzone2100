#ifndef STATISTICS_H_DEFINED
#define STATISTICS_H_DEFINED

#include "lib/widget/widget.h"
#include "lib/widget/form.h"
#include "lib/widget/jsontable.h"
#include "lib/widget/dropdown.h"

#include "lib/framework/frame.h"

#include "objectdef.h"

class StatisticsWindow : public W_FORM {
public:
	StatisticsWindow();
	~StatisticsWindow();

	static std::shared_ptr<StatisticsWindow> make();

	virtual void display(int xOffset, int yOffset) override;
	virtual void run(W_CONTEXT *psContext) override;

private:
	WzText cachedTitleText;
	WzText cachedTextMin;
	WzText cachedTextMax;
};

bool statisticsWindowShutdown();
void statisticsWindowShow();

void StatisticsHistoryUpdate();

#endif /* end of include guard: STATISTICS_H_DEFINED */
