/***************************************************************************/
/*
 * Sequence.c
 *
 * Sequence setup and video control
 *
 * based on eidos example code
 *
 *
 */
/***************************************************************************/

// Standard include file
#include "frame.h"
#include "sequence.h"

// Direct Draw and Sound Include files
#include <ddraw.h>
#define SEQUENCE_SOUND
#ifdef SEQUENCE_SOUND
	#include <dsound.h>
#endif
//#define CRAP_VIDEO

 
// ESCAPE include file
#include "streamer.h"


/***************************************************************************/
/*
 *	Local Definitions
 */
/***************************************************************************/

typedef struct _aStruct
{
	BOOL	bBool;
} A_STRUCT;

#define TEXT_BOXES
#define VIDEO_WIDTH 640
#define VIDEO_HEIGHT 480


/***************************************************************************/
/*
 *	local Variables
 */
/***************************************************************************/

/* 
	The following variables/structures have scope over the entire module as the 
	sound callback function needs access to them,  as well as the the main() 
	function. 
*/

int					LastUpdated;
LPMOVIEHANDLE		mhandle = NULL;
LPVIDEOHANDLE		vhandle = NULL;
LPSOUNDHANDLE		shandle = NULL;
LPBYTE				localBuffer;
int					movieWidth, movieHeight;

LPDIRECTSOUNDBUFFER	lpDSSB = NULL;
LPBYTE				soundbuffer1 = NULL;
LPBYTE				soundbuffer2 = NULL;
ULONG				temp;
UDWORD				lowBitMask;
BOOL				bPlayerOn;
BOOL				bSmallVideo = FALSE;
BOOL				bTextBoxes = FALSE;
/***************************************************************************/
/*
 *	Local ProtoTypes
 */
/***************************************************************************/

void SoundCallBackFunc( LPSOUNDHANDLE shandle );

/***************************************************************************/
/*
 *	Source
 */
/***************************************************************************/

