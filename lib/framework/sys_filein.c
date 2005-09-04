//====================================================
//Many thanks to  Jaakko Keränen Doomsday project  for the routines!
//====================================================
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <stdlib.h> 
#include <malloc.h>


#include "sys_zipfile.h"

#define MAX_FILES	  5120					//1024 *5	hmm.. too much?

void    F_ResetDirec(void);

static WZFILE files[MAX_FILES];			//Guess this is enough for now, though we should check to make sure. -Q
//==============================================
#ifndef WIN32
#define false 0
#define true 1
//===========================
#include <unistd.h>
//============================
//==============================================
char   *_fullpath(char *full, const char *original, int maxLen)
{
	static char buf[PATH_MAX];

	getcwd(buf,sizeof(buf));
	strcat(buf,"/");
	debug(LOG_WZ, "full is %s", original);
	debug(LOG_WZ, "directory is %s", buf);
	strcat(buf,original);
	debug(LOG_WZ, "New directory is %s", buf);
	memcpy(full,buf,sizeof(char)*PATH_MAX);	//NO! fix this not 255 limit!
	return buf;
}
#endif
//===========================================================================
// F_CloseAll
//===========================================================================
void F_CloseAll(void)
{
	int     i;

	for(i = 0; i < MAX_FILES; i++)
		if(files[i].flags.open)
			F_Close(files + i);
}
//===========================================================================
//===========================================================================
// F_ShutdownDirec
//===========================================================================
void F_ShutdownDirec(void)
{
//	F_ResetDirec();
	F_CloseAll();
}

//===========================================================================
// F_Access
//  Returns true if the file can be opened for reading.
//===========================================================================
int F_Access(const char *path)
{
	WZFILE  *file = F_Open(path, "rx");// Open for reading, but don't buffer anything.

	if(!file)
		return FALSE;
	F_Close(file);
	return TRUE;
}
//===========================================================================
// F_GetFreeFile
//===========================================================================
WZFILE  *F_GetFreeFile(void)
{
	int     i;

	for(i = 0; i < MAX_FILES; i++)
		if(!files[i].flags.open)
		{
			memset(files + i, 0, sizeof(WZFILE));
			return files + i;
		}
	return NULL;
}
//===========================================================================
//* This only works on real files.
static unsigned int F_GetLastModified(const char *path)
{
#ifdef WIN32
	struct _stat s;
	_stat(path, &s);
	return s.st_mtime;
#else
	struct stat s;
	stat(path, &s);
	return s.st_mtime;
#endif
}
//===========================================================================
// F_OpenFile
//===========================================================================
WZFILE  *F_OpenFile(const char *path, const char *mymode)
{
	WZFILE  *file = F_GetFreeFile();
	char    mode[8];

	if(!file)		return NULL;

	strcpy(mode, "r");						// Open for reading.
	if(strchr(mymode, 't'))
		strcat(mode, "t");
	if(strchr(mymode, 'b'))
		strcat(mode, "b");

	file->data = fopen(path, mode);			// Try opening as a real file.
	if(!file->data)
		return NULL;			// Can't find the file.
	file->flags.open = TRUE;
	file->flags.file = TRUE;
	file->lastModified = F_GetLastModified(path);	
	return file;
}
//===========================================================================
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void   *M_Malloc(uint size)				//Was gonna add debugging, but no time, so we can drop these.
{
	return malloc(size);
}
void M_Free(void *ptr)
{
	free(ptr);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//===========================================================================
// F_OpenZip
//===========================================================================
WZFILE  *F_OpenZip(int zipIndex, BOOL dontBuffer)
{
	WZFILE  *file = F_GetFreeFile();

	if(!file)		return NULL;


	file->flags.open = TRUE;
	file->flags.file = FALSE;
	file->lastModified = Zip_GetLastModified(zipIndex);		// Init and load in the data.
	if(!dontBuffer)
	{
		file->size = Zip_GetSize(zipIndex);
		file->pos = file->data = M_Malloc(file->size);
		Zip_Read(zipIndex, file->data);
	}
	return file;
}
//===========================================================================
// F_Open
//===========================================================================
WZFILE *F_Open(const char *path, const char *mode)
{
	char full[PATH_MAX];
	BOOL dontBuffer;

	if(!mode)		mode = "";
	dontBuffer = (strchr(mode, 'x') != NULL);

	_fullpath(full, path, PATH_MAX);//trans

	if(!strchr(mode, 'f'))		// Doesn't need to be a real file?
	{
		int foundZip = Zip_Find(full);					// First check the Zip directory.
		if(foundZip)			return F_OpenZip(foundZip, dontBuffer);
	}
	return F_OpenFile(full, mode);						// Try to open as a real file, then.
}
//===========================================================================
// F_Close
//===========================================================================
void F_Close(WZFILE *file)
{
	if(!file->flags.open)		return;
	if(file->flags.file)
	{
		fclose(file->data);
	}
	else
	{	
	if(file->data)			M_Free(file->data);				// Free the stored data.
	}
	memset(file, 0, sizeof(*file));
}

//===========================================================================
// F_Read
//  Returns the number of bytes read (up to 'count').
//===========================================================================
int F_Read(void *dest, int count, WZFILE *file)
{
	int     bytesleft;

	if(!file->flags.open)		return 0;
	if(file->flags.file)		// Normal file?
	{
		count = fread(dest, 1, count, file->data);
		if(feof((FILE *) file->data))			file->flags.eof = TRUE;
		return count;
	}
	bytesleft = file->size - (file->pos - (char *) file->data);	// Is there enough room in the file?
	if(count > bytesleft)
	{
		count = bytesleft;
		file->flags.eof = TRUE;
	}
	if(count)
	{
		memcpy(dest, file->pos, count);
		file->pos += count;
	}
	return count;
}
//===========================================================================
//===========================================================================
// F_Tell
//===========================================================================
int F_Tell(WZFILE *file)
{
	if(!file->flags.open)		return 0;
	if(file->flags.file)		return ftell(file->data);
	return file->pos - (char *) file->data;
}
//===========================================================================
// F_Seek
//  Returns the current position in the file, before the move, as an offset
//  from the beginning of the file.
//===========================================================================
int F_Seek(WZFILE *file, int offset, int whence)
{
	int     oldpos = F_Tell(file);

	if(!file->flags.open)		return 0;
	file->flags.eof = FALSE;
	if(file->flags.file)		fseek(file->data, offset, whence);
	else
	{
		if(whence == SEEK_SET)			file->pos = (char *) file->data + offset;
		else if(whence == SEEK_END)			file->pos = (char *) file->data + (file->size + offset);
		else if(whence == SEEK_CUR)			file->pos += offset;
	}
	return oldpos;
}
//===========================================================================
// F_Rewind
//===========================================================================
void F_Rewind(WZFILE *file)
{
	F_Seek(file, 0, SEEK_SET);
}
//===========================================================================
// Returns the length of the file, in bytes.  Stream position is not affected.
int F_Length(WZFILE *file)
{
	int     length, currentPosition;

	if(!file)		return 0;

	currentPosition = F_Seek(file, 0, SEEK_END);
	length = F_Tell(file);
	F_Seek(file, currentPosition, SEEK_SET);
	return length;
}
//==========================================================================
// Returns the time when the file was last modified, as seconds sincethe Epoch.  Returns zero if the file is not found.
unsigned int F_LastModified(const char *fileName)
{
	WZFILE *file = F_Open(fileName, "rx");			// Try to open the file, but don't buffer any contents.
	unsigned modified = 0;

	if(!file)		return 0;
	modified = file->lastModified;
	F_Close(file);
	return modified;
}
//===========================================================================

