#ifndef _difficulty_h
#define _difficulty_h

typedef enum _difficulty_level
{
DL_EASY,
DL_NORMAL,
DL_HARD,
DL_TOUGH,
DL_KILLER
} DIFFICULTY_LEVEL;

extern void	setDifficultyLevel( DIFFICULTY_LEVEL lev);
extern DIFFICULTY_LEVEL	getDifficultyLevel( void );
extern SDWORD	modifyForDifficultyLevel(SDWORD basicVal,BOOL IsPlayer);
extern void setModifiers(FRACT Player,FRACT Enemy);


#endif
