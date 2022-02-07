// A fixed-up version of the PHYSFS_mountMemory function that properly calls the destructor func
//
// WZ Modifications:
//  - Fix calling the destructor func
//  - Some minor tweaks to use std::atomic, `new` for __PHYSFS_MemoryIoInfo2
//
// Copyright (c) 2021 Warzone 2100 Project
// Licensed under the same terms as the original license (below).
//
// Original License:
//
//	Copyright (c) 2001-2021 Ryan C. Gordon <icculus@icculus.org> and others.
//
//	This software is provided 'as-is', without any express or implied
//	warranty.  In no event will the authors be held liable for any damages
//	arising from the use of this software.
//
//	Permission is granted to anyone to use this software for any purpose,
//	including commercial applications, and to alter it and redistribute it
//	freely, subject to the following restrictions:
//
//	1. The origin of this software must not be misrepresented; you must not
//	   claim that you wrote the original software. If you use this software
//	   in a product, an acknowledgment in the product documentation would be
//	   appreciated but is not required.
//	2. Altered source versions must be plainly marked as such, and must not be
//	   misrepresented as being the original software.
//	3. This notice may not be removed or altered from any source distribution.
//

#include "physfs_memoryio.h"
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <atomic>

#if defined(__GNUC__)
# pragma GCC diagnostic ignored "-Wcast-qual"
#endif

#if defined(_MSC_VER) && !defined(__clang__)
# pragma warning (disable : 4127) // conditional expression is constant
#endif

#if defined(HAS_PHYSFS_IO_SUPPORT)

/* PHYSFS_Io implementation for i/o to a memory buffer... */

#define BAIL(e, r) do { if (e != 0) { PHYSFS_setErrorCode(e); } return r; } while (0)
#define BAIL_IF(c, e, r) do { if (c) { if (e != 0) { PHYSFS_setErrorCode(e); } return r; } } while (0)
#define GOTO_IF(c, e, g) do { if (c) { if (e != 0) { PHYSFS_setErrorCode(e); } goto g; } } while (0)
#define BAIL_IF_ERRPASS(c, r) do { if (c) { return r; } } while (0)

typedef struct __PHYSFS_MemoryIoInfo2
{
	const PHYSFS_uint8 *buf = nullptr;
	PHYSFS_uint64 len = 0;
	PHYSFS_uint64 pos = 0;
	PHYSFS_Io *parent = nullptr;
	std::atomic<int> refcount;
	void (*destruct)(void *) = nullptr;
} MemoryIoInfo2;

static PHYSFS_sint64 memoryIo2_read(PHYSFS_Io *io, void *buf, PHYSFS_uint64 len)
{
	MemoryIoInfo2 *info = (MemoryIoInfo2 *) io->opaque;
	const PHYSFS_uint64 avail = info->len - info->pos;
	assert(avail <= info->len);

	if (avail == 0)
		return 0;  /* we're at EOF; nothing to do. */

	if (len > avail)
		len = avail;

	memcpy(buf, info->buf + info->pos, (size_t) len);
	info->pos += len;
	return len;
} /* memoryIo2_read */

static PHYSFS_sint64 memoryIo2_write(PHYSFS_Io *io, const void *buffer,
									PHYSFS_uint64 len)
{
	BAIL(PHYSFS_ERR_OPEN_FOR_READING, -1);
} /* memoryIo2_write */

static int memoryIo2_seek(PHYSFS_Io *io, PHYSFS_uint64 offset)
{
	MemoryIoInfo2 *info = (MemoryIoInfo2 *) io->opaque;
	BAIL_IF(offset > info->len, PHYSFS_ERR_PAST_EOF, 0);
	info->pos = offset;
	return 1;
} /* memoryIo2_seek */

static PHYSFS_sint64 memoryIo2_tell(PHYSFS_Io *io)
{
	const MemoryIoInfo2 *info = (MemoryIoInfo2 *) io->opaque;
	return (PHYSFS_sint64) info->pos;
} /* memoryIo2_tell */

static PHYSFS_sint64 memoryIo2_length(PHYSFS_Io *io)
{
	const MemoryIoInfo2 *info = (MemoryIoInfo2 *) io->opaque;
	return (PHYSFS_sint64) info->len;
} /* memoryIo2_length */

