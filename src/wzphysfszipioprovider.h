// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2025  Warzone 2100 Project

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

#include "lib/wzmaplib/plugins/ZipIOProvider/include/ZipIOProvider.h"
#include "lib/framework/physfs_ext.h"

class WzZipIOPHYSFSSourceReadProvider : public WzZipIOSourceReadProvider
{
protected:
	WzZipIOPHYSFSSourceReadProvider();
public:
	static std::shared_ptr<WzZipIOPHYSFSSourceReadProvider> make(const std::string& path);

	virtual ~WzZipIOPHYSFSSourceReadProvider();

	virtual optional<uint64_t> tell() override;
	virtual bool seek(uint64_t pos) override;
	virtual optional<uint64_t> readBytes(void *buffer, uint64_t len) override;
	virtual optional<uint64_t> fileSize() override;
	virtual optional<uint64_t> modTime() override;
private:
	PHYSFS_File *fp = nullptr;
	uint64_t fileLength = 0;
	PHYSFS_Stat metaData;
};
