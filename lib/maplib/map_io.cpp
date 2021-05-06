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

#include "map_io.h"
#include <cstdio>

#if defined(_WIN32)
# define WIN32_LEAN_AND_MEAN
# define WIN32_EXTRA_LEAN
# undef NOMINMAX
# define NOMINMAX 1
# include <windows.h>
#endif

#if !defined(_WIN32)
# include <fcntl.h>
#endif

#include "3rdparty/SDL_endian.h"

namespace WzMap {

bool BinaryIOStream::readULE8(uint8_t *pVal)
{
	uint8_t val = 0;
	auto result = readBytes(&val, sizeof(uint8_t));
	if (!result.has_value()) { return false; }
	if (pVal)
	{
		*pVal = val;
	}
	return result.value() == sizeof(uint8_t);
}
bool BinaryIOStream::readULE16(uint16_t *pVal)
{
	uint16_t val = 0;
	auto result = readBytes(&val, sizeof(uint16_t));
	if (!result.has_value()) { return false; }
	if (pVal)
	{
		*pVal = SDL_SwapLE16(val);
	}
	return result.value() == sizeof(uint16_t);
}
bool BinaryIOStream::readULE32(uint32_t *pVal)
{
	uint32_t val = 0;
	auto result = readBytes(&val, sizeof(uint32_t));
	if (!result.has_value()) { return false; }
	if (pVal)
	{
		*pVal = SDL_SwapLE32(val);
	}
	return result.value() == sizeof(uint32_t);
}
bool BinaryIOStream::readSLE8(int8_t *pVal)
{
	uint8_t val = 0;
	auto result = readBytes(&val, sizeof(int8_t));
	if (!result.has_value()) { return false; }
	if (pVal)
	{
		*pVal = val;
	}
	return result.value() == sizeof(int8_t);
}
bool BinaryIOStream::readSLE16(int16_t *pVal)
{
	int16_t val = 0;
	auto result = readBytes(&val, sizeof(int16_t));
	if (!result.has_value()) { return false; }
	if (pVal)
	{
		*pVal = SDL_SwapLE16(val);
	}
	return result.value() == sizeof(int16_t);
}
bool BinaryIOStream::readSLE32(int32_t *pVal)
{
	int32_t val = 0;
	auto result = readBytes(&val, sizeof(int32_t));
	if (!result.has_value()) { return false; }
	if (pVal)
	{
		*pVal = SDL_SwapLE32(val);
	}
	return result.value() == sizeof(int32_t);
}

bool BinaryIOStream::writeULE8(uint8_t val)
{
	auto result = writeBytes(&val, sizeof(val));
	if (!result.has_value()) { return false; }
	return result.value() == sizeof(val);
}
bool BinaryIOStream::writeULE16(uint16_t val)
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	val = SDL_Swap16(val);
#endif
	auto result = writeBytes(&val, sizeof(val));
	if (!result.has_value()) { return false; }
	return result.value() == sizeof(val);
}
bool BinaryIOStream::writeULE32(uint32_t val)
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	val = SDL_Swap32(val);
#endif
	auto result = writeBytes(&val, sizeof(val));
	if (!result.has_value()) { return false; }
	return result.value() == sizeof(val);
}
bool BinaryIOStream::writeSLE8(int8_t val)
{
	auto result = writeBytes(&val, sizeof(val));
	if (!result.has_value()) { return false; }
	return result.value() == sizeof(val);
}
bool BinaryIOStream::writeSLE16(int16_t val)
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	val = SDL_Swap16(val);
#endif
	auto result = writeBytes(&val, sizeof(val));
	if (!result.has_value()) { return false; }
	return result.value() == sizeof(val);
}
bool BinaryIOStream::writeSLE32(int32_t val)
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	val = SDL_Swap32(val);
#endif
	auto result = writeBytes(&val, sizeof(val));
	if (!result.has_value()) { return false; }
	return result.value() == sizeof(val);
}

