/*
	This file is part of Warzone 2100.
	Copyright (C) 2020-2021  Warzone 2100 Project

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
 * @file
 * Definitions for scrollable JSON table functions.
 */

#ifndef __INCLUDED_LIB_WIDGET_JSONTABLE_H__
#define __INCLUDED_LIB_WIDGET_JSONTABLE_H__

#include "widget.h"
#include "form.h"
#include <3rdparty/json/json.hpp>

#include <vector>
#include <memory>
#include <unordered_map>

class ScrollableTableWidget;

class PathBarWidget : public WIDGET
{
public:
	typedef std::function<void (PathBarWidget&, size_t componentIndex)> ONCLICK_PATH_HANDLER;
public:
	PathBarWidget(const std::string& pathSeparator)
	: WIDGET()
	, pathSeparatorStr(pathSeparator)
	{ }
	~PathBarWidget() {}
public:
	static std::shared_ptr<PathBarWidget> make(const std::string& pathSeparator);

	void display(int xOffset, int yOffset) override;
	void geometryChanged() override { relayoutComponentButtons(); }
public:
	void pushPathComponent(const std::string& pathComponent);
	void popPathComponents(size_t numComponents = 1);
	void reset();
	const std::vector<std::string>& currentPath() const;
	std::string currentPathStr() const;
	size_t numPathComponents() const { return pathComponents.size(); }
	void setOnClickPath(const ONCLICK_PATH_HANDLER& func)
	{
		onClickPath = func;
	}
private:
	std::shared_ptr<W_BUTTON> createPathComponentButton(const std::string& pathComponent, size_t newComponentIndex);
	void relayoutComponentButtons();
private:
	std::string pathSeparatorStr;
	std::shared_ptr<W_LABEL> pathSeparator;
	std::vector<std::string> pathComponents;
	std::unordered_map<size_t, std::shared_ptr<W_BUTTON>> componentButtons;
	std::shared_ptr<W_LABEL> ellipsis;
	ONCLICK_PATH_HANDLER onClickPath;
};

class JSONTableWidget : public W_FORM
{
public:
	typedef std::function<void (JSONTableWidget&)> CallbackFunc;
public:
	JSONTableWidget() : W_FORM() {}
	~JSONTableWidget();

public:
	static std::shared_ptr<JSONTableWidget> make(const std::string& title = std::string());
	void updateData(const nlohmann::ordered_json& json, bool tryToPreservePath = false);
	void updateData(const nlohmann::json& json, bool tryToPreservePath = false);
	bool pushJSONPath(const std::string& keyInCurrentJSONPointer);
	size_t popJSONPath(size_t numLevels = 1);
	std::string currentJSONPathStr() const;
	std::string dumpDataAtCurrentPath() const;
	void setUpdateButtonFunc(const CallbackFunc& updateButtonFunc, optional<UDWORD> automaticUpdateInterval = nullopt);
	void setAutomaticUpdateInterval(optional<UDWORD> newAutomaticUpdateInterval = nullopt);

public:
	virtual void display(int xOffset, int yOffset) override;
	virtual void geometryChanged() override;
	virtual void screenSizeDidChange(int oldWidth, int oldHeight, int newWidth, int newHeight) override;
	virtual void run(W_CONTEXT *psContext) override;

private:
	void updateData_Internal(const std::function<void ()>& setJsonFunc, bool tryToPreservePath);
	bool rootJsonHasExpandableChildren() const;
	void refreshUpdateButtonState();
	void resizeColumnWidths(bool overrideUserColumnSizing);
	void rebuildTableFromJSON(bool overrideUserColumnSizing);
	bool pushJSONPath(const std::vector<std::string>& pathInCurrentJSONPointer, bool overrideUserColumnSizing);
	std::vector<std::string> currentPathFromRoot() const;
	void resetPath();
	void displayOptionsOverlay(const std::shared_ptr<WIDGET>& psParent);
	std::shared_ptr<WIDGET> createOptionsPopoverForm();

private:
	CallbackFunc							updateButtonFunc;
	optional<UDWORD>						automaticUpdateInterval;
	UDWORD									lastUpdateTime;

	std::shared_ptr<W_LABEL>				titleLabel;
	std::shared_ptr<PathBarWidget>			pathBar;
	std::shared_ptr<ScrollableTableWidget>	table;

	std::shared_ptr<W_BUTTON>				optionsButton;
	std::shared_ptr<W_BUTTON>				updateButton;
	std::shared_ptr<W_SCREEN>				optionsOverlayScreen;

	optional<nlohmann::ordered_json>		_orderedjson;
	std::vector<nlohmann::ordered_json*>	_orderedjson_currentPointer;

	optional<nlohmann::json>				_json;
	std::vector<nlohmann::json*>			_json_currentPointer;

	std::vector<int>						currentMaxColumnWidths = {0,0,0};
	std::vector<std::string>				actualPushedPathComponents;

private:
	template<typename json_type>
	friend void setTableToJson(const json_type& json, JSONTableWidget& jsonTable);
};

#endif // __INCLUDED_LIB_WIDGET_JSONTABLE_H__
