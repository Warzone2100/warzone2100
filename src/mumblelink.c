#include "mumblelink.h"
#include "lib/framework/frame.h"
#include "lib/ivis_common/pievector.h"
#include <wchar.h>

typedef struct
{
	uint32_t        uiVersion;
	int32_t         uiTick;
	Vector3f        position;
	Vector3f        forward;
	Vector3f        up;
	wchar_t         name[256];
} LinkedMem;

#if defined(WZ_OS_UNIX)
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#endif

#if defined(WZ_OS_WIN)
typedef HANDLE FileHandle;
#define EMPTY_FILEHANDLE NULL
#define MAP_FAILED NULL
#elif defined(WZ_OS_UNIX)
typedef int FileHandle;
#define EMPTY_FILEHANDLE -1
#endif

static const FileHandle EmptyFileHandle = EMPTY_FILEHANDLE;

static FileHandle SharedMem = EMPTY_FILEHANDLE;
static LinkedMem* lm = NULL;

bool InitMumbleLink()
{
	// When "doubly" initialising just say we're succesful
	if (lm != NULL)
		return true;

	// Open shared memory (should be created by Mumble)
	if (SharedMem == EmptyFileHandle)
	{
#if defined(WZ_OS_WIN)
		SharedMem = OpenFileMappingW(FILE_MAP_ALL_ACCESS, false, L"MumbleLink");
#elif defined(WZ_OS_UNIX)
		char* memname;
		sasprintf(&memname, "/MumbleLink.%d", (int)getuid());
		SharedMem = shm_open(memname, O_RDWR, S_IRUSR | S_IWUSR);
#endif
		if (SharedMem == EmptyFileHandle)
		{
#if defined(WZ_OS_WIN)
			debug(LOG_NEVER, "Failed to open a shared memory object: %d", GetLastError());
#elif defined(WZ_OS_UNIX)
			debug(LOG_NEVER, "Failed to open a shared memory object: %s", strerror(errno));
#endif
			return false;
		}
	}

	// Map a LinkedMem instance onto shared memory
	if (lm == NULL)
	{
#if defined(WZ_OS_WIN)
		lm = (LinkedMem *)MapViewOfFile(SharedMem, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(*lm));
#elif defined(WZ_OS_UNIX)
		lm = (LinkedMem *)mmap(NULL, sizeof(*lm), PROT_READ | PROT_WRITE, MAP_SHARED, SharedMem, 0);
#endif
		if (lm == MAP_FAILED)
		{
#if defined(WZ_OS_WIN)
			debug(LOG_ERROR, "Failed to map shared memory into local memory: %d", GetLastError());
#elif defined(WZ_OS_UNIX)
			debug(LOG_ERROR, "Failed to map shared memory into local memory: %s", strerror(errno));
			lm = NULL;
#endif
			return false;
		}
	}

	wcsncpy(lm->name, L"Warzone 2100", sizeof(lm->name));
	// Guarantee to NUL terminate
	lm->name[sizeof(lm->name) - 1] = L'\0';

	return true;
}
#if defined(WZ_OS_WIN)
#elif defined(WZ_OS_UNIX)
#endif

bool CloseMumbleLink()
{
	if (lm != NULL)
	{
#if defined(WZ_OS_WIN)
		const bool success = UnmapViewOfFile(lm);
#elif defined(WZ_OS_UNIX)
		const bool success = (munmap(lm, sizeof(*lm)) == 0);
#endif
		if (!success)
		{
#if defined(WZ_OS_WIN)
			debug(LOG_ERROR, "Failed to unmap shared memory out of local memory: %d", GetLastError());
#elif defined(WZ_OS_UNIX)
			debug(LOG_ERROR, "Failed to unmap shared memory out of local memory: %s", strerror(errno));
#endif
			return false;
		}

		lm = NULL;
	}

	if (SharedMem != EmptyFileHandle)
	{
#if defined(WZ_OS_WIN)
		const bool success = CloseHandle(SharedMem);
#elif defined(WZ_OS_UNIX)
		const bool success = (close(SharedMem) == 0);
#endif
		if (!success)
		{
#if defined(WZ_OS_WIN)
			debug(LOG_ERROR, "Failed to close a shared memory object: %d", GetLastError());
#elif defined(WZ_OS_UNIX)
			debug(LOG_ERROR, "Failed to close a shared memory object: %s", strerror(errno));
#endif
			return false;
		}

		SharedMem = EmptyFileHandle;
	}

	return true;
}

#if defined(WZ_OS_UNIX)
static const int32_t GetTickCount(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	const int32_t milliSeconds = tv.tv_usec / 1000 + tv.tv_sec * 1000;
	return milliSeconds;
}
#endif

bool UpdateMumbleSoundPos(const Vector3f pos, const Vector3f forward, const Vector3f up)
{
	if (lm == NULL)
	{
		return false;
	}

	lm->position = pos;
	lm->forward  = Vector3f_Normalise(forward);
	lm->up       = Vector3f_Normalise(up);

	// Protocol version of Mumble's "Link" plugin
	lm->uiVersion = 1;

	/* This is used to indicate that this data has been updated. Thus to
	 * avoid locking conflicts it must be the last change we do to this
	 * structure.
	 */
	lm->uiTick = GetTickCount();

	return true;
}
