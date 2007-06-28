#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include <dos.h>
#include <io.h>
#endif

#include "fbf.h"
#include "ivispatch.h"
#include "frame.h"

#define MAXBUFFERS			5
#define BUFFERSIZE			1024
#define CMASK						0377

//*************************************************************************

static struct {
	int	open;
	int	mode;
	int8 	*buffer;
	int8	*b;
	int 	n;
	FILE 	*fp;
	int 	buffersize;
} fbf[MAXBUFFERS];

//*************************************************************************
//*** (internal)
//* find free file descriptor block
//*
//* on exit:	fbf_getslot() >= 0  -> file block slot number
//*				 -1 -> all blocks are used up
//******
static int _fbf_getslot(void)
{
	int i;

	for (i = 0; i < MAXBUFFERS; i++) {
		if (!fbf[i].open) {
			return (i);
		}
	}

	return (-1);
}

//*************************************************************************
//*** open file for read or write (mutually exclusive)
//*
//* on entry:	fname = filename to open
//*					mode = FBF_MODE_ (read/write)
//*					buffsize >0 -> buffer size to use
//*			 		<0 -> use BUFFERSIZE (default)
//* on exit:		iV_FileOpen() >=0 -> file descriptor fd
//*			   		<0  -> error:
//*						iV_FBF_UNKNOWNMODE - R/W only
//*						iV_FBF_TOOMANYOPEN
//*						iV_FBF_OPENFAILED
//*						iV_FBF_OUTOFMEMORY - could not claim buffer
//******
int iV_FileOpen(STRING *fname, int mode, int buffersize)
{
	int s;
	FILE *fp;

	if ((s = _fbf_getslot()) == -1) {
		return (iV_FBF_TOOMANYOPEN);
	}

	switch (mode) {
		case iV_FBF_MODE_R:
			if ((fp = fopen(fname,"rb")) == NULL)
				return (iV_FBF_OPENFAILED);
			break;

		case iV_FBF_MODE_W:
			if ((fp = fopen(fname,"wb")) == NULL)
				return (iV_FBF_OPENFAILED);
			break;

		default:
			return (iV_FBF_UNKNOWNMODE);
	}

	if (buffersize < 0) {
		buffersize = BUFFERSIZE;
	}

	if ((fbf[s].buffer = ((int8 *) iV_HeapAlloc(buffersize))) == NULL) {
		fclose(fp);
		return (iV_FBF_OUTOFMEMORY);
	}

	fbf[s].open = 1;
	fbf[s].fp = fp;
	fbf[s].b = fbf[s].buffer;
	fbf[s].n = 0;
	fbf[s].mode = mode;
	fbf[s].buffersize = buffersize;

	return (s);
}


//*************************************************************************
//*** read one char from file
//*
//* pre:		iV_FileOpen(fd,iV_FBF_MODE_R,...)
//*
//* on entry:	fd = file descriptor
//*
//* on exit:	iV_FileGet() = char || EOF
//******
int iV_FileGet(int fd)
{
	if (fbf[fd].n == 0) {
		fbf[fd].n = fread(fbf[fd].buffer,sizeof(int8),fbf[fd].buffersize,fbf[fd].fp);
		fbf[fd].b = fbf[fd].buffer;
	}

	return (( --fbf[fd].n>= 0) ? (*fbf[fd].b++ & CMASK) : EOF);
}

//*************************************************************************
//*** close an open file
//*
//* pre:		iV_FileOpen(fd,...,...)
//*
//* on entry:	fd = file descriptor
//******
void iV_FileClose(int fd)
{
	if (fbf[fd].open) {
		if ((fbf[fd].mode == iV_FBF_MODE_W) && (fbf[fd].n > 0)) {
			fwrite(fbf[fd].buffer,sizeof(int8),fbf[fd].n,fbf[fd].fp);
		}

		if (fbf[fd].buffer) {
			iV_HeapFree(fbf[fd].buffer,fbf[fd].buffersize);
			fbf[fd].buffer = NULL;
		}

		if (fbf[fd].fp) {
			fclose(fbf[fd].fp);
		}

		fbf[fd].open = 0;
	}
}

