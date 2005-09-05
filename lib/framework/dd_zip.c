//**************************************************************************************
// Many thanks goes out to Jaakko Keränen for all his hard work for his routines.
//**************************************************************************************

// HEADER FILES ------------------------------------------------------------

#include <stdlib.h>
#include <search.h>
#include <malloc.h>
#include <zlib.h>

#include "frame.h"

#include "sys_zipfile.h"

//#define unsigned int USHORT
//from dd_share.h

#ifdef __BIG_ENDIAN__
	short           ShortSwap(short);
	long            LongSwap(long);
    float           FloatSwap(float);
#define SHORT(x)	ShortSwap(x)
#define LONG(x)		LongSwap(x)
#define FLOAT(x)    FloatSwap(x)
#else
#define SHORT(x)	(x)
#define LONG(x)		(x)
#define FLOAT(x)    (x)
#endif

#define USHORT(x)   ((unsigned short) SHORT(x))
#define ULONG(x)    ((unsigned long) LONG(x))

#define MAX_OF(x, y)	((x) > (y)? (x) : (y))
#define MIN_OF(x, y)	((x) < (y)? (x) : (y))
// MACROS ------------------------------------------------------------------

#define SIG_LOCAL_FILE_HEADER	0x04034b50
#define SIG_CENTRAL_FILE_HEADER	0x02014b50
#define SIG_END_OF_CENTRAL_DIR	0x06054b50

// Maximum tolerated size of the comment.
#define	MAXIMUM_COMMENT_SIZE	2048

// This is the length of the central directory end record (without the
// comment, but with the signature).
#define CENTRAL_END_SIZE		22

// File header flags.
#define ZFH_ENCRYPTED			0x1
#define ZFH_COMPRESSION_OPTS	0x6
#define ZFH_DESCRIPTOR			0x8
#define ZFH_COMPRESS_PATCHED	0x20	// Not supported.

// Compression methods.
enum {
	ZFC_NO_COMPRESSION = 0,		// Supported format.
	ZFC_SHRUNK,
	ZFC_REDUCED_1,
	ZFC_REDUCED_2,
	ZFC_REDUCED_3,
	ZFC_REDUCED_4,
	ZFC_IMPLODED,
	ZFC_DEFLATED = 8,           // The only supported compression (via zlib).
	ZFC_DEFLATED_64,
	ZFC_PKWARE_DCL_IMPLODED
};

// TYPES -------------------------------------------------------------------

typedef struct package_s {
	struct package_s *next;
	char    name[256];
	WZFILE  *file;
	int     order;
} package_t;

typedef struct zipentry_s {
	char   *name;				// Relative path (from the base path).
	package_t *package;
	unsigned int offset;		// Offset from the beginning of the package.
	unsigned int size;
    unsigned int deflatedSize;  /* Size of the original deflated entry.
                                   If the entry is not compressed, this
                                   is set to zero. */
	unsigned int lastModified;  // Unix timestamp.
	int priority;			//priority # of the file
	char priorityname[8];	// good for 9,999,999 versions.
} zipentry_t;

#pragma pack(1)
typedef struct localfileheader_s {
	uint    signature;
	ushort  requiredVersion;
	ushort  flags;
	ushort  compression;
	ushort  lastModTime;
	ushort  lastModDate;
	uint    crc32;
	uint    compressedSize;
	uint    size;
	ushort  fileNameSize;
	ushort  extraFieldSize;
} localfileheader_t;

typedef struct descriptor_s {
	uint    crc32;
	uint    compressedSize;
	uint    size;
} descriptor_t;

typedef struct centralfileheader_s {
	uint    signature;
	ushort  version;
	ushort  requiredVersion;
	ushort  flags;
	ushort  compression;
	ushort  lastModTime;
	ushort  lastModDate;
	uint    crc32;
	uint    compressedSize;
	uint    size;
	ushort  fileNameSize;
	ushort  extraFieldSize;
	ushort  commentSize;
	ushort  diskStart;
	ushort  internalAttrib;
	uint    externalAttrib;
	uint    relOffset;

	/*  
	 * file name (variable size)
	 * extra field (variable size)
	 * file comment (variable size)
	 */
} centralfileheader_t;