//buffer render for software_window 3DFX_window and 3DFX_fullscreen modes
// SOFT_WINDOWED video size 16bit rgb 555 mode (convert to 8bit from the buffer)
// D3D_WINDOWED video size 16bit uses screen pixel mode
// 3DFX_WINDOWED video size 16bit BGR 565 mode
// 3DFX_FULLSCREEN 640 * 480 BGR 565 mode
BOOL seq_SetSequenceForBuffer(char* filename, VIDEO_MODE mode, LPDIRECTSOUND lpDS, int startTime, DDPIXELFORMAT	*DDPixelFormat, PERF_MODE perfMode)
{
	long				width, height;
	long				pixelBitDepth = 16;
	long				precision, channels;
	long				compression;
	BOOL				bCompression;
	DSBUFFERDESC		DS_bd;
//	DDPIXELFORMAT		DDPixelFormat;
	WAVEFORMATEX		pcmwf;
	BYTE				ap = 0,	ac = 0, rp = 0,	rc = 0, gp = 0,	gc = 0, bp = 0, bc = 0;
	ULONG				mask;

	mhandle = NULL;
	vhandle = NULL;
	shandle = NULL;
	lpDSSB = NULL;

	if (perfMode == VIDEO_PERF_FULLSCREEN)
	{
		bSmallVideo = FALSE;
	}
	else
	{
		bSmallVideo = TRUE;
	}

	/*
	// Initialise the movie with a 2MB buffersize
	*/
	if ( Streamer_InitMovie(	&mhandle,
								NULL,
								0,
								filename,
								2<<20,
								SIM_NONE) != STREAMER_OK ) return FALSE;

	movieWidth = Movie_GetXSize( mhandle );
	movieHeight = Movie_GetYSize( mhandle );

	/*
	// Initialise the video playback environment
	*/
	//		Streamer_InitVideo( 	LPVIDEOHANDLE *handle,LPMOVIEHANDLE mhandle,UINT moviexsize,UINT movieysize,INT videoleft,INT videotop,INT viewportleft,INT viewporttop,UINT viewportwidth,UINT viewportheight,UINT properties,LPLONG  bufferPixelWidth,LPLONG  bufferPixelDepth)

	if (mode == VIDEO_3DFX_FULLSCREEN)
	{
		if (((movieWidth <= (VIDEO_WIDTH/2)) && (movieHeight <= (VIDEO_HEIGHT/2))) && !bSmallVideo)//render doubled
		{
			if ( Streamer_InitVideo(	&vhandle,
										mhandle,
										movieWidth,
										movieHeight,
										0,
										0,
										0,
										0,
										0,
										0,
										DFLAG_INVIEWPORT | DFLAG_DOUBLED,// | DFLAG_CLEAROUTPUT | //DFLAG_DOUBLED | 
										&width,
										&height ) != STREAMER_OK ) return FALSE;
			movieWidth *= 2;
			movieHeight *= 2;
			Streamer_SetVideoPitch(vhandle,movieWidth,movieHeight);
		}
		else
		{
			if ( Streamer_InitVideo(	&vhandle,
										mhandle,
										movieWidth,
										movieHeight,
										0,
										0,
										0,
										0,
										0,
										0,
										DFLAG_INVIEWPORT,// | DFLAG_CLEAROUTPUT | //DFLAG_DOUBLED | 
										&width,
										&height ) != STREAMER_OK ) return FALSE;
			Streamer_SetVideoPitch(vhandle,movieWidth,movieHeight);
		}
	}
	else //if ((mode == VIDEO_SOFT_WINDOW) || (mode == VIDEO_D3D_WINDOW) || (mode == VIDEO_3DFX_WINDOW))
	{
		if ( Streamer_InitVideo(	&vhandle,
									mhandle,
									movieWidth,
									movieHeight,
									0,
									0,
									0,
									0,
									0,
									0,
									DFLAG_INVIEWPORT,// | DFLAG_DOUBLED, | DFLAG_CLEAROUTPUT | //DFLAG_DOUBLED | 
									&width,
									&height ) != STREAMER_OK ) return FALSE;
		Streamer_SetVideoPitch(vhandle,movieWidth,movieHeight);
	}


#ifdef SEQUENCE_SOUND
	// check for the presence of audio within the rpl file
	if (	Movie_GetSoundChannels( mhandle ) &&
			Movie_GetSoundPrecision( mhandle ) &&
			Movie_GetSoundRate( mhandle ) &&
			(lpDS != NULL))
	{
		//if so initialise Direct sound playback
		precision = Movie_GetSoundPrecision( mhandle);
		channels = Movie_GetSoundChannels( mhandle );
		if (precision == 4)
		{
			compression = 4;
			bCompression = FALSE;
		}
		else
		{
			compression = 1;
			bCompression = TRUE;
		}

		if ( Streamer_InitSound(	SoundCallBackFunc,
									&shandle,
									16384,
									compression,
									bCompression,
									4096,
									channels ) != STREAMER_OK ) return FALSE;
		// Attempt to create an instance of direct sound - 
		// for information on DirectSound refer to Microsoft documentation

		memset( &pcmwf, 0, sizeof(pcmwf));

		pcmwf.wFormatTag = WAVE_FORMAT_PCM;
		pcmwf.nChannels = (WORD) Movie_GetSoundChannels( mhandle );
		pcmwf.nSamplesPerSec = Movie_GetSoundRate( mhandle );
		pcmwf.wBitsPerSample = 16;					// there are always 16 bits after ADPCM is decompressed or if its raw PCM
		pcmwf.nBlockAlign = (pcmwf.nChannels * pcmwf.wBitsPerSample)/8;
		pcmwf.nAvgBytesPerSec = pcmwf.nSamplesPerSec * pcmwf.nBlockAlign;
		pcmwf.cbSize = 0;

		memset( &DS_bd, 0, sizeof( DS_bd ));
		DS_bd.dwSize = sizeof ( DS_bd );
		DS_bd.dwFlags = DSBCAPS_CTRLFREQUENCY| DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME; //DSBCAPS_CTRLDEFAULT;
		DS_bd.dwBufferBytes = 32768;
		DS_bd.lpwfxFormat = &pcmwf;

		if ( lpDS->lpVtbl->CreateSoundBuffer(lpDS,
										&DS_bd,
										&lpDSSB,
										NULL ) != DS_OK) return FALSE;

		if ( lpDSSB->lpVtbl->Lock(lpDSSB,
									0,
									16384,
									(void**)&soundbuffer1,
									&temp,
									NULL,
									0,
									0 ) != DS_OK ) return FALSE;

		soundbuffer2 = soundbuffer1+16384;

		Streamer_SetSoundBuffer( shandle, 0, soundbuffer1 );
		Streamer_SetSoundBuffer( shandle, 1, soundbuffer2 );
	}
#endif //SEQUENCE_SOUND
	

	/*
	// This does the preload of movie data from the file and fills the audio double buffers (which
	//  is why they were locked)
	*/
	if ( Streamer_InitStreaming(	mhandle,
									vhandle,
									shandle ) != STREAMER_OK ) return FALSE;
	

#ifdef SEQUENCE_SOUND
	if ( shandle )
	{
		lpDSSB->lpVtbl->Unlock(lpDSSB,
						soundbuffer1,
						16384,
						NULL,
						0 );

//jps mar4		Streamer_SetSoundDecodeMode( shandle, SSDM_SECONDBUFFER );	
		Streamer_SetSoundDecodeMode( shandle, SSDM_IDLE	);	
	}
	LastUpdated = SSDM_SECONDBUFFER;
#endif //SEQUENCE_SOUND

	/*
	// Cannot playback if not 16bit mode 
	*/
	if (mode == VIDEO_SOFT_WINDOW)
	{
		//555 RGB
		ap = 15;
		ac = 1;
		rp = 10;
		rc = 5;
		gp = 5;
		gc = 5;
		bp = 0;
		bc = 5;
	}
	else if ((mode == VIDEO_3DFX_FULLSCREEN) || (mode == VIDEO_3DFX_WINDOW)) 
	{
		// 565 BGR
		ap = 16;
		ac = 0;
		rp = 0;
		rc = 5;
		gp = 5;
		gc = 6;
		bp = 11;
		bc = 5;
	}
	else if (mode == VIDEO_D3D_WINDOW)
	{
		/*
		// Cannot playback if not 16bit mode 
		*/
		if( DDPixelFormat->dwRGBBitCount == 16 )
		{
			/*
			// Find out the RGB type of the surface and tell the codec...
			*/
			mask = DDPixelFormat->dwRGBAlphaBitMask;

			if(mask!=0)
			{
				while(!(mask & 1))
				{
					mask>>=1;
					ap++;
				}
			}

			while((mask & 1))
			{
				mask>>=1;
				ac++;
			}

			mask = DDPixelFormat->dwRBitMask;

			if(mask!=0)
			{
				while(!(mask & 1))
				{
					mask>>=1;
					rp++;
				}
			}

			while((mask & 1))
			{
				mask>>=1;
				rc++;
			}

			mask = DDPixelFormat->dwGBitMask;

			if(mask!=0)
			{
				while(!(mask & 1))
				{
					mask>>=1;
					gp++;
				}
			}

			while((mask & 1))
			{
				mask>>=1;
				gc++;
			}

			mask = DDPixelFormat->dwBBitMask;

			if(mask!=0)
			{
				while(!(mask & 1))
				{
					mask>>=1;
					bp++;
				}
			}

			while((mask & 1))
			{
				mask>>=1;
				bc++;
			}
		}
		else
		{
			//assume 555
			ap = 15;
			ac = 1;
			rp = 10;
			rc = 5;
			gp = 5;
			gc = 5;
			bp = 0;
			bc = 5;
		}
		lowBitMask = 0xffff - (1<<bp) - (1<<gp) - (1<<rp);
		if (ac)
		{
			lowBitMask -= (1<<ap);
		}
	}
	// Set the video pixel RGB format
	if ( Streamer_SetPixelFormat( vhandle, SPF_BPP16, ap, ac, rp, rc, gp, gc, bp, bc ) != STREAMER_OK )
		return FALSE;

#ifdef SEQUENCE_SOUND
	if ( shandle )
	{
		// Begin sound playback
//mar4		if ( lpDSSB->lpVtbl->Play(lpDSSB, 0, 0, 0) != DS_OK )
		if ( lpDSSB->lpVtbl->Play(lpDSSB, 0, 0, DSBPLAY_LOOPING) != DS_OK )
				return VIDEO_SOUND_ERROR;
	}
#endif //SEQUENCE_SOUND

	return TRUE;

}

