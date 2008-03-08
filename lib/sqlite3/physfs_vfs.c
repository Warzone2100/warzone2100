/*
	Copyright (c) 2008, Giel van Schijndel
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are
	met:

	    * Redistributions of source code must retain the above copyright
	      notice, this list of conditions and the following disclaimer.
	    * Redistributions in binary form must reproduce the above copyright
	      notice, this list of conditions and the following disclaimer in
	      the documentation and/or other materials provided with the
	      distribution.
	    * Neither the name of the Warzone Resurrection Project nor the names
	      of its contributors may be used to endorse or promote products
	      derived from this software without specific prior written
	      permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
	A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
	OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
	TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
	PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
	LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
	NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "lib/framework/frame.h"
#include "lib/framework/strlfuncs.h"
#include "physfs_vfs.h"
#include "sqlite3.h"
#include <physfs.h>
#include <assert.h>

typedef struct physfs_sqlite3_file physfs_sqlite3_file;

/** "Subclassed" sqlite3_file for this PhysicsFS VFS.
 */
struct physfs_sqlite3_file
{
	sqlite3_file SQLite3_file;
	PHYSFS_file* file;
	bool NOOP;                      /**< Flag which indicates whether all write operations should be no-ops (and reads should fail) */
};

/** Closes the given file and deallocates any associated resources.
 *  \param f the physfs_sqlite3_file to close
 *  \return SQLITE_OK on success.
 */
static int xClose(sqlite3_file* f)
{
	physfs_sqlite3_file * const file = (physfs_sqlite3_file * const)file;
	assert(&file->SQLite3_file == f);

	if (file->NOOP)
		return SQLITE_OK;

	PHYSFS_close(file->file);

	return SQLITE_OK;
}

/** Reads bytes from the file into the given output buffer.
 *  \param f        the physfs_sqlite3_file to read from.
 *  \param[out] dst the memory buffer to write to.
 *  \param iAmt     the amount of bytes to read from the file.
 *  \param iOfst    the (absolute) offset into the file to start reading from.
 *  \return SQLITE_OK on success, SQLITE_IOERR_SHORT_READ if we couldn't read
 *          the full amount of \c iAmt bytes because EOF was reached, or
 *          SQLITE_IOERR_READ for any other error.
 */
static int xRead(sqlite3_file* f, void* dst, int iAmt, sqlite3_int64 iOfst)
{
	PHYSFS_sint64 nRead;

	physfs_sqlite3_file * const file = (physfs_sqlite3_file * const)file;
	assert(&file->SQLite3_file == f);

	if (file->NOOP)
		return SQLITE_IOERR;

	/* Seek to the given offset */
	if (!PHYSFS_seek(file->file, iOfst))
		return SQLITE_IOERR_READ;

	nRead = PHYSFS_read(file->file, dst, 1, iAmt);

	if (nRead < 0)
		return SQLITE_IOERR_READ;
	else if (nRead < iAmt)
		return SQLITE_IOERR_SHORT_READ;
	else
		return SQLITE_OK;
}

/** Dummy write implementation (doesn't do anything at all).
 *  \return SQLITE_IOERR_WRITE
 */
static int xWrite(sqlite3_file* f, const void* src, int iAmt, sqlite3_int64 iOfst)
{
	physfs_sqlite3_file * const file = (physfs_sqlite3_file * const)file;
	assert(&file->SQLite3_file == f);

	if (file->NOOP)
		return SQLITE_OK;

	return SQLITE_IOERR_WRITE;
}

/** Dummy truncate implementation (doesn't do anything at all).
 *  \return SQLITE_IOERR_TRUNCATE
 */
static int xTruncate(sqlite3_file* f, sqlite3_int64 size)
{
	physfs_sqlite3_file * const file = (physfs_sqlite3_file * const)file;
	assert(&file->SQLite3_file == f);

	if (file->NOOP)
		return SQLITE_OK;

	return SQLITE_IOERR_TRUNCATE;
}

/** Dummy sync implementation (doesn't do anything at all).
 *  \return SQLITE_IOERR_FSYNC
 */
static int xSync(sqlite3_file* f, int flags)
{
	physfs_sqlite3_file * const file = (physfs_sqlite3_file * const)file;
	assert(&file->SQLite3_file == f);

	if (file->NOOP)
		return SQLITE_OK;

	return SQLITE_IOERR_FSYNC;
}

/** Determines the size of the given file.
 *  \param f          the physfs_sqlite3_file to determine the size of.
 *  \param[out] pSize to write the size of this file to.
 *  \return 
 */
static int xFileSize(sqlite3_file* f, sqlite3_int64* pSize)
{
	PHYSFS_sint64 size;

	physfs_sqlite3_file * const file = (physfs_sqlite3_file * const)file;
	assert(&file->SQLite3_file == f);

	if (file->NOOP)
		return SQLITE_IOERR;

	size = PHYSFS_fileLength(file->file);

	if (size < 0)
		return SQLITE_IOERR_FSTAT;

	*pSize = size;

	return SQLITE_OK;
}

static int xLock(sqlite3_file* f, int level)
{
	switch (level)
	{
		case SQLITE_LOCK_NONE:
		case SQLITE_LOCK_SHARED:
			return SQLITE_OK;

		case SQLITE_LOCK_RESERVED:
		case SQLITE_LOCK_PENDING:
		case SQLITE_LOCK_EXCLUSIVE:
		default:
			return SQLITE_IOERR_RDLOCK;
	}
}

