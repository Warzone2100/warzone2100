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

#ifndef __INCLUDED_QTSCRIPTDEBUG_H__
#define __INCLUDED_QTSCRIPTDEBUG_H__

#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__) && (9 <= __GNUC__)
// not push / pop because this is needed for the generated qtscriptdebug moc cpp file
# pragma GCC diagnostic ignored "-Wdeprecated-copy" // Workaround Qt < 5.13 `deprecated-copy` issues with GCC 9
#endif

// **NOTE: Qt headers _must_ be before platform specific headers so we don't get conflicts.
#include <QtGui/QStandardItemModel>
#include <QtCore/QHash>
#include <QtCore/QSignalMapper>
#include <QtWidgets/QDialog>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QComboBox>

#include <functional>

#include "lib/framework/frame.h"
#include "basedef.h"
#include "droiddef.h"
#include "structuredef.h"
#include "researchdef.h"
#include "featuredef.h"

class QScriptEngine;
class QModelIndex;
class QLineEdit;

#include "wzapi.h"
#include <unordered_map>

typedef std::unordered_map<wzapi::scripting_instance *, QStandardItemModel *> MODELMAP;
typedef std::unordered_map<wzapi::scripting_instance *, QLineEdit *> EDITMAP;

class ScriptDebugger : public QDialog
{
	Q_OBJECT

public:
	ScriptDebugger(const MODELMAP &models, QStandardItemModel *triggerModel, QStandardItemModel *labelModel);
	~ScriptDebugger();
	void selected(const BASE_OBJECT *psObj);
	void updateMessages();
	void update();

private:
	QTabWidget tab;
	QStandardItemModel *labelModel;
	QStandardItemModel selectedModel;
	QStandardItemModel messageModel;
	QStandardItemModel viewdataModel;
	QStandardItemModel mainModel;
	QStandardItemModel playerModel[MAX_PLAYERS];
	QTreeView selectedView;
	QTreeView labelView;
	QTreeView triggerView;
	QTreeView messageView;
	QTreeView viewdataView;
	QTreeView mainView;
	QComboBox aiScriptComboBox;
	QComboBox aiPlayerComboBox;
	MODELMAP modelMap;
	EDITMAP editMap;
	int powerValue = 0;

	QPushButton *createButton(const QString &text, const char *slot, QWidget *parent);

protected slots:
	void labelClickedIdx(const QModelIndex &idx);
	void labelClicked();
	void labelClickedAll();
	void labelClickedActive();
	void labelClear();
	void runClicked(wzapi::scripting_instance * instance);
	void updateModels();
	void powerEditing(const QString &value);
	void powerEditingFinished();
	void playerButtonClicked(int value);
	void droidButtonClicked();
	void structButtonClicked();
	void featButtonClicked();
	void researchButtonClicked();
	void sensorsButtonClicked();
	void deityButtonClicked();
	void gatewayButtonClicked();
	void weatherButtonClicked();
	void revealButtonClicked();
	void shadowButtonClicked();
	void fogButtonClicked();
	void attachScriptClicked();
	void debuggerClosed();
};

typedef std::function<void ()> jsDebugShutdownHandlerFunction;
void jsDebugCreate(const MODELMAP &models, QStandardItemModel *triggerModel, QStandardItemModel *labelModel, const jsDebugShutdownHandlerFunction& shutdownFunc);
bool jsDebugShutdown();

// jsDebugSelected() and jsDebugMessageUpdate() defined in qtscript.h since it is used widely

#endif
