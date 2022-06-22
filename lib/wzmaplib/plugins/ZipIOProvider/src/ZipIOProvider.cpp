/*
	This file is part of Warzone 2100.
	Copyright (C) 2022  Warzone 2100 Project

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

// A WzMap::IOProvider implementation that uses libzip (https://libzip.org/) to support loading from zip archives
// You must link to libzip (and any of its required dependencies)

#include "ZipIOProvider.h"

#include <memory>
#include <unordered_set>
#include <cstring>
#include <algorithm>

#include <limits>
#include <zip.h> // from libzip

#include "../src/map_internal.h"
#include <wzmaplib/map_io.h>

#if defined(_WIN32)
# define WIN32_LEAN_AND_MEAN
# define WIN32_EXTRA_LEAN
# undef NOMINMAX
# define NOMINMAX 1
# include <windows.h>
#endif

class WrappedZipArchive
{
public:
	typedef std::function<void ()> PostCloseFunc;
public:
	WrappedZipArchive(zip_t* pZip, PostCloseFunc postCloseFunc = nullptr)
	: pZip(pZip)
	, postCloseFunc(postCloseFunc)
	{ }
	~WrappedZipArchive()
	{
		close();
	}
	zip_t* handle() const
	{
		return pZip;
	}
protected:
	void close()
	{
		if (!pZip) { return; }
		if (readOnly)
		{
			zip_discard(pZip);
		}
		else
		{
			zip_close(pZip);
		}
		pZip = nullptr;
		if (postCloseFunc)
		{
			postCloseFunc();
		}
	}
private:
	zip_t* pZip = nullptr;
	PostCloseFunc postCloseFunc;
	bool readOnly = false;
};

class WzMapBinaryZipIOStream : public WzMap::BinaryIOStream
{
private:
	WzMapBinaryZipIOStream(std::shared_ptr<WrappedZipArchive> zipArchive, WzMap::BinaryIOStream::OpenMode mode)
	: m_zipArchive(zipArchive)
	, m_mode(mode)
	{}
public:
	static std::unique_ptr<WzMapBinaryZipIOStream> openForReading(zip_uint64_t zipArchiveIndex, std::shared_ptr<WrappedZipArchive> zipArchive)
	{
		if (zipArchive == nullptr)
		{
			return nullptr;
		}
		auto result = std::unique_ptr<WzMapBinaryZipIOStream>(new WzMapBinaryZipIOStream(zipArchive, WzMap::BinaryIOStream::OpenMode::READ));
		result->m_pReadHandle = zip_fopen_index(zipArchive->handle(), zipArchiveIndex, 0);
		return result;
	}

	static std::unique_ptr<WzMapBinaryZipIOStream> openForWriting(const std::string& filename, std::shared_ptr<WrappedZipArchive> zipArchive, bool fixedLastMod = false)
	{
		if (filename.empty() || zipArchive == nullptr)
		{
			return nullptr;
		}
		auto result = std::unique_ptr<WzMapBinaryZipIOStream>(new WzMapBinaryZipIOStream(zipArchive, WzMap::BinaryIOStream::OpenMode::WRITE));
		result->m_filename = filename;
		result->m_fixedLastMod = fixedLastMod;
		return result;
	}

public:
	virtual ~WzMapBinaryZipIOStream()
	{
		close();
	};

	virtual optional<size_t> readBytes(void *buffer, size_t len) override
	{
		if (!m_pReadHandle) { return nullopt; }
		size_t extraByte = 0;
		if (m_extraByteRead.has_value())
		{
			uint8_t tmpByte = m_extraByteRead.value();
			memcpy(buffer, &tmpByte, 1);
			m_extraByteRead.reset();
			buffer = reinterpret_cast<char*>(buffer) + 1;
			len -= 1;
			extraByte = 1;
		}
		auto result = zip_fread(m_pReadHandle, buffer, len);
		if (result < 0)
		{
			// failed
			if (extraByte)
			{
				return extraByte;
			}
			return nullopt;
		}
		return static_cast<size_t>(result) + extraByte;
	}

	virtual optional<size_t> writeBytes(const void *buffer, size_t len) override
	{
		// Expand writeBuffer if needed
		if (m_writeBufferCapacity - m_writeBufferLen < len)
		{
			size_t newCapacity = std::max<size_t>(m_writeBufferCapacity + std::max<size_t>((m_writeBufferCapacity / 2), len), 1024);
			auto newBuffer = (unsigned char*)realloc(m_writeBuffer, newCapacity);
			if (newBuffer == NULL)
			{
				return nullopt;
			}
			m_writeBuffer = newBuffer;
			m_writeBufferCapacity = newCapacity;
		}
		// Copy to writeBuffer
		memcpy(m_writeBuffer + m_writeBufferLen, buffer, len);
		m_writeBufferLen += len;

		return len;
	}

	virtual bool close() override
	{
		// If reading
		if (m_pReadHandle != nullptr)
		{
			zip_fclose(m_pReadHandle);
			m_pReadHandle = nullptr;
			m_filename.clear();
		}
		// If writing
		else if (m_writeBufferLen > 0)
		{
			if (m_writeBuffer == nullptr)
			{
				return false; // should not happen
			}
			zip_source_t *s = zip_source_buffer(m_zipArchive->handle(), m_writeBuffer, m_writeBufferLen, 1);
			if (s == NULL)
			{
				// Failed to create a source buffer from the write buffer?
				free(m_writeBuffer);
				m_writeBuffer = nullptr;
				m_writeBufferLen = 0;
				m_writeBufferCapacity = 0;
				m_filename.clear();
				return false;
			}
			// From this point on, libzip is now responsible for freeing the m_writeBuffer
			// Reset local variables
			m_writeBuffer = nullptr;
			m_writeBufferLen = 0;
			m_writeBufferCapacity = 0;
			// Add the source data to the zip as a file
			if (m_filename.empty()) // should not happen
			{
				zip_source_free(s);
				return false;
			}
			zip_int64_t result = zip_file_add(m_zipArchive->handle(), m_filename.c_str(), s, ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8);
			// NOTE: Do *not* call zip_source_free(s) if zip_file_add succeeds!
			if (result < 0)
			{
				// Failed to write file
				zip_source_free(s);
				m_filename.clear();
				return false;
			}
			if (m_fixedLastMod)
			{
				// Use Jan 1, 1980 + 12 hours and 1 minute to avoid time zone weirdness (matching "strip-nondeterminism" script behavior)
				zip_file_set_dostime(m_zipArchive->handle(), static_cast<zip_uint64_t>(result), 0x6020, 0x21, 0);
			}
			m_filename.clear();
		}
		return true;
	}

	virtual bool endOfStream() override
	{
		if (m_mode != WzMap::BinaryIOStream::OpenMode::READ)
		{
			return false;
		}
		if (!m_pReadHandle) { return false; }
		if (m_extraByteRead.has_value())
		{
			// at least one more byte to read!
			return false;
		}
		// attempt to read a byte
		uint8_t tmpByte;
		if (!readULE8(&tmpByte))
		{
			// read failed - assume end of stream
			return true;
		}
		// otherwise store the extra byte we read (for the next call to readBytes)
		m_extraByteRead = tmpByte;
		return false;
	}
private:
	std::shared_ptr<WrappedZipArchive> m_zipArchive;
	optional<WzMap::BinaryIOStream::OpenMode> m_mode;
	// reading
	zip_file_t *m_pReadHandle = nullptr;
	optional<uint8_t> m_extraByteRead;
	// writing
	std::string m_filename;
	unsigned char* m_writeBuffer = nullptr;
	size_t m_writeBufferLen = 0;
	size_t m_writeBufferCapacity = 0;
	bool m_fixedLastMod = false;
};

static zip_int64_t wz_zip_name_locate_impl(zip_t *archive, const char *fname, zip_flags_t flags, bool useWindowsPathWorkaroundIfNeeded)
{
	auto zipLocateResult = zip_name_locate(archive, fname, flags);
	if (zipLocateResult < 0)
	{
		// Failed to find a file with this name
		if (useWindowsPathWorkaroundIfNeeded && archive != NULL && fname != NULL)
		{
			// Replace all '/' in the input fname with '\\' and try again
			std::string fNameAdjusted = fname;
			std::replace(fNameAdjusted.begin(), fNameAdjusted.end(), '/', '\\');
			zipLocateResult = zip_name_locate(archive, fNameAdjusted.c_str(), flags);
		}
	}
	return zipLocateResult;
}

#define malformed_windows_path_separators_workaround \
	((m_foundMalformedWindowsPathSeparators.has_value()) ? m_foundMalformedWindowsPathSeparators.value() : determineIfMalformedWindowsPathSeparatorWorkaround())

#define wz_zip_name_locate(archive, fname, flags) \
	wz_zip_name_locate_impl(archive, fname, flags, malformed_windows_path_separators_workaround)

std::shared_ptr<WzMapZipIO> WzMapZipIO::openZipArchiveFS(const char* fileSystemPath, bool extraConsistencyChecks, bool readOnly)
{
	if (fileSystemPath == nullptr) { return nullptr; }
	struct zip_error error;
	zip_error_init(&error);
#if defined(_WIN32)
	// Special win32 handling (convert path from UTF-8 to UTF-16 and use the wide-char win32 source functions)
	std::vector<wchar_t> wFileSystemPathStr;
	if (!WzMap::win_utf8ToUtf16(fileSystemPath, wFileSystemPathStr))
	{
		return nullptr;
	}
	zip_source_t* s = zip_source_win32w_create(wFileSystemPathStr.data(), 0, -1, &error);
#else
	zip_source_t* s = zip_source_file_create(fileSystemPath, 0, -1, &error);
#endif
	if (s == NULL)
	{
		// Failed to create source / open file
		zip_error_fini(&error);
		return nullptr;
	}
	int flags = 0;
	if (extraConsistencyChecks)
	{
		flags |= ZIP_CHECKCONS;
	}
	if (readOnly)
	{
		flags |= ZIP_RDONLY;
	}
	zip_t* pZip = zip_open_from_source(s, flags, &error);
	if (pZip == NULL)
	{
		// Failed to open from source
		zip_source_free(s);
		zip_error_fini(&error);
		return nullptr;
	}
	zip_error_fini(&error);
	auto result = std::shared_ptr<WzMapZipIO>(new WzMapZipIO());
	result->m_zipArchive = std::make_shared<WrappedZipArchive>(pZip);
	return result;
}

std::shared_ptr<WzMapZipIO> WzMapZipIO::createZipArchiveFS(const char* fileSystemPath, bool fixedLastMod /*= false*/)
{
	if (fileSystemPath == nullptr) { return nullptr; }
	if (*fileSystemPath == '\0') { return nullptr; }

	struct zip_error error;
	zip_error_init(&error);

	// Create new (empty) in-memory source buffer
	zip_source_t *pMemSource = zip_source_buffer_create(NULL, 0, 1, &error);
	if (pMemSource == NULL)
	{
		// Failed to create source
		zip_error_fini(&error);
		return nullptr;
	}

	int flags = ZIP_TRUNCATE;
	zip_t* pZip = zip_open_from_source(pMemSource, flags, &error);
	if (pZip == NULL)
	{
		// Failed to open from source
		zip_source_free(pMemSource);
		zip_error_fini(&error);
		return nullptr;
	}

	zip_error_fini(&error);
	zip_source_keep(pMemSource); // explicitly keep the buffer around after the in-memory zip file is "closed"

	auto result = std::shared_ptr<WzMapZipIO>(new WzMapZipIO());
	std::string writeNewZipOutputPath = fileSystemPath;
	result->m_zipArchive = std::make_shared<WrappedZipArchive>(pZip, [pMemSource, writeNewZipOutputPath]() {
		// This closure is run after the zip file is "closed"

		if (zip_source_is_deleted(pMemSource))
		{
			// the zip is empty, so do nothing
			zip_source_free(pMemSource);
			return;
		}

		zip_stat_t zst;
		if (zip_source_stat(pMemSource, &zst) < 0)
		{
			zip_source_free(pMemSource);
			return;
		}

		if (zip_source_open(pMemSource) < 0)
		{
			zip_source_free(pMemSource);
			return;
		}

		std::vector<char> zipDataBuffer(zst.size, 0);

		auto readResult = zip_source_read(pMemSource, zipDataBuffer.data(), zst.size);
		if (readResult < 0 || static_cast<zip_uint64_t>(readResult) < zst.size)
		{
			zip_source_close(pMemSource);
			zip_source_free(pMemSource);
			return;
		}

		zip_source_close(pMemSource);
		zip_source_free(pMemSource);

		//  Write out the zipDataBuffer to a file at the writeNewZipOutputPath
		WzMap::StdIOProvider stdIOProvider;
		if (!stdIOProvider.writeFullFile(writeNewZipOutputPath, zipDataBuffer.data(), static_cast<uint32_t>(zipDataBuffer.size())))
		{
			// Failed to write out zip buffer data
			return;
		}
	});
	result->m_fixedLastMod = fixedLastMod;
	return result;
}

