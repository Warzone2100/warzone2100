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


/* Basic numeric types */
typedef unsigned	char	UBYTE;
typedef	signed		char	SBYTE;
typedef signed		char	STRING;
typedef	unsigned	short	UWORD;
typedef	signed		short	SWORD;
typedef	unsigned	int		UDWORD;
typedef	signed		int		SDWORD;

typedef	int	BOOL;

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

/* locale types */

#define LOCAL                   static
#define STATIC                  static
#define REGISTER                register
#define FAST                    register
#define IMPORT                  extern
#define VOID                    void



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

// now in fractions.h #define ROUND(x) ((x)>=0 ? (SDWORD)((x) + 0.5) : (SDWORD)((x) - 0.5))

/* Boolean operators */

#define	AND &&
#define OR ||
#define NOT !

/* Break loop construct */

#define FOREVER for (;;)


#endif