class WzMapBinaryStdIOStream : public WzMap::BinaryIOStream
{
private:
#if defined(_WIN32)
	optional<std::vector<wchar_t>> utf8ToUtf16Win(const char* pStr)
	{
		int wstr_len = MultiByteToWideChar(CP_UTF8, 0, pStr, -1, NULL, 0);
		if (wstr_len <= 0)
		{
			fprintf(stderr, "Could not not convert string from UTF-8; MultiByteToWideChar failed with error %d: %s\n", GetLastError(), pStr);
			return nullopt;
		}
		std::vector<wchar_t> wstr_filename(wstr_len, 0);
		if (MultiByteToWideChar(CP_UTF8, 0, pStr, -1, &wstr_filename[0], wstr_len) == 0)
		{
			fprintf(stderr, "Could not not convert string from UTF-8; MultiByteToWideChar[2] failed with error %d: %s\n", GetLastError(), pStr);
			return nullopt;
		}
		return wstr_filename;
	}
#endif
public:
	WzMapBinaryStdIOStream(const char* pFilename, const char mode)
	{
		std::string modeStr;
		modeStr.push_back(mode); // r or w
		modeStr += "b"; // ensure binary file mode
	#if defined(_WIN32)
		// On Windows, path strings passed to fopen() are interpreted using the ANSI or OEM codepage
		// (and not as UTF-8). To support Unicode paths, the string must be converted to a wide-char
		// string and passed to _wfopen.
		auto wstrFilename = utf8ToUtf16Win(pFilename);
		if (!wstrFilename.has_value())
		{
			return;
		}
		auto wstrMode = utf8ToUtf16Win(modeStr.c_str());
		if (!wstrMode.has_value())
		{
			return;
		}
		wstrMode.value().push_back(L'N'); // Append "N" to ensure file is non-inheritable
		pFile = _wfopen(wstrFilename.value().data(), wstrMode.value().data());
	#else
		pFile = fopen(pFilename, modeStr.c_str());
		if (pFile)
		{
			// set file to non-inheritable
			int fd = fileno(pFile);
			if (fd != -1)
			{
				fcntl(fd, F_SETFD, FD_CLOEXEC);
			}
		}
	#endif
	}

	~WzMapBinaryStdIOStream()
	{
		if (pFile != nullptr)
		{
			fclose(pFile);
			pFile = nullptr;
		}
	}

	bool openedFile() const
	{
		return pFile != nullptr;
	}

	virtual optional<size_t> readBytes(void *buffer, size_t len) override
	{
		if (feof(pFile))
		{
			return 0;
		}
		size_t result = fread(buffer, 1, len, pFile);
		if (ferror(pFile))
		{
			return nullopt;
		}
		return result;
	};

	virtual optional<size_t> writeBytes(const void *buffer, size_t len) override
	{
		size_t result = fwrite(buffer, 1, len, pFile);
		if (ferror(pFile))
		{
			return nullopt;
		}
		return result;
	};
	virtual bool endOfStream() override
	{
		bool endOfStream = (feof(pFile) != 0);
		if (!endOfStream)
		{
			// feof may only be set once a read fails
			fpos_t position;
			if (fgetpos(pFile, &position) != 0)
			{
				// fgetpos call failed?
				// for now, shortcut the rest of this
				return endOfStream;
			}
			// attempt to read a byte
			uint8_t tmpByte;
			if (!readULE8(&tmpByte))
			{
				// read failed - check feof again
				endOfStream = (feof(pFile) != 0);
			}
			// restore original position
			fsetpos(pFile, &position);
		}
		return endOfStream;
	}

private:
	FILE* pFile = nullptr;
};

static char modeToStdIOMode(BinaryIOStream::OpenMode mode)
{
	switch (mode)
	{
		case BinaryIOStream::OpenMode::READ:
			return 'r';
		case BinaryIOStream::OpenMode::WRITE:
			return 'w';
	}
	return 'r'; // silence warning
}

std::unique_ptr<BinaryIOStream> StdIOProvider::openBinaryStream(const std::string& filename, BinaryIOStream::OpenMode mode)
{
	WzMapBinaryStdIOStream* pStream = new WzMapBinaryStdIOStream(filename.c_str(), modeToStdIOMode(mode));
	if (!pStream->openedFile())
	{
		delete pStream;
		return nullptr;
	}
	return std::unique_ptr<BinaryIOStream>(pStream);
}

bool StdIOProvider::loadFullFile(const std::string& filename, std::vector<char>& fileData)
{
	auto pStream = openBinaryStream(filename, BinaryIOStream::OpenMode::READ);
	if (!pStream)
	{
		return false;
	}
	size_t chunkSize = 512*1024;
	std::vector<char> data;
	size_t readPos = 0;
	optional<size_t> bytesRead = 0;
	do {
		data.resize(data.size() + chunkSize);
		bytesRead = pStream->readBytes(&(data[readPos]), chunkSize);
	} while (bytesRead.has_value() && bytesRead.value() == chunkSize);
	if (!bytesRead.has_value())
	{
		// failed reading
		return false;
	}
	data.resize(readPos + bytesRead.value()); // truncate to exact length read
	fileData = std::move(data);
	return true;
}

bool StdIOProvider::writeFullFile(const std::string& filename, const char *ppFileData, uint32_t fileSize)
{
	auto pStream = openBinaryStream(filename, BinaryIOStream::OpenMode::WRITE);
	if (!pStream)
	{
		return false;
	}
	optional<size_t> bytesWritten = pStream->writeBytes(ppFileData, fileSize);
	if (!bytesWritten.has_value())
	{
		// failed writing
		return false;
	}
	if (bytesWritten.value() != fileSize)
	{
		// failed writing all of the data
		return false;
	}
	return true;
}

} // namespace WzMap
