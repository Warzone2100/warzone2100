#ifndef __TYPEDEFS_INCLUDED__
#define	__TYPEDEFS_INCLUDED__

typedef signed long SLONG;
typedef signed short SWORD;
typedef signed char SBYTE;
typedef unsigned long ULONG;
typedef unsigned short UWORD;
typedef unsigned char UBYTE;

#define UBYTE_MAX 0xff
#define UWORD_MAX 0xffff
#define UDWORD_MAX 0xffffffff

typedef signed		char	STRING;
typedef	unsigned	int		UDWORD;
typedef	signed		int		SDWORD;

typedef int BOOL;

#ifndef NULL
#define NULL (0)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef TRUE
#define TRUE (1)
#endif

#define	setRect(rct,x0,y0,x1,y1) (rct)->top=y0; (rct)->left=x0; \
											(rct)->bottom=y1; (rct)->right=x1
#define	setRectWH(rct,x,y,width,height) (rct)->top=y; (rct)->left=x; \
											(rct)->bottom=y+height; (rct)->right=x+width

#endif
