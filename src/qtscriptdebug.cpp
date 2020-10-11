/*
	This file is part of Warzone 2100.
	Copyright (C) 2013-2020  Warzone 2100 Project

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
/**
 * @file qtscriptdebug.cpp
 *
 * New scripting system - debug GUI
 */

#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__) && (9 <= __GNUC__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdeprecated-copy" // Workaround Qt < 5.13 `deprecated-copy` issues with GCC 9
#endif

// **NOTE: Qt headers _must_ be before platform specific headers so we don't get conflicts.
#include <QtCore/QHash>
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptValue>
#include <QtScript/QScriptValueIterator>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtGui/QStandardItemModel>

#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__) && (9 <= __GNUC__)
# pragma GCC diagnostic pop // Workaround Qt < 5.13 `deprecated-copy` issues with GCC 9
#endif

#include "qtscriptdebug.h"

#ifndef GLM_ENABLE_EXPERIMENTAL
	#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/string_cast.hpp>

#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/wzconfig.h"
#include "lib/netplay/netplay.h"

#include "action.h"
#include "difficulty.h"
#include "multiplay.h"
#include "objects.h"
#include "power.h"
#include "hci.h"
#include "display.h"
#include "keybind.h"
#include "loop.h"
#include "mission.h"
#include "message.h"
#include "transporter.h"
#include "template.h"
#include "multiint.h"

#include "qtscript.h"
#include "qtscriptfuncs.h"

typedef QList<QStandardItem *> QStandardItemList;

static ScriptDebugger *globalDialog = nullptr;
static jsDebugShutdownHandlerFunction globalDialogShutdownHandler;
bool doUpdateModels = false;

// ----------------------------------------------------------

QPushButton *ScriptDebugger::createButton(const QString &text, const char *slot, QWidget *parent)
{
	QPushButton *button = new QPushButton(text, parent);
	connect(button, SIGNAL(pressed()), this, slot);
	button->setDefault(false);
	button->setAutoDefault(false);
	return button;
}

void ScriptDebugger::researchButtonClicked()
{
	kf_FinishAllResearch();
}

void ScriptDebugger::sensorsButtonClicked()
{
	kf_ToggleSensorDisplay();
}

void ScriptDebugger::deityButtonClicked()
{
	kf_ToggleGodMode();
}

void ScriptDebugger::gatewayButtonClicked()
{
	kf_ToggleShowGateways();
}

void ScriptDebugger::weatherButtonClicked()
{
	kf_ToggleWeather();
}

void ScriptDebugger::revealButtonClicked()
{
	kf_ToggleVisibility();
}

void ScriptDebugger::shadowButtonClicked()
{
	setDrawShadows(!getDrawShadows());
}

void ScriptDebugger::fogButtonClicked()
{
	kf_ToggleFog();
}

static const QStringList view_type = { "RES", "RPL", "PROX", "RPLX", "BEACON" };

static void fillViewdataModel(QStandardItemModel &m)
{
	int row = 0;
	m.setRowCount(0);
	m.setHorizontalHeaderLabels({"Name", "Type", "Source"});
	std::vector<WzString> keys = getViewDataKeys();
	for (const WzString& key : keys)
	{
		VIEWDATA *ptr = getViewData(key);
		ASSERT(ptr->type < view_type.size(), "Bad viewdata type");
		m.setItem(row, 0, new QStandardItem(QString::fromUtf8(key.toUtf8().c_str())));
		m.setItem(row, 1, new QStandardItem(view_type.at(ptr->type)));
		m.setItem(row, 2, new QStandardItem(QString::fromUtf8(ptr->fileName.toUtf8().c_str())));
		row++;
	}
}

static void fillMessageModel(QStandardItemModel &m)
{
	const QStringList msg_type = { "RESEARCH", "CAMPAIGN", "MISSION", "PROXIMITY" };
	const QStringList msg_data_type = { "DEFAULT", "BEACON" };
	const QStringList obj_type = { "DROID", "STRUCTURE", "FEATURE", "PROJECTILE", "TARGET" };
	int row = 0;
	m.setRowCount(0);
	m.setHorizontalHeaderLabels({"ID", "Type", "Data Type", "Player", "Name", "ViewData Type"});
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		for (const MESSAGE *psCurr = apsMessages[i]; psCurr != nullptr; psCurr = psCurr->psNext)
		{
			ASSERT(psCurr->type < msg_type.size(), "Bad message type");
			ASSERT(psCurr->dataType < msg_data_type.size(), "Bad viewdata type");
			m.setItem(row, 0, new QStandardItem(QString::number(psCurr->id)));
			m.setItem(row, 1, new QStandardItem(msg_type.at(psCurr->type)));
			m.setItem(row, 2, new QStandardItem(msg_data_type.at(psCurr->dataType)));
			m.setItem(row, 3, new QStandardItem(QString::number(psCurr->player)));
			ASSERT(!psCurr->pViewData || !psCurr->psObj, "Both viewdata and object in message should be impossible!");
			if (psCurr->pViewData)
			{
				ASSERT(psCurr->pViewData->type < view_type.size(), "Bad viewdata type");
				m.setItem(row, 4, new QStandardItem(QString::fromUtf8(psCurr->pViewData->name.toUtf8().c_str())));
				m.setItem(row, 5, new QStandardItem(view_type.at(psCurr->pViewData->type)));
			}
			else if (psCurr->psObj)
			{
				m.setItem(row, 4, new QStandardItem(objInfo(psCurr->psObj)));
				m.setItem(row, 5, new QStandardItem(obj_type.at(psCurr->psObj->type)));
			}
			row++;
		}
	}
}

