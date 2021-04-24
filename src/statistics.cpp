#include "statistics.h"

#include "lib/widget/button.h"

#include "hci.h"
#include "intdisplay.h"
#include "frontend.h"
#include "multiint.h"
#include "multistat.h"
#include "intdisplay.h"
#include "research.h"
#include "frend.h"
#include "scores.h"
// #include "string_ext.h"
#include "lib/ivis_opengl/bitimage.h"

#define STATISTICS_WINDOW_HEIGHT_MAX 440

#define TITLE_TOP_PADDING 5
#define TITLE_HEIGHT 24
#define TITLE_BOTTOM (TITLE_TOP_PADDING + TITLE_HEIGHT)

const PIELIGHT WZCOL_DEBUG_FILL_COLOR = pal_RGBA(25, 0, 110, 220);
const PIELIGHT WZCOL_DEBUG_FILL_COLOR_DARK = pal_RGBA(10, 0, 70, 250);
const PIELIGHT WZCOL_DEBUG_BORDER_LIGHT = pal_RGBA(255, 255, 255, 80);
const PIELIGHT WZCOL_DEBUG_INDETERMINATE_PROGRESS_COLOR = pal_RGBA(240, 240, 255, 180);

static std::shared_ptr<W_SCREEN> statisticsScreen = nullptr;
static std::shared_ptr<StatisticsWindow> statisticsDialog = nullptr;

static std::shared_ptr<W_BUTTON> makeCornerButton(const char* text);

std::vector<double> KDhistory;

void AddKDtoHistory(double a) {
	KDhistory.push_back(a);
}

void StatisticsHistoryUpdate() {
	if(missionData.unitsLost == 0) {
		AddKDtoHistory(getSelectedPlayerUnitsKilled());
	} else {
		AddKDtoHistory((double)getSelectedPlayerUnitsKilled() / (double)missionData.unitsLost);
	}
}

StatisticsWindow::StatisticsWindow() {

}

StatisticsWindow::~StatisticsWindow() {

}

void StatisticsWindow::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int x1 = x0 + width();
	int y1 = y0 + height();

	// draw "drop-shadow"
	int dropShadowX0 = std::max<int>(x0 - 6, 0);
	int dropShadowY0 = std::max<int>(y0 - 6, 0);
	int dropShadowX1 = std::min<int>(x1 + 6, pie_GetVideoBufferWidth());
	int dropShadowY1 = std::min<int>(y1 + 6, pie_GetVideoBufferHeight());
	pie_UniTransBoxFill((float)dropShadowX0, (float)dropShadowY0, (float)dropShadowX1, (float)dropShadowY1, pal_RGBA(0, 0, 0, 40));

	// draw background + border
	pie_UniTransBoxFill((float)x0, (float)y0, (float)x1, (float)y1, pal_RGBA(5, 0, 15, 170));
	iV_Box(x0, y0, x1, y1, pal_RGBA(255, 255, 255, 150));

	// draw title
	int titleXPadding = 10;
	cachedTitleText.setText(_("Stats"), font_regular_bold);

	Vector2i textBoundingBoxOffset(0, 0);
	textBoundingBoxOffset.y = y0 + TITLE_TOP_PADDING + (TITLE_HEIGHT - cachedTitleText.lineSize()) / 2;

	int fw = cachedTitleText.width();
	int fx = x0 + titleXPadding + (width() - titleXPadding - fw) / 2;
	float fy = float(textBoundingBoxOffset.y) - float(cachedTitleText.aboveBase());
	cachedTitleText.render(textBoundingBoxOffset.x + fx, fy, WZCOL_FORM_TEXT);

	int KDsize = KDhistory.size();
	int LastRecords = 550;
	int GraphOX = 10;
	int GraphOY = 30;
	int GraphH = 200;
	double PeriodMax = 1.0;

	float GboxX0 = x0+GraphOX;
	float GboxY0 = y0+GraphOY;
	float GboxX1 = GboxX0+LastRecords;
	float GboxY1 = GboxY0+GraphH+1;
	pie_UniTransBoxFill(GboxX0, GboxY0, GboxX1, GboxY1, pal_RGBA(5, 0, 15, 170));
	iV_Box(GboxX0, GboxY0, GboxX1, GboxY1, pal_RGBA(255, 255, 255, 150));
	for(int s = 0; s < LastRecords; s++) {
		if(KDsize-LastRecords+s >= 0) {
			if(abs(KDhistory[KDsize-LastRecords+s]) > PeriodMax) {
				PeriodMax = abs(KDhistory[KDsize-LastRecords+s]);
			}
		}
	}
	cachedCountsText.setText(astringf("%d measures, %.3lf max, %.3lf latest", KDsize, PeriodMax, KDhistory.back()), font_regular);
	cachedCountsText.render(x0+10, y0+231+20, WZCOL_TEXT_BRIGHT);
	// Line1->setString(astringf("max: %.5f total %d", PeriodMax, KDsize));
	for(int s = 0; s < LastRecords; s++) {
		int p = KDsize-LastRecords+s;
		if(p < 0) {
			continue;
		}
		double a = KDhistory[p];
		double ap = a;
		if(p+1 < KDsize) {
			ap = KDhistory[p+1];
		}
		int centerY = GboxY0+GraphH/2+1;
		const PIELIGHT cred = pal_RGBA(255, 50, 50, 255);
		const PIELIGHT credbright = pal_RGBA(255, 10, 10, 255);
		const PIELIGHT creddull = pal_RGBA(255, 90, 90, 255);
		const PIELIGHT cgreen = pal_RGBA(50, 255, 50, 255);
		const PIELIGHT cgreenbright = pal_RGBA(10, 255, 10, 255);
		const PIELIGHT cgreendull = pal_RGBA(90, 255, 90, 255);
		PIELIGHT c = pal_RGBA(255, 255, 255, 255);
		int d = 0;
		if(a > 1) {
			d = -(((GraphH/2)/PeriodMax)*KDhistory[p]-GraphH/2);
			if(ap < a) {
				c = cgreendull;
			} else if(ap > a) {
				c = cgreenbright;
			} else {
				c = cgreen;
			}
		} else if(a < 1) {
			d = ((GraphH/2)/PeriodMax) - ((GraphH/2)/PeriodMax)*KDhistory[p];
			if(ap > a) {
				c = creddull;
			} else if(ap < a) {
				c = credbright;
			} else {
				c = cred;
			}
		} else {
			iV_Line(GboxX0+s, centerY, GboxX0+s-1, centerY, c);
			continue;
		}
		iV_Line(GboxX0+s, centerY, GboxX0+s, centerY+d, c);
	}
}