static PHYSFS_Io *memoryIo2_duplicate(PHYSFS_Io *io)
{
	MemoryIoInfo2 *info = (MemoryIoInfo2 *) io->opaque;
	MemoryIoInfo2 *newinfo = NULL;
	PHYSFS_Io *parent = info->parent;
	PHYSFS_Io *retval = NULL;

	/* avoid deep copies. */
	assert((!parent) || (!((MemoryIoInfo2 *) parent->opaque)->parent) );

	/* share the buffer between duplicates. */
	if (parent != NULL)  /* dup the parent, increment its refcount. */
		return parent->duplicate(parent);

	/* we're the parent. */

	retval = (PHYSFS_Io *) malloc(sizeof (PHYSFS_Io));
	BAIL_IF(!retval, PHYSFS_ERR_OUT_OF_MEMORY, NULL);
	newinfo = new MemoryIoInfo2();
	if (!newinfo)
	{
		free(retval);
		BAIL(PHYSFS_ERR_OUT_OF_MEMORY, NULL);
	} /* if */

//	__PHYSFS_ATOMIC_INCR(&info->refcount);
	info->refcount.fetch_add(1);

//	memset(newinfo, '\0', sizeof (*info));
	newinfo->buf = info->buf;
	newinfo->len = info->len;
	newinfo->pos = 0;
	newinfo->parent = io;
	newinfo->refcount.store(0);
	newinfo->destruct = NULL;

	memcpy(retval, io, sizeof (*retval));
	retval->opaque = newinfo;
	return retval;
} /* memoryIo2_duplicate */

static int memoryIo2_flush(PHYSFS_Io *io) { return 1;  /* it's read-only. */ }

static void memoryIo2_destroy(PHYSFS_Io *io)
{
	MemoryIoInfo2 *info = (MemoryIoInfo2 *) io->opaque;
	PHYSFS_Io *parent = info->parent;

	if (parent != NULL)
	{
		assert(info->buf == ((MemoryIoInfo2 *) info->parent->opaque)->buf);
		assert(info->len == ((MemoryIoInfo2 *) info->parent->opaque)->len);
		assert(info->refcount.load() == 0);
		assert(info->destruct == NULL);
		delete info;
		free(io);
		parent->destroy(parent);  /* decrements refcount. */
		return;
	} /* if */

	/* we _are_ the parent. */
	assert(info->refcount.load() > 0);  /* even in a race, we hold a reference. */

//	if (__PHYSFS_ATOMIC_DECR(&info->refcount) == 0)
	if (info->refcount.fetch_sub(1) == 1)
	{
		void (*destruct)(void *) = info->destruct;
		void *buf = (void *) info->buf;
		io->opaque = NULL;  /* kill this here in case of race. */
		delete info;
		free(io);
		if (destruct != NULL)
			destruct(buf);
	} /* if */
} /* memoryIo2_destroy */


static const PHYSFS_Io __PHYSFS_memoryIoInterface2 =
{
	0, NULL,
	memoryIo2_read,
	memoryIo2_write,
	memoryIo2_seek,
	memoryIo2_tell,
	memoryIo2_length,
	memoryIo2_duplicate,
	memoryIo2_flush,
	memoryIo2_destroy
};

PHYSFS_Io *__PHYSFS_createMemoryIo2(const void *buf, PHYSFS_uint64 len,
								   void (*destruct)(void *))
{
	PHYSFS_Io *io = NULL;
	MemoryIoInfo2 *info = NULL;

	io = (PHYSFS_Io *) malloc(sizeof (PHYSFS_Io));
	GOTO_IF(!io, PHYSFS_ERR_OUT_OF_MEMORY, createMemoryIo_failed);
	info = new MemoryIoInfo2();
	GOTO_IF(!info, PHYSFS_ERR_OUT_OF_MEMORY, createMemoryIo_failed);

//	memset(info, '\0', sizeof (*info));
	info->buf = (const PHYSFS_uint8 *) buf;
	info->len = len;
	info->pos = 0;
	info->parent = NULL;
	info->refcount.store(1);
	info->destruct = destruct;

	memcpy(io, &__PHYSFS_memoryIoInterface2, sizeof (*io));
	io->opaque = info;
	return io;

createMemoryIo_failed:
	if (info != NULL) delete info;
	if (io != NULL) free(io);
	return NULL;
} /* __PHYSFS_createMemoryIo2 */

int PHYSFS_mountMemory_fixed(const void *buf, PHYSFS_uint64 len, void (*del)(void *),
					   const char *fname, const char *mountPoint,
					   int appendToPath)
{
	int retval = 0;
	PHYSFS_Io *io = NULL;

	BAIL_IF(!buf, PHYSFS_ERR_INVALID_ARGUMENT, 0);
	BAIL_IF(!fname, PHYSFS_ERR_INVALID_ARGUMENT, 0);

	io = __PHYSFS_createMemoryIo2(buf, len, del);
	BAIL_IF_ERRPASS(!io, 0);
	retval = PHYSFS_mountIo(io, fname, mountPoint, appendToPath);
	if (!retval)
	{
		/* docs say not to call (del) in case of failure, so cheat. */
		MemoryIoInfo2 *info = (MemoryIoInfo2 *) io->opaque;
		info->destruct = NULL;
		io->destroy(io);
	} /* if */

	return retval;
} /* PHYSFS_mountMemory_fixed */

#else // !defined(HAS_PHYSFS_IO_SUPPORT)

int PHYSFS_mountMemory_fixed(const void *buf, PHYSFS_uint64 len, void (*del)(void *),
					   const char *fname, const char *mountPoint,
					   int appendToPath)
{
	// unsupported
	return 0;
} /* PHYSFS_mountMemory_fixed */

#endif // defined(HAS_PHYSFS_IO_SUPPORT)
