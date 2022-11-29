/*
	This file is part of Warzone 2100.
	Copyright (C) 2020  Warzone 2100 Project

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
#ifndef _WZ_PROPERTY_PROVIDERS_H_
#define _WZ_PROPERTY_PROVIDERS_H_

#include "3rdparty/propertymatcher.h"
#include <vector>
#include <memory>

class BuildPropertyProvider : public PropertyMatcher::PropertyProvider {
public:
	virtual ~BuildPropertyProvider();
	bool getPropertyValue(const std::string& property, std::string& output_value) override;
};

class EnvironmentPropertyProvider : public PropertyMatcher::PropertyProvider {
public:
	virtual ~EnvironmentPropertyProvider();
	bool getPropertyValue(const std::string& property, std::string& output_value) override;
public:
	enum class EnvironmentProperty {
		FIRST_LAUNCH,
		INSTALLED_PATH,
		WIN_INSTALLED_BINARIES,
		WIN_LOADEDMODULES,
		WIN_LOADEDMODULENAMES,
		// WZ 4.3.3+
		ENV_VAR_NAMES,
		SYSTEM_RAM, // system RAM in MiB
	};
private:
	std::string GetCurrentEnvironmentPropertyValue(const EnvironmentProperty& property);
private:
	// caches
	std::string firstLaunchDateStr;
	std::string binDirFilesStr;
	std::string processModulesStr;
	std::string processModuleNamesStr;
};

class CombinedPropertyProvider : public PropertyMatcher::PropertyProvider {
public:
	CombinedPropertyProvider(const std::vector<std::shared_ptr<PropertyMatcher::PropertyProvider>>& providers)
	{
		for (auto& provider : providers)
		{
			if (!provider) continue;
			propertyProviders.push_back(provider);
		}
	}
	virtual ~CombinedPropertyProvider();
	bool getPropertyValue(const std::string& property, std::string& output_value) override;
private:
	std::vector<std::shared_ptr<PropertyMatcher::PropertyProvider>> propertyProviders;
};

#endif //_WZ_PROPERTY_PROVIDERS_H_
