/*
 * NetAudio.c
 *
 * intra/internet audio
 */

#include "dsound.h"
#include "frame.h"
#include "netplay.h"

// ////////////////////////////////////////////////////////////////////////

// playback buffer. You may not want to use netplay to handle any playback..
static LPDIRECTSOUND				lpDirectSound;					//pointer to dsound interface.
static LPDIRECTSOUNDBUFFER			lpDirectSoundBuffer;			//pointer to play buffer.
static DSBUFFERDESC					playbackBuff;							//description of playback buffer

// capture buffer
static LPDIRECTSOUNDCAPTURE			lpDirectSoundCapture;
static LPDIRECTSOUNDCAPTUREBUFFER	lpDirectSoundCaptureBuffer;
static DSCBUFFERDESC				captureBuff;

static BOOL							sampling		= FALSE;
static BOOL							ourDSPointer	= FALSE;

#define								SAMPLERATE		22050 //11025
#define								BITSPERSAMPLE	8
#define								AUDIOCHANNELS	1	
#define								SAMPLETIME		2

BOOL		NETprocessAudioCapture	(VOID);
BOOL		NETstopAudioCapture		(VOID);
BOOL		NETstartAudioCapture	(VOID);
BOOL		NETshutdownAudioCapture	(VOID);
BOOL		NETinitAudioCapture		(VOID);
static BOOL sendAudioBuffer			(BOOL bNow);
static VOID sendAudioComplete		(VOID);

static BOOL	setupPlayBuffer			(VOID);
static BOOL	setupSoundPlay			(VOID);
BOOL		NETinitPlaybackBuffer	(VOID *pDs);
static VOID stopIncomingAudio		(VOID);
VOID		NETplayIncomingAudio	(NETMSG *pMsg);
BOOL		NETqueueIncomingAudio	(LPBYTE lpbSoundData, DWORD dwSoundBytes,BOOL bStream);
BOOL		NETshutdownAudioPlayback(VOID);