std::shared_ptr<StatisticsWindow> StatisticsWindow::make() {
	// copypasted straight from WZScriptDebugger::make
	auto result = std::make_shared<StatisticsWindow>();

	result->enableMinimizing("Stats", WZCOL_FORM_TEXT);

	result->id = MULTIOP_OPTIONS;
	int newFormWidth = FRONTEND_TOPFORM_WIDEW;
	result->setGeometry((pie_GetVideoBufferWidth() - newFormWidth) / 2, 10, newFormWidth, pie_GetVideoBufferHeight() - 50);
	result->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		// update the height
		int newHeight = std::min<int>(pie_GetVideoBufferHeight() - 50, STATISTICS_WINDOW_HEIGHT_MAX);
		// update the x if necessary to ensure the entire form is visible
		int x0 = std::min(psWidget->x(), pie_GetVideoBufferWidth() - psWidget->width());
		// update the y if necessary to ensure the entire form is visible
		int y0 = std::min(psWidget->y(), pie_GetVideoBufferHeight() - newHeight);
		psWidget->setGeometry(x0, y0, psWidget->width(), newHeight);
	}));
	result->userMovable = true;

	auto minimizeButton = makeCornerButton("\u21B8"); // ↸
	result->attach(minimizeButton);
	minimizeButton->addOnClickHandler([](W_BUTTON& button){
		auto psParent = std::dynamic_pointer_cast<StatisticsWindow>(button.parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		widgScheduleTask([psParent](){
			psParent->toggleMinimized();
		});
	});

	W_BUTINIT sButInit;
	sButInit.id = IDOBJ_CLOSE;
	sButInit.pTip = _("Close");
	sButInit.pDisplay = intDisplayImageHilight;
	sButInit.UserData = PACKDWORD_TRI(0, IMAGE_CLOSEHILIGHT, IMAGE_CLOSE);
	auto closeButton = std::make_shared<W_BUTTON>(&sButInit);
	result->attach(closeButton);
	closeButton->addOnClickHandler([](W_BUTTON& button){
		auto psParent = std::dynamic_pointer_cast<StatisticsWindow>(button.parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		widgScheduleTask([](){
			statisticsWindowShutdown();
		});
	});
	closeButton->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		auto psParent = std::dynamic_pointer_cast<StatisticsWindow>(psWidget->parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		psWidget->setGeometry(psParent->width() - CLOSE_WIDTH, 0, CLOSE_WIDTH, CLOSE_HEIGHT);
	}));

	// result->Line1 = std::make_shared<W_LABEL>();
	// result->attach(result->Line1);
	// result->Line1->setFont(font_regular, WZCOL_FORM_TEXT);
	// result->Line1->setString("Initializing...");
	// result->Line1->setGeometry(10, 131, result->Line1->getMaxLineWidth(), iV_GetTextLineSize(font_regular));


	return result;
}