//*************************************************************************
//*** write one char to file
//*
//* pre:			iV_FileOpen(fd,iV_FBF_MODE_W,...)
//*
//* on entry:	fd = file descriptor
//*	 			c  = char to write
//*
//* on exit:	iV_FilePut() =
//******
int iV_FilePut(int fd, int8 c)
{
	int i = 1;

	if (fbf[fd].n == fbf[fd].buffersize) {
		i = fwrite(fbf[fd].buffer, sizeof(int8), fbf[fd].buffersize, 
		           fbf[fd].fp);
		fbf[fd].n = 0;
		fbf[fd].b = fbf[fd].buffer;
	}

	*fbf[fd].b++ = c;
	fbf[fd].n++;

	return (i);
}

//*************************************************************************
//*** seek to absolute file position
//*
//* pre:			iV_FileOpen(fd,iV_FBF_MODE_R,...)
//*
//* on entry:	fd 		= file descriptor
//*	 			where  	= where in the file to seek to
//*				seek		= seek mode (iV_FBF_SEEK_SET, iV_FBF_SEEK_END)
//*
//* on exit:	iV_FileSeek == -1 -> error
//******
int iV_FileSeek(int fd, int where, int seek)
{
	fbf[fd].n = 0;

	return (fseek(fbf[fd].fp,where,seek));
}

//*************************************************************************
//*** find size of an open file
//*
//* pre:			iV_FileOpen(fd,iV_FBF_MODE_R,...)
//*
//* on entry:	fd	= file descriptor
//*
//* returns		size of file or -1 if error
//******
int32 iV_FileSizeOpen(int fd)
{
	int32 pos, size;
	FILE *fp;

	size = -1;

	if (fbf[fd].open) {
		fp = fbf[fd].fp;
		if (fp) {
			pos = ftell(fp);
			fseek(fp,0,SEEK_END);
			size = ftell(fp);
			fseek(fp,pos,SEEK_SET);
		}
	}

	return (size);
}

//*************************************************************************
//*** find size of a file (opens file, get size, close)
//*
//*
//* on entry:	fd	= file descriptor
//*
//* returns		size of file or -1 if error
//******
int32 iV_FileSize(STRING *filename)
{
	int fd;
	int32 size;

	size = -1;

	if ((fd = iV_FileOpen(filename, iV_FBF_MODE_R, iV_FBF_DEFAULT_BUFFER)) < 0) {
		return -1;
	}

	size = iV_FileSizeOpen(fd);

	iV_FileClose(fd);

	return (size);
}

//*************************************************************************
//*** save file
//*
//* on entry:	filename = file name to save
//*				data		= pointer to data to save
//*				size		= size of data (bytes)
//*
//* returns		TRUE if saved ok else FALSE
//******
iBool iV_FileSave(STRING *filename, uint8 *data, int32 size)
{
	FILE *fp;

	if ((fp = fopen(filename,"wb")) == NULL) {
		return FALSE;
	}

	fwrite(data,sizeof(uint8),size,fp);

	fclose(fp);

	return TRUE;
}

//*************************************************************************
//*** load file
//*
//* on entry:	filename = file name to load
//*				data		= pointer to data to load to
//*
//* returns		TRUE if loaded ok else FALSE
//******
iBool iV_FileLoad(STRING *filename, uint8 *data)
{
	FILE *fp;
	int32 size;

	size = iV_FileSize(filename);

	if ((fp = fopen(filename,"rb")) == NULL) {
		return FALSE;
	}

	if ((int32)fread(data,sizeof(uint8),size,fp) != size) {
		fclose(fp);
		return FALSE;
	}

	return TRUE;
}