static int xUnlock(sqlite3_file* f, int level)
{
	return SQLITE_OK;
}

static int xCheckReservedLock(sqlite3_file* f)
{
	return 0;
}

static int xFileControl(sqlite3_file* f, int op, void *pArg)
{
	return 0;
}

static int xSectorSize(sqlite3_file* f)
{
	return 512;
}

static int xDeviceCharacteristics(sqlite3_file* f)
{
	return 0;
}

static const sqlite3_io_methods physfs_sqlite3_io_methods =
{
	1,                      /**< iVersion */
	xClose,                 /**< Closes the physfs_sqlite3_file */
	xRead,                  /**< Reads from the physfs_sqlite3_file */
	xWrite,                 /**< Intended to write from the physfs_sqlite3_file */
	xTruncate,              /**< Intended to truncate the physfs_sqlite3_file */
	xSync,                  /**< Intended to flush data out of OS cache into non-volatile memory */
	xFileSize,              /**< Deterimines the physfs_sqlite3_file's size */
	xLock,                  /**< Acquires a lock on the physfs_sqlite3_file */
	xUnlock,                /**< Releases a lock on the physfs_sqlite3_file */
	xCheckReservedLock,     /**< Checks whether a "reserved" (or higher) level lock is acquired on this physfs_sqlite3_file */
	xFileControl,           /**< Intended to execute the given opcode */
	xSectorSize,            /**< Determines the size of sectors for the underlying filesystem */
	xDeviceCharacteristics, /**< Returns a bitmask describing the underlying filesystem's characteristics */
};

static int xOpen(sqlite3_vfs* pVfs, const char* zName, sqlite3_file* f, int flags, int* pOutFlags)
{
	physfs_sqlite3_file * const file = (physfs_sqlite3_file * const)file;
	assert(&file->SQLite3_file == f);

	/* Assign the "overloaded" I/O functions to this file object */
	file->SQLite3_file.pMethods = &physfs_sqlite3_io_methods;

	/* We don't support journals in this implementation */
	if (flags & SQLITE_OPEN_MAIN_JOURNAL
	 || flags & SQLITE_OPEN_TEMP_JOURNAL
	 || flags & SQLITE_OPEN_MASTER_JOURNAL
	 || flags & SQLITE_OPEN_SUBJOURNAL)
	{
		file->file = 0;
		file->NOOP = true;
	}
	else /* if (flags & SQLITE_OPEN_MAIN_DB
	      || flags & SQLITE_OPEN_TEMP_DB
	      || flags & SQLITE_OPEN_TRANSIENT_DB) */
	{
		file->file = PHYSFS_openRead(zName);
		file->NOOP = false;

		if (!file->file)
			return SQLITE_IOERR;
	}

	*pOutFlags = SQLITE_OPEN_READONLY;

	return SQLITE_OK;
}

static int xDelete(sqlite3_vfs* pVfs, const char* zName, int syncDir)
{
	if (PHYSFS_delete(zName))
		return SQLITE_OK;
	else
		return SQLITE_IOERR_DELETE;
}

static int xAccess(sqlite3_vfs* pVfs, const char* zName, int flags)
{
	switch (flags)
	{
		case SQLITE_ACCESS_EXISTS:
			return PHYSFS_exists(zName);

		case SQLITE_ACCESS_READ:
		{
			PHYSFS_file* f = PHYSFS_openRead(zName);
			if (!f)
				return false;

			PHYSFS_close(f);
			return true;
		}

		case SQLITE_ACCESS_READWRITE:
			return false;

		default:
			return false;
	}
}

static int xGetTempname(sqlite3_vfs* pVfs, int nOut, char* zOut)
{
	return SQLITE_IOERR;
}

/** \return non-zero when no truncation occurred, zero otherwise.
 */
static int xFullPathname(sqlite3_vfs* pVfs, const char* zName, int nOut, char* zOut)
{
	return (strlcpy(zOut, zName, nOut) < nOut);
}

static void* xDlOpen(sqlite3_vfs* pVfs, const char* zFilename)
{
	return 0;
}

static void xDlError(sqlite3_vfs* pVfs, int nByte, char* zErrMsg)
{
	strlcpy(zErrMsg, "DlOpen and DlSym API isn't supported for this (PhysicsFS \"physfs\") VFS.", nByte);
}

static void *xDlSym(sqlite3_vfs* pVfs, void* dl, const char* zSymbol)
{
	return 0;
}

static void xDlClose(sqlite3_vfs* pVfs, void* dl)
{
}

static int xRandomness(sqlite3_vfs* pVfs, int nByte, char* zOut)
{
	return SQLITE_IOERR;
}

static int xSleep(sqlite3_vfs* pVfs, int microseconds)
{
	return SQLITE_IOERR;
}

static int xCurrentTime(sqlite3_vfs* pVfs, double* curTime)
{
	return SQLITE_IOERR;
}

static sqlite3_vfs physfs_sqlite3_vfs =
{
	1,                              /**< Structure version number */
	sizeof(physfs_sqlite3_vfs),     /**< Size of subclassed sqlite3_file */
	256,                            /**< Maximum file pathname length */
	0,                              /**< Next registered VFS (managed by SQLite) */
	"physfs",                       /**< Name of this virtual file system */
	0,                              /**< Pointer to application-specific data */
	xOpen,
	xDelete,
	xAccess,
	xGetTempname,
	xFullPathname,
	xDlOpen,
	xDlError,
	xDlSym,
	xDlClose,
	xRandomness,
	xSleep,
	xCurrentTime,
};
