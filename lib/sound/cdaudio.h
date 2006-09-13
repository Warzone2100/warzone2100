/***************************************************************************/

#ifndef _CDAUDIO_H_
#define _CDAUDIO_H_

/***************************************************************************/

BOOL	cdAudio_Open( char* user_musicdir );
BOOL	cdAudio_Close( void );
BOOL	cdAudio_PlayTrack( SDWORD iTrack );
BOOL	cdAudio_Stop( void );
BOOL	cdAudio_Pause( void );
BOOL	cdAudio_Resume( void );
void	cdAudio_Update(void);

/***************************************************************************/

#endif	/* _CDAUDIO_H_ */

/***************************************************************************/
