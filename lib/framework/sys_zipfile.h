//*****************************************************************************
// ********  Many thanks goes out to Jaakko Keränen for all his hard work for his routines!
//*****************************************************************************

#ifndef __ZIPFILE_IO_H__
#define __ZIPFILE_IO_H__

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#else
#include "frame.h"
//#include <types.h>
#endif


#ifdef WIN32
typedef unsigned int	uint;
typedef unsigned short	ushort;
#define DIR_SEP_CHAR 	'\\'						// fix these for linux
#define DIR_SEP_STR		"\\"					
#define DIR_WRONG_SEP_CHAR 	'/'					//.....
#define PATH_MAX	256			//max path length
#else											//for linux
#define DIR_SEP_CHAR 		'/'
#define DIR_SEP_STR		"/"
#define DIR_WRONG_SEP_CHAR 	'\\'
#define PATH_MAX 256
#endif

#include <stdio.h>

#define deof(file) ((file)->flags.eof != 0)

typedef struct {
	struct DFILE_flags_s {
		unsigned char   open:1;
		unsigned char   file:1;
		unsigned char   eof:1;
	} flags;
	int             size;
	void           *data;
	char           *pos;
	unsigned int    lastModified;
} WZFILE;

//typedef enum filetype_e {
//	FT_NORMAL,
//	FT_DIRECTORY
//} filetype_t;

// typedef int     (*f_forall_func_t) (const char *fn, filetype_t type,void *parm);

void            	F_ShutdownDirec(void);
int             	F_Access(const char *path);
WZFILE          *F_Open(const char *path, const char *mode);
WZFILE          *F_OpenZip(int zipIndex, BOOL dontBuffer);
void            	F_Close(WZFILE * file);
int             	F_Length(WZFILE * file);
int             	F_Read(void *dest, int count, WZFILE * file);
int             	F_Tell(WZFILE * file);
int             	F_Seek(WZFILE * file, int offset, int whence);
void            	F_Rewind(WZFILE * file);
unsigned int    F_LastModified(const char *fileName);
//int             	F_ForAll(const char *filespec, void *parm, f_forall_func_t func);
//int             	F_GetC(WZFILE * file);
//void            F_InitDirec(void);

// Zip entry indices are invalidated when a new Zip file is read.
void 		Zip_Init(void);
void          	Zip_Shutdown(void);
BOOL        	Zip_Open(const char *fileName, WZFILE * prevOpened);
int		Zip_Find(const char *fileName);
int		Zip_Find_MP(const char *fileName);
void	Zip_Find_MPmaps(const char *fileName);
int		Zip_Iterate(int (*iterator) (const char *, void *),	void *parm);
uint		Zip_GetSize(int index);
uint		Zip_Read(int index, void *buffer);
uint		Zip_GetLastModified(int index);



#endif
