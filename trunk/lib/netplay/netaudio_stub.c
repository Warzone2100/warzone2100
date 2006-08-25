/*
 * NetAudio.c
 *
 * Internet audio. Team communication using microphone etc.. Currently
 * just an unimplemented skeleton.
 */

#include "lib/framework/frame.h"
#include "netplay.h"

// ////////////////////////////////////////////////////////////////////////
BOOL NETinitAudioCapture(VOID)
{
	debug(LOG_SOUND, "NETinitAudioCapture");
	return FALSE;
}

// ////////////////////////////////////////////////////////////////////////
BOOL NETshutdownAudioCapture(VOID)
{
	debug(LOG_SOUND, "NETshutdownAudioCapture");
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////
BOOL NETstartAudioCapture(VOID)
{
	debug(LOG_SOUND, "NETstartAudioCapture");
	return FALSE;
}
// ////////////////////////////////////////////////////////////////////////
BOOL NETstopAudioCapture(VOID)
{
	debug(LOG_SOUND, "NETstopAudioCapture");
	return FALSE;
}


// ////////////////////////////////////////////////////////////////////////
// update the pointers and process the buffer accordingly.
BOOL NETprocessAudioCapture(VOID)
{
	debug(LOG_SOUND, "NETprocessAudioCapture");
	return FALSE;
}

// ////////////////////////////////////////////////////////////////////////

BOOL NETinitPlaybackBuffer(void *pSoundBuffer)
{
	debug(LOG_SOUND, "NETinitPlaybackBuffer");
	return FALSE;
}

// ////////////////////////////////////////////////////////////////////////
// handle the playback buffer.
BOOL NETqueueIncomingAudio( void *pSoundData, DWORD soundBytes,BOOL bStream)
{
	debug(LOG_SOUND, "NETqueueIncomingAudio");
	return FALSE;
}

// ////////////////////////////////////////////////////////////////////////
// Handle a incoming message that needs to be played
void NETplayIncomingAudio(NETMSG *pMsg)
{
	debug(LOG_SOUND, "NETplayIncomingAudio");
}

// ////////////////////////////////////////////////////////////////////////
// close it all down
BOOL NETshutdownAudioPlayback(VOID)
{
	debug(LOG_SOUND, "NETshutdownAudioPlayback");
	return TRUE;
}