static void fillMainModel(QStandardItemModel &m)
{
	const QStringList lev_type = { "LDS_COMPLETE", "LDS_CAMPAIGN", "LDS_CAMSTART", "LDS_CAMCHANGE",
	                               "LDS_EXPAND", "LDS_BETWEEN", "LDS_MKEEP", "LDS_MCLEAR",
	                               "LDS_EXPAND_LIMBO", "LDS_MKEEP_LIMBO", "LDS_NONE",
	                               "LDS_MULTI_TYPE_START", "CAMPAIGN", "", "SKIRMISH", "", "", "",
	                               "MULTI_SKIRMISH2", "MULTI_SKIRMISH3", "MULTI_SKIRMISH4" };
	const QStringList difficulty_type = { "EASY", "NORMAL", "HARD", "INSANE" };
	int row = 0;
	m.setRowCount(0);
	m.setHorizontalHeaderLabels({"Key", "Value"});

#define B2Q(_b) (_b ? QString("true") : QString("false"))
#define KEYVAL(_key, _val) m.setItem(row, 0, new QStandardItem(_key)); m.setItem(row, 1, new QStandardItem(_val)); row++;

	int8_t gameType = static_cast<int8_t>(game.type);
	int8_t missionType = static_cast<int8_t>(mission.type);
	ASSERT(gameType < lev_type.size() && gameType != 13 && gameType != 15 && gameType != 16 && gameType != 17, "Bad LEVEL_TYPE for game.type");
	KEYVAL("game.type", lev_type.at(gameType));
	KEYVAL("game.scavengers", B2Q(game.scavengers));
	KEYVAL("game.map", QString(game.map));
	KEYVAL("game.maxPlayers", QString::number(game.maxPlayers));
	KEYVAL("transporterGetLaunchTime", QString::number(transporterGetLaunchTime()));
	KEYVAL("missionIsOffworld", B2Q(missionIsOffworld()));
	KEYVAL("missionCanReEnforce", B2Q(missionCanReEnforce()));
	KEYVAL("missionForReInforcements", B2Q(missionForReInforcements()));
	ASSERT(missionType < lev_type.size() && missionType != 13 && missionType != 15 && missionType != 16 && missionType != 17, "Bad LEVEL_TYPE for mission.type");
	KEYVAL("mission.type", lev_type.at(missionType));
	KEYVAL("getDroidsToSafetyFlag", B2Q(getDroidsToSafetyFlag()));
	KEYVAL("scavengerSlot", QString::number(scavengerSlot()));
	KEYVAL("scavengerPlayer", QString::number(scavengerPlayer()));
	KEYVAL("bMultiPlayer", B2Q(bMultiPlayer));
	ASSERT(getDifficultyLevel() < difficulty_type.size(), "Bad DIFFICULTY_LEVEL");
	KEYVAL("difficultyLevel", difficulty_type.at(getDifficultyLevel()));
	KEYVAL("loopPieCount", QString::number(loopPieCount));
	KEYVAL("loopPolyCount", QString::number(loopPolyCount));
	KEYVAL("allowDesign", B2Q(allowDesign));
	KEYVAL("includeRedundantDesigns", B2Q(includeRedundantDesigns));

	int features;
	int droids;
	int structures;
	objCount(&droids, &structures, &features);
	KEYVAL("No. droids", QString::number(droids));
	KEYVAL("No. structures", QString::number(structures));
	KEYVAL("No. features", QString::number(features));
}

static void fillPlayerModel(QStandardItemModel &m, int i)
{
	int row = 0;
	m.setRowCount(0);
	m.setHorizontalHeaderLabels({"Key", "Value"});
	KEYVAL("ingame.skScores score", QString::number(ingame.skScores[i][0]));
	KEYVAL("ingame.skScores kills", QString::number(ingame.skScores[i][1]));
	KEYVAL("NetPlay.players.name", NetPlay.players[i].name);
	KEYVAL("NetPlay.players.position", QString::number(NetPlay.players[i].position));
	KEYVAL("NetPlay.players.colour", QString::number(NetPlay.players[i].colour));
	KEYVAL("NetPlay.players.allocated", QString::number(NetPlay.players[i].allocated));
	KEYVAL("NetPlay.players.team", QString::number(NetPlay.players[i].team));
	KEYVAL("NetPlay.players.ai", QString::number(NetPlay.players[i].ai));
	KEYVAL("NetPlay.players.difficulty", QString::number(static_cast<int8_t>(NetPlay.players[i].difficulty)));
	KEYVAL("NetPlay.players.autoGame", QString::number(NetPlay.players[i].autoGame));
	KEYVAL("NetPlay.players.IPtextAddress", NetPlay.players[i].IPtextAddress);
	KEYVAL("Current power", QString::number(getPower(i)));
	KEYVAL("Extracted power", QString::number(getExtractedPower(i)));
	KEYVAL("Wasted power", QString::number(getWastedPower(i)));

#undef B2Q
#undef KEYVAL
}

