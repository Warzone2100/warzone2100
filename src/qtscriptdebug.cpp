/*
	This file is part of Warzone 2100.
	Copyright (C) 2013  Warzone 2100 Project

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

#include <QtCore/QHash>
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptValue>
#include <QtScript/QScriptValueIterator>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtGui/QDialog>
#include <QtGui/QTreeView>
#include <QtGui/QTabWidget>
#include <QtGui/QPushButton>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QStandardItemModel>

#include "lib/framework/wzapp.h"
#include "lib/framework/wzconfig.h"

#include "qtscript.h"
#include "qtscriptfuncs.h"

static ScriptDebugger *globalDialog = NULL;

// ----------------------------------------------------------

ScriptDebugger::ScriptDebugger(const MODELMAP &models, QStandardItemModel *triggerModel) : QDialog(NULL, Qt::Window)
{
	// Add globals
	for (MODELMAP::const_iterator i = models.constBegin(); i != models.constEnd(); i++)
	{
		QScriptEngine *engine = i.key();
		QStandardItemModel *m = i.value();
		QTreeView *view = new QTreeView();
		view->setSelectionMode(QAbstractItemView::NoSelection);
		view->setModel(m);
		QString scriptName = engine->globalObject().property("scriptName").toString();
		int player = engine->globalObject().property("me").toInt32();
		tab.addTab(view, scriptName + ":" + QString::number(player));
	}

	// Add triggers
	triggerView.setModel(triggerModel);
	triggerView.resizeColumnToContents(0);
	triggerView.setSelectionMode(QAbstractItemView::NoSelection);
	triggerView.setSelectionBehavior(QAbstractItemView::SelectRows);
	tab.addTab(&triggerView, "Triggers");

	// Add labels
	labelModel = createLabelModel();
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
	setSizeGripEnabled(true);
	show();
	raise();
	activateWindow();
}

void ScriptDebugger::labelClicked()
{
	QItemSelectionModel *selected = labelView.selectionModel();
	QModelIndex idx = selected->currentIndex();
	QStandardItem *item = labelModel->itemFromIndex(labelModel->index(idx.row(), 0));
	showLabel(item->text());
}

void ScriptDebugger::labelClickedIdx(const QModelIndex &idx)
{
	QStandardItem *item = labelModel->itemFromIndex(labelModel->index(idx.row(), 0));
	showLabel(item->text());
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
