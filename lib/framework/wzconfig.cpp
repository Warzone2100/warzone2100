/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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

/** @file wzconfig.cpp
 *  Ini related functions.
 */

// Get platform defines before checking for them.
// Qt headers MUST come before platform specific stuff!
#include "wzconfig.h"
#include "file.h"

WzConfig::~WzConfig()
{
	if (mWarning == ReadAndWrite)
	{
		ASSERT(mObjStack.empty(), "Some json groups have not been closed, stack size %lu.", mObjStack.size());
		std::ostringstream stream;
		stream << mRoot.dump(4) << std::endl;
		std::string jsonString = stream.str();
		saveFile(mFilename.toUtf8().c_str(), jsonString.c_str(), jsonString.size());
	}
	debug(LOG_SAVE, "%s %s", mWarning == ReadAndWrite? "Saving" : "Closing", mFilename.toUtf8().c_str());
}

static nlohmann::json jsonMerge(nlohmann::json original, const nlohmann::json& override)
{
	for (auto it_override = override.begin(); it_override != override.end(); ++it_override)
	{
		auto key = it_override.key();
		auto it_original = original.find(key);
		if (it_override.value().is_object() && (it_original != original.end()))
		{
			original[key] = jsonMerge(it_original.value(), it_override.value());
		}
		else if (it_override.value().is_null())
		{
			original.erase(key);
		}
		else
		{
			original[key] = it_override.value();
		}
	}
	return original;
}

WzConfig::WzConfig(const WzString &name, WzConfig::warning warning)
: mArray(nlohmann::json::array())
{
	UDWORD size;
	char *data;

	mFilename = name;
	mStatus = true;
	mWarning = warning;
	pCurrentObj = &mRoot;

	if (!PHYSFS_exists(name.toUtf8().c_str()))
	{
		if (warning == ReadOnly)
		{
			mStatus = false;
			return;
		}
		else if (warning == ReadOnlyAndRequired)
		{
			debug(LOG_FATAL, "Missing required file %s", name.toUtf8().c_str());
			abort();
		}
		else if (warning == ReadAndWrite)
		{
			return;
		}
	}
	if (!loadFile(name.toUtf8().c_str(), &data, &size))
	{
		debug(LOG_FATAL, "Could not open \"%s\"", name.toUtf8().c_str());
	}

	try {
		mRoot = nlohmann::json::parse(data, data + size);
	}
	catch (const std::exception &e) {
		ASSERT(false, "JSON document from %s is invalid: %s", name.toUtf8().c_str(), e.what());
	}
	catch (...) {
		debug(LOG_FATAL, "Unexpected exception parsing JSON %s", name.toUtf8().c_str());
	}
	pCurrentObj = &mRoot;
	ASSERT(!mRoot.is_null(), "JSON document from %s is null", name.toUtf8().c_str());
	ASSERT(mRoot.is_object(), "JSON document from %s is not an object. Read: \n%s", name.toUtf8().c_str(), data);
	free(data);
	char **diffList = PHYSFS_enumerateFiles("diffs");
	for (char **i = diffList; *i != nullptr; i++)
	{
		std::string str(std::string("diffs/") + *i + std::string("/") + name.toUtf8().c_str());
		if (!PHYSFS_exists(str.c_str()))
		{
			continue;
		}
		if (!loadFile(str.c_str(), &data, &size))
		{
			debug(LOG_FATAL, "jsondiff file \"%s\" could not be opened!", name.toUtf8().c_str());
		}
		nlohmann::json tmpJson;
		try {
			tmpJson = nlohmann::json::parse(data, data + size);
		}
		catch (const std::exception &e) {
			ASSERT(false, "JSON diff from %s is invalid: %s", name.toUtf8().c_str(), e.what());
		}
		catch (...) {
			debug(LOG_FATAL, "Unexpected exception parsing JSON diff from %s", name.toUtf8().c_str());
		}
		ASSERT(!tmpJson.is_null(), "JSON diff from %s is null", name.toUtf8().c_str());
		ASSERT(tmpJson.is_object(), "JSON diff from %s is not an object. Read: \n%s", name.toUtf8().c_str(), data);
		mRoot = jsonMerge(mRoot, tmpJson);
		free(data);
		debug(LOG_INFO, "jsondiff \"%s\" loaded and merged", str.c_str());
	}
	PHYSFS_freeList(diffList);
	debug(LOG_SAVE, "Opening %s", name.toUtf8().c_str());
	pCurrentObj = &mRoot;
}