ScriptDebugger::ScriptDebugger(const MODELMAP &models, QStandardItemModel *triggerModel, QStandardItemModel *_labelModel)
: QDialog(nullptr, Qt::Window)
, labelModel(_labelModel)
{
	modelMap = models;

	// Add main page
	QWidget *mainWidget = new QWidget(this);
	QVBoxLayout *mainLayout = new QVBoxLayout;
	QHBoxLayout *placementLayout = new QHBoxLayout;
	placementLayout->addWidget(createButton("Add droids", SLOT(droidButtonClicked()), this));
	placementLayout->addWidget(createButton("Add structures", SLOT(structButtonClicked()), this));
	placementLayout->addWidget(createButton("Add features", SLOT(featButtonClicked()), this));
	mainLayout->addLayout(placementLayout);
	QHBoxLayout *miscLayout = new QHBoxLayout;
	miscLayout->addWidget(createButton("Research all", SLOT(researchButtonClicked()), this));
	miscLayout->addWidget(createButton("Show sensors", SLOT(sensorsButtonClicked()), this));
	miscLayout->addWidget(createButton("Shadows", SLOT(shadowButtonClicked()), this));
	miscLayout->addWidget(createButton("Fog", SLOT(fogButtonClicked()), this));
	mainLayout->addLayout(miscLayout);
	QHBoxLayout *worldLayout = new QHBoxLayout;
	worldLayout->addWidget(createButton("Show gateways", SLOT(gatewayButtonClicked()), this));
	worldLayout->addWidget(createButton("Reveal all", SLOT(deityButtonClicked()), this));
	worldLayout->addWidget(createButton("Weather", SLOT(weatherButtonClicked()), this));
	worldLayout->addWidget(createButton("Reveal mode", SLOT(revealButtonClicked()), this));
	mainLayout->addLayout(worldLayout);
	// selected player
	QHBoxLayout *selectedPlayerLayout = new QHBoxLayout;
	QLabel *selectPlayerLabel = new QLabel("Selected Player:", this);
	QComboBox *playerComboBox = new QComboBox(this);
	for (int i = 0; i < game.maxPlayers; i++)
	{
		playerComboBox->addItem(QString::number(i));
	}
	connect(playerComboBox, SIGNAL(activated(int)), this, SLOT(playerButtonClicked(int)));
	selectedPlayerLayout->addWidget(selectPlayerLabel);
	selectedPlayerLayout->addWidget(playerComboBox);
	mainLayout->addLayout(selectedPlayerLayout);
	// attach script
	QHBoxLayout *addAILayout = new QHBoxLayout;
	QLabel *addAILabel = new QLabel("Attach AI to player:", this);
	for (int i = 0; i < game.maxPlayers; i++)
	{
		aiPlayerComboBox.addItem(QString::number(i));
	}
	const std::vector<WzString> AIs = getAINames();
	for (const WzString &name : AIs)
	{
		aiScriptComboBox.addItem(QString::fromUtf8(name.toUtf8().c_str()));
	}
	addAILayout->addWidget(addAILabel);
	addAILayout->addWidget(&aiScriptComboBox);
	addAILayout->addWidget(&aiPlayerComboBox);
	addAILayout->addWidget(createButton("Attach", SLOT(attachScriptClicked()), this));
	mainLayout->addLayout(addAILayout);
	// power
	QHBoxLayout *powerLayout = new QHBoxLayout;
	QLabel *powerLabel = new QLabel("Power:", this);
	QLineEdit *powerLineEdit = new QLineEdit(this);
	powerLineEdit->setText(QString::number(getPower(selectedPlayer)));
	connect(powerLineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(powerEditing(const QString&)));
	connect(powerLineEdit, SIGNAL(returnPressed()), this, SLOT(powerEditingFinished()));
	powerLayout->addWidget(powerLabel);
	powerLayout->addWidget(powerLineEdit);
	mainLayout->addLayout(powerLayout);
	// globals view
	mainView.setModel(&mainModel);
	mainView.setSelectionMode(QAbstractItemView::NoSelection);
	mainView.setSelectionBehavior(QAbstractItemView::SelectRows);
	mainLayout->addWidget(&mainView);
	mainWidget->setLayout(mainLayout);
	fillMainModel(mainModel);
	mainView.resizeColumnToContents(0);
	tab.addTab(mainWidget, "Main");

	selectedView.setModel(&selectedModel);
	selectedView.setSelectionMode(QAbstractItemView::NoSelection);
	selectedView.setSelectionBehavior(QAbstractItemView::SelectRows);
	tab.addTab(&selectedView, "Selected");

	// Add globals
	QTabWidget *contextsTab = new QTabWidget(this);
	for (MODELMAP::const_iterator i = models.constBegin(); i != models.constEnd(); ++i)
	{
		QWidget *dummyWidget = new QWidget(this);
		QScriptEngine *engine = i.key();
		QStandardItemModel *m = i.value();
		m->setParent(this); // take ownership to avoid memory leaks
		QTreeView *view = new QTreeView(this);
		view->setSelectionMode(QAbstractItemView::NoSelection);
		view->setModel(m);
		QString scriptName = engine->globalObject().property("scriptName").toString();
		int player = engine->globalObject().property("me").toInt32();
		QLineEdit *lineEdit = new QLineEdit(this);
		QVBoxLayout *layout = new QVBoxLayout;
		QHBoxLayout *layout2 = new QHBoxLayout;
		QPushButton *updateButton = new QPushButton("Update", this);
		QPushButton *button = new QPushButton("Run", this);
		connect(button, &QPushButton::pressed, [=] { runClicked(engine); });
		connect(updateButton, SIGNAL(pressed()), this, SLOT(updateModels()));
		editMap.insert(engine, lineEdit); // store this for slot
		layout->addWidget(view);
		layout2->addWidget(updateButton);
		layout2->addWidget(lineEdit);
		layout2->addWidget(button);
		layout->addLayout(layout2);
		dummyWidget->setLayout(layout);
		contextsTab->addTab(dummyWidget, scriptName + ":" + QString::number(player));
	}
	tab.addTab(contextsTab, "Contexts");

	QTabWidget *playersTab = new QTabWidget(this);
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		QTreeView *view = new QTreeView(this);
		view->setSelectionMode(QAbstractItemView::NoSelection);
		view->setSelectionBehavior(QAbstractItemView::SelectRows);
		view->setModel(&playerModel[i]);
		fillPlayerModel(playerModel[i], i);
		view->resizeColumnToContents(0);
		playersTab->addTab(view, QString::number(i));
	}
	tab.addTab(playersTab, "Players");

	// Add triggers
	triggerModel->setParent(this); // take ownership to avoid memory leaks
	triggerView.setModel(triggerModel);
	triggerView.resizeColumnToContents(0);
	triggerView.setSelectionMode(QAbstractItemView::NoSelection);
	triggerView.setSelectionBehavior(QAbstractItemView::SelectRows);
	tab.addTab(&triggerView, "Triggers");

	// Add messages
	QTabWidget *messTab = new QTabWidget(this);
	messageView.setModel(&messageModel);
	messageView.setSelectionMode(QAbstractItemView::NoSelection);
	messageView.setSelectionBehavior(QAbstractItemView::SelectRows);
	messTab->addTab(&messageView, "Created");
	fillMessageModel(messageModel);
	messageView.resizeColumnToContents(0);
	// Add viewdata
	viewdataView.setModel(&viewdataModel);
	viewdataView.setSelectionMode(QAbstractItemView::NoSelection);
	viewdataView.setSelectionBehavior(QAbstractItemView::SelectRows);
	messTab->addTab(&viewdataView, "Viewdata");
	fillViewdataModel(viewdataModel);
	viewdataView.resizeColumnToContents(0);
	tab.addTab(messTab, "Messages");

	// Add labels
	labelModel->setParent(this); // take ownership to avoid memory leaks
	labelView.setModel(labelModel);
	labelView.resizeColumnToContents(0);
	labelView.setSelectionMode(QAbstractItemView::SingleSelection);
	labelView.setSelectionBehavior(QAbstractItemView::SelectRows);
	connect(&labelView, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(labelClickedIdx(const QModelIndex &)));
	QPushButton *buttonShow = new QPushButton("Show selected", this);
	QPushButton *buttonShowAll = new QPushButton("Show all", this);
	QPushButton *buttonShowActive = new QPushButton("Show active", this);
	QPushButton *buttonClear = new QPushButton("Clear", this);
	connect(buttonShow, SIGNAL(pressed()), this, SLOT(labelClicked()));
	connect(buttonShowAll, SIGNAL(pressed()), this, SLOT(labelClickedAll()));
	connect(buttonShowActive, SIGNAL(pressed()), this, SLOT(labelClickedActive()));
	connect(buttonClear, SIGNAL(pressed()), this, SLOT(labelClear()));
	QVBoxLayout *labelLayout = new QVBoxLayout;
	labelLayout->addWidget(&labelView);
	labelLayout->addWidget(buttonShow);
	labelLayout->addWidget(buttonShowAll);
	labelLayout->addWidget(buttonShowActive);
	labelLayout->addWidget(buttonClear);
	QWidget *dummyWidget = new QWidget(this);
	dummyWidget->setLayout(labelLayout);
	tab.addTab(dummyWidget, "Labels");

	// Handle dialog closing
	connect(this, SIGNAL(finished(int)), this, SLOT(debuggerClosed()));

	// Set up dialog
	QHBoxLayout *layout = new QHBoxLayout;
	layout->addWidget(&tab);
	setLayout(layout);
	resize(640, 700);
	setSizeGripEnabled(true);
	show();
	raise();
	powerLineEdit->setFocusPolicy(Qt::StrongFocus);
	powerLineEdit->setFocus();
	activateWindow();
}