WzMapZipIO::~WzMapZipIO()
{ }

std::unique_ptr<WzMap::BinaryIOStream> WzMapZipIO::openBinaryStream(const std::string& filename, WzMap::BinaryIOStream::OpenMode mode)
{
	std::unique_ptr<WzMap::BinaryIOStream> pStream;
	switch (mode)
	{
		case WzMap::BinaryIOStream::OpenMode::READ:
		{
			auto zipLocateResult = wz_zip_name_locate(m_zipArchive->handle(), filename.c_str(), ZIP_FL_ENC_GUESS);
			if (zipLocateResult < 0)
			{
				// Failed to find a file with this name
				return nullptr;
			}
			zip_uint64_t zipFileIndex = static_cast<zip_uint64_t>(zipLocateResult);
			pStream = WzMapBinaryZipIOStream::openForReading(zipFileIndex, m_zipArchive);
			break;
		}
		case WzMap::BinaryIOStream::OpenMode::WRITE:
			pStream = WzMapBinaryZipIOStream::openForWriting(filename, m_zipArchive, m_fixedLastMod);
			break;
	}
	return pStream;
}

bool WzMapZipIO::loadFullFile(const std::string& filename, std::vector<char>& fileData)
{
	auto zipLocateResult = wz_zip_name_locate(m_zipArchive->handle(), filename.c_str(), ZIP_FL_ENC_GUESS);
	if (zipLocateResult < 0)
	{
		// Failed to find a file with this name
		return false;
	}
	zip_uint64_t zipFileIndex = static_cast<zip_uint64_t>(zipLocateResult);
	// Get the expected length of the file
	struct zip_stat st;
	zip_stat_init(&st);
	if (zip_stat_index(m_zipArchive->handle(), zipFileIndex, 0, &st) != 0)
	{
		// zip_stat failed for file??
		return false;
	}
	if (!(st.valid & ZIP_STAT_SIZE))
	{
		// couldn't get the file size??
		return false;
	}
	if (std::numeric_limits<uint32_t>::max() < st.size)
	{
		// size is too big!
		return false;
	}
	auto readStream = WzMapBinaryZipIOStream::openForReading(zipFileIndex, m_zipArchive);
	if (!readStream)
	{
		return false;
	}
	// read the entire file
	fileData.clear();
	fileData.resize(static_cast<size_t>(st.size));
	auto result = readStream->readBytes(fileData.data(), fileData.size());
	if (!result.has_value())
	{
		// read failed
		return false;
	}
	if (result.value() != fileData.size())
	{
		// read was short
		fileData.clear();
		return false;
	}
	readStream->close();
	return true;
}