std::vector<WzString> WzConfig::childGroups() const
{
	std::vector<WzString> keys;
	for (auto it = pCurrentObj->begin(); it != pCurrentObj->end(); ++it)
	{
		if (it.value().is_object())
		{
			keys.push_back(WzString::fromUtf8(it.key().c_str()));
		}
	}
	return keys;
}

std::vector<WzString> WzConfig::childKeys() const
{
	std::vector<WzString> keys;
	for (auto it = pCurrentObj->begin(); it != pCurrentObj->end(); ++it)
	{
		keys.push_back(WzString::fromUtf8(it.key().c_str()));
	}
	return keys;
}

bool WzConfig::contains(const WzString &key) const
{
	return pCurrentObj->find(key.toUtf8()) != pCurrentObj->end();
}

json_variant WzConfig::value(const WzString &key, const json_variant &defaultValue) const
{
	return json_getValue(*pCurrentObj, key, defaultValue);
}

nlohmann::json WzConfig::json(const WzString &key, const nlohmann::json &defaultValue) const
{
	auto it = pCurrentObj->find(key.toUtf8());
	if (it == pCurrentObj->end())
	{
		return defaultValue;
	}
	else
	{
		return it.value();
	}
}

WzString WzConfig::string(const WzString &key, const WzString &defaultValue) const
{
	auto it = pCurrentObj->find(key.toUtf8());
	if (it == pCurrentObj->end())
	{
		return defaultValue;
	}
	else
	{
		// Use json_variant to support conversion from other value types to string
		bool ok = false;
		WzString stringValue = json_variant(it.value()).toWzString(&ok);
		if (!ok)
		{
			ASSERT(false, "Failed to convert value of key \"%s\" to string", key.toUtf8().c_str());
			return defaultValue;
		}
		return stringValue;
	}
}

void WzConfig::setVector3f(const WzString &name, const Vector3f &v)
{
	(*pCurrentObj)[name.toUtf8()] = nlohmann::json::array({ v.x, v.y, v.z });
}

Vector3f WzConfig::vector3f(const WzString &name)
{
	Vector3f r(0.0, 0.0, 0.0);
	auto it = pCurrentObj->find(name.toUtf8());
	if (it == pCurrentObj->end())
	{
		return r;
	}
	auto v = it.value();
	ASSERT(v.size() == 3, "%s: Bad list of %s", mFilename.toUtf8().c_str(), name.toUtf8().c_str());
	try {
		r.x = v[0];
		r.y = v[1];
		r.z = v[2];
	}
	catch (const std::exception &e) {
		ASSERT(false, "%s: Bad list of %s; exception: %s", mFilename.toUtf8().c_str(), name.toUtf8().c_str(), e.what());
	}
	catch (...) {
		debug(LOG_FATAL, "Unexpected exception encountered in vector3f");
	}
	return r;
}

void WzConfig::setVector3i(const WzString &name, const Vector3i &v)
{
	(*pCurrentObj)[name.toUtf8()] = nlohmann::json::array({ v.x, v.y, v.z });
}

Vector3i WzConfig::vector3i(const WzString &name)
{
	Vector3i r(0, 0, 0);
	auto it = pCurrentObj->find(name.toUtf8());
	if (it == pCurrentObj->end())
	{
		return r;
	}
	auto v = it.value();
	ASSERT(v.size() == 3, "%s: Bad list of %s", mFilename.toUtf8().c_str(), name.toUtf8().c_str());
	try {
		r.x = v[0];
		r.y = v[1];
		r.z = v[2];
	}
	catch (const std::exception &e) {
		ASSERT(false, "%s: Bad list of %s; exception: %s", mFilename.toUtf8().c_str(), name.toUtf8().c_str(), e.what());
	}
	catch (...) {
		debug(LOG_FATAL, "Unexpected exception encountered in vector3f");
	}
	return r;
}

void WzConfig::setVector2i(const WzString &name, const Vector2i &v)
{
	(*pCurrentObj)[name.toUtf8()] = nlohmann::json::array({ v.x, v.y });
}

