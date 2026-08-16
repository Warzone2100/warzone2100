// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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

#include "wzhashablefile.h"

#include "lib/framework/frame.h"
#include "lib/framework/wzstring.h"
#include "lib/framework/physfs_ext.h"

#include <LaunchInfo.h>
#include <sodium.h>

#include <cstring>
#include <vector>

#if defined(WZ_OS_WIN)
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#else
# include <fcntl.h>
# include <sys/stat.h>
# include <unistd.h>
# include <cerrno>
#endif

namespace
{

constexpr size_t HASH_CHUNK_SIZE = 1024 * 1024;

// Minimal read-only file handle, private to this translation unit.
class ReadOnlyFile
{
public:
	~ReadOnlyFile() { close(); }

	bool open(const std::string& utf8Path)
	{
#if defined(WZ_OS_WIN)
		std::vector<uint16_t> wPath = WzString::fromUtf8(utf8Path).toUtf16();
		wPath.push_back(0);
		handle = CreateFileW(reinterpret_cast<const wchar_t*>(wPath.data()), GENERIC_READ,
							 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
							 nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		return handle != INVALID_HANDLE_VALUE;
#else
		do
		{
			fd = ::open(utf8Path.c_str(), O_RDONLY | O_CLOEXEC);
		} while (fd == -1 && errno == EINTR);
		return fd != -1;
#endif
	}

	optional<uint64_t> size() const
	{
#if defined(WZ_OS_WIN)
		LARGE_INTEGER fileSize;
		if (!GetFileSizeEx(handle, &fileSize))
		{
			return nullopt;
		}
		return static_cast<uint64_t>(fileSize.QuadPart);
#else
		struct stat st;
		if (fstat(fd, &st) != 0)
		{
			return nullopt;
		}
		return static_cast<uint64_t>(st.st_size);
#endif
	}

	// Returns bytes read (0 on EOF), or nullopt on error
	optional<size_t> read(void* buffer, size_t maxLen)
	{
#if defined(WZ_OS_WIN)
		DWORD bytesRead = 0;
		if (!ReadFile(handle, buffer, static_cast<DWORD>(maxLen), &bytesRead, nullptr))
		{
			return nullopt;
		}
		return static_cast<size_t>(bytesRead);
#else
		ssize_t bytesRead;
		do
		{
			bytesRead = ::read(fd, buffer, maxLen);
		} while (bytesRead == -1 && errno == EINTR);
		if (bytesRead < 0)
		{
			return nullopt;
		}
		return static_cast<size_t>(bytesRead);
#endif
	}

	// Reads exactly len bytes at offset (without disturbing the sequential read position on POSIX)
	bool readAt(uint64_t offset, void* buffer, size_t len)
	{
#if defined(WZ_OS_WIN)
		OVERLAPPED overlapped;
		memset(&overlapped, 0, sizeof(overlapped));
		size_t totalRead = 0;
		while (totalRead < len)
		{
			uint64_t currOffset = offset + totalRead;
			overlapped.Offset = static_cast<DWORD>(currOffset & 0xFFFFFFFFull);
			overlapped.OffsetHigh = static_cast<DWORD>(currOffset >> 32);
			DWORD bytesRead = 0;
			if (!ReadFile(handle, static_cast<char*>(buffer) + totalRead, static_cast<DWORD>(len - totalRead), &bytesRead, &overlapped) || bytesRead == 0)
			{
				return false;
			}
			totalRead += bytesRead;
		}
		return true;
#else
		size_t totalRead = 0;
		while (totalRead < len)
		{
			ssize_t bytesRead = ::pread(fd, static_cast<char*>(buffer) + totalRead, len - totalRead, static_cast<off_t>(offset + totalRead));
			if (bytesRead == -1 && errno == EINTR)
			{
				continue;
			}
			if (bytesRead <= 0)
			{
				return false;
			}
			totalRead += static_cast<size_t>(bytesRead);
		}
		return true;
#endif
	}

	void close()
	{
#if defined(WZ_OS_WIN)
		if (handle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(handle);
			handle = INVALID_HANDLE_VALUE;
		}
#else
		if (fd != -1)
		{
			::close(fd);
			fd = -1;
		}
#endif
	}

private:
#if defined(WZ_OS_WIN)
	HANDLE handle = INVALID_HANDLE_VALUE;
#else
	int fd = -1;
#endif
};

bool isUsableImagePath(const std::string& path)
{
	// launchinfo uses "<not initialized>"-style placeholders on failure
	return !path.empty() && path.front() != '<';
}

} // anonymous namespace

optional<HashableFile> HashableFile::ownExecutable()
{
	std::string resolvedPath = LaunchInfo::getCurrentProcessDetails().imageFileName.fullPath();
	bool haveResolvedPath = isUsableImagePath(resolvedPath);
	bool useProcSelfExe = false;
#if defined(WZ_OS_LINUX)
	useProcSelfExe = (access("/proc/self/exe", R_OK) == 0);
#endif
	if (!haveResolvedPath && !useProcSelfExe)
	{
		return nullopt;
	}
	// launchinfo (whereami) only reports OS-provided paths, failing rather than guessing
	return HashableFile(haveResolvedPath ? std::move(resolvedPath) : std::string("/proc/self/exe"),
						useProcSelfExe, PathConfidence::KernelAuthoritative);
}

optional<HashableFile> HashableFile::mountedDataArchive(const std::string& realPath)
{
	bool isMounted = false;
	char** searchPath = PHYSFS_getSearchPath();
	for (char** i = searchPath; *i != nullptr; ++i)
	{
		if (realPath == *i)
		{
			isMounted = true;
			break;
		}
	}
	PHYSFS_freeList(searchPath);
	if (!isMounted)
	{
		return nullopt;
	}
	return HashableFile(realPath, false, PathConfidence::KernelAuthoritative);
}

optional<HashableFile::WholeFileHash> HashableFile::hashWholeFile(const std::atomic<bool>* stopFlag) const
{
	ReadOnlyFile file;
	if (!file.open(m_openViaProcSelfExe ? "/proc/self/exe" : m_displayPath))
	{
		return nullopt;
	}

	crypto_hash_sha256_state state;
	crypto_hash_sha256_init(&state);

	std::vector<unsigned char> buffer(HASH_CHUNK_SIZE);
	uint64_t totalBytes = 0;
	while (true)
	{
		if (stopFlag && stopFlag->load(std::memory_order_relaxed))
		{
			return nullopt;
		}
		optional<size_t> bytesRead = file.read(buffer.data(), buffer.size());
		if (!bytesRead.has_value())
		{
			return nullopt;
		}
		if (bytesRead.value() == 0)
		{
			break;
		}
		crypto_hash_sha256_update(&state, buffer.data(), bytesRead.value());
		totalBytes += bytesRead.value();
	}

	WholeFileHash result;
	static_assert(Sha256::Bytes == crypto_hash_sha256_BYTES, "Size mismatch");
	crypto_hash_sha256_final(&state, result.hash.bytes);
	result.fileSize = totalBytes;
	return result;
}

optional<std::array<uint8_t, wzkeyedhash::HASH_BYTES>> HashableFile::keyedHash(
	const std::vector<uint8_t>& nonce, const std::string& requestId, uint32_t targetIndex,
	const std::string& targetKind, const std::vector<wzkeyedhash::Range>& ranges,
	const std::atomic<bool>* stopFlag) const
{
	ReadOnlyFile file;
	if (!file.open(m_openViaProcSelfExe ? "/proc/self/exe" : m_displayPath))
	{
		return nullopt;
	}
	optional<uint64_t> fileSize = file.size();
	if (!fileSize.has_value())
	{
		return nullopt;
	}
	auto readAt = [&file, stopFlag](uint64_t offset, void* buffer, size_t len) -> bool {
		if (stopFlag && stopFlag->load(std::memory_order_relaxed))
		{
			return false;
		}
		return file.readAt(offset, buffer, len);
	};
	return wzkeyedhash::computeKeyedHash(nonce, requestId, targetIndex,
										 wzkeyedhash::targetDescriptor(targetKind, ranges),
										 ranges, fileSize.value(), readAt);
}

optional<std::vector<wzmachohash::SliceCanonicalHash>> HashableFile::machoCanonicalHashes(const std::atomic<bool>* stopFlag) const
{
	ReadOnlyFile file;
	if (!file.open(m_openViaProcSelfExe ? "/proc/self/exe" : m_displayPath))
	{
		return nullopt;
	}
	optional<uint64_t> fileSize = file.size();
	if (!fileSize.has_value())
	{
		return nullopt;
	}
	auto readAt = [&file, stopFlag](uint64_t offset, void* buffer, size_t len) -> bool {
		if (stopFlag && stopFlag->load(std::memory_order_relaxed))
		{
			return false;
		}
		return file.readAt(offset, buffer, len);
	};
	std::string errorDetails;
	auto result = wzmachohash::computeCanonicalHashes(readAt, fileSize.value(), errorDetails);
	if (!result.has_value())
	{
		return nullopt;
	}
	return result;
}
