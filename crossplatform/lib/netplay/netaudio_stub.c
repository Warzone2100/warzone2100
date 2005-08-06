/*
 * NetAudio.c
 *
 * intra/internet audio
 */

#include "frame.h"
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

BOOL NETinitPlaybackBuffer(LPDIRECTSOUND pDs)
{
	debug(LOG_SOUND, "NETinitPlaybackBuffer");
	return FALSE;
}

// ////////////////////////////////////////////////////////////////////////
// handle the playback buffer.
BOOL NETqueueIncomingAudio( LPBYTE lpbSoundData, DWORD dwSoundBytes,BOOL bStream)
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