bool WzMapZipIO::writeFullFile(const std::string& filename, const char *ppFileData, uint32_t fileSize)
{
	auto writeStream = WzMapBinaryZipIOStream::openForWriting(filename, m_zipArchive, m_fixedLastMod);
	if (!writeStream)
	{
		return false;
	}
	// write the entire file
	auto result = writeStream->writeBytes(ppFileData, fileSize);
	if (!result.has_value())
	{
		// write failed
		return false;
	}
	if (result.value() != fileSize)
	{
		// write was short??
		return false;
	}
	writeStream->close();

	m_cachedDirectoriesList.clear(); // for now, just clear so it's re-generated if enumerateFolders is called
	return true;
}

bool WzMapZipIO::makeDirectory(const std::string& directoryPath)
{
	// We could explicitly write a directory entry in the zip here, but since this is not necessary just return true
	return true;
}

const char* WzMapZipIO::pathSeparator() const
{
	return "/";
}

static inline bool isUnsafeZipEntryName(const std::string& filename)
{
	if (filename.empty())
	{
		return true; // unexpected empty name
	}

	// Check for directory traversal
	// This will reject *any* filenames with ".." in them, but that should never happen for expected filenames in WZ archives
	if (filename.find("..") != std::string::npos)
	{
		return true;
	}

	// Reject paths that start with the path separator (or the Windows path separator)
	if (filename.front() == '/' || filename.front() == '\\')
	{
		return true;
	}

	// Reject paths that appear to start with a drive letter (Windows)
	if (filename.size() >= 2 && filename[1] == ':'
		&& ((filename[0] >= 'A' && filename[0] <= 'Z') || (filename[0] >= 'a' && filename[0] <= 'z')))
	{
		return true;
	}

	return false;
}