Vector2i WzConfig::vector2i(const WzString &name)
{
	Vector2i r(0, 0);
	auto it = pCurrentObj->find(name.toUtf8());
	if (it == pCurrentObj->end())
	{
		return r;
	}
	auto v = it.value();
	ASSERT(v.size() == 2, "Bad list of %s", name.toUtf8().c_str());
	try {
		r.x = v[0];
		r.y = v[1];
	}
	catch (const std::exception &e) {
		ASSERT(false, "%s: Bad list of %s; exception: %s", mFilename.toUtf8().c_str(), name.toUtf8().c_str(), e.what());
	}
	catch (...) {
		debug(LOG_FATAL, "Unexpected exception encountered in vector3f");
	}
	return r;
}

bool WzConfig::beginGroup(const WzString &prefix)
{
	mObjNameStack.push_back(mName);
	mObjStack.push_back(pCurrentObj);
	mName = prefix;
	if (mWarning == ReadAndWrite)
	{
		mNewObjStack.push_back(nlohmann::json::object());
		pCurrentObj = &mNewObjStack.back();
	}
	else
	{
		auto it = pCurrentObj->find(prefix.toUtf8());
		// Check if mObj contains the prefix
		if (it == pCurrentObj->end()) // handled in this way for backwards compatibility
		{
			mNewObjStack.push_back(nlohmann::json::object());
			pCurrentObj = &mNewObjStack.back();
			return false;
		}
		ASSERT(it.value().is_object(), "%s: beginGroup() on non-object key \"%s\"", mFilename.toUtf8().c_str(), prefix.toUtf8().c_str());
		pCurrentObj = &it.value();
	}
	return true;
}

void WzConfig::endGroup()
{
	ASSERT(!mObjStack.empty(), "An endGroup() too much!");
	if (mWarning == ReadAndWrite)
	{
		nlohmann::json *pLatestObj = pCurrentObj;
		pCurrentObj = mObjStack.back();
		mObjStack.pop_back();
		if (!mNewObjStack.empty() && &mNewObjStack.back() == pLatestObj)
		{
			(*pCurrentObj)[mName.toUtf8()] = *pLatestObj;
			mNewObjStack.pop_back();
		}
		else
		{
			// this object is not a "new" object, and so is already source from the current obj
			debug(LOG_INFO, "Got it");
		}
		mName = mObjNameStack.back();
		mObjNameStack.pop_back();
	}
	else
	{
		if (!mNewObjStack.empty() && &mNewObjStack.back() == pCurrentObj)
		{
			mNewObjStack.pop_back();
		}
		mName = mObjNameStack.back();
		mObjNameStack.pop_back();
		pCurrentObj = mObjStack.back();
		mObjStack.pop_back();
	}
}

// This function is intended to be paired with endArray().
//
// [Creation Case]:
//
// By calling beginArray with a name, a new array value at the desired key (name) is created in the current object.
// The current object is then set to a **new JSON object**, which can be further manipulated.
//
// Calling nextArrayItem() inserts the current object into the current array, and then sets the current object
// to a new JSON object.
//
// Calling endArray() then finalizes and sets the array value in the (original / parent) object for key "name".
//
// [Read-only Case]:
//
// Similarly, beginArray() let's you traverse the array of objects found at key "name". (With some backwards-compatibility quirks.)
//
// The current object is set to the first object in the array. Subsequent calls to nextArrayItem() will increment which array object
// the current object points to.
//
void WzConfig::beginArray(const WzString &name)
{
	ASSERT(mArray.empty(), "beginArray() cannot be nested");
	mObjNameStack.push_back(mName);
	mObjStack.push_back(pCurrentObj);
	mName = name;
	if (mWarning == ReadAndWrite)
	{
		mNewObjStack.push_back(nlohmann::json::object());
		pCurrentObj = &mNewObjStack.back();
	}
	else
	{
		auto it = pCurrentObj->find(name.toUtf8());
		// Check if mObj contains the name
		if (it == pCurrentObj->end()) // handled in this way for backwards compatibility
		{
			return;
		}
		ASSERT(it.value().is_array(), "%s: beginArray() on non-array key \"%s\"", mFilename.toUtf8().c_str(), name.toUtf8().c_str());
		mArray = it.value();
		ASSERT(mArray.front().is_object(), "%s: beginArray() on non-object array \"%s\"", mFilename.toUtf8().c_str(), name.toUtf8().c_str());
		pCurrentObj = &mArray.front();
	}
}