void ScriptDebugger::debuggerClosed()
{
	// Asynchronously trigger a jsDebugShutdown (on the main thread) outside of signal processing
	wzAsyncExecOnMainThread([] {
		jsDebugShutdown();
	});
}

void ScriptDebugger::attachScriptClicked()
{
	const QString &script = aiScriptComboBox.currentText();
	const int player = aiPlayerComboBox.currentIndex();
	jsAutogameSpecific(QStringToWzString("multiplay/skirmish/" + script), player);
	debug(LOG_INFO, "Script attached - close and reopen debug window to see its context");
}

void ScriptDebugger::updateMessages()
{
	fillMessageModel(messageModel);
	messageView.resizeColumnToContents(0);
}

void ScriptDebugger::update()
{
	fillMainModel(mainModel);
	mainView.resizeColumnToContents(0);
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		fillPlayerModel(playerModel[i], i);
	}
}

void ScriptDebugger::runClicked(QObject *obj)
{
	QScriptEngine *engine = (QScriptEngine *)obj;
	QLineEdit *line = editMap.value(engine);
	if (line)
	{
		jsEvaluate(engine, line->text());
	}
}

void ScriptDebugger::updateModels()
{
	doUpdateModels = true;
}

void ScriptDebugger::labelClear()
{
	clearMarks();
}