bool WzMapZipIO::enumerateFilesInternal(const std::string& basePath, bool recurse, const std::function<bool (const char* file)>& enumFunc)
{
	if (!enumFunc)
	{
		return false;
	}
	zip_int64_t result = zip_get_num_entries(m_zipArchive->handle(), 0);
	if (result < 0)
	{
		return false;
	}
	std::string basePathToSearch = basePath;
	bool emptyBasePath = basePathToSearch.empty();
	if (!emptyBasePath && basePathToSearch.back() != '/')
	{
		basePathToSearch += '/';
	}
	if (basePathToSearch == "/")
	{
		basePathToSearch = "";
		emptyBasePath = true;
	}
	std::string nameStr;
	for (zip_uint64_t idx = 0; idx < static_cast<zip_uint64_t>(result); idx++)
	{
		const char *name = zip_get_name(m_zipArchive->handle(), idx, ZIP_FL_ENC_GUESS);
		if (name == NULL)
		{
			continue;
		}
		if (*name == '\0')
		{
			continue;
		}
		nameStr = name;

		if (malformed_windows_path_separators_workaround)
		{
			zip_uint8_t opsys = ZIP_OPSYS_UNIX;
			if (zip_file_get_external_attributes(m_zipArchive->handle(), idx, 0, &opsys, NULL) == 0)
			{
				if (opsys == ZIP_OPSYS_DOS && strchr(name, '\\') != nullptr)
				{
					std::replace(nameStr.begin(), nameStr.end(), '\\', '/');
				}
			}
		}

		if (!emptyBasePath && strncmp(basePathToSearch.c_str(), nameStr.c_str(), basePathToSearch.size()) != 0)
		{
			continue;
		}

		// filter out unsafe entry paths
		if (isUnsafeZipEntryName(nameStr))
		{
			continue;
		}

		// filter out entries that end with "/" (these are dedicated directory entries)
		if (nameStr.back() == '/')
		{
			continue;
		}

		if (!recurse && nameStr.find('/', basePathToSearch.length()) != std::string::npos)
		{
			continue;
		}

		// remove prefix from path
		if (!emptyBasePath)
		{
			nameStr = nameStr.substr(basePathToSearch.length());
		}

		if (!enumFunc(nameStr.c_str()))
		{
			break;
		}
	}
	return true;
}