typedef struct centralend_s {
	ushort  disk;
	ushort  centralStartDisk;
	ushort  diskEntryCount;
	ushort  totalEntryCount;
	uint    size;
	uint    offset;
	ushort  commentSize;
} centralend_t;

#pragma pack()

void    Zip_RemoveDuplicateFiles(void);

static package_t *zipRoot;
static zipentry_t *zipFiles;
static int zipNumFiles, zipAllocatedFiles;
//=======================================
void Dir_FixSlashes(char *path)
{
	int     i, len = strlen(path);

	for(i = 0; i < len; i++)
	{
		if(path[i] == DIR_WRONG_SEP_CHAR)			// NOTE!  Will have to check if this is correct for linux! -Q
			path[i] = DIR_SEP_CHAR;
	}
}
//==========================================================
//  Initializes the zip file database.
void Zip_Init(void)
{
	zipRoot = NULL;
	zipFiles = 0;
	zipNumFiles = 0;
	zipAllocatedFiles = 0;
}
//==========================================================
// Shuts down the zip file database and frees all resources.
void Zip_Shutdown(void)
{
	package_t *pack, *next;
	int     i;

	for(pack = zipRoot; pack; pack = next)			// Close package files and free the nodes.
	{
		next = pack->next;
		if(pack->file)
			F_Close(pack->file);
		free(pack);
	}

	
	for(i = 0; i < zipNumFiles; i++)				// Free the file directory.
		free(zipFiles[i].name);
	free(zipFiles);

	zipRoot = NULL;
	zipFiles = NULL;
	zipNumFiles = 0;
	zipAllocatedFiles = 0;
}
//==========================================================
// Allocate a zipentry array element and return a pointer to it. Duplicate entries are removed later.
zipentry_t *Zip_NewFile(const char *name)
{
	int     oldCount = zipNumFiles;
	BOOL changed = FALSE;

	zipNumFiles++;
	while(zipNumFiles > zipAllocatedFiles)
	{
		zipAllocatedFiles *= 2;											// Double the size of the array.
		if(zipAllocatedFiles <= 0)		zipAllocatedFiles = 1;
		
		zipFiles = realloc(zipFiles, sizeof(zipentry_t) * zipAllocatedFiles);		// Realloc the zipentry array.
		changed = TRUE;
	}

	if(changed)
	{
		memset(zipFiles + oldCount, 0,sizeof(zipentry_t) * (zipAllocatedFiles - oldCount));	// Clear the new memory.
	}

	zipFiles[oldCount].name = malloc(strlen(name) + 1);		// Take a copy of the name. This is freed in Zip_Shutdown().
	strcpy(zipFiles[oldCount].name, name);

	return zipFiles + oldCount;			// Return a pointer to the first new zipentry.
}
//==========================================================
// * Sorts all the zip entries alphabetically. All the paths are absolute.
int  Zip_EntrySorter(const void *a, const void *b)	//C_DECL but that is default for compiler...
{
	return stricmp(((const zipentry_t *) a)->name,
				   ((const zipentry_t *) b)->name);		// Compare the names.
}
//==========================================================
// * Sorts all the zip entries alphabetically.
void Zip_SortFiles(void)
{
	qsort(zipFiles, zipNumFiles, sizeof(zipentry_t), Zip_EntrySorter);	// Sort all the zipentries by name. 
}
//==========================================================
// * Adds a new package to the list of packages.
package_t *Zip_NewPackage(void)	// When duplicates are removed, newer packages are favored.
{
	static int packageCounter = 0;

	package_t *newPack = calloc(sizeof(package_t), 1);

	newPack->next = zipRoot;
	newPack->order = packageCounter++;
	return zipRoot = newPack;
}
//==========================================================
// * Finds the central directory end record in the end of the file.
// * Returns true if it successfully located. This gets awfully slow if the comment is long.
BOOL Zip_LocateCentralDirectory(WZFILE *file)
{
	//      int length = F_Length(file); 
	int     pos = CENTRAL_END_SIZE;	// Offset from the end.
	uint    signature;

	// Start from the earliest location where the signature might be.
	while(pos < MAXIMUM_COMMENT_SIZE)
	{
		F_Seek(file, -pos, SEEK_END);

		
		F_Read(&signature, 4, file);// Is this is signature?
//		if(ULONG(signature) == SIG_END_OF_CENTRAL_DIR)
		if(signature == SIG_END_OF_CENTRAL_DIR)
		{
			return TRUE;			// This is it!
		}
		pos++;			// Move backwards.
	}
	return FALSE;		// Scan was not successful.
}
//==========================================================
// Copies num characters (up to destSize) and adds a terminating NULL.
void Zip_CopyStr(char *dest, const char *src, int num, int destSize)
{
	if(num >= destSize)	num = destSize - 1;		// Only copy as much as we can.
	memcpy(dest, src, num);
	dest[num] = 0;
}
//==========================================================
// The path inside the pack is mapped to another virtual location.
void Zip_MapPath(char *path)
{
 // not used for now. -Q
}
//==========================================================
// Opens the file zip, reads the directory and stores the info for later
// access. If prevOpened is not NULL, all data will be read from there.
BOOL Zip_Open(const char *fileName, WZFILE *prevOpened)
{
	centralend_t summary;
	zipentry_t *entry;
	package_t *pack;
	void   *directory;
	char   *pos;
	char    buf[512];
	int     index;
	WZFILE  *file;

	if(prevOpened == NULL)
	{
		if((file = F_Open(fileName, "rb")) == NULL)		// Try to open the file.
		{
			debug(LOG_ERROR, "Zip_Open: file %s was NOT found!", fileName);
			return FALSE;
		}
	}
	else
	{
		file = prevOpened;					// Use the previously opened file.
	}
	
	if(!Zip_LocateCentralDirectory(file))	// Scan the end of the file for the central directory end record.
	{
		debug(LOG_ERROR, "Zip_Open: Central directory not found!  Corrupted file?");
	}
	F_Read(&summary, sizeof(summary), file);			// Read the central directory end record.

	// Does the summary say something we don't like?
	if(USHORT(summary.diskEntryCount) != USHORT(summary.totalEntryCount))
//	if(summary.diskEntryCount != summary.totalEntryCount)
	{
		debug(LOG_ERROR, "Multipart Zip files are not supported.");
	}

	// Read the entire central directory into memory.
	directory = malloc(ULONG(summary.size));

	F_Seek(file, ULONG(summary.offset), SEEK_SET);
	F_Read(directory, ULONG(summary.size), file);

	pack = Zip_NewPackage();
	strcpy(pack->name, fileName);
	pack->file = file;

	
	pos = directory;
	for(index = 0; index < USHORT(summary.totalEntryCount);		// Read all the entries.
		index++, pos += sizeof(centralfileheader_t))
	{
		localfileheader_t localHeader;
		centralfileheader_t *header = (void *) pos;
		char   *nameStart = pos + sizeof(centralfileheader_t);

		
		pos +=											
			USHORT(header->fileNameSize) + USHORT(header->extraFieldSize) +
			USHORT(header->commentSize);			// Advance the cursor past the variable sized fields.

		Zip_CopyStr(buf, nameStart, USHORT(header->fileNameSize), sizeof(buf));

		
		if(buf[USHORT(header->fileNameSize) - 1] == '/' && 
            ULONG(header->size) == 0) continue;		// Directories are skipped.

		
		if(USHORT(header->compression) != ZFC_NO_COMPRESSION &&
           USHORT(header->compression) != ZFC_DEFLATED)		// Do we support the format of this file?
		{
			debug(LOG_ERROR, "This wz file uses an unsupported compression!");
		}
		if(USHORT(header->flags) & ZFH_ENCRYPTED)
		{
			debug(LOG_ERROR, "Encryption is not supported!");
		}

		
		Dir_FixSlashes(buf);			// Convert all slashes to the host OS's directory separator

		// We can add this file to the zipentry list.
		entry = Zip_NewFile(buf);
		entry->package = pack;
		entry->size = ULONG(header->size);
		entry->priority=atoi(buf);
        if(USHORT(header->compression) == ZFC_DEFLATED)		// Compressed using the deflate
        {
             entry->deflatedSize = ULONG(header->compressedSize);
        }
        else
        {
             entry->deflatedSize = 0;											 // No compression.
        }
        
		
		entry->lastModified = file->lastModified;	// The modification date is inherited from the real file (note recursion).

		// Read the local file header, which contains the correct
		// extra field size (Info-ZIP!).
		F_Seek(file, ULONG(header->relOffset), SEEK_SET);
		F_Read(&localHeader, sizeof(localHeader), file);

		entry->offset =
			ULONG(header->relOffset) + sizeof(localfileheader_t) +
			USHORT(header->fileNameSize) + USHORT(localHeader.extraFieldSize);
	}

	// The central directory is no longer needed.
	free(directory);

	Zip_SortFiles();
	Zip_RemoveDuplicateFiles();
	// File successfully opened!
	return TRUE;
}
//==========================================================
/*
 * If two or more packages have the same file, the file from the last
 * loaded package is the one to use.  Others will be removed from the
 * directory.  The entries must be sorted before this can be done.
 */