void ScriptDebugger::labelClickedAll()
{
	clearMarks();
	markAllLabels(false);
}

void ScriptDebugger::labelClickedActive()
{
	clearMarks();
	markAllLabels(true);
}

void ScriptDebugger::labelClicked()
{
	QItemSelectionModel *selected = labelView.selectionModel();
	if (selected)
	{
		QModelIndex idx = selected->currentIndex();
		QStandardItem *item = labelModel->itemFromIndex(labelModel->index(idx.row(), 0));
		if (item)
		{
			showLabel(item->text());
		}
	}
}

void ScriptDebugger::powerEditing(const QString &value)
{
	powerValue = value.toInt();
}

void ScriptDebugger::powerEditingFinished()
{
	setPower(selectedPlayer, powerValue);
}

void ScriptDebugger::playerButtonClicked(int value)
{
	// Do not change realSelectedPlayer here, so game doesn't pause.
	const int oldSelectedPlayer = selectedPlayer;
	selectedPlayer = value;
	NetPlay.players[selectedPlayer].allocated = !NetPlay.players[selectedPlayer].allocated;
	NetPlay.players[oldSelectedPlayer].allocated = !NetPlay.players[oldSelectedPlayer].allocated;
}

void ScriptDebugger::droidButtonClicked()
{
	intOpenDebugMenu(OBJ_DROID);
}

void ScriptDebugger::structButtonClicked()
{
	intOpenDebugMenu(OBJ_STRUCTURE);
}

void ScriptDebugger::featButtonClicked()
{
	intOpenDebugMenu(OBJ_FEATURE);
}

void ScriptDebugger::labelClickedIdx(const QModelIndex &idx)
{
	QStandardItem *item = labelModel->itemFromIndex(labelModel->index(idx.row(), 0));
	if (item)
	{
		showLabel(item->text());
	}
}

static void setPair(int &row, QStandardItemModel &model, const QString &key, const QString &value)
{
	model.setItem(row, 0, new QStandardItem(key));
	model.setItem(row, 1, new QStandardItem(value));
	row++;
}

static void setPair(int &row, QStandardItemModel &model, const QStandardItemList& list)
{
	model.appendRow(list);
	row++;
}

static const char *getObjType(const BASE_OBJECT *psObj)
{
	switch (psObj->type)
	{
	case OBJ_DROID: return "Droid";
	case OBJ_STRUCTURE: return "Structure";
	case OBJ_FEATURE: return "Feature";
	case OBJ_PROJECTILE: return "Projectile";
	default: break;
	}
	return "Unknown";
}

template<typename T>
static QString arrayToString(const T *array, int length)
{
	QStringList l;
	for (int i = 0; i < length; i++)
	{
		l.append(QString::number(array[i]));
	}
	return l.join(", ");
}