/*
 * directX fullscreeen render uses local buffer to store previous frame data
 * directX 640 * 480 16bit rgb mode render through local buffer to back buffer
 */
BOOL seq_SetSequence(char* filename, LPDIRECTDRAWSURFACE4 lpDDSF, LPDIRECTSOUND lpDS, int startTime, char* lpBF, PERF_MODE perfMode)
{
	long				width, height;
	long				pixelBitDepth;
	long				precision, channels;
	long				compression;
	BOOL				bCompression;

	DDSURFACEDESC2		DD_sd;
	DDPIXELFORMAT		DDPixelFormat;
	ULONG				mask;
	BYTE				ap = 0,	ac = 0, rp = 0,	rc = 0, gp = 0,	gc = 0, bp = 0, bc = 0;

	DSBUFFERDESC		DS_bd;
	WAVEFORMATEX		pcmwf;

	mhandle = NULL;
	vhandle = NULL;
	shandle = NULL;
	lpDSSB == NULL;

	if (perfMode == VIDEO_PERF_FULLSCREEN)
	{
		bSmallVideo = FALSE;
	}
	else
	{
		bSmallVideo = TRUE;
	}


		//get display mode	
		DD_sd.dwSize = sizeof( DD_sd );
		if ( lpDDSF->lpVtbl->GetSurfaceDesc(lpDDSF, &DD_sd ) != DD_OK )
			return FALSE;
		pixelBitDepth = DD_sd.ddpfPixelFormat.dwRGBBitCount;
		/*
		// Initialise the movie with a 2MB buffersize
		*/
		if ( Streamer_InitMovie(	&mhandle,
									NULL,
									0,
									filename,
									2<<20,
									SIM_NONE) != STREAMER_OK ) return FALSE;

		movieWidth = Movie_GetXSize( mhandle );
		movieHeight = Movie_GetYSize( mhandle );

		//malloc a buffer for video playback
		localBuffer = lpBF;//always 16bit
		memset(localBuffer, 0, VIDEO_WIDTH * VIDEO_HEIGHT * sizeof(WORD));
		/*
		// Initialise the video playback environment
		*/
//		Streamer_InitVideo( 	LPVIDEOHANDLE *handle,LPMOVIEHANDLE mhandle,UINT moviexsize,UINT movieysize,INT videoleft,INT videotop,INT viewportleft,INT viewporttop,UINT viewportwidth,UINT viewportheight,UINT properties,LPLONG  bufferPixelWidth,LPLONG  bufferPixelDepth)
		if (((movieWidth <= (VIDEO_WIDTH/2)) && (movieHeight <= (VIDEO_HEIGHT/2))) && !bSmallVideo)//render doubled
		{
			if ( Streamer_InitVideo(	&vhandle,
										mhandle,
										movieWidth,
										movieHeight,
										0,
										0,
										0,
										0,
										0,
										0,
										DFLAG_INVIEWPORT | DFLAG_DOUBLED,// | DFLAG_CLEAROUTPUT | //DFLAG_DOUBLED | 
										&width,
										&height ) != STREAMER_OK ) return FALSE;
			movieWidth *= 2;
			movieHeight *= 2;
		}
		else
		{
			if ( Streamer_InitVideo(	&vhandle,
										mhandle,
										movieWidth,
										movieHeight,
										0,
										0,
										0,
										0,
										0,
										0,
										DFLAG_INVIEWPORT,// | DFLAG_CLEAROUTPUT | //DFLAG_DOUBLED | 
										&width,
										&height ) != STREAMER_OK ) return FALSE;
		}
		if ( Streamer_SetVideoPitch(vhandle,movieWidth,movieHeight) != STREAMER_OK) return FALSE;

#ifdef SEQUENCE_SOUND
	// check for the presence of audio
	if (	Movie_GetSoundChannels( mhandle ) &&
			Movie_GetSoundPrecision( mhandle ) &&
			Movie_GetSoundRate( mhandle ) &&
			(lpDS != NULL))
	{
		//if so initialise Direct sound playback
		precision = Movie_GetSoundPrecision( mhandle);
		channels = Movie_GetSoundChannels( mhandle );
		if (precision == 4)
		{
			compression = 4;
			bCompression = FALSE;
		}
		else
		{
			compression = 1;
			bCompression = TRUE;
		}

		if ( Streamer_InitSound(	SoundCallBackFunc,
									&shandle,
									16384,
									compression,
									bCompression,
									4096,
									channels ) != STREAMER_OK ) return FALSE;
		// Attempt to create an instance of direct sound - 
		// for information on DirectSound refer to Microsoft documentation

		memset( &pcmwf, 0, sizeof(pcmwf));

		pcmwf.wFormatTag = WAVE_FORMAT_PCM;
		pcmwf.nChannels = (WORD) Movie_GetSoundChannels( mhandle );
		pcmwf.nSamplesPerSec = Movie_GetSoundRate( mhandle );
		pcmwf.wBitsPerSample = 16;					// there are always 16 bits after ADPCM is decompressed or if its raw PCM
		pcmwf.nBlockAlign = (pcmwf.nChannels * pcmwf.wBitsPerSample)/8;
		pcmwf.nAvgBytesPerSec = pcmwf.nSamplesPerSec * pcmwf.nBlockAlign;
		pcmwf.cbSize = 0;

		memset( &DS_bd, 0, sizeof( DS_bd ));
		DS_bd.dwSize = sizeof ( DS_bd );
		DS_bd.dwFlags = DSBCAPS_CTRLFREQUENCY| DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME; //DSBCAPS_CTRLDEFAULT;
		DS_bd.dwBufferBytes = 32768;
		DS_bd.lpwfxFormat = &pcmwf;

		bPlayerOn = FALSE;
		
		if ( lpDS->lpVtbl->CreateSoundBuffer(lpDS,
										&DS_bd,
										&lpDSSB,
										NULL ) != DS_OK) return FALSE;


		if ( lpDSSB->lpVtbl->Lock(lpDSSB,
							0,
							32768,
							(void**)&soundbuffer1,
							&temp,
							NULL,
							0,
							0 ) != DS_OK ) return FALSE;

		soundbuffer2 = soundbuffer1+16384;

		Streamer_SetSoundBuffer( shandle, 0, soundbuffer1 );
		Streamer_SetSoundBuffer( shandle, 1, soundbuffer2 );
	}
#endif //SEQUENCE_SOUND

		/*
		// This does the preload of movie data from the file and fills the audio double buffers (which
		//  is why they were locked)
		*/
		if ( Streamer_InitStreaming(	mhandle,
										vhandle,
										shandle ) != STREAMER_OK ) return FALSE;
		

#ifdef SEQUENCE_SOUND
	if ( shandle )
	{
		lpDSSB->lpVtbl->Unlock(lpDSSB,
						soundbuffer1,
						16384,
						NULL,
						0 );

//jps mar4		Streamer_SetSoundDecodeMode( shandle, SSDM_SECONDBUFFER );	
		Streamer_SetSoundDecodeMode( shandle, SSDM_IDLE );	
	}
	LastUpdated = SSDM_SECONDBUFFER;
#endif //SEQUENCE_SOUND
		
		DDPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		if ( lpDDSF->lpVtbl->GetPixelFormat(lpDDSF, &DDPixelFormat ) != DD_OK ) return FALSE;

		/*
		// Cannot playback if not 16bit mode 
		*/
		if( DDPixelFormat.dwRGBBitCount == 16 )
		{
			/*
			// Find out the RGB type of the surface and tell the codec...
			*/
			mask = DDPixelFormat.dwRGBAlphaBitMask;

			if(mask!=0)
			{
				while(!(mask & 1))
				{
					mask>>=1;
					ap++;
				}
			}

			while((mask & 1))
			{
				mask>>=1;
				ac++;
			}

			mask = DDPixelFormat.dwRBitMask;

			if(mask!=0)
			{
				while(!(mask & 1))
				{
					mask>>=1;
					rp++;
				}
			}

			while((mask & 1))
			{
				mask>>=1;
				rc++;
			}

			mask = DDPixelFormat.dwGBitMask;

			if(mask!=0)
			{
				while(!(mask & 1))
				{
					mask>>=1;
					gp++;
				}
			}

			while((mask & 1))
			{
				mask>>=1;
				gc++;
			}

			mask = DDPixelFormat.dwBBitMask;

			if(mask!=0)
			{
				while(!(mask & 1))
				{
					mask>>=1;
					bp++;
				}
			}

			while((mask & 1))
			{
				mask>>=1;
				bc++;
			}
		}
		else
		{
			//assume 555
			ap = 15;
			ac = 1;
			rp = 10;
			rc = 5;
			gp = 5;
			gc = 5;
			bp = 0;
			bc = 5;
		}
		lowBitMask = 0xffff - (1<<bp) - (1<<gp) - (1<<rp);
		if (ac)
		{
			lowBitMask -= (1<<ap);
		}
		/*
		// Set the video pixel RGB format
		*/
		if ( Streamer_SetPixelFormat( vhandle, SPF_BPP16, ap, ac, rp, rc, gp, gc, bp, bc ) != STREAMER_OK )
			return FALSE;

#ifdef SEQUENCE_SOUND
		if ( shandle )
		{
			// Begin sound playback
//mar4			if ( lpDSSB->lpVtbl->Play(lpDSSB, 0, 0, 0) != DS_OK )
			if ( lpDSSB->lpVtbl->Play(lpDSSB, 0, 0, DSBPLAY_LOOPING) != DS_OK )
					return VIDEO_SOUND_ERROR;
		}
#endif //SEQUENCE_SOUND

	return TRUE;

}