void WzConfig::nextArrayItem()
{
	if (mWarning == ReadAndWrite)
	{
		mArray.push_back(*pCurrentObj);
		if (!mNewObjStack.empty() && &mNewObjStack.back() == pCurrentObj)
		{
			mNewObjStack.pop_back();
		}
		mNewObjStack.push_back(nlohmann::json::object());
		pCurrentObj = &mNewObjStack.back();
	}
	else
	{
		if (!mArray.empty())
		{
			mArray.erase(mArray.begin());
			if (!mArray.empty())
			{
				pCurrentObj = &mArray.front();
			}
			else
			{
				mNewObjStack.push_back(nlohmann::json::object());
				pCurrentObj = &mNewObjStack.back();
			}
		}
	}
}

int WzConfig::remainingArrayItems()
{
	return mArray.size();
}

void WzConfig::endArray()
{
	if (mWarning == ReadAndWrite)
	{
		if (!pCurrentObj->empty())
		{
			mArray.push_back(*pCurrentObj);
		}
		if (!mNewObjStack.empty() && &mNewObjStack.back() == pCurrentObj)
		{
			mNewObjStack.pop_back();
		}
		pCurrentObj = mObjStack.back();
		mObjStack.pop_back();
		if (!mArray.empty())
		{
			ASSERT(pCurrentObj != nullptr, "pCurrentObj is null");
			(*pCurrentObj)[mName.toUtf8()] = mArray;
		}
		mName = mObjNameStack.back();
		mObjNameStack.pop_back();
	}
	else
	{
		if (!mNewObjStack.empty() && &mNewObjStack.back() == pCurrentObj)
		{
			mNewObjStack.pop_back();
		}
		mName = mObjNameStack.back();
		mObjNameStack.pop_back();
		pCurrentObj = mObjStack.back();
		mObjStack.pop_back();
	}
	mArray = nlohmann::json::array();
}

void WzConfig::setValue(const WzString &key, const nlohmann::json &value)
{
	ASSERT(pCurrentObj != nullptr, "pCurrentObj is null");
	(*pCurrentObj)[key.toUtf8()] = value;
}

void WzConfig::set(const WzString &key, const nlohmann::json &value)
{
	setValue(key, value);
}

// MARK: json_variant

json_variant::json_variant(int i)
: mObj(i)
{ }

json_variant::json_variant(const char * str)
: mObj(str)
{ }

json_variant::json_variant(const std::string & str)
: mObj(str)
{ }

json_variant::json_variant(const QString & str)
: mObj(str.toUtf8().constData())
{ }

json_variant::json_variant(const WzString & str)
: mObj(str.toUtf8())
{ }

const nlohmann::json& json_variant::jsonValue() const
{
	return mObj;
}

#include <typeinfo>
template<typename TYPE>
static TYPE json_variant_toType(const json_variant& json, bool *ok, TYPE defaultValue)
{
	TYPE result = defaultValue;
	try {
		result = json.jsonValue().get<TYPE>();
		if (ok != nullptr)
		{
			*ok = true;
		}
	}
	catch (const std::exception &e) {
		debug(LOG_WARNING, "Failed to convert json_variant to %s because of error: %s", typeid(TYPE).name(), e.what());
		if (ok != nullptr)
		{
			*ok = false;
		}
		result = defaultValue;
	}
	catch (...) {
		debug(LOG_FATAL, "Unexpected exception encountered: json_variant::toType<%s>", typeid(TYPE).name());
	}
	return result;
}

int json_variant::toInt(bool *ok /*= nullptr*/) const
{
	if (mObj.is_number())
	{
		return json_variant_toType<int>(*this, ok, 0);
	}
	else if (mObj.is_boolean())
	{
		bool result = json_variant_toType<bool>(*this, ok, false);
		return (uint)result;
	}
	else if (mObj.is_string())
	{
		std::string result = json_variant_toType<std::string>(*this, ok, std::string());
		try {
			return std::stoi(result);
		}
		catch (const std::exception &e) {
			debug(LOG_WARNING, "Failed to convert string '%s' to int because of error: %s", result.c_str(), e.what());
			return 0;
		}
	}
	else if (mObj.is_null())
	{
		return 0;
	}
	else
	{
		return 0;
	}
}

