#ifndef _3dfxmode_h
#define _3dfxmode_h
extern BOOL	init3dfx( void );
extern void reset3dfx(void);
extern void close3dfx( void );
// Deativate 3DFX when the game looses focus
BOOL gl_Deactivate(void);
// Reactive the 3DFX when the game gets focus
BOOL gl_Reactivate(void);
#endif