int seq_ClearMovie(void)
{
	/* Shutdown Sound, Video and Movie*/
	Streamer_ShutDownSound(	&shandle );
	Streamer_ShutDownVideo(	&vhandle );
	Streamer_ShutDownMovie(	&mhandle );
	shandle == NULL;
	vhandle == NULL;
	mhandle == NULL;
	return TRUE;
}


/*
 * buffer render for software_window 3DFX_window and 3DFX_fullscreen modes
 * SOFT_WINDOWED video size 16bit rgb 555 mode (convert to 8bit from the buffer)
 * 3DFX_WINDOWED video size 16bit BGR 565 mode
 * 3DFX_FULLSCREEN 640 * 480 BGR 565 mode
 */
int	seq_RenderOneFrameToBuffer(char *lpSF, int skip, SDWORD subMin, SDWORD subMax)
{
	STRESULT			sRes	= STREAMER_OK;
	int					frame;
	DWORD	CurrentPos, CurrentWritePos;
	WORD	*blitSrc;
	SDWORD	i, j;
	SDWORD	subYstart, subYend, subXstart, subXend;
	WORD	pixel;

	if ((mhandle == NULL) || (vhandle == NULL))
	{
		return VIDEO_FRAME_ERROR;
	}

	bTextBoxes = FALSE;
#ifdef TEXT_BOXES
	if ((!bSmallVideo) && (subMin <= subMax))
	{
		bTextBoxes = TRUE;
	}
#endif

	/*
	// Calculate how far through the movie we should be based on the time elasped since playback started
	// If we are not going to decode audio and video then call Streamer_Stream() with a frames to play setting of 0
	// this will replenish the cyclic buffer if it is needed.
	*/
	/*
	// Stream one frame of video to the surface
	*/
	sRes = Streamer_Stream(	mhandle,
							vhandle,
							shandle,
							NULL,
							skip,
							lpSF,
							NULL,
							NULL,
							0 );

	frame = Movie_GetCurrentFrame(mhandle);

#ifdef TEXT_BOXES	
	//fade region for sequences
	if (bTextBoxes)
	{
		subYstart = subMin - 10;
		subYend = subMax + 6;
		subXstart = 14;
		subXend = 624;

		if (subYstart < 0)
		{
			subYstart = 0;
		}
		if (subYend > VIDEO_HEIGHT)
		{
			subYend = VIDEO_HEIGHT;
		}

		
		blitSrc = (WORD*)lpSF + movieWidth * subYstart;
		for (j = subYstart;j < subYend;j++)
		{
			for (i = 0;i <  movieWidth;i++)
			{
				pixel = blitSrc[i];
				if ((i > subXstart) && (i < subXend))
				{
					pixel &=lowBitMask;
					pixel >>= 1;
					pixel += 0x2;
				}
				blitSrc[i] = pixel; 
			}
			blitSrc += movieWidth;
		}
	}	
#endif


#ifdef SEQUENCE_SOUND
	if ( shandle )
	{
		
		if ( Streamer_GetSoundDecodeMode( shandle ) == SSDM_IDLE )
		{
			
			if (lpDSSB->lpVtbl->GetCurrentPosition(lpDSSB, &CurrentPos, &CurrentWritePos ) != DS_OK)		
				return VIDEO_SOUND_ERROR;

			if(( LastUpdated==SSDM_SECONDBUFFER )&&( CurrentPos > 16384 ))
			{
				Streamer_SetSoundDecodeMode( shandle, SSDM_FIRSTBUFFER );

				lpDSSB->lpVtbl->Lock(lpDSSB,
								0,
								16384,
								(void**)&soundbuffer1,
								&temp,
								NULL,
								0,
								0 );
			}
			else if(( LastUpdated==SSDM_FIRSTBUFFER )&&( CurrentPos < 16384 ))
			{
				Streamer_SetSoundDecodeMode( shandle, SSDM_SECONDBUFFER );

				lpDSSB->lpVtbl->Lock(lpDSSB,
								16384,
								16384,
								(void**)&soundbuffer2,
								&temp,
								NULL,
								0,
								0 );
			}
		}
	}
//mar4	lpDSSB->lpVtbl->Play(lpDSSB, 0, 0, DSBPLAY_LOOPING);
#endif //SEQUENCE_SOUND

	if (sRes == STREAMER_OK)
	{
		return frame;
	}
	else
	{
		return VIDEO_FINISHED;
	}
}


