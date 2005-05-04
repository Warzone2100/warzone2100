/***************************************************************************/

#include <SDL/SDL.h>

#include "frame.h"
#include "audio.h"
#include "cdaudio.h"

/***************************************************************************/

#define	CD_MAX_VOL		0xFFFF
#define	CD_ID_NOT_FOUND		-1
#define	CD_LEFT_CHANNEL_MASK	0xFFFF

#define	CD_NO_TRACK_SELECTED	-1

/***************************************************************************/

SDL_CD *cdAudio_dev;

/***************************************************************************/
/* cdAudio Subclass procedure */

/***************************************************************************/

BOOL
cdAudio_Open( void )
{ 
	if(!SDL_CDNumDrives()) {
		printf("No CDROM devices available\n");
		return FALSE;
	}

	cdAudio_dev = SDL_CDOpen(0);

	if(!cdAudio_dev){
		printf("Couldn't open drive: %s\n", SDL_GetError());
		return FALSE;
	}

	SDL_CDStatus(cdAudio_dev);

	/* Print Volume info */
	/*
	{
		int cur_track;
		int min, sec, frame;

		printf("Name: %s\n", SDL_CDName(0));
		printf("Tracks: %d\n", cdAudio_dev->numtracks);
		for(cur_track=0; cur_track < cdAudio_dev->numtracks; cur_track++){
			FRAMES_TO_MSF(cdAudio_dev->track[cur_track].length, &min, &sec, &frame);
			printf("\tTrack %d: Length %d:%d\n", cur_track, min, sec);
		}
	}
	*/

	return TRUE;
} 

/***************************************************************************/

BOOL
cdAudio_Close( void )
{ 
	if (cdAudio_dev != NULL) {
		SDL_CDClose(cdAudio_dev);
	}

	return TRUE;
} 

/***************************************************************************/

BOOL
cdAudio_PlayTrack( SDWORD iTrack )
{ 
	if (cdAudio_dev != NULL) {
		SDL_CDPlayTracks(cdAudio_dev, iTrack-1, 0, 1, 0);
	}
	return TRUE;
} 

/***************************************************************************/

BOOL
cdAudio_Stop( void )
{ 
	if (cdAudio_dev != NULL) {
		SDL_CDStop(cdAudio_dev);
	}
	return TRUE;
} 

/***************************************************************************/

BOOL
cdAudio_Pause( void )
{ 
	if (cdAudio_dev != NULL) {
		SDL_CDPause(cdAudio_dev);
	}
	return TRUE;
} 

/***************************************************************************/

BOOL
cdAudio_Resume( void )
{ 
	if (cdAudio_dev != NULL) {
		SDL_CDResume(cdAudio_dev);
	}
	return TRUE;
} 

/***************************************************************************/

void cdAudio_SetVolume( SDWORD iVol )
{
}

/***************************************************************************/

void cdAudio_Update() {
}

