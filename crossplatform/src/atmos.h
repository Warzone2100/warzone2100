#ifndef _atmos_h
#define _atmos_h

typedef struct _atmosParticle
{
UBYTE		status;
UBYTE		type;
UDWORD		size;
PIEVECTORF	position;
PIEVECTORF	velocity; 
iIMDShape	*imd;	
} ATPART;

typedef	enum
{
WT_RAINING,
WT_SNOWING,
WT_NONE
} WT_CLASS;

extern void		atmosInitSystem			( void );
extern void		atmosUpdateSystem		( void );
extern void		renderParticle		( ATPART *psPart );
extern void		atmosDrawParticles	( void );
extern void		atmosSetWeatherType	( WT_CLASS type );
extern WT_CLASS		atmosGetWeatherType ( void );
typedef struct _mistlocale
{
UBYTE	type;
UBYTE	val;
UBYTE	base;
SBYTE	vec;
} MISTAREA;


extern MISTAREA *pMistValues;

#endif