uint json_variant::toUInt(bool *ok /*= nullptr*/) const
{
	if (mObj.is_number())
	{
		return json_variant_toType<uint>(*this, ok, 0);
	}
	else if (mObj.is_boolean())
	{
		bool result = json_variant_toType<bool>(*this, ok, false);
		return (uint)result;
	}
	else if (mObj.is_string())
	{
		std::string result = json_variant_toType<std::string>(*this, ok, std::string());
		try {
			unsigned long ulongValue = std::stoul(result);
			if (ulongValue > std::numeric_limits<uint>::max())
			{
				debug(LOG_WARNING, "Failed to convert string '%s' to uint because of error: value is > std::numeric_limits<uint>::max()", result.c_str());
				return 0;
			}
			return (uint)ulongValue;
		}
		catch (const std::exception &e) {
			debug(LOG_WARNING, "Failed to convert string '%s' to uint because of error: %s", result.c_str(), e.what());
			return 0;
		}
	}
	else if (mObj.is_null())
	{
		return 0;
	}
	else
	{
		return 0;
	}
}

#include <algorithm>

bool json_variant::toBool() const
{
	if (mObj.is_boolean())
	{
		return json_variant_toType<bool>(*this, nullptr, false);
	}
	else if (mObj.is_number())
	{
		double result = json_variant_toType<double>(*this, nullptr, 0);
		return result != 0;
	}
	else if (mObj.is_string())
	{
		std::string result = json_variant_toType<std::string>(*this, nullptr, std::string());
		std::transform(result.begin(), result.end(), result.begin(), [](const std::string::value_type& c) { return std::tolower(c, std::locale::classic()); });
		return !result.empty() && result != "0" && result != "false";
	}
	else
	{
		debug(LOG_INFO, "Expected a json value that can be converted to a boolean, instead received: %s", mObj.type_name());
		return false;
	}
}

double json_variant::toDouble(bool *ok /*= nullptr*/) const
{
	if (mObj.is_number_float())
	{
		return json_variant_toType<double>(*this, ok, 0.0);
	}
	else if (mObj.is_number_unsigned())
	{
		json::number_unsigned_t intValue = json_variant_toType<json::number_unsigned_t>(*this, ok, 0);
		return (double)intValue;
	}
	else if (mObj.is_number_integer())
	{
		json::number_integer_t intValue = json_variant_toType<json::number_integer_t>(*this, ok, 0);
		return (double)intValue;
	}
	else if (mObj.is_boolean())
	{
		return (double)json_variant_toType<bool>(*this, ok, false);
	}
	else if (mObj.is_string())
	{
		std::string result = json_variant_toType<std::string>(*this, ok, std::string());
		try {
			return std::stod(result);
		}
		catch (const std::exception &e) {
			debug(LOG_WARNING, "Failed to convert string '%s' to double because of error: %s", result.c_str(), e.what());
			return 0;
		}
	}
	else if (mObj.is_null())
	{
		return 0;
	}
	else
	{
		return 0;
	}
}

float json_variant::toFloat(bool *ok /*= nullptr*/) const
{
	if (mObj.is_number_float())
	{
		return json_variant_toType<float>(*this, ok, 0.0);
	}
	else if (mObj.is_number_unsigned())
	{
		json::number_unsigned_t intValue = json_variant_toType<json::number_unsigned_t>(*this, ok, 0);
		return (float)intValue;
	}
	else if (mObj.is_number_integer())
	{
		json::number_integer_t intValue = json_variant_toType<json::number_integer_t>(*this, ok, 0);
		return (float)intValue;
	}
	else if (mObj.is_boolean())
	{
		return (float)json_variant_toType<bool>(*this, ok, false);
	}
	else if (mObj.is_string())
	{
		std::string result = json_variant_toType<std::string>(*this, ok, std::string());
		try {
			return std::stof(result);
		}
		catch (const std::exception &e) {
			debug(LOG_WARNING, "Failed to convert string '%s' to float because of error: %s", result.c_str(), e.what());
			return 0;
		}
	}
	else if (mObj.is_null())
	{
		return 0;
	}
	else
	{
		return 0;
	}
}

