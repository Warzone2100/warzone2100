/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2022  Warzone 2100 Project

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

#include "../include/wzmaplib/map_io.h"
#include "map_internal.h"
#include <cstdio>
#include <cstring>
#include <list>

#if defined(_WIN32)
# define WIN32_LEAN_AND_MEAN
# define WIN32_EXTRA_LEAN
# undef NOMINMAX
# define NOMINMAX 1
# include <windows.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#if !defined(_WIN32)
# include <fcntl.h>
# include <dirent.h> // TODO: Also use CMake check for dirent.h
#else
# include <direct.h> // For _mkdir / _wmkdir on Windows
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

bool BinaryIOStream::writeUBE32(uint32_t val)
{
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	val = SDL_Swap32(val);
#endif
	auto result = writeBytes(&val, sizeof(val));
	if (!result.has_value()) { return false; }
	return result.value() == sizeof(val);
}
bool BinaryIOStream::writeSBE32(int32_t val)
{
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	val = SDL_Swap32(val);
#endif
	auto result = writeBytes(&val, sizeof(val));
	if (!result.has_value()) { return false; }
	return result.value() == sizeof(val);
}

bool IOProvider::enumerateFiles(const std::string& basePath, const std::function<bool (const char* file)>& enumFunc)
{
	// Must implement in subclasses
	return false;
}

bool IOProvider::enumerateFolders(const std::string& basePath, const std::function<bool (const char* file)>& enumFunc)
{
	// Must implement in subclasses
	return false;
}

std::string IOProvider::pathJoin(const std::string& a, const std::string& b)
{
	std::string fullPath = a;
	if (b.empty())
	{
		return fullPath;
	}
	std::string separator = pathSeparator();
	if (!fullPath.empty() && !WzMap::strEndsWith(fullPath, separator))
	{
		fullPath += separator;
	}
	fullPath += b;
	return fullPath;
}

std::string IOProvider::pathDirName(const std::string& fullPath)
{
	// Split the path at the path separator
	std::string pathSep = pathSeparator();
	if (pathSep.empty())
	{
		return fullPath;
	}
	size_t dirnameLength = fullPath.size();
	size_t lastPos = dirnameLength;
	do {
		lastPos = dirnameLength - pathSep.length();
		dirnameLength = fullPath.rfind(pathSep, lastPos);
	} while (dirnameLength != std::string::npos && dirnameLength == lastPos);

	return (dirnameLength != std::string::npos) ? fullPath.substr(0, dirnameLength) : "";
}

std::string IOProvider::pathBaseName(const std::string& fullPath)
{
	// Split the path at the path separator
	std::string pathSep = pathSeparator();
	if (pathSep.empty())
	{
		return fullPath;
	}
	size_t dirnameLength = fullPath.size();
	size_t lastPos = dirnameLength;
	do {
		lastPos = dirnameLength - pathSep.length();
		dirnameLength = fullPath.rfind(pathSep, lastPos);
	} while (dirnameLength != std::string::npos && dirnameLength == lastPos);

	return (dirnameLength != std::string::npos) ? fullPath.substr(dirnameLength + 1) : fullPath;
}

bool IOProvider::enumerateFilesRecursive(const std::string& basePath, const std::function<bool (const char* file)>& enumFunc)
{
	// A base implementation of recursive enumeration that utilizes the regular enumerateFiles/Folders functions.
	// If a particular IOProvider can provide a more optimal implementation, it can override this.

	// Step 1: Enumerate all the files in this folder
	bool endEnum = false;
	bool enumSuccess = enumerateFiles(basePath, [&](const char* file) -> bool {
		if (!enumFunc(file))
		{
			endEnum = true;
			return false;
		}
		return true;
	});
	if (!enumSuccess) { return false; }
	if (endEnum) { return true; }

	// Step 2: Enumerate all the folders in this folder, enumerating all the files in each
	enumSuccess = enumerateFoldersRecursive(basePath, [&](const char* folder) -> bool {
		bool enumFilesSuccess = enumerateFiles(pathJoin(basePath, folder), [&](const char* file) -> bool {
			std::string relativePath = pathJoin(folder, file);
			if (!enumFunc(relativePath.c_str()))
			{
				endEnum = true;
				return false; // stop enumerating files
			}
			return true; // continue enumerating files
		});
		if (!enumFilesSuccess || endEnum)
		{
			return false; // stop enumerating folders
		}
		return true; // continue enumerating folders
	});

	if (!enumSuccess) { return false; }
	return true;
}