void Zip_RemoveDuplicateFiles(void)					//Yeah...change this! -Q
{
	int     i;
	BOOL modified = FALSE;
	zipentry_t *entry, *loser;

	// One scan through the directory is enough.
	for(i = 0, entry = zipFiles; i < zipNumFiles - 1; i++, entry++)
	{
		// Is this entry the same as the next one?
		if(!stricmp(entry[0].name, entry[1].name)) 
		{	
			//printf("ZIP=======Replaced!======>%s vs %s\n",entry[0].name, entry[1].name);
			// One of these should be removed. The newer one survives.
			if(entry[0].package->order > entry[1].package->order)
				loser = entry + 1;
			else
				loser = entry;

			// Overwrite the loser with the last entry in the entire entry directory.
			free(loser->name);
			memcpy(loser, &zipFiles[zipNumFiles - 1], sizeof(zipentry_t));
			memset(&zipFiles[zipNumFiles - 1], 0, sizeof(zipentry_t));
			zipNumFiles--;						// One less entry.
			modified = TRUE;
		}
	}

	if(modified)
	{
		Zip_SortFiles();					// Sort the entries again.
	}
}
//==========================================================
/*
 * Iterates through the zipentry list. If the finder func returns true,
 * the iteration stops and the the 1-based index of the zipentry is 
 * returned. Parm is passed to the finder func. Returns zero if nothing
 * is found.
 */