WzString json_variant::toWzString(bool *ok /*= nullptr*/) const
{
	if (mObj.is_string())
	{
		return json_variant_toType<WzString>(*this, ok, WzString());
	}
	else if (mObj.is_boolean())
	{
		bool result = json_variant_toType<bool>(*this, ok, false);
		return (result) ? WzString::fromUtf8("true") : WzString::fromUtf8("false");
	}
	else if (mObj.is_number_unsigned())
	{
		json::number_unsigned_t intValue = json_variant_toType<json::number_unsigned_t>(*this, ok, 0);
		return WzString::number(intValue);
	}
	else if (mObj.is_number_integer())
	{
		json::number_integer_t intValue = json_variant_toType<json::number_integer_t>(*this, ok, 0);
		return WzString::number(intValue);
	}
	else if (mObj.is_number_float())
	{
		json::number_float_t floatValue = json_variant_toType<json::number_float_t>(*this, ok, 0);
		return WzString::number(floatValue);
	}
	else if (mObj.is_null())
	{
		return WzString();
	}
	else
	{
		if (ok != nullptr)
		{
			*ok = false;
		}
		return WzString();
	}
}

std::vector<WzString> json_variant::toWzStringList() const
{
	std::vector<WzString> result;

	if (!mObj.is_array())
	{
		// attempt to convert the non-array to a string, and return as a vector of 1 entry (if the string is non-empty)
		WzString selfAsString = toWzString();
		if (!selfAsString.isEmpty())
		{
			result.push_back(selfAsString);
		}
		return result;
	}

	WzString str;
	bool ok = false;
	for (const auto& v : mObj)
	{
		// Use json_variant to support conversion from other value types to string
		str = json_variant(v).toWzString(&ok);
		if (!ok)
		{
			debug(LOG_WARNING, "Encountered a JSON value in the array that cannot be converted to a string (type: %s)", v.type_name());
			break;
		}
		result.push_back(str);
	}
	return result;
}

QString json_variant::toString() const
{
	if (mObj.is_string())
	{
		return QString::fromUtf8(json_variant_toType<std::string>(*this, nullptr, std::string()).c_str());
	}
	else if (mObj.is_boolean())
	{
		bool result = json_variant_toType<bool>(*this, nullptr, false);
		return (result) ? "true" : "false";
	}
	else if (mObj.is_number_unsigned())
	{
		json::number_unsigned_t intValue = json_variant_toType<json::number_unsigned_t>(*this, nullptr, 0);
		return QString::number(intValue);
	}
	else if (mObj.is_number_integer())
	{
		json::number_integer_t intValue = json_variant_toType<json::number_integer_t>(*this, nullptr, 0);
		return QString::number(intValue);
	}
	else if (mObj.is_number_float())
	{
		json::number_float_t floatValue = json_variant_toType<json::number_float_t>(*this, nullptr, 0);
		return QString::number(floatValue);
	}
	else if (mObj.is_null())
	{
		return QString();
	}
	else
	{
		return QString();
	}
}

QStringList json_variant::toStringList() const
{
	QStringList result;

	if (!mObj.is_array())
	{
		// attempt to convert the non-array to a string, and return as a vector of 1 entry (if the string is non-empty)
		QString selfAsString = toString();
		if (!selfAsString.isEmpty())
		{
			result.append(selfAsString);
		}
		return result;
	}

	std::string str;
	for (const auto& v : mObj)
	{
		try {
			str = v.get<std::string>();
		}
		catch (const std::exception &e) {
			debug(LOG_WARNING, "Encountered a JSON value in the array that cannot be converted to a string; error: %s", e.what());
			break;
		}
		catch (...) {
			debug(LOG_FATAL, "Encountered an unexpected exception in json_variant::toStringList()");
			break;
		}
		result.append(QString::fromUtf8(str.c_str()));
	}
	return result;
}

// Convenience methods

json_variant json_getValue(const nlohmann::json& json, const WzString &key, const json_variant &defaultValue /*= json_variant()*/)
{
	auto it = json.find(key.toUtf8());
	if (it == json.end())
	{
		return defaultValue;
	}
	else
	{
		return json_variant(it.value());
	}
}

json_variant json_getValue(const nlohmann::json& json, nlohmann::json::size_type idx, const json_variant &defaultValue /*= json_variant()*/)
{
	try {
		return json_variant(json.at(idx));
	}
	catch (const std::exception&) {
		return defaultValue;
	}
	catch (...) {
		debug(LOG_FATAL, "Encountered unexpected exception in json_getValue");
		return defaultValue;
	}
}
