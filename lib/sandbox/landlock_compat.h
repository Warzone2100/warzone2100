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

/// <linux/landlock.h>, plus the pieces a given copy of it may be too old to have, and the syscall wrappers
/// glibc does not provide.

#pragma once

#include <cstdint>
#include <sys/syscall.h>
#include <unistd.h>

#if defined(__has_include)
# if __has_include(<linux/landlock.h>)
#  include <linux/landlock.h>
# endif
#endif

// A build host from before Landlock existed
#if !defined(LANDLOCK_ACCESS_FS_EXECUTE)
# define LANDLOCK_ACCESS_FS_EXECUTE     (1ULL << 0)
# define LANDLOCK_ACCESS_FS_WRITE_FILE  (1ULL << 1)
# define LANDLOCK_ACCESS_FS_READ_FILE   (1ULL << 2)
# define LANDLOCK_ACCESS_FS_READ_DIR    (1ULL << 3)
# define LANDLOCK_ACCESS_FS_REMOVE_DIR  (1ULL << 4)
# define LANDLOCK_ACCESS_FS_REMOVE_FILE (1ULL << 5)
# define LANDLOCK_ACCESS_FS_MAKE_CHAR   (1ULL << 6)
# define LANDLOCK_ACCESS_FS_MAKE_DIR    (1ULL << 7)
# define LANDLOCK_ACCESS_FS_MAKE_REG    (1ULL << 8)
# define LANDLOCK_ACCESS_FS_MAKE_SOCK   (1ULL << 9)
# define LANDLOCK_ACCESS_FS_MAKE_FIFO   (1ULL << 10)
# define LANDLOCK_ACCESS_FS_MAKE_BLOCK  (1ULL << 11)
# define LANDLOCK_ACCESS_FS_MAKE_SYM    (1ULL << 12)
# define LANDLOCK_CREATE_RULESET_VERSION (1U << 0)
# define LANDLOCK_RULE_PATH_BENEATH 1
struct landlock_path_beneath_attr
{
	uint64_t allowed_access;
	int32_t parent_fd;
} __attribute__((packed));
#endif

// Rights and flags from an ABI later than the header
#if !defined(LANDLOCK_ACCESS_FS_REFER)
# define LANDLOCK_ACCESS_FS_REFER (1ULL << 13)
#endif
#if !defined(LANDLOCK_ACCESS_FS_TRUNCATE)
# define LANDLOCK_ACCESS_FS_TRUNCATE (1ULL << 14)
#endif
#if !defined(LANDLOCK_RESTRICT_SELF_TSYNC)
# define LANDLOCK_RESTRICT_SELF_TSYNC (1U << 3)
#endif

// The same numbers on every architecture
#if !defined(__NR_landlock_create_ruleset)
# define __NR_landlock_create_ruleset 444
#endif
#if !defined(__NR_landlock_add_rule)
# define __NR_landlock_add_rule 445
#endif
#if !defined(__NR_landlock_restrict_self)
# define __NR_landlock_restrict_self 446
#endif

namespace wz::sandbox::landlock
{

/// Carries only the rights member, fixing the size at what landlock_create_ruleset() has always accepted.
struct RulesetAttr
{
	uint64_t handledAccessFs = 0;
};

inline int createRuleset(const RulesetAttr* attr, size_t size, uint32_t flags)
{
	return static_cast<int>(syscall(__NR_landlock_create_ruleset, attr, size, flags));
}

inline int addRule(int rulesetFd, int ruleType, const void* attr, uint32_t flags)
{
	return static_cast<int>(syscall(__NR_landlock_add_rule, rulesetFd, ruleType, attr, flags));
}

inline int restrictSelf(int rulesetFd, uint32_t flags)
{
	return static_cast<int>(syscall(__NR_landlock_restrict_self, rulesetFd, flags));
}

} // namespace wz::sandbox::landlock