bool WzMapZipIO::enumerateFoldersInternal(const std::string& basePath, bool recurse, const std::function<bool (const char* file)>& enumFunc)
{
	if (!enumFunc)
	{
		return false;
	}

	if (m_cachedDirectoriesList.empty())
	{
		// Valid ZIP files may or may not contain dedicated directory entries (which end in "/")
		// So the only way to be sure is to enumerate everything and build a listing of all possible directories

		std::unordered_set<std::string> foundDirectoriesSet;
		zip_int64_t result = zip_get_num_entries(m_zipArchive->handle(), 0);
		if (result < 0)
		{
			return false;
		}
		for (zip_uint64_t idx = 0; idx < static_cast<zip_uint64_t>(result); idx++)
		{
			const char *name = zip_get_name(m_zipArchive->handle(), idx, ZIP_FL_ENC_GUESS);
			if (name == NULL)
			{
				continue;
			}
			std::string nameStr = name;
			if (nameStr.empty())
			{
				continue;
			}
			// support non-conforming Zip files that use Windows path separators (certain old compressor tools on Windows)
			if (malformed_windows_path_separators_workaround)
			{
				zip_uint8_t opsys = ZIP_OPSYS_UNIX;
				if (zip_file_get_external_attributes(m_zipArchive->handle(), idx, 0, &opsys, NULL) == 0)
				{
					if (opsys == ZIP_OPSYS_DOS && nameStr.find('\\') != std::string::npos)
					{
						std::replace(nameStr.begin(), nameStr.end(), '\\', '/');
					}
				}
			}

			// filter out unsafe entry paths
			if (isUnsafeZipEntryName(nameStr))
			{
				continue;
			}

			// entries that end with "/" are dedicated directory entries

			size_t directoryDepth = 0;
			while (!nameStr.empty())
			{
				if (nameStr.back() != '/')
				{
					// otherwise, trim everything after the last "/" (i.e. trim the filename / basename)
					// to get the parent directory path
					auto lastSlashPos = nameStr.rfind('/');
					if (lastSlashPos == std::string::npos)
					{
						// no slash found
						break;
					}
					nameStr = nameStr.substr(0, lastSlashPos + 1);
				}

				auto setInsertResult = foundDirectoriesSet.insert(nameStr);
				if (setInsertResult.second)
				{
					m_cachedDirectoriesList.push_back(nameStr);
				}
				directoryDepth++;

				// remove any trailing "/"
				size_t numTrailingSlash = 0;
				for (auto it_r = nameStr.rbegin(); it_r != nameStr.rend(); it_r++)
				{
					if (*it_r != '/')
					{
						break;
					}
					numTrailingSlash++;
				}
				nameStr.resize(nameStr.size() - numTrailingSlash);
			}
		}

		// sort the directory list
		std::sort(m_cachedDirectoriesList.begin(), m_cachedDirectoriesList.end());
	}

	std::string basePathToSearch = basePath;
	bool emptyBasePath = basePathToSearch.empty();
	if (!emptyBasePath && basePathToSearch.back() != '/')
	{
		basePathToSearch += '/';
	}
	if (basePathToSearch == "/")
	{
		basePathToSearch = "";
		emptyBasePath = true;
	}
	for (const auto& dirPath : m_cachedDirectoriesList)
	{
		if (!emptyBasePath && strncmp(basePathToSearch.c_str(), dirPath.c_str(), basePathToSearch.size()) != 0)
		{
			continue;
		}
		// exclude exact match of basePathToSearch
		if (dirPath.length() == basePathToSearch.length())
		{
			continue;
		}
		// only recurse into subdirectories if permitted
		if (!recurse)
		{
			size_t firstSlashPos = dirPath.find('/', basePathToSearch.length());
			if (firstSlashPos != std::string::npos && firstSlashPos != (dirPath.length() - 1))
			{
				continue;
			}
		}
		// remove prefix from dirPath
		std::string relativeDirPath = dirPath.substr(basePathToSearch.length());
		if (!enumFunc(relativeDirPath.c_str()))
		{
			break;
		}
	}
	return true;
}