// Using ^ to denote stats that are in templates, and as such do not change.
// Using : to denote stats that come from structure specializations.
QStandardItemList componentToString(const QString &name, const COMPONENT_STATS *psStats, int player)
{
	QStandardItem *key = new QStandardItem(name);
	QStandardItem *value = new QStandardItem(getName(psStats));
	key->appendRow(QStandardItemList{ new QStandardItem("^Id"), new QStandardItem(QString::fromUtf8(psStats->id.toUtf8().c_str())) });
	key->appendRow(QStandardItemList{ new QStandardItem("^Power"), new QStandardItem(QString::number(psStats->buildPower)) });
	key->appendRow(QStandardItemList{ new QStandardItem("^Build Points"), new QStandardItem(QString::number(psStats->buildPoints)) });
	key->appendRow(QStandardItemList{ new QStandardItem("^Weight"), new QStandardItem(QString::number(psStats->weight)) });
	key->appendRow(QStandardItemList{ new QStandardItem("^Hit points"), new QStandardItem(QString::number(psStats->getUpgrade(player).hitpoints)) });
	key->appendRow(QStandardItemList{ new QStandardItem("^Hit points +% of total"), new QStandardItem(QString::number(psStats->getUpgrade(player).hitpointPct)) });
	key->appendRow(QStandardItemList{ new QStandardItem("^Designable"), new QStandardItem(QString::number(psStats->designable)) });
	if (psStats->compType == COMP_BODY)
	{
		const BODY_STATS *psBody = (const BODY_STATS *)psStats;
		key->appendRow(QStandardItemList{ new QStandardItem("^Size"), new QStandardItem(QString::number(psBody->size)) });
		key->appendRow(QStandardItemList{ new QStandardItem("^Max weapons"), new QStandardItem(QString::number(psBody->weaponSlots)) });
		key->appendRow(QStandardItemList{ new QStandardItem("^Body class"), new QStandardItem(QString::fromUtf8(psBody->bodyClass.toUtf8().c_str())) });
	}
	else if (psStats->compType == COMP_PROPULSION)
	{
		const PROPULSION_STATS *psProp = (const PROPULSION_STATS *)psStats;
		key->appendRow(QStandardItemList{ new QStandardItem("^Hit points +% of body"), new QStandardItem(QString::number(psProp->upgrade[player].hitpointPctOfBody)) });
		key->appendRow(QStandardItemList{ new QStandardItem("^Max speed"), new QStandardItem(QString::number(psProp->maxSpeed)) });
		key->appendRow(QStandardItemList{ new QStandardItem("^Propulsion type"), new QStandardItem(QString::number(psProp->propulsionType)) });
		key->appendRow(QStandardItemList{ new QStandardItem("^Turn speed"), new QStandardItem(QString::number(psProp->turnSpeed)) });
		key->appendRow(QStandardItemList{ new QStandardItem("^Spin speed"), new QStandardItem(QString::number(psProp->spinSpeed)) });
		key->appendRow(QStandardItemList{ new QStandardItem("^Spin angle"), new QStandardItem(QString::number(psProp->spinAngle)) });
		key->appendRow(QStandardItemList{ new QStandardItem("^Skid decelaration"), new QStandardItem(QString::number(psProp->skidDeceleration)) });
		key->appendRow(QStandardItemList{ new QStandardItem("^Deceleration"), new QStandardItem(QString::number(psProp->deceleration)) });
		key->appendRow(QStandardItemList{ new QStandardItem("^Acceleration"), new QStandardItem(QString::number(psProp->acceleration)) });
	}
	else if (psStats->compType == COMP_BRAIN)
	{
		const BRAIN_STATS *psBrain = (const BRAIN_STATS *)psStats;
		QStringList ranks;
		for (const std::string &s : psBrain->rankNames)
		{
			ranks.append(QString::fromStdString(s));
		}
		QStringList thresholds;
		for (int t : psBrain->upgrade[player].rankThresholds)
		{
			thresholds.append(QString::number(t));
		}
		key->appendRow(QStandardItemList{ new QStandardItem("^Base command limit"), new QStandardItem(QString::number(psBrain->upgrade[player].maxDroids)) });
		key->appendRow(QStandardItemList{ new QStandardItem("^Extra command limit by level"), new QStandardItem(QString::number(psBrain->upgrade[player].maxDroidsMult)) });
		key->appendRow(QStandardItemList{ new QStandardItem("^Rank names"), new QStandardItem(ranks.join(", ")) });
		key->appendRow(QStandardItemList{ new QStandardItem("^Rank thresholds"), new QStandardItem(thresholds.join(", ")) });
	}
	else if (psStats->compType == COMP_REPAIRUNIT)
	{
		const REPAIR_STATS *psRepair = (const REPAIR_STATS *)psStats;
		key->appendRow(QStandardItemList{ new QStandardItem("^Repair time"), new QStandardItem(QString::number(psRepair->time)) });
		key->appendRow(QStandardItemList{ new QStandardItem("^Base repair points"), new QStandardItem(QString::number(psRepair->upgrade[player].repairPoints)) });
	}
	else if (psStats->compType == COMP_ECM)
	{
		const ECM_STATS *psECM = (const ECM_STATS *)psStats;
		key->appendRow(QStandardItemList{ new QStandardItem("^Base range"), new QStandardItem(QString::number(psECM->upgrade[player].range)) });
	}
	else if (psStats->compType == COMP_SENSOR)
	{
		const SENSOR_STATS *psSensor = (const SENSOR_STATS *)psStats;
		key->appendRow(QStandardItemList{ new QStandardItem("^Sensor type"), new QStandardItem(QString::number(psSensor->type)) });
		key->appendRow(QStandardItemList{ new QStandardItem("^Base range"), new QStandardItem(QString::number(psSensor->upgrade[player].range)) });
	}
	else if (psStats->compType == COMP_CONSTRUCT)
	{
		const CONSTRUCT_STATS *psCon = (const CONSTRUCT_STATS *)psStats;
		key->appendRow(QStandardItemList{ new QStandardItem("^Base construct points"), new QStandardItem(QString::number(psCon->upgrade[player].constructPoints)) });
	}
	return QStandardItemList { key, value };
}

