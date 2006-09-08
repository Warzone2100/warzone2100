/* Handles the two CD issue */
/* Alex McLean */

/* seems to be responsible for playing music in-game,
 * move that to somewhere else
 */

#include "lib/framework/frame.h"

#include "lib/sound/audio.h"
#include "lib/sound/track.h"
#include "cdspan.h"

static STRING		g_szCurDriveName[MAX_STR] = "";

void
cdspan_PlayInGameAudio( STRING szFileName[], SDWORD iVol )
{
	STRING szStream[MAX_STR];//	szDrive[MAX_STR] = "",
	BOOL	bPlaying = FALSE;

	audio_StopAll();

	sprintf( szStream, "%s%s", g_szCurDriveName, szFileName );
	bPlaying = audio_PlayStream( szStream, iVol, NULL );

	/* try playing from hard disk */
	if ( bPlaying == FALSE )
	{
		sprintf( szStream, "audio\\%s", szFileName );
		bPlaying = audio_PlayStream( szStream, iVol, NULL );
	}
}
