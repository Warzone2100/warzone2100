#ifndef _cheat_h
#define _cheat_h
extern	void	setCheatCodeStatus(BOOL val);
extern	BOOL	getCheatCodeStatus( void );
extern	BOOL	attemptCheatCode(STRING	*pName);
unsigned char	*xorString(unsigned char *string);
#endif
