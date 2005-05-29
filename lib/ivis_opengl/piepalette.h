#ifndef _piePalette_
#define _piePalette_

#include "piedef.h"
//*************************************************************************

#define PALETTE_MAX	8

#define PALETTE_SIZE	256
#define PALETTE_SHADE_LEVEL 16

#define COL_TRANS			0
#define COL_BLACK			colours[0]
#define COL_BLUE			colours[1]
#define COL_GREEN			colours[2]
#define COL_CYAN			colours[3]
#define COL_RED				colours[4]
#define COL_MAGENTA  		colours[5]
#define COL_BROWN			colours[6]
#define COL_GREY			colours[7]
#define COL_DARKGREY		colours[8]
#define COL_LIGHTBLUE		colours[9]
#define COL_LIGHTGREEN		colours[10]
#define COL_LIGHTCYAN		colours[11]
#define COL_LIGHTRED		colours[12]
#define COL_LIGHTMAGENTA	colours[13]
#define COL_YELLOW			colours[14]
#define COL_WHITE			colours[15]

//*************************************************************************

extern uint8		colours[];
extern uint8		palShades[PALETTE_SIZE * PALETTE_SHADE_LEVEL];
extern uint8		transLookup[PALETTE_SIZE][PALETTE_SIZE];
extern UWORD	palette16Bit[PALETTE_SIZE];	//16 bit version of the present palette


//*************************************************************************
extern void		pal_Init(void);
extern void		pal_ShutDown(void);
extern void		pal_BuildAdjustedShadeTable( void );
extern uint8	pal_GetNearestColour(uint8 r, uint8 g, uint8 b);
extern int		pal_AddNewPalette(iColour *pal);
extern void		pal_SelectPalette(int n);
extern void		pal_PaletteSet(void);
extern BOOL		pal_Make16BitPalette(void);
extern iColour*	pie_GetGamePal(void);
extern PALETTEENTRY*	pie_GetWinPal(void);

extern void	pie_BuildSoftwareTransparency( void );


#endif
