/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2021  Warzone 2100 Project

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

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

// MARK: -

namespace WzMap {

class BinaryIOStream
{
public:
	enum class OpenMode
	{
		READ,
		WRITE
	};

public:
	virtual ~BinaryIOStream() {};

	virtual optional<size_t> readBytes(void *buffer, size_t len) = 0;
	virtual optional<size_t> writeBytes(const void *buffer, size_t len) = 0;
	virtual bool endOfStream() = 0;
	virtual bool close() = 0;

	virtual bool readULE8(uint8_t *pVal);
	virtual bool readULE16(uint16_t *pVal);
	virtual bool readULE32(uint32_t *pVal);
	virtual bool readSLE8(int8_t *pVal);
	virtual bool readSLE16(int16_t *pVal);
	virtual bool readSLE32(int32_t *pVal);

	virtual bool writeULE8(uint8_t pVal);
	virtual bool writeULE16(uint16_t pVal);
	virtual bool writeULE32(uint32_t pVal);
	virtual bool writeSLE8(int8_t pVal);
	virtual bool writeSLE16(int16_t pVal);
	virtual bool writeSLE32(int32_t pVal);

	virtual bool writeUBE32(uint32_t pVal);
	virtual bool writeSBE32(int32_t pVal);
};

class IOProvider
{
protected:
	IOProvider() {};

public:
	virtual ~IOProvider() {};

public:
	virtual std::unique_ptr<BinaryIOStream> openBinaryStream(const std::string& filename, BinaryIOStream::OpenMode mode) = 0;
	enum class LoadFullFileResult
	{
		SUCCESS = 0,
		FAILURE_OPEN,
		FAILURE_READ,
		FAILURE_EXCEEDS_MAXFILESIZE
	};
	virtual LoadFullFileResult loadFullFile(const std::string& filename, std::vector<char>& fileData, uint32_t maxFileSize = 0, bool appendNullCharacter = false) = 0;
	virtual bool writeFullFile(const std::string& filename, const char *ppFileData, uint32_t fileSize) = 0;

	// Creates a directory (and any required parent directories) and returns true on success (or if the directory already exists)
	virtual bool makeDirectory(const std::string& directoryPath) = 0;

	virtual const char* pathSeparator() const = 0;

	// Check if a file exists at a path
	virtual bool fileExists(const std::string& filePath) = 0;

	// enumFunc receives each enumerated file, and returns true to continue enumeration, or false to shortcut / stop enumeration
	virtual bool enumerateFiles(const std::string& basePath, const std::function<bool (const char* file)>& enumFunc);

	// enumFunc receives each enumerated subfolder, and returns true to continue enumeration, or false to shortcut / stop enumeration
	virtual bool enumerateFolders(const std::string& basePath, const std::function<bool (const char* file)>& enumFunc);

	virtual bool enumerateFilesRecursive(const std::string& basePath, const std::function<bool (const char* file)>& enumFunc);
	virtual bool enumerateFoldersRecursive(const std::string& basePath, const std::function<bool (const char* file)>& enumFunc);

public:
	std::string pathJoin(const std::string& a, const std::string& b);
	std::string pathDirName(const std::string& fullPath);
	std::string pathBaseName(const std::string& fullPath);
};

// MARK: - Default implementation, using C stdio

class StdIOProvider : public IOProvider
{
public:
	virtual std::unique_ptr<BinaryIOStream> openBinaryStream(const std::string& filename, BinaryIOStream::OpenMode mode) override;
	virtual IOProvider::LoadFullFileResult loadFullFile(const std::string& filename, std::vector<char>& fileData, uint32_t maxFileSize = 0, bool appendNullCharacter = false) override;
	virtual bool writeFullFile(const std::string& filename, const char *ppFileData, uint32_t fileSize) override;
	virtual bool makeDirectory(const std::string& directoryPath) override;

	virtual const char* pathSeparator() const override;

	virtual bool fileExists(const std::string& filePath) override;

	// Check if a folder exists at a path
	// NOTE: dirPath should *not* contain a trailing pathSeparator (or this may always return false)
	bool folderExists(const std::string& dirPath);

	virtual bool enumerateFiles(const std::string& basePath, const std::function<bool (const char* file)>& enumFunc) override;
	virtual bool enumerateFolders(const std::string& basePath, const std::function<bool (const char* file)>& enumFunc) override;

private:
	bool internalMakeDir(const std::string& dirPath);
};

} // namespace WzMap