void ScriptDebugger::selected(const BASE_OBJECT *psObj)
{
	selectedModel.setRowCount(0);
	selectedModel.setHorizontalHeaderLabels({"Key", "Value"});
	int row = 0;
	setPair(row, selectedModel, "Name", objInfo(psObj));
	setPair(row, selectedModel, "Type", getObjType(psObj));
	setPair(row, selectedModel, "Id", QString::number(psObj->id));
	setPair(row, selectedModel, "Player", QString::number(psObj->player));
	setPair(row, selectedModel, "Born", QString::number(psObj->born));
	setPair(row, selectedModel, "Died", QString::number(psObj->died));
	setPair(row, selectedModel, "Group", QString::number(psObj->group));
	setPair(row, selectedModel, "Watched tiles", QString::number(psObj->watchedTiles.size()));
	setPair(row, selectedModel, "Last hit", QString::number(psObj->timeLastHit));
	setPair(row, selectedModel, "Hit points", QString::number(psObj->body));
	setPair(row, selectedModel, "Periodical start", QString::number(psObj->periodicalDamageStart));
	setPair(row, selectedModel, "Periodical damage", QString::number(psObj->periodicalDamage));
	setPair(row, selectedModel, "Animation event", QString::number(psObj->animationEvent));
	setPair(row, selectedModel, "Number of weapons", QString::number(psObj->numWeaps));
	setPair(row, selectedModel, "Last hit weapon", QString::number(psObj->lastHitWeapon));
	setPair(row, selectedModel, "Visible", arrayToString(psObj->visible, MAX_PLAYERS));
	setPair(row, selectedModel, "Seen last tick", arrayToString(psObj->seenThisTick, MAX_PLAYERS));
	QStandardItem *weapKey = new QStandardItem("Weapons");
	for (int i = 0; i < psObj->numWeaps; i++)
	{
		if (psObj->asWeaps[i].nStat > 0)
		{
			WEAPON_STATS *psWeap = asWeaponStats + psObj->asWeaps[i].nStat;
			QStandardItemList list = componentToString(QString::number(i), psWeap, psObj->player);
			QStandardItem *weapSubKey = list[0];
			weapSubKey->appendRow(QStandardItemList{ new QStandardItem("Ammo"), new QStandardItem(QString::number(psObj->asWeaps[i].ammo)) });
			weapSubKey->appendRow(QStandardItemList{ new QStandardItem("Last fired time"), new QStandardItem(QString::number(psObj->asWeaps[i].lastFired)) });
			weapSubKey->appendRow(QStandardItemList{ new QStandardItem("Shots fired"), new QStandardItem(QString::number(psObj->asWeaps[i].shotsFired)) });
			weapSubKey->appendRow(QStandardItemList{ new QStandardItem("Used ammo"), new QStandardItem(QString::number(psObj->asWeaps[i].usedAmmo)) });
			weapSubKey->appendRow(QStandardItemList{ new QStandardItem("Origin"), new QStandardItem(QString::number(psObj->asWeaps[i].origin)) });
			weapKey->appendRow(list);
		}
	}
	selectedModel.setItem(row++, 0, weapKey);
	if (psObj->type == OBJ_DROID)
	{
		const DROID *psDroid = castDroid(psObj);
		setPair(row, selectedModel, "Droid type", QString::number(psDroid->droidType));
		setPair(row, selectedModel, "Weight", QString::number(psDroid->weight));
		setPair(row, selectedModel, "Base speed", QString::number(psDroid->baseSpeed));
		setPair(row, selectedModel, "Original hit points", QString::number(psDroid->originalBody));
		setPair(row, selectedModel, "Experience", QString::number(psDroid->experience));
		setPair(row, selectedModel, "Frustrated time", QString::number(psDroid->lastFrustratedTime));
		setPair(row, selectedModel, "Resistance", QString::number(psDroid->resistance));
		setPair(row, selectedModel, "Secondary order", QString::number(psDroid->secondaryOrder));
		setPair(row, selectedModel, "Action", QString(getDroidActionName(psDroid->action)));
		setPair(row, selectedModel, "Action position", QString::fromStdString(glm::to_string(psDroid->actionPos)));
		setPair(row, selectedModel, "Action started", QString::number(psDroid->actionStarted));
		setPair(row, selectedModel, "Action points", QString::number(psDroid->actionPoints));
		setPair(row, selectedModel, "Illumination", QString::number(psDroid->illumination));
		setPair(row, selectedModel, "Blocked bits", QString::number(psDroid->blockedBits));
		setPair(row, selectedModel, "Move status", QString::number(psDroid->sMove.Status));
		setPair(row, selectedModel, "Move index", QString::number(psDroid->sMove.pathIndex));
		setPair(row, selectedModel, "Move points", QString::number(psDroid->sMove.asPath.size()));
		setPair(row, selectedModel, "Move destination", QString::fromStdString(glm::to_string(psDroid->sMove.destination)));
		setPair(row, selectedModel, "Move source", QString::fromStdString(glm::to_string(psDroid->sMove.src)));
		setPair(row, selectedModel, "Move target", QString::fromStdString(glm::to_string(psDroid->sMove.target)));
		setPair(row, selectedModel, "Move bump pos", QString::fromStdString(glm::to_string(psDroid->sMove.bumpPos)));
		setPair(row, selectedModel, "Move speed", QString::number(psDroid->sMove.speed));
		setPair(row, selectedModel, "Move direction", QString::number(psDroid->sMove.moveDir));
		setPair(row, selectedModel, "Move bump dir", QString::number(psDroid->sMove.bumpDir));
		setPair(row, selectedModel, "Move bump time", QString::number(psDroid->sMove.bumpTime));
		setPair(row, selectedModel, "Move last bump", QString::number(psDroid->sMove.lastBump));
		setPair(row, selectedModel, "Move pause time", QString::number(psDroid->sMove.pauseTime));
		setPair(row, selectedModel, "Move shuffle start", QString::number(psDroid->sMove.shuffleStart));
		setPair(row, selectedModel, "Move vert speed", QString::number(psDroid->sMove.iVertSpeed));
		setPair(row, selectedModel, componentToString("Body", asBodyStats + psDroid->asBits[COMP_BODY], psObj->player));
		setPair(row, selectedModel, componentToString("Brain", asBrainStats + psDroid->asBits[COMP_BRAIN], psObj->player));
		setPair(row, selectedModel, componentToString("Propulsion", asPropulsionStats + psDroid->asBits[COMP_PROPULSION], psObj->player));
		setPair(row, selectedModel, componentToString("ECM", asECMStats + psDroid->asBits[COMP_ECM], psObj->player));
		setPair(row, selectedModel, componentToString("Sensor", asSensorStats + psDroid->asBits[COMP_SENSOR], psObj->player));
		setPair(row, selectedModel, componentToString("Construct", asConstructStats + psDroid->asBits[COMP_CONSTRUCT], psObj->player));
		setPair(row, selectedModel, componentToString("Repair", asRepairStats + psDroid->asBits[COMP_REPAIRUNIT], psObj->player));
	}
	else if (psObj->type == OBJ_STRUCTURE)
	{
		const STRUCTURE *psStruct = castStructure(psObj);
		setPair(row, selectedModel, "Build points", QString::number(psStruct->currentBuildPts));
		setPair(row, selectedModel, "Build rate", QString::number(psStruct->buildRate));
		setPair(row, selectedModel, "Resistance", QString::number(psStruct->resistance));
		setPair(row, selectedModel, "Foundation depth", QString::number(psStruct->foundationDepth));
		setPair(row, selectedModel, "Capacity", QString::number(psStruct->capacity));
		setPair(row, selectedModel, "^Type", QString::number(psStruct->pStructureType->type));
		setPair(row, selectedModel, "^Build points", QString::number(structureBuildPointsToCompletion(*psStruct)));
		setPair(row, selectedModel, "^Power points", QString::number(psStruct->pStructureType->powerToBuild));
		setPair(row, selectedModel, "^Height", QString::number(psStruct->pStructureType->height));
		setPair(row, selectedModel, componentToString("ECM", psStruct->pStructureType->pECM, psObj->player));
		setPair(row, selectedModel, componentToString("Sensor", psStruct->pStructureType->pSensor, psObj->player));
		if (psStruct->pStructureType->type == REF_REARM_PAD)
		{
			setPair(row, selectedModel, ":timeStarted", QString::number(psStruct->pFunctionality->rearmPad.timeStarted));
			setPair(row, selectedModel, ":timeLastUpdated", QString::number(psStruct->pFunctionality->rearmPad.timeLastUpdated));
			setPair(row, selectedModel, ":Rearm target", QString(objInfo(psStruct->pFunctionality->rearmPad.psObj)));
		}
	}
	else if (psObj->type == OBJ_FEATURE)
	{
		const FEATURE *psFeat = castFeature(psObj);
		setPair(row, selectedModel, "^Feature type", QString::number(psFeat->psStats->subType));
		setPair(row, selectedModel, "^Needs drawn", QString::number(psFeat->psStats->tileDraw));
		setPair(row, selectedModel, "^Visible at start", QString::number(psFeat->psStats->visibleAtStart));
		setPair(row, selectedModel, "^Damageable", QString::number(psFeat->psStats->damageable));
		setPair(row, selectedModel, "^Hit points", QString::number(psFeat->psStats->body));
		setPair(row, selectedModel, "^Armour", QString::number(psFeat->psStats->armourValue));
	}
	selectedView.resizeColumnToContents(0);
}

ScriptDebugger::~ScriptDebugger()
{
}

void jsDebugMessageUpdate()
{
	if (globalDialog)
	{
		globalDialog->updateMessages();
	}
}

void jsDebugUpdate()
{
	if (globalDialog)
	{
		globalDialog->update(); // quick update every second
	}
}

void jsDebugSelected(const BASE_OBJECT *psObj)
{
	if (globalDialog)
	{
		globalDialog->selected(psObj);
	}
}

bool jsDebugShutdown()
{
	delete globalDialog;
	globalDialog = nullptr;
	if(globalDialogShutdownHandler)
	{
		globalDialogShutdownHandler();
		globalDialogShutdownHandler = nullptr;
	}
	return true;
}

void jsDebugCreate(const MODELMAP &models, QStandardItemModel *triggerModel, QStandardItemModel *labelModel, const jsDebugShutdownHandlerFunction& shutdownFunc)
{
	jsDebugShutdown();
	globalDialogShutdownHandler = shutdownFunc;
	globalDialog = new ScriptDebugger(models, triggerModel, labelModel);
}
