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
printf("NETinitAudioCapture\n");
	return FALSE;
}

// ////////////////////////////////////////////////////////////////////////
BOOL NETshutdownAudioCapture(VOID)
{
printf("NETshutdownAudioCapture\n");
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////
BOOL NETstartAudioCapture(VOID)
{
printf("NETstartAudioCapture\n");
	return FALSE;
}
// ////////////////////////////////////////////////////////////////////////
BOOL NETstopAudioCapture(VOID)
{
printf("NETstopAudioCapture\n");
	return FALSE;
}


// ////////////////////////////////////////////////////////////////////////
// update the pointers and process the buffer accordingly.
BOOL NETprocessAudioCapture(VOID)
{
printf("NETprocessAudioCapture\n");
	return FALSE;
}

// ////////////////////////////////////////////////////////////////////////

BOOL NETinitPlaybackBuffer(LPDIRECTSOUND pDs)
{
printf("NETinitPlaybackBuffer\n");
	return FALSE;
}

// ////////////////////////////////////////////////////////////////////////
// handle the playback buffer.
BOOL NETqueueIncomingAudio( LPBYTE lpbSoundData, DWORD dwSoundBytes,BOOL bStream)
{ 
printf("NETqueueIncomingAudio\n");
    return FALSE; 
}

// ////////////////////////////////////////////////////////////////////////
// Handle a incoming message that needs to be played
void NETplayIncomingAudio(NETMSG *pMsg)
{	
printf("NETplayIncomingAudio\n");
}

// ////////////////////////////////////////////////////////////////////////
// close it all down
BOOL NETshutdownAudioPlayback(VOID)
{
printf("NETshutdownAudioPlayback\n");
	return TRUE;
}

