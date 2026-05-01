#ifndef WZ_IOS_LIBPROC_SHIM_H
#define WZ_IOS_LIBPROC_SHIM_H

#include <stddef.h>
#include <sys/types.h>

#define PROC_PIDPATHINFO_MAXSIZE 4096

static inline int proc_pidpath(pid_t pid, void *buffer, unsigned int buffersize)
{
	(void)pid;
	if (buffer != NULL && buffersize > 0)
	{
		((char *)buffer)[0] = '\0';
	}
	return 0;
}

#endif // WZ_IOS_LIBPROC_SHIM_H
