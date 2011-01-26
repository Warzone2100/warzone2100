/*
	This file is part of Warzone 2100.
	Copyright (C) 2011  Warzone 2100 Project

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
 * @file qtscriptfuncs.cpp
 *
 * New scripting system -- script functions
 */

#include "qtscriptfuncs.h"

#include <QtScript/QScriptValue>

#include "console.h"
#include "map.h"

// All script functions should be prefixed with "js_" then followed by same name as in script.

static QScriptValue convStructure(STRUCTURE *psStruct, QScriptEngine *engine)
{
	QScriptValue value = engine->newObject();
	value.setProperty("id", psStruct->id, QScriptValue::ReadOnly);
	return value;
}

static QScriptValue js_enumStruct(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue statsName = context->argument(0);
	QScriptValue targetPlayer = context->argument(1);
	QScriptValue lookingPlayer = context->argument(2);
	QList<STRUCTURE *> matches;

	int player = targetPlayer.toInt32();
	int looking = -1;
	QString stat = statsName.toString();
	if (!lookingPlayer.isUndefined())	// third arg optional
	{
		looking = lookingPlayer.toInt32();
	}
	ASSERT_OR_RETURN(QScriptValue(), player < MAX_PLAYERS && player >= 0, "Target player index out of range: %d", player);
	ASSERT_OR_RETURN(QScriptValue(), looking < MAX_PLAYERS && looking >= -1, "Looking player index out of range: %d", looking);
	for (STRUCTURE *psStruct = apsStructLists[player]; psStruct; psStruct = psStruct->psNext)
	{
		if ((looking == -1 || psStruct->visible[looking]) && stat.compare(psStruct->pStructureType->pName) == 0)
		{
			matches.push_back(psStruct);
		}
	}
	if (matches.size() == 0)
	{
		return QScriptValue();
	}
	QScriptValue result = engine->newArray(matches.size());
	for (int i = 0; i < matches.size(); i++)
	{
		STRUCTURE *psStruct = matches.at(i);
		result.setProperty(i, convStructure(psStruct, engine));
	}
	return result;
}

// is this really useful?
static QScriptValue js_debug(QScriptContext *context, QScriptEngine *engine)
{
	QString result;
	for (int i = 0; i < context->argumentCount(); ++i)
	{
		if (i != 0)
		{
			result.append(QLatin1String(" "));
		}
		QString s = context->argument(i).toString();
		if (context->state() == QScriptContext::ExceptionState)
		{
			break;
		}
		result.append(s);
	}
	qWarning(result.toAscii().constData());
	return QScriptValue();
}

static QScriptValue js_scavengerPlayer(QScriptContext *context, QScriptEngine *engine)
{
	Q_UNUSED(context);
	Q_UNUSED(engine);
	return QScriptValue(scavengerPlayer());
}

// TODO, should cover scrShowConsoleText, scrAddConsoleText, scrTagConsoleText and scrConsole
static QScriptValue js_console(QScriptContext *context, QScriptEngine *engine)
{
	int player = engine->globalObject().property("me").toInt32();
	if (player == selectedPlayer)
	{
		QString result;
		for (int i = 0; i < context->argumentCount(); ++i)
		{
			if (i != 0)
			{
				result.append(QLatin1String(" "));
			}
			QString s = context->argument(i).toString();
			if (context->state() == QScriptContext::ExceptionState)
			{
				break;
			}
			result.append(s);
		}
		//permitNewConsoleMessages(true);
		//setConsolePermanence(true,true);
		addConsoleMessage(result.toAscii().constData(), CENTRE_JUSTIFY, SYSTEM_MESSAGE);
		//permitNewConsoleMessages(false);
	}
	return QScriptValue();
}

#if 0
static QScriptValue js_getDerrick(QScriptContext *context, QScriptEngine *engine)
{
	BASE_OBJECT *psObj = NULL;
	QScriptValue param = context->argument(0);
	int i = param.toInt32();

	if (i < (int)derricks.size())
	{
		const int x = derricks[i].x;
		const int y = derricks[i].y;
		MAPTILE *psTile = worldTile(x, y);
		if (psTile)
		{
			psObj = psTile->psObject;
		}
	}
	return convStructure(psObj, engine);
}
#endif

// ----------------------------------------------------------------------------------------

bool registerFunctions(QScriptEngine *engine)
{
	// Register functions to the script engine here
	//engine->globalObject().setProperty("getDerrick", engine->newFunction(js_getDerrick));
	engine->globalObject().setProperty("debug", engine->newFunction(js_debug));
	engine->globalObject().setProperty("console", engine->newFunction(js_console));
	engine->globalObject().setProperty("scavengerPlayer", engine->newFunction(js_scavengerPlayer));
	engine->globalObject().setProperty("enumStruct", engine->newFunction(js_enumStruct));
	return true;
}