int Zip_Iterate(int (*iterator) (const char *, void *), void *parm)
{
	int     i;

	for(i = 0; i < zipNumFiles; i++)
		if(iterator(zipFiles[i].name, parm))
			return i + 1;

	// Nothing was accepted.
	return 0;
}
//==========================================================
void mystrcat(char *dst, const char *src)
//lol... we can get rid of this, and use the others -Q
{
	int i;
	int slen=0,dlen=0;
	slen=strlen(src);
	dlen=strlen(dst);
	for(i=0;i<slen;i++)
	{
		dst[i+dlen]=src[i];
	}
	dst[i+dlen]='\0';
}
//==========================================================
//reverse sort	compare function								//we need to go highest patch to lowest.  In this case, it would be
int Rcompare (const void * a, const void * b)				//11 to 1 or 0 depending if we want to include the warzone.wz file in this
{																			//also. -Q
  return ( *(int*)b - *(int*)a );
}
//~~~~~~~~~~~~~~~
static int prinum=1;					//mods start at 1
static int prilist[100];				//100 mods? 
extern char addonmaps[600][256];	// this will hold all the "addon.lev"(s) that we find. Only hold 600 for now
static int DOONCE=0;				// info processing
extern BOOL						pQUEUE	;		//ON for MP off for SP -Q
//**********************************************************************
//* This Find will be priority based, in that first it will find the highest priority files
//* first....
//*********************************************************************
int Zip_Find_MP(const char *fileName)					//side note, we are not using the other find routine now, since
{																		//looks like we need to do priority.
	int i=0;															//works for the most part, but could be fixed up much better. -Q
	int begin, end;
	char	  PriorityPath[PATH_MAX];
	int NumList;
	int found=0;
	// Init the search.
	begin =  0;
	NumList =1;
	end = zipNumFiles - 1;
	
	if(!DOONCE)					//yeah..yeah.. fix later.... it works OK now though.  -Q
	{
	memset(prilist,0,sizeof(int)*100);
	while(begin <= end)
	{
	//first we check highest priority folder we got...
	 if(zipFiles[begin].priority!=0)
	 {	
//		 for(i=0;i<NumList;i++)	//scan to make sure we only have 1 reference.			//naww -Q
//		 {
			 if(prilist[prinum-1]!=zipFiles[begin].priority)
			 {
		prilist[prinum]=zipFiles[begin].priority;
			prinum++;
		if(prinum >99)
		{	printf(" ****  Fatal error!\n");
			printf("Coyote says tough luck buddy, 20 is more than enough! Clean up your directory!\n");
			exit(-1);
		}

//			NumList++;
//			break;
			 }
			 else
			 {	
//				 break;
			 }
//		 }
//		prilist[prinum]=zipFiles[begin].priority;
//		prinum++;
	 }
	begin++;
	}

	//revrese sort the list...
	qsort(prilist, prinum, sizeof(int), Rcompare);		//Um, we could go from prinum to 0, but just in case...
	for(i=0;i<prinum;i++)	//scan to make sure we only have 1 reference.
	{
//		printf(" We got this in the queue: %d \n",prilist[i]);
	}
	DOONCE=1;
	}//end DOONCE;
	if(pQUEUE)	//bMultiPlayer			//multiplayer flag (ON) then do this!		
	{
//		printf("---------$$$$$$ MP= %d $$$$$$$$$$$  LOOKING FOR  %s\n",bMultiPlayer,fileName);
		for(i=0;i<prinum;i++)
		{	
			if(!stricmp("addon.lev",fileName)) break;
			if(prilist[i]==0) break;				// 0= warzone.wz (if we go that route)
			sprintf(PriorityPath,"%02d\\",prilist[i]);
			mystrcat(PriorityPath,fileName);
//			strncat(PriorityPath,fileName,strlen(fileName));
//			strcat(PriorityPath,fileName);
			found= Zip_Find(PriorityPath);
			if(found!=0)
			{
//				printf("---------$$$$$$$$$$$$$$$$$  found %d, %s\n",found,PriorityPath);
				return found;
			}
		}
	}
//	printf("---------$$$$$$$$$$$$$$$$$  NORMAL find %s\n",fileName);
	return Zip_Find(fileName);
}
//=====================================================================
void Zip_Find_MPmaps(const char *fileName)					//side note, we are not using the other find routine now, since
{																				//looks like we need to do priority.
	int i=0,j=0;															//works for the most part, but could be fixed up much better. -Q
	int begin, end;
	char	  PriorityPath[PATH_MAX];
	int NumList;
	int found=0;

	// Init the search.
	begin =  0;
	NumList =1;
	end = zipNumFiles - 1;

	if(pQUEUE)	//bMultiPlayer			//multiplayer flag (ON) then do this!		// FORCE this thing for now... 0=SP 1=MP // NEED TO FIX****** -Q
	{
		for(i=prinum;i>=0;i--)			//MAPS load *BACKWARDS*!! ie 0 to max #...
		{	
			if(stricmp("addon.lev",fileName)) continue;
//			if(prilist[i]==0) continue;
			sprintf(PriorityPath,"%02d\\",prilist[i]);
			mystrcat(PriorityPath,fileName);
//			strncat(PriorityPath,fileName,strlen(fileName));
//			strcat(PriorityPath,fileName);
			found= Zip_Find(PriorityPath);
			if(found!=0)
			{
//				printf("---------$$$$$$$$$$$$$$$$$  found %d, %s\n",found,PriorityPath);
				memcpy(&addonmaps[j][0],PriorityPath,strlen(PriorityPath));
				j++;
			}
		}
	}
	debug(LOG_WZ, "Done searching for addon.lev files.  We found %d", j);
}