bool WzMapZipIO::enumerateFiles(const std::string& basePath, const std::function<bool (const char* file)>& enumFunc)
{
	return enumerateFilesInternal(basePath, false, enumFunc);
}

bool WzMapZipIO::enumerateFolders(const std::string& basePath, const std::function<bool (const char* file)>& enumFunc)
{
	return enumerateFoldersInternal(basePath, false, enumFunc);
}

bool WzMapZipIO::enumerateFilesRecursive(const std::string& basePath, const std::function<bool (const char* file)>& enumFunc)
{
	return enumerateFilesInternal(basePath, true, enumFunc);
}

bool WzMapZipIO::enumerateFoldersRecursive(const std::string& basePath, const std::function<bool (const char* file)>& enumFunc)
{
	return enumerateFoldersInternal(basePath, true, enumFunc);
}

bool WzMapZipIO::determineIfMalformedWindowsPathSeparatorWorkaround()
{
	zip_int64_t result = zip_get_num_entries(m_zipArchive->handle(), 0);
	if (result < 0) { return false; }
	for (zip_uint64_t idx = 0; idx < static_cast<zip_uint64_t>(result); idx++)
	{
		const char *name = zip_get_name(m_zipArchive->handle(), idx, ZIP_FL_ENC_GUESS);
		if (name == NULL)
		{
			continue;
		}
		zip_uint8_t opsys;
		if (zip_file_get_external_attributes(m_zipArchive->handle(), idx, 0, &opsys, NULL) == 0)
		{
			if (opsys == ZIP_OPSYS_DOS)
			{
				if (strchr(name, '\\') != nullptr)
				{
					m_foundMalformedWindowsPathSeparators = true;
					return true;
				}
				else if (strchr(name, '/') != nullptr)
				{
					m_foundMalformedWindowsPathSeparators = false;
					return false;
				}
				continue;
			}
			else
			{
				m_foundMalformedWindowsPathSeparators = false;
				return false;
			}
		}
	}

	m_foundMalformedWindowsPathSeparators = false;
	return false;
}

std::string WzMapZipIO::getZipLibraryVersionString()
{
	const char* verStr = zip_libzip_version();
	if (!verStr)
	{
		return "libzip/unknown";
	}
	return std::string("libzip/") + verStr;
}
