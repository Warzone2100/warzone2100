#ifndef _fbf_
#define _fbf_

#include <stdio.h>
#include "ivisdef.h"

#define iV_FBF_OPENFAILED		-1
#define iV_FBF_TOOMANYOPEN		-2
#define iV_FBF_OUTOFMEMORY		-3
#define iV_FBF_UNKNOWNMODE		-4
#define iV_FBF_OK					0
#define iV_FBF_MODE_R			0
#define iV_FBF_MODE_W			1
#define iV_FBF_MODE_WR			2
#define iV_FBF_MODE_CR			3
#define iV_FBF_SEEK_SET			SEEK_SET
#define iV_FBF_SEEK_END			SEEK_END
#define iV_FBF_DEFAULT_BUFFER	-1

//*************************************************************************

extern int iV_FileOpen(char *fname, int mode, int buffersize);
extern int iV_FileGet(int fd);
extern void iV_FileClose(int fd);
extern int iV_FilePut(int fd, int8 c);
extern int iV_FileSeek(int fd, int where, int seek);
extern int32 iV_FileSize(char *filename);
extern int32 iV_FileSizeOpen(int fd);
extern iBool iV_FileLoad(char *filename, uint8 *data);
extern iBool iV_FileSave(char *filename, uint8 *data, int32 size);

#endif
