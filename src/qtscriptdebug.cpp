/*
	This file is part of Warzone 2100.
	Copyright (C) 2013-2015  Warzone 2100 Project

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

#include "qtscriptdebug.h"

#if defined(WZ_CC_MSVC)
#include "qtscriptdebug.h.moc"         // this is generated on the pre-build event.
#endif

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
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtGui/QStandardItemModel>

#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/wzconfig.h"
#include "lib/netplay/netplay.h"

#include "multiplay.h"
#include "power.h"
#include "hci.h"
#include "display.h"
#include "keybind.h"

#include "qtscript.h"
#include "qtscriptfuncs.h"

static ScriptDebugger *globalDialog = NULL;
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

ScriptDebugger::ScriptDebugger(const MODELMAP &models, QStandardItemModel *triggerModel) : QDialog(NULL, Qt::Window)
{
	modelMap = models;
	QSignalMapper *signalMapper = new QSignalMapper(this);

	// Add main page
	QWidget *mainWidget = new QWidget(this);
	QVBoxLayout *mainLayout = new QVBoxLayout();
	QHBoxLayout *placementLayout = new QHBoxLayout();
	placementLayout->addWidget(createButton("Add droids", SLOT(droidButtonClicked()), this));
	placementLayout->addWidget(createButton("Add structures", SLOT(structButtonClicked()), this));
	placementLayout->addWidget(createButton("Add features", SLOT(featButtonClicked()), this));
	mainLayout->addLayout(placementLayout);
	QHBoxLayout *miscLayout = new QHBoxLayout();
	miscLayout->addWidget(createButton("Research all", SLOT(researchButtonClicked()), this));
	miscLayout->addWidget(createButton("Show sensors", SLOT(sensorsButtonClicked()), this));
	miscLayout->addWidget(createButton("Shadows", SLOT(shadowButtonClicked()), this));
	miscLayout->addWidget(createButton("Fog", SLOT(fogButtonClicked()), this));
	mainLayout->addLayout(miscLayout);
	QHBoxLayout *worldLayout = new QHBoxLayout();
	worldLayout->addWidget(createButton("Show all", SLOT(deityButtonClicked()), this));
	worldLayout->addWidget(createButton("Weather", SLOT(weatherButtonClicked()), this));
	worldLayout->addWidget(createButton("Reveal mode", SLOT(revealButtonClicked()), this));
	mainLayout->addLayout(worldLayout);
	QHBoxLayout *selectedPlayerLayout = new QHBoxLayout();
	QLabel *selectPlayerLabel = new QLabel("Selected Player:");
	QComboBox *playerComboBox = new QComboBox;
	for (int i = 0; i < game.maxPlayers; i++)
	{
		playerComboBox->addItem(QString::number(i));
	}
	connect(playerComboBox, SIGNAL(activated(int)), this, SLOT(playerButtonClicked(int)));
	selectedPlayerLayout->addWidget(selectPlayerLabel);
	selectedPlayerLayout->addWidget(playerComboBox);
	mainLayout->addLayout(selectedPlayerLayout);
	QHBoxLayout *powerLayout = new QHBoxLayout();
	QLabel *powerLabel = new QLabel("Power:");
	QLineEdit *powerLineEdit = new QLineEdit;
	powerLineEdit->setText(QString::number(getPower(selectedPlayer)));
	connect(powerLineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(powerEditing(const QString&)));
	connect(powerLineEdit, SIGNAL(returnPressed()), this, SLOT(powerEditingFinished()));
	powerLayout->addWidget(powerLabel);
	powerLayout->addWidget(powerLineEdit);
	mainLayout->addLayout(powerLayout);
	mainWidget->setLayout(mainLayout);
	tab.addTab(mainWidget, "Main");

	// Add globals
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
		QVBoxLayout *layout = new QVBoxLayout();
		QHBoxLayout *layout2 = new QHBoxLayout();
		QPushButton *updateButton = new QPushButton("Update", this);
		QPushButton *button = new QPushButton("Run", this);
		connect(button, SIGNAL(pressed()), signalMapper, SLOT(map()));
		connect(updateButton, SIGNAL(pressed()), this, SLOT(updateModels()));
		signalMapper->setMapping(button, engine);
		editMap.insert(engine, lineEdit); // store this for slot
		layout->addWidget(view);
		layout2->addWidget(updateButton);
		layout2->addWidget(lineEdit);
		layout2->addWidget(button);
		layout->addLayout(layout2);

		dummyWidget->setLayout(layout);
		tab.addTab(dummyWidget, scriptName + ":" + QString::number(player));
	}
	connect(signalMapper, SIGNAL(mapped(QObject *)), this, SLOT(runClicked(QObject *)));

	// Add triggers
	triggerModel->setParent(this); // take ownership to avoid memory leaks
	triggerView.setModel(triggerModel);
	triggerView.resizeColumnToContents(0);
	triggerView.setSelectionMode(QAbstractItemView::NoSelection);
	triggerView.setSelectionBehavior(QAbstractItemView::SelectRows);
	tab.addTab(&triggerView, "Triggers");

	// Add labels
	labelModel = createLabelModel();
	labelModel->setParent(this); // take ownership to avoid memory leaks
	labelView.setModel(labelModel);
	labelView.resizeColumnToContents(0);
	labelView.setSelectionMode(QAbstractItemView::SingleSelection);
	labelView.setSelectionBehavior(QAbstractItemView::SelectRows);
	connect(&labelView, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(labelClickedIdx(const QModelIndex &)));
	QPushButton *button = new QPushButton("Show", this);
	connect(button, SIGNAL(pressed()), this, SLOT(labelClicked()));
	QVBoxLayout *labelLayout = new QVBoxLayout(this);
	labelLayout->addWidget(&labelView);
	labelLayout->addWidget(button);
	QWidget *dummyWidget = new QWidget(this);
	dummyWidget->setLayout(labelLayout);
	tab.addTab(dummyWidget, "Labels");

	// Set up dialog
	QHBoxLayout *layout = new QHBoxLayout(this);
	layout->addWidget(&tab);
	setLayout(layout);
	resize(400, 500);
	setSizeGripEnabled(true);
	show();
	raise();
	powerLineEdit->setFocusPolicy(Qt::StrongFocus);
	powerLineEdit->setFocus();
	activateWindow();
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

ScriptDebugger::~ScriptDebugger()
{
}

bool jsDebugShutdown()
{
	delete globalDialog;
	globalDialog = NULL;
	return true;
}

void jsDebugCreate(const MODELMAP &models, QStandardItemModel *triggerModel)
{
	if (globalDialog)
	{
		delete globalDialog;
	}
	globalDialog = new ScriptDebugger(models, triggerModel);
}