/*
 * render one frame to a direct draw surface (normally the back buffer)
 * directX 640 * 480 16bit rgb mode render through local buffer to back buffer
 */
int	seq_RenderOneFrame(LPDIRECTDRAWSURFACE4	lpDDSF, int skip, SDWORD subMin, SDWORD subMax)
{
	DDSURFACEDESC2	DD_sd; 
	HRESULT			hRes;
	STRESULT		sRes = STREAMER_OK;
	WORD	*blitDest, *blitSrc;
	int		i, j;
	WORD	pixel;
	BOOL	bDoubled;
	int		frame;
	DWORD	CurrentPos, CurrentWritePos;
	SDWORD	borderX, borderY;
	SDWORD	subYstart, subYend, subXstart, subXend;

	if ((mhandle == NULL) || (vhandle == NULL))
	{
		return VIDEO_FRAME_ERROR;
	}

	
	if (bPlayerOn == FALSE)
	{
		bPlayerOn = TRUE;
	}

	bTextBoxes = FALSE;
#ifdef TEXT_BOXES
	if ((!bSmallVideo) && (subMin <= subMax))
	{
		bTextBoxes = TRUE;
	}
#endif

	// Calculate how far through the movie we should be based on the time elasped since playback started
	// If we are not going to decode audio and video then call Streamer_Stream() with a frames to play setting of 0
	// this will replenish the cyclic buffer if it is needed.

	sRes = Streamer_Stream(	mhandle,
							vhandle,
							shandle,
							NULL,
							skip,
							localBuffer,
							NULL,
							NULL,
							0 );

	frame = Movie_GetCurrentFrame(mhandle);
	if (sRes == STREAMER_FINISHEDAUDIO)
	{
		DBPRINTF(("STREAMER_FINISHEDAUDIO %d\n",sRes));
		shandle = NULL;
		sRes = STREAMER_OK;
	}
	else if (sRes != STREAMER_OK)
	{
		DBPRINTF(("STREAMER_STREAM ERROR %d %d\n",sRes,frame));
	}

	// We lock the surface before blitting video to it.
	DD_sd.dwSize = sizeof( DD_sd );
	if ( lpDDSF->lpVtbl->GetSurfaceDesc(lpDDSF, &DD_sd ) != DD_OK )
	return FALSE;
	
	hRes = lpDDSF->lpVtbl->Lock(lpDDSF, NULL, &DD_sd,DDLOCK_WAIT, NULL);
	if (hRes != DD_OK)
	{
		DBERROR(("Sequence player back  buffer lock failed:\n%s", DDErrorToString(hRes)));
		return VIDEO_SURFACE_ERROR;
	}

	if (hRes == DD_OK)
	{
		if (!bSmallVideo)
		{
			borderX = ((SDWORD)DD_sd.dwWidth - VIDEO_WIDTH)/2;
			borderY = ((SDWORD)DD_sd.dwHeight - VIDEO_HEIGHT)/4;
		}
		else
		{
			borderX = ((SDWORD)DD_sd.dwWidth - VIDEO_WIDTH/2)/2;
			borderY = ((SDWORD)DD_sd.dwHeight - VIDEO_HEIGHT/2)/4;
		}
		
		blitDest = (WORD*)DD_sd.lpSurface + borderX + borderY * DD_sd.lPitch; 
		bDoubled = FALSE;
		if ((movieWidth * 2) <= (int)DD_sd.dwWidth)
		{
			if ((movieHeight * 2) <= (int)DD_sd.dwHeight)
			{
				if (DD_sd.ddpfPixelFormat.dwRGBBitCount == 16)
				{
					if (!bSmallVideo)
					{
						bDoubled = TRUE;
					}
				}
			}
		}
		blitSrc = (WORD*)localBuffer;

		if (bTextBoxes)
		{
			subYstart = subMin - 10;//only ever in fullscreen mode
			subYend = subMax + 6;
			subXstart = 14;
			subXend = 624;
			
			if (subYstart < 0)
			{
				subYstart = 0;
			}
			if (subYend > VIDEO_HEIGHT)
			{
				subYend = VIDEO_HEIGHT;
			}
		}
		else
		{
			subYstart = movieHeight;
			subYend = movieHeight;
			subXstart = movieWidth;
			subXend = movieWidth;
		}


		//blit videoBuffer to ddSurface
		for (j = 0;j <  subYstart;j++)
		{
			for (i = 0;i <  movieWidth;i++)
			{
				pixel = blitSrc[i]; 
				blitDest[i] = pixel; 
			}
			blitSrc += movieWidth;
			((LPBYTE)blitDest) += DD_sd.lPitch;
		}
		if (bTextBoxes)
		{	
			for (j = subYstart;j < subYend;j++)
			{
				for (i = 0;i <  movieWidth;i++)
				{
					pixel = blitSrc[i];
					if ((i > subXstart) && (i < subXend))
					{
						pixel &=lowBitMask;
						pixel >>= 1;
						pixel += 0x2;
					}
					blitDest[i] = pixel; 
				}
				blitSrc += movieWidth;
				((LPBYTE)blitDest) += DD_sd.lPitch;
			}
			for (j = subYend;j <  movieHeight;j++)
			{
				for (i = 0;i <  movieWidth;i++)
				{
					pixel = blitSrc[i]; 
					blitDest[i] = pixel; 
				}
				blitSrc += movieWidth;
				((LPBYTE)blitDest) += DD_sd.lPitch;
			}
		}
	}

	// We can unlock the suurface now as we have finished with it, 
	// until the next decode is required
	lpDDSF->lpVtbl->Unlock(lpDDSF, DD_sd.lpSurface );
	

#ifdef SEQUENCE_SOUND
	if ( shandle )
	{
		
		if ( Streamer_GetSoundDecodeMode( shandle ) == SSDM_IDLE )
		{
			
			if (lpDSSB->lpVtbl->GetCurrentPosition(lpDSSB, &CurrentPos, &CurrentWritePos ) != DS_OK)		
				return VIDEO_SOUND_ERROR;

			if(( LastUpdated==SSDM_SECONDBUFFER )&&( CurrentPos > 16384 ))
			{
				Streamer_SetSoundDecodeMode( shandle, SSDM_FIRSTBUFFER );

				lpDSSB->lpVtbl->Lock(lpDSSB,
								0,
								16384,
								(void**)&soundbuffer1,
								&temp,
								NULL,
								0,
								0 );
			}
			else if(( LastUpdated==SSDM_FIRSTBUFFER )&&( CurrentPos < 16384 ))
			{
				Streamer_SetSoundDecodeMode( shandle, SSDM_SECONDBUFFER );

				lpDSSB->lpVtbl->Lock(lpDSSB,
								16384,
								16384,
								(void**)&soundbuffer2,
								&temp,
								NULL,
								0,
								0 );
			}
		}
	}
//	lpDSSB->lpVtbl->Play(lpDSSB, 0, 0, DSBPLAY_LOOPING);
#endif //SEQUENCE_SOUND

	if (sRes == STREAMER_OK)
	{
		return frame;
	}
	else
	{
		return VIDEO_FINISHED;
	}
}

