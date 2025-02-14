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

#include "lib/framework/wzglobal.h"

#include "descriptor_set.h"

#ifdef WZ_OS_WIN
# include "lib/netplay/tcp/select_descriptor_set.h"
#else
# include "lib/netplay/tcp/poll_descriptor_set.h"
#endif

std::unique_ptr<IDescriptorSet> IDescriptorSet::create(PollEventType eventType)
{
	// For now, use `select()` on Windows instead of `poll()` because of a bug in
	// Windows versions prior to "Windows 10 2004", which can lead to `poll()`
	// function timing out on socket connection errors instead of returning an error early.
	//
	// For more information on the bug, see: https://stackoverflow.com/questions/21653003/is-this-wsapoll-bug-for-non-blocking-sockets-fixed
	// and also https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-wsapoll#remarks
	switch (eventType)
	{
	case PollEventType::READABLE:
	{
#ifdef WZ_OS_WIN
		IDescriptorSet* rawPtr = new tcp::SelectDescriptorSet<PollEventType::READABLE>();
		return std::unique_ptr<IDescriptorSet>(rawPtr);
#else
		IDescriptorSet* rawPtr = new tcp::PollDescriptorSet<PollEventType::READABLE>();
		return std::unique_ptr<IDescriptorSet>(rawPtr);
#endif
	}
	case PollEventType::WRITABLE:
	{
#ifdef WZ_OS_WIN
		IDescriptorSet* rawPtr = new tcp::SelectDescriptorSet<PollEventType::WRITABLE>();
		return std::unique_ptr<IDescriptorSet>(rawPtr);
#else
		IDescriptorSet* rawPtr = new tcp::PollDescriptorSet<PollEventType::WRITABLE>();
		return std::unique_ptr<IDescriptorSet>(rawPtr);
#endif
	}
	default:
		ASSERT(false, "Unexpected PollEventType value: %d", static_cast<int>(eventType));
		return nullptr;
	}
}