// ////////////////////////////////////////////////////////////////////////
static BOOL sendAudioBuffer(BOOL bNow)
{
	DWORD			read;
	static DWORD	lastread=0;
	HRESULT			hr;
	NETMSG			msg;
	
	UWORD			size=0;

	LPVOID			lpvPtr1,lpvPtr2;
	DWORD			dwBytes1=0,dwBytes2=0;

	hr = IDirectSoundCaptureBuffer_GetCurrentPosition(lpDirectSoundCaptureBuffer,NULL,&read);
	if(hr != DS_OK)
	{
		return FALSE;
	}

	// if buffer is still pretty empty
	if(lastread <= read)		// non wrapped.
	{
		size = (UWORD) (read - lastread);
	}
	else					// wrapped
	{
		size = (UWORD) (read + (captureBuff.dwBufferBytes - lastread));
	}
	

	if(!bNow)
	{
		if(size < ((MaxMsgSize*3)/4)  )
		{
			return TRUE;
		}
	}

	// send an audio packet if getting near the end of the buffer.
	if(size > MaxMsgSize)
	{
		size = MaxMsgSize;
		DBPRINTF(("NETPLAY:lost some audio\n"));
	}

	// lock an area
	hr = IDirectSoundCaptureBuffer_Lock(lpDirectSoundCaptureBuffer, //Obtain mem add. of write block.
							 lastread,							// start at
							 size,								// read this much
							 &lpvPtr1,&dwBytes1,										
							 &lpvPtr2,&dwBytes2,	
							 0									//DSCBLOCK_ENTIREBUFFER	
							);

	lastread = lastread+size;
	if(lastread >captureBuff.dwBufferBytes)						// wrapped
	{
		lastread = lastread - captureBuff.dwBufferBytes;
	}
	

	if(hr != DS_OK)
	{
		return FALSE;
	}

	// read it out
	memcpy(msg.body,lpvPtr1,dwBytes1);							// copy first part into buffer

	if (lpvPtr2 != NULL)										// second part.
	{
		memcpy(((UBYTE *)msg.body)+dwBytes1,lpvPtr2,dwBytes2);	// buffer contents			
	}
		
	// unlock area.
	hr =  IDirectSoundCaptureBuffer_Unlock(lpDirectSoundCaptureBuffer,  
										lpvPtr1,dwBytes1,
										lpvPtr2,dwBytes2);
	if(hr != DS_OK)
	{
		return FALSE;
	}
	
	// may want to compress it! 
//MMRESULT acmFormatSuggest( HACMDRIVER had, LPWAVEFORMATEX pwfxSrc, LPWAVEFORMATEX pwfxDst, DWORD cbwfxDst, DWORD fdwSuggest 
  

	
	// send the buffer elsewhere
	msg.type = AUDIOMSG;
	msg.size = (UWORD)(dwBytes1+dwBytes2);
	NETbcast(&msg,FALSE);

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////

static VOID sendAudioComplete(VOID)
{
	NETMSG msg;

	msg.type = AUDIOMSG;
	msg.size = 1;
	NETbcast(&msg,FALSE);
}

// ////////////////////////////////////////////////////////////////////////
BOOL NETinitAudioCapture(VOID)
{
	HRESULT				hr;
	WAVEFORMATEX        wfx;

	NetPlay.bAllowCaptureRecord = FALSE;

	hr = DirectSoundCaptureCreate(	NULL,&lpDirectSoundCapture,	NULL);
	if(hr != DS_OK)
	{
		DBPRINTF(("NETPLAY:Failed to create capture interface."));
		if(hr == DSERR_NOAGGREGATION)
		{
			DBPRINTF(("DSERR_NOAGGREGATION"));
		}
		if(hr == DSERR_OUTOFMEMORY)
		{
			DBPRINTF(("DSERR_OUTOFMEMORY"));
		}
		if(hr == DSERR_INVALIDPARAM)
		{
			DBPRINTF(("DSERR_INVALIDPARAM"));
		}
		return FALSE;
	}
	
	ZeroMemory(&captureBuff, sizeof(captureBuff));
	lpDirectSoundCaptureBuffer = NULL;
 
	wfx.wFormatTag			= WAVE_FORMAT_PCM; 	//set audio format
	wfx.nChannels			= AUDIOCHANNELS;
	wfx.nSamplesPerSec		= SAMPLERATE;			
	wfx.nAvgBytesPerSec		= SAMPLERATE * (BITSPERSAMPLE /8) * AUDIOCHANNELS  ;	
	wfx.nBlockAlign			= (AUDIOCHANNELS * BITSPERSAMPLE) / 8 ; 
	wfx.wBitsPerSample		= BITSPERSAMPLE;	
	wfx.cbSize				= 0;	

	captureBuff.dwSize		= sizeof(DSCBUFFERDESC);
	captureBuff.dwFlags		= DSCBCAPS_WAVEMAPPED;
	captureBuff.dwBufferBytes= SAMPLETIME * wfx.nAvgBytesPerSec;		// SAMPLETIME second’s worth of audio
	captureBuff.lpwfxFormat =  &wfx;
 
	hr = IDirectSoundCapture_CreateCaptureBuffer(lpDirectSoundCapture,
												&captureBuff,
												&lpDirectSoundCaptureBuffer,
												NULL);	
	if(hr != DS_OK)
	{
		if( hr == DSERR_INVALIDPARAM)
		{DBPRINTF(("NETPLAY:no capturebuffer,inv param"));
		}
		if( hr == DSERR_BADFORMAT)
		{DBPRINTF(("NETPLAY:no capturebuffer,badformat"));
		}
		if( hr == DSERR_GENERIC)
		{DBPRINTF(("NETPLAY:no capturebuffer,generic"));
		}
		if( hr == DSERR_NODRIVER)
		{DBPRINTF(("NETPLAY:no capturebuffer,nodriver"));
		}
		if( hr == DSERR_OUTOFMEMORY)
		{DBPRINTF(("NETPLAY:no capturebuffer,memory"));	
		}
		if( hr == DSERR_UNINITIALIZED)
		{DBPRINTF(("NETPLAY:no capturebuffer,uninit"));
		}
		return FALSE;
	}

	NetPlay.bAllowCaptureRecord = TRUE;
//	allowCapture = TRUE;
	return(TRUE);
}

// ////////////////////////////////////////////////////////////////////////
BOOL NETshutdownAudioCapture(VOID)
{
	NetPlay.bAllowCaptureRecord = FALSE;

	if(lpDirectSoundCaptureBuffer)
	{
		IDirectSoundCaptureBuffer_Release(lpDirectSoundCaptureBuffer);
	}
	
	if(lpDirectSoundCapture)
	{
		IDirectSoundCapture_Release(lpDirectSoundCapture);
	}
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////
BOOL NETstartAudioCapture(VOID)
{
	HRESULT	hr;

	if(!NetPlay.bAllowCaptureRecord)
	{
		return FALSE;
	}
	hr = IDirectSoundCaptureBuffer_Start(lpDirectSoundCaptureBuffer,DSCBSTART_LOOPING );//capture the sound
	if(hr!= DS_OK)
	{
		return FALSE;
	}

	sampling = TRUE;

	return TRUE;
}
// ////////////////////////////////////////////////////////////////////////
BOOL NETstopAudioCapture(VOID)
{
	HRESULT hr;
	if(!NetPlay.bAllowCaptureRecord)
	{
		return FALSE;
	}
	hr = IDirectSoundCaptureBuffer_Stop(lpDirectSoundCaptureBuffer);
	if(hr!= DS_OK)
	{
		return FALSE;
	}
	sampling = FALSE;

	// send any remaining stuff left in the buffer.
		
	sendAudioBuffer(TRUE);

	sendAudioComplete();

	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////
// update the pointers and process the buffer accordingly.
BOOL NETprocessAudioCapture(VOID)
{
	if(!NetPlay.bAllowCaptureRecord)
	{
		return FALSE;
	}
	if(sampling)
	{
		sendAudioBuffer(FALSE);
	}

	// Check Playback pointers HERE!

	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// Basic Sound Playback Functions.
// a simple circular buffer in which to place incoming net audio.


//open the dsound interface.
static BOOL setupSoundPlay(VOID)
{
	HWND		hwnd;
	HRESULT     hr;

	hwnd = frameGetWinHandle();

	hr =DirectSoundCreate(NULL, &lpDirectSound, NULL);							// create dsound object
	if (hr != DS_OK)
	{
		DBPRINTF(("NETPLAY Failed to create dsound interface.\n"));	
		return (FALSE);
	}

	hr =IDirectSound_SetCooperativeLevel(lpDirectSound, hwnd, DSSCL_PRIORITY);	// Set coop level
	if (hr != DS_OK)
	{
		DBPRINTF(("NETPLAY Failed to create playback buffer..\n"));	
		return (FALSE);
	}

	IDirectSound_Compact(lpDirectSound);										// compact things 
	return (TRUE);
}


//	init the playback buffer
static BOOL setupPlayBuffer(VOID)
{ 
    PCMWAVEFORMAT	pcmwf; 
    HRESULT			hr; 
  
    memset(&pcmwf, 0, sizeof(PCMWAVEFORMAT));									// Set up wave format structure. 
    pcmwf.wf.wFormatTag			= WAVE_FORMAT_PCM; 
    pcmwf.wf.nChannels			= AUDIOCHANNELS; 
    pcmwf.wf.nSamplesPerSec		= SAMPLERATE; 
    pcmwf.wf.nBlockAlign		= (AUDIOCHANNELS * BITSPERSAMPLE) / 8;
    pcmwf.wf.nAvgBytesPerSec	= SAMPLERATE * (BITSPERSAMPLE /8) * AUDIOCHANNELS  ;	//pcmwf.wf.nSamplesPerSec * pcmwf.wf.nBlockAlign; 
	pcmwf.wBitsPerSample		= BITSPERSAMPLE; 
  
    memset(&playbackBuff, 0, sizeof(DSBUFFERDESC));								// Set up DSBUFFERDESC structure. Zero it out. 
    playbackBuff.dwSize			= sizeof(DSBUFFERDESC); 
    playbackBuff.dwFlags		= DSBCAPS_CTRLFREQUENCY| DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME; //DSBCAPS_CTRLDEFAULT;							// Need default controls (pan, volume, frequency). 
    
    playbackBuff.dwBufferBytes	= SAMPLETIME * pcmwf.wf.nAvgBytesPerSec;		// SAMPLETIME-second buffer. 
    playbackBuff.lpwfxFormat= (LPWAVEFORMATEX)&pcmwf; 
    
    hr = lpDirectSound->lpVtbl->CreateSoundBuffer(lpDirectSound,				// Create buffer. 
        &playbackBuff, &lpDirectSoundBuffer, NULL); 
    
	if(hr != DS_OK)
	{
		DBPRINTF(("NETPLAY: Failed to create playback buffer.\n"));	
		lpDirectSoundBuffer = NULL;												// Failed. 
		return FALSE;
	}
	    
	return TRUE; 
}

// ////////////////////////////////////////////////////////////////////////

BOOL NETinitPlaybackBuffer(LPDIRECTSOUND pDs)
{
	NetPlay.bAllowCapturePlay	= FALSE;
	NetPlay.bCaptureInUse		= FALSE;

	if( pDs == NULL)						// init the pointers.
	{
		ourDSPointer= TRUE;
		if( FALSE == setupSoundPlay() )
		{
			return FALSE;
		}
	}
	else
	{
		ourDSPointer= FALSE;
		lpDirectSound = pDs;
	}

	if( FALSE == setupPlayBuffer() )		// setup play buffer.
	{
		return TRUE;
	}

	NetPlay.bAllowCapturePlay	= TRUE;
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////
static VOID stopIncomingAudio(VOID)
{
	LPVOID	lpvPtr1=NULL; 
    DWORD	dwBytes1=0; 
    HRESULT hr; 

	IDirectSoundBuffer_Stop(lpDirectSoundBuffer);

	hr = IDirectSoundBuffer_Lock(lpDirectSoundBuffer,	// clear out and start again
								0,					
								0,	
								&lpvPtr1,&dwBytes1,
								NULL,NULL,
								DSBLOCK_ENTIREBUFFER); 
	if(DS_OK == hr) 
	{     	
	    ZeroMemory( ((UBYTE *)lpvPtr1),playbackBuff.dwBufferBytes);	// remove old stuff
	
		hr = IDirectSoundBuffer_Unlock(lpDirectSoundBuffer, lpvPtr1, dwBytes1, 0,0);			//clear buffer.
	}

	IDirectSoundBuffer_SetCurrentPosition(lpDirectSoundBuffer,0);								// reset the play position

}

// ////////////////////////////////////////////////////////////////////////
// handle the playback buffer.
BOOL NETqueueIncomingAudio( LPBYTE lpbSoundData, DWORD dwSoundBytes,BOOL bStream)
{ 
    LPVOID lpvPtr1,lpvPtr2=NULL; 
    DWORD dwBytes1,dwBytes2=0; 
	

    HRESULT hr; 

	if(!NetPlay.bAllowCapturePlay)
	{
		return FALSE;
	}

    // Obtain memory address of write block. This will be in two parts
    // if the block wraps around.
	if(!bStream)
	{
		// clear out and start again
		hr = IDirectSoundBuffer_Lock(lpDirectSoundBuffer,
									0,			//offset
									dwSoundBytes,		//size (ignored)
									&lpvPtr1,&dwBytes1,
									NULL,NULL,
									DSBLOCK_ENTIREBUFFER); 
	}
	else	// stream nicely
	{

		hr = IDirectSoundBuffer_Lock(lpDirectSoundBuffer,
									0,			//offset
									dwSoundBytes,		//size
									&lpvPtr1,&dwBytes1,
									&lpvPtr2,&dwBytes2,
									DSBLOCK_FROMWRITECURSOR ); 
	}
    
	if(DS_OK == hr) 
	{     	
		if(bStream)
		{	
			// clear out unwanted stuff.
			CopyMemory(lpvPtr1, lpbSoundData, dwBytes1); // Write to pointers. 
			if (NULL != lpvPtr2)
			{ 
				CopyMemory(lpvPtr2, (lpbSoundData+dwBytes1), dwBytes2);
			}
		}	
		else	// not streaming
		{
			CopyMemory(lpvPtr1, lpbSoundData, dwSoundBytes);
	        ZeroMemory( ((UBYTE *)lpvPtr1)+dwSoundBytes,(playbackBuff.dwBufferBytes)-dwSoundBytes);	// remove old stuff
		}		

		hr = IDirectSoundBuffer_Unlock(lpDirectSoundBuffer, lpvPtr1, dwBytes1, lpvPtr2,dwBytes2); 
		if(DS_OK == hr)																		// Success. 
		{
			IDirectSoundBuffer_Play(lpDirectSoundBuffer,0,0,0); //DSBPLAY_LOOPING );								// now play the sound         
			if(hr == DS_OK)
			{
				return TRUE;
			}
			DBERROR(("NETPLAY: failed to play incoming sample"));
        } 
    } 
    return FALSE; 
}



// ////////////////////////////////////////////////////////////////////////
// Handle a incoming message that needs to be played
void NETplayIncomingAudio(NETMSG *pMsg)
{	
	if(!NetPlay.bAllowCapturePlay)
	{
		return;
	}
#if 1
	if(pMsg->size ==1)
	{
		stopIncomingAudio();
		NetPlay.bCaptureInUse = FALSE;
	}
	else
	{
		NETqueueIncomingAudio(pMsg->body, pMsg->size,TRUE);		// use the circular buffer...
		NetPlay.bCaptureInUse = TRUE;
	}


#else
	// dont queue. crappy old method.
	stopIncomingAudio();
	IDirectSoundBuffer_SetCurrentPosition(lpDirectSoundBuffer,0);	// reset the play position
	NETqueueIncomingAudio(pMsg->body, pMsg->size,FALSE);		// don't use the circular buffer...
#endif
	
}

// ////////////////////////////////////////////////////////////////////////
// close it all down
BOOL NETshutdownAudioPlayback(VOID)
{

	if(lpDirectSoundCaptureBuffer)
	{
		IDirectSoundBuffer_Release(lpDirectSoundBuffer);
	}
	
	if(lpDirectSoundCapture && ourDSPointer )
	{
		IDirectSound_Release(lpDirectSound);
	}

	NetPlay.bCaptureInUse = FALSE;
	NetPlay.bAllowCapturePlay = FALSE;
	
	return TRUE;
}


