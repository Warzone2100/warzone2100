/***************************************************************************/

#ifndef _PLAYLIST_H_
#define _PLAYLIST_H_

/***************************************************************************/

void PlayList_Init(void);
void PlayList_Quit(void);
char PlayList_Read(const char* path);
void PlayList_SetTrack(unsigned int t);
char* PlayList_CurrentSong(void);
char* PlayList_NextSong(void);
void PlayList_DeleteCurrentSong(void);

/***************************************************************************/

#endif /* _PLAYLIST_H_ */

/***************************************************************************/
