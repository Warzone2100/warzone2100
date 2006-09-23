/*
 * types.h
 *
 * Simple type definitions.
 *
 */
#ifndef _types_h
#define _types_h

/* Check the header files have been included from frame.h if they
 * are used outside of the framework library.
 */

#if !defined(_frame_h) && !defined(FRAME_LIB_INCLUDE)
#error Framework header files MUST be included from Frame.h ONLY.
#endif

#include <SDL/SDL_types.h>

#ifdef _MSC_VER
# define INT8_MIN               (-128)
# define INT16_MIN              (-32767-1)
# define INT32_MIN              (-2147483647-1)
# define INT8_MAX               (127)
# define INT16_MAX              (32767)
# define INT32_MAX              (2147483647)
# define UINT8_MAX              (255)
# define UINT16_MAX             (65535)
# define UINT32_MAX             (4294967295U)
#else
/* Compilers that have support for C99 have all of the above defined in stdint.h */
# include <stdint.h>
#endif // _MSC_VER

/* Basic numeric types */
typedef Uint8 UBYTE;
typedef Sint8 SBYTE;
typedef Uint16 UWORD;
typedef Sint16 SWORD;
typedef Uint32 UDWORD;
typedef Sint32 SDWORD;
typedef char STRING;

#ifndef WIN32

// win32 functions to POSIX
#define wsprintf sprintf

#define MAKELONG(low,high)     ((LONG)(((WORD)(low)) | (((DWORD)((WORD)(high))) << 16)))

#define WARZONEGUID 0
#define VER_PLATFORM_WIN32_WINDOWS 1


#define DRIVE_CDROM  5
#define INVALID_HANDLE_VALUE       0

#define REG_OPTION_NON_VOLATILE       0
#define KEY_ALL_ACCESS       0
#define ERROR_SUCCESS       0
#define REG_DWORD       0
#define REG_SZ       0
#define REG_BINARY       0
#define HKEY_LOCAL_MACHINE       0

#ifndef APIENTRY
#define APIENTRY

#endif // WIN32

typedef void * HKEY;
typedef int GUID;
typedef void * LPGUID;
typedef int CRITICAL_SECTION;
typedef int HWND;
typedef void * HANDLE;
typedef int HINSTANCE;
typedef int HRESULT;
typedef int LRESULT;
typedef int HCURSOR;
typedef int WPARAM;
typedef int LPARAM;

typedef int     BOOL;
typedef char CHAR;
typedef unsigned char UCHAR;
typedef unsigned char BYTE;
typedef short SHORT;
typedef unsigned short USHORT;
typedef signed short WORD;
typedef unsigned int UINT;
typedef signed int DWORD;
typedef long LONG;
typedef unsigned long ULONG;
typedef unsigned char * LPBYTE;
typedef signed int * LPDWORD;
typedef char  * LPSTR;

typedef struct
{
    CHAR      cFileName[260];
} WIN32_FIND_DATA;

typedef struct {
  UBYTE peRed;
  UBYTE peGreen;
  UBYTE peBlue;
  UBYTE peFlags;
} PALETTEENTRY;

typedef struct
{
  int  x;
  int  y;
} POINT;

#endif /* !WIN32 */

#ifndef INC_DIRECTX

typedef int DPID;

#endif

#ifndef  _MSC_VER	// this breaks on .net, it wants the other format.  --Qamly
#define _inline inline
#define __inline inline
#else
#define _inline __inline
#endif //_MSC_VER

/* Numeric size defines */
#define UBYTE_MAX	0xff
#define SBYTE_MIN	(-128) //(0x80)
#define SBYTE_MAX	0x7f
#define UWORD_MAX	0xffff
#define SWORD_MIN	(-32768) //(0x8000)
#define SWORD_MAX	0x7fff
#define UDWORD_MAX	0xffffffff
#define SDWORD_MIN	(0x80000000)
#define SDWORD_MAX	0x7fffffff

/* Standard Defines */
#ifndef NULL
#define NULL	(0)
#endif

#ifndef TRUE
#define TRUE	(1)
#define FALSE	(0)
#endif


/* defines for ONEINX - use

   if (ONEINX)
		{
		code.....
		}

*/

#define	ONEINTWO				rand()%2==0
#define ONEINTHREE				rand()%3==0
#define ONEINFOUR				rand()%4==0
#define ONEINFIVE				rand()%5==0
#define ONEINSIX				rand()%6==0
#define ONEINSEVEN				rand()%7==0
#define ONEINEIGHT				rand()%8==0
#define ONEINNINE				rand()%9==0
#define ONEINTEN				rand()%10==0

#define ONEINTWENTY				rand()%20==0
#define ONEINTHIRTY				rand()%30==0
#define ONEINFORTY				rand()%40==0
#define ONEINFIFTY				rand()%50==0
#define ONEINSIXTY				rand()%60==0
#define ONEINSEVENTY			rand()%70==0
#define ONEINEIGHTY				rand()%80==0
#define ONEINNINETY				rand()%90==0

#define ONEINHUNDRED			rand()%100==0
#define ONEINTHOUSAND			rand()%1000==0
#define ONEINTENTHOUSAND		rand()%10000==0
#define ONEINHUNDREDTHOUSAND	rand()%100000==0
#define ONEINMILLION			rand()%1000000==0


#define	ABSDIF(a,b) ((a)>(b) ? (a)-(b) : (b)-(a))
#define CAT(a,b) a##b

/* Boolean operators */

#define	AND &&
#define OR ||
#define NOT !

/* Break loop construct */

#define FOREVER for (;;)


#endif