//==========================================================
//**********************************************************************
/*
 * Find a specific path in the zipentry list. Relative paths are converted
 * to absolute ones. A binary search is used (the entries have been sorted).
 * Good performance: O(lg n). Returns zero if nothing is found.
 */
int Zip_Find(const char *fileName)					//yes, this works fine for normal finding stuff...-Q
{
	int begin, end, mid;
	int     relation;
	char    fullPath[PATH_MAX];

	
	strcpy(fullPath, fileName);				// Convert to an absolute path.
//	printf("Find -> %s \n",fullPath);
	Dir_FixSlashes(fullPath);
//	printf("2Find -> %s \n",fullPath);
//	Dir_MakeAbsolute(fullPath);

	// Init the search.
	begin = 0;
	end = zipNumFiles - 1;

	while(begin <= end)
	{
		mid = (begin + end) / 2;

		// How does this compare?
		relation = stricmp(fullPath, zipFiles[mid].name);
		if(!relation)
		{
			// Got it! We return a 1-based index.
			return mid + 1;
		}
		if(relation < 0)
		{
			// What we are searching must be in the first half.
			end = mid - 1;
		}
		else
		{
			// Then it must be in the second half.
			begin = mid + 1;
		}
	}

	// It wasn't found.
	return 0;
}
//==========================================================
//  Uses zlib to inflate a compressed entry.  Returns true if successful.
BOOL Zip_Inflate(void *in, uint inSize, void *out, uint outSize)				
{												
    z_stream stream;
    int result;

    memset(&stream, 0, sizeof(stream));
    stream.next_in = in;
    stream.avail_in = inSize;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.next_out = out;
    stream.avail_out = outSize;

    if(inflateInit2(&stream, -MAX_WBITS) != Z_OK)
        return FALSE;
    
    result = inflate(&stream, Z_FINISH);				// Do the inflation in one call.

    if(stream.total_out != outSize)
    {
		printf("Zip_Inflate failed!  Corrupted data?\n");
        return FALSE;
    }
    inflateEnd(&stream);							  // We're done.
    return TRUE;
}
//==========================================================
// Returns the size of a zipentry.
uint Zip_GetSize(int index)
{
	index--;
	if(index < 0 || index >= zipNumFiles)
		return 0;				// Doesn't exist.
	return zipFiles[index].size;
}
//==========================================================
// Reads a zipentry into the buffer.  The caller must make sure that
// the buffer is large enough.  Zip_GetSize() returns the size. Returns the number of bytes read.
uint Zip_Read(int index, void *buffer)					//ok, fixed this. looks OK for now.  -Q
{
	package_t *pack;
	zipentry_t *entry;
    void *compressedData;
    BOOL result;

	index--;
	if(index < 0 || index >= zipNumFiles)
		return 0;				// Doesn't exist.

	entry = zipFiles + index;
	pack = entry->package;

	F_Seek(pack->file, entry->offset, SEEK_SET);

    if(entry->deflatedSize > 0)
    {
        // Read the compressed data into a buffer.
        compressedData = malloc(entry->deflatedSize);
        F_Read(compressedData, entry->deflatedSize, pack->file);

        // Run zlib's inflate to decompress.
        result = Zip_Inflate(compressedData, entry->deflatedSize,
                             buffer, entry->size);

        free(compressedData);

        if(!result)
            return 0; // Inflate failed.
    }
    else
    {
        // Read the uncompressed data directly to the buffer provided
        // by the caller.
        F_Read(buffer, entry->size, pack->file);
    }

	return entry->size;
}
//==========================================================
// * Return the last modified timestamp of the zip entry.
uint Zip_GetLastModified(int index)				//naw.. better not do this, since people can screw with timestamps...
{
	index--;
	if(index < 0 || index >= zipNumFiles)
		return 0; // Doesn't exist.

	return zipFiles[index].lastModified;
}
//==========================================================
