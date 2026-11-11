// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2020-2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version published by
	the Warzone 2100 Project, set out in COPYING.SECTION7 in the
	top-level directory of the source distribution. This notice must be
	kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
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
		// WZ 4.4.0+
		CONTAINER_TYPE,
		// WZ 4.5.0+
		EMSCRIPTEN_WINDOW_URL
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
		propertyProviders.reserve(providers.size());
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