bool IOProvider::enumerateFoldersRecursive(const std::string& basePath, const std::function<bool (const char* file)>& enumFunc)
{
	// A base implementation of recursive enumeration that utilizes the regular enumerateFolders function.
	// If a particular IOProvider can provide a more optimal implementation, it can override this.

	std::list<std::string> foldersList;
	foldersList.push_back(""); // "" is used for basePath
	std::string currentPath;
	bool endEnum = false;
	while (!foldersList.empty())
	{
		// enumerate the last folder in the foldersList
		auto it = std::prev(foldersList.end());
		currentPath = *it;
		bool enumSuccess = enumerateFolders(pathJoin(basePath, currentPath), [&](const char* subfolder) -> bool {
			std::string fullRelativePath = currentPath + subfolder;
			// call enumFunc with the enumerated subfolder
			if (!enumFunc(fullRelativePath.c_str()))
			{
				endEnum = true;
				return false;
			}

			// push this subfolder onto the stack
			foldersList.push_back(fullRelativePath);

			return true; // continue enumerating
		});

		// remove the item just processed from the stack
		foldersList.erase(it);

		if (!enumSuccess)
		{
			return false;
		}
		if (endEnum) { return true; }
	}
	return true;
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
			fprintf(stderr, "Could not not convert string from UTF-8; MultiByteToWideChar failed with error %lu: %s\n", GetLastError(), pStr);
			return nullopt;
		}
		std::vector<wchar_t> wstr_filename(wstr_len, 0);
		if (MultiByteToWideChar(CP_UTF8, 0, pStr, -1, &wstr_filename[0], wstr_len) == 0)
		{
			fprintf(stderr, "Could not not convert string from UTF-8; MultiByteToWideChar[2] failed with error %lu: %s\n", GetLastError(), pStr);
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
		close();
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

	virtual bool close() override
	{
		if (pFile == nullptr)
		{
			return false;
		}

		fclose(pFile);
		pFile = nullptr;
		return true;
	}

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

static bool stdIOFolderExistsInternal(const std::string& dirPath)
{
#if defined(_WIN32)
	std::vector<wchar_t> wDirPath;
	if (!win_utf8ToUtf16(dirPath.c_str(), wDirPath))
	{
		return false;
	}
	struct _stat buf;
	int result = _wstat(wDirPath.data(), &buf);
	if (result != 0)
	{
		return false;
	}
	return (buf.st_mode & _S_IFDIR) == _S_IFDIR;
#else
	struct stat buf;
	int result = stat(dirPath.c_str(), &buf);
	if (result != 0)
	{
		return false;
	}
	return (buf.st_mode & S_IFDIR) == S_IFDIR;
#endif
}

bool StdIOProvider::internalMakeDir(const std::string& dirPath)
{
#if defined(_WIN32)
	std::vector<wchar_t> wDirPath;
	if (!win_utf8ToUtf16(dirPath.c_str(), wDirPath))
	{
		return false;
	}
	return _wmkdir(wDirPath.data()) == 0;
#else
	return mkdir(dirPath.c_str(), 0755) == 0;
#endif
}

// Creates a directory (and any required parent directories) and returns true on success (or if the directory already exists)
bool StdIOProvider::makeDirectory(const std::string& directoryPath)
{
	bool result = false;
	size_t currSeparatorPos = 0;
	currSeparatorPos = directoryPath.find(pathSeparator(), currSeparatorPos + 1);
	do
	{
		std::string currentSubpath = directoryPath.substr(0, currSeparatorPos);
		result = stdIOFolderExistsInternal(currentSubpath);
		if (!result)
		{
			result = internalMakeDir(currentSubpath);
			if (!result)
			{
				return false;
			}
		}
		if (currSeparatorPos == std::string::npos)
		{
			return result;
		}
		currSeparatorPos = directoryPath.find(pathSeparator(), currSeparatorPos + 1);
	} while (true);

	return result;
}

const char* StdIOProvider::pathSeparator() const
{
#if defined(_WIN32)
	return "\\";
#else
	return "/";
#endif
}

enum EnumDirFlags
{
	ENUM_RECURSE = 0x01,
	ENUM_FILES = 0x02,
	ENUM_FOLDERS = 0x04
};
bool enumerateDirInternal(const std::string& rootBasePath, const std::string& basePath, int enumDirFlags, const std::function<bool (const char* file)>& enumFunc)
{
#if defined(_WIN32)
	// List files inside the basePath directory (recursing into subdirectories)
	std::string findFileStr = rootBasePath;
	if (!findFileStr.empty() && findFileStr.back() != '\\')
	{
		findFileStr += "\\";
	}
	if (!basePath.empty())
	{
		findFileStr += basePath;
		if (basePath.back() != '\\')
		{
			findFileStr += "\\";
		}
	}
	findFileStr += "*";
	std::vector<wchar_t> wFindFileStr;
	if (!win_utf8ToUtf16(findFileStr.c_str(), wFindFileStr))
	{
		return false;
	}
	WIN32_FIND_DATAW ffd;
	HANDLE hFind = FindFirstFileW(wFindFileStr.data(), &ffd);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	std::vector<char> u8_buffer(MAX_PATH, 0);
	std::string fullFilePath;
	do {
		// convert ffd.cFileName to UTF-8
		if (!win_utf16toUtf8(ffd.cFileName, u8_buffer))
		{
			// encoding conversion error...
			continue;
		}
		if (strcmp(u8_buffer.data(), ".") == 0 || strcmp(u8_buffer.data(), "..") == 0)
		{
			continue;
		}
		fullFilePath = basePath + "\\" + u8_buffer.data();
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			// found a file
			if ((enumDirFlags & ENUM_FILES) == ENUM_FILES)
			{
				if (!enumFunc(fullFilePath.c_str()))
				{
					break;
				}
			}
		}
		else
		{
			if (((enumDirFlags & ENUM_FOLDERS) == ENUM_FOLDERS))
			{
				if (!enumFunc(fullFilePath.c_str()))
				{
					break;
				}
			}
			if (((enumDirFlags & ENUM_RECURSE) == ENUM_RECURSE))
			{
				// recurse into subfolder
				enumerateDirInternal(rootBasePath, fullFilePath, enumDirFlags, enumFunc);
			}
		}
	} while (FindNextFileW(hFind, &ffd) != 0);
	FindClose(hFind);
#else
	struct dirent *dir = NULL;
	std::string dirToSearch = rootBasePath;
	if (!dirToSearch.empty() && dirToSearch.back() != '/')
	{
		dirToSearch += "/";
	}
	dirToSearch += basePath;
	DIR *d = opendir(dirToSearch.c_str());
	if (d == NULL)
	{
		return false;
	}
	std::string fullFilePath;
	while ((dir = readdir(d)))
	{
		if (strcmp(dir->d_name, "." ) == 0 || strcmp(dir->d_name, "..") == 0)
		{
			continue;
		}
		fullFilePath = basePath + "/" + dir->d_name;
		if (dir->d_type == DT_DIR)
		{
			// recurse
			if (((enumDirFlags & ENUM_FOLDERS) == ENUM_FOLDERS))
			{
				if (!enumFunc(fullFilePath.c_str()))
				{
					break;
				}
			}
			if (((enumDirFlags & ENUM_RECURSE) == ENUM_RECURSE))
			{
				enumerateDirInternal(rootBasePath, fullFilePath, enumDirFlags, enumFunc);
			}
		}
		else if ((enumDirFlags & ENUM_FILES) == ENUM_FILES)
		{
			// found a file
			if (!enumFunc(fullFilePath.c_str()))
			{
				break;
			}
		}
	}
	closedir(d);
#endif
	return true;
}

// enumFunc receives each enumerated file, and returns true to continue enumeration, or false to shortcut / stop enumeration
bool StdIOProvider::enumerateFiles(const std::string& basePath, const std::function<bool (const char* file)>& enumFunc)
{
	return enumerateDirInternal(basePath, "", ENUM_FILES, enumFunc);
}

// enumFunc receives each enumerated subfolder, and returns true to continue enumeration, or false to shortcut / stop enumeration
bool StdIOProvider::enumerateFolders(const std::string& basePath, const std::function<bool (const char* file)>& enumFunc)
{
	return enumerateDirInternal(basePath, "", ENUM_FOLDERS, enumFunc);
}

} // namespace WzMap