BOOL	seq_RefreshVideoBuffers(void)
{
	STRESULT		sRes;
	int		badDataCount;
	DWORD	CurrentPos, CurrentWritePos;

	if ((mhandle == NULL) || (vhandle == NULL))
	{
		return FALSE;
	}

	// Here we attempt to replenish the cyclic buffer but asking 
	//   streamer_stream() to stream zero frames,  this should be 
	//   done when it is not yet time to decode more video 
	/* This ensures that there is sufficient data in the buffer
	   to satisfy the streamer */
	/* keep trying until the bad data rate problem disappears,
	   through sufficient data in buffer */
	badDataCount = 0;
	do
	{
		sRes = Streamer_Stream(	mhandle,
								vhandle,
								shandle,
								NULL,
								0,
								NULL, 
								NULL,
								NULL,
								0 );
		badDataCount++;
	}
	while (( sRes == STREAMER_BADDATARATE ) && (badDataCount < MAX_BAD_DATA));

#ifdef SEQUENCE_SOUND
	if ( shandle )
	{
		
		if ( Streamer_GetSoundDecodeMode( shandle ) == SSDM_IDLE )
		{
			
			if (lpDSSB->lpVtbl->GetCurrentPosition(lpDSSB, &CurrentPos, &CurrentWritePos ) != DS_OK)		
				return VIDEO_SOUND_ERROR;

			if(( LastUpdated==SSDM_SECONDBUFFER )&&( CurrentPos > 16384 ))
			{
				Streamer_SetSoundDecodeMode( shandle, SSDM_FIRSTBUFFER );

				lpDSSB->lpVtbl->Lock(lpDSSB,
								0,
								16384,
								(void**)&soundbuffer1,
								&temp,
								NULL,
								0,
								0 );
			}
			else if(( LastUpdated==SSDM_FIRSTBUFFER )&&( CurrentPos < 16384 ))
			{
				Streamer_SetSoundDecodeMode( shandle, SSDM_SECONDBUFFER );

				lpDSSB->lpVtbl->Lock(lpDSSB,
								16384,
								16384,
								(void**)&soundbuffer2,
								&temp,
								NULL,
								0,
								0 );
			}
		}
	}
#endif //SEQUENCE_SOUND

	
	
	if (sRes == STREAMER_OK)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL	seq_ShutDown(void)
{
	if ( lpDSSB	)
	{
		lpDSSB->lpVtbl->Stop(lpDSSB);
		lpDSSB->lpVtbl->Release(lpDSSB);	
	}
	/* Shutdown Sound, Video and Movie*/
	Streamer_ShutDownSound(	&shandle );
	Streamer_ShutDownVideo(	&vhandle );
	Streamer_ShutDownMovie(	&mhandle );
	lpDSSB = NULL;
	shandle == NULL;
	vhandle == NULL;
	mhandle == NULL;
	return TRUE;
}

void SoundCallBackFunc( LPSOUNDHANDLE shandle )
{
	long		state;

	state = Streamer_GetSoundDecodeMode( shandle );

	if ( state == SSDM_FIRSTBUFFER )
	{
		lpDSSB->lpVtbl->Unlock(lpDSSB,
						soundbuffer1,
						16384,
						NULL,
						0 );
	}
	else if ( state == SSDM_SECONDBUFFER )
	{
		lpDSSB->lpVtbl->Unlock(lpDSSB,
						soundbuffer2,
						16384,
						NULL,
						0 );
	}
	
	Streamer_SetSoundDecodeMode( shandle, SSDM_IDLE );
	LastUpdated = state;
	return;
}


BOOL	seq_GetFrameSize(SDWORD *pWidth, SDWORD* pHeight)
{
	if (mhandle)
	{
		*pWidth = movieWidth;
		*pHeight = movieHeight;
		return TRUE;
	}
	else
	{
		*pWidth = 0;
		*pHeight = 0;
		return FALSE;
	}
}

int seq_GetCurrentFrame(void)
{
	if (mhandle)
	{
		return Movie_GetCurrentFrame(mhandle);
	}
	else
	{
		return -1;
	}
}

int seq_GetFrameTimeInClicks(void)
{
	float	frameTime;
	if (mhandle)
	{
		frameTime = 1000.0f/Movie_GetFrameRate(mhandle);
		return (int)frameTime;
	}
	else
	{
		return 40;
	}

}

int seq_GetTotalFrames(void)
{
	if (mhandle)
	{
		return Movie_GetTotalFrames(mhandle);
	}
	else
	{
		return -1;
	}
}

