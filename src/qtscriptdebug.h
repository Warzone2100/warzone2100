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

#ifndef __INCLUDED_QTSCRIPTDEBUG_H__
#define __INCLUDED_QTSCRIPTDEBUG_H__

#include "lib/framework/frame.h"
#include "basedef.h"
#include "droiddef.h"
#include "structuredef.h"
#include "researchdef.h"
#include "featuredef.h"

class QScriptEngine;
class QStandardItemModel;
class QModelIndex;
class QLineEdit;

#include <QtCore/QHash>
#include <QtCore/QSignalMapper>
#include <QtWidgets/QDialog>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTreeView>

typedef QHash<QScriptEngine *, QStandardItemModel *> MODELMAP;
typedef QHash<QScriptEngine *, QLineEdit *> EDITMAP;

class ScriptDebugger : public QDialog
{
	Q_OBJECT

public:
	ScriptDebugger(const MODELMAP &models, QStandardItemModel *triggerModel);
	~ScriptDebugger();

private:
	QTabWidget tab;
	QStandardItemModel *labelModel;
	QTreeView labelView;
	QTreeView triggerView;
	MODELMAP modelMap;
	EDITMAP editMap;

protected slots:
	void labelClickedIdx(const QModelIndex &idx);
	void labelClicked();
	void runClicked(QObject *obj);
	void updateModels();
};

void jsDebugCreate(const MODELMAP &models, QStandardItemModel *triggerModel);
bool jsDebugShutdown();

#endif
