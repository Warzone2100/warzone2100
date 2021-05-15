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
#include <string>
#include <memory>
#include <vector>
#include <optional-lite/optional.hpp>
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
};

class IOProvider
{
protected:
	IOProvider() {};

public:
	virtual ~IOProvider() {};

public:
	virtual std::unique_ptr<BinaryIOStream> openBinaryStream(const std::string& filename, BinaryIOStream::OpenMode mode) = 0;
	virtual bool loadFullFile(const std::string& filename, std::vector<char>& fileData) = 0;
	virtual bool writeFullFile(const std::string& filename, const char *ppFileData, uint32_t fileSize) = 0;
};

// MARK: - Default implementation, using C stdio

class StdIOProvider : public IOProvider
{
public:
	virtual std::unique_ptr<BinaryIOStream> openBinaryStream(const std::string& filename, BinaryIOStream::OpenMode mode) override;
	virtual bool loadFullFile(const std::string& filename, std::vector<char>& fileData) override;
	virtual bool writeFullFile(const std::string& filename, const char *ppFileData, uint32_t fileSize) override;
};

} // namespace WzMap