void StatisticsWindow::run(W_CONTEXT *psContext) {
	this->W_FORM::run(psContext);
}

struct CornerButtonDisplayCache {
	WzText text;
};
static void CornerButtonDisplayFunc(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset) {
	assert(psWidget->pUserData != nullptr);
	CornerButtonDisplayCache& cache = *static_cast<CornerButtonDisplayCache*>(psWidget->pUserData);

	W_BUTTON *psButton = dynamic_cast<W_BUTTON*>(psWidget);
	ASSERT_OR_RETURN(, psButton, "psWidget is null");

	int x0 = psButton->x() + xOffset;
	int y0 = psButton->y() + yOffset;
	int x1 = x0 + psButton->width();
	int y1 = y0 + psButton->height();

	bool haveText = !psButton->pText.isEmpty();

//	bool isDown = (psButton->getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	bool isDisabled = (psButton->getState() & WBUT_DISABLE) != 0;
	bool isHighlight = (psButton->getState() & WBUT_HIGHLIGHT) != 0;

	if (isHighlight) {
		pie_UniTransBoxFill(x0+1.0f, static_cast<float>(y0 + 1), static_cast<float>(x1 - 1), static_cast<float>(y1 - 1), WZCOL_DEBUG_FILL_COLOR);
		iV_Box(x0 + 1, y0 + 1, x1 - 1, y1 - 1, pal_RGBA(255, 255, 255, 150));
	}

	if (haveText) {
		cache.text.setText(psButton->pText.toUtf8(), psButton->FontID);
		int fw = cache.text.width();
		int fx = x0 + (psButton->width() - fw) / 2;
		int fy = y0 + (psButton->height() - cache.text.lineSize()) / 2 - cache.text.aboveBase();
		PIELIGHT textColor = WZCOL_FORM_TEXT;
		if (isDisabled)
		{
			cache.text.render(fx + 1, fy + 1, WZCOL_FORM_LIGHT);
			textColor = WZCOL_FORM_DISABLE;
		}
		cache.text.render(fx, fy, textColor);
	}
	if (isDisabled) {
		iV_TransBoxFill(x0, y0, x0 + psButton->width(), y0 + psButton->height());
	}
}
static std::shared_ptr<W_BUTTON> makeCornerButton(const char* text) {
	auto button = std::make_shared<W_BUTTON>();
	button->setString(text);
	button->FontID = font_regular_bold;
	button->displayFunction = CornerButtonDisplayFunc;
	button->pUserData = new CornerButtonDisplayCache();
	button->setOnDelete([](WIDGET *psWidget) {
		assert(psWidget->pUserData != nullptr);
		delete static_cast<CornerButtonDisplayCache *>(psWidget->pUserData);
		psWidget->pUserData = nullptr;
	});
	int minButtonWidthForText = iV_GetTextWidth(text, button->FontID);
	int buttonSize = std::max({minButtonWidthForText + 10, 18, iV_GetTextLineSize(button->FontID)});
	button->setGeometry(0, 0, buttonSize, buttonSize);
	return button;
}

bool statisticsWindowShutdown() {
	if (statisticsScreen) {
		widgRemoveOverlayScreen(statisticsScreen);
	}
	if (statisticsDialog) {
		statisticsDialog->deleteLater();
		statisticsDialog = nullptr;
	}
	statisticsScreen = nullptr;
	return true;
}

void statisticsWindowShow() {
	if(statisticsDialog) {
		debug(LOG_INFO, "Tried open tech view twice");//%p %p", techviewDialog, techviewScreen);
	}
	statisticsScreen = W_SCREEN::make();
	widgRegisterOverlayScreen(statisticsScreen, std::numeric_limits<uint16_t>::max() - 3); // lol pastdue said it's alright
	statisticsScreen->psForm->hide(); // from wzscriptdebug
	statisticsDialog = StatisticsWindow::make();
	statisticsScreen->psForm->attach(statisticsDialog);
}
