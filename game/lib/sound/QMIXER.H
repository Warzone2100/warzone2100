//
// QMixer for Windows and Macintosh.
// Copyright (c) 1995-1998 QSound Labs, Inc.  All Rights Reserved.
// Version 4.13, July 26, 1998.
//  
// Based on Microsoft's WaveMix API.
// Copyright (c) 1993, 1994  Microsoft Corporation.  All Rights Reserved.
//


#ifndef QMIXER_H
#define QMIXER_H

#if !defined(_QMIXIMP)
    #define _QMIXIMP __declspec(dllimport)
#endif

DECLARE_HANDLE(HQMIXER);

//
// Constants for qprocessing
//
#define Q_PAN_MIN      0
#define Q_PAN_MAX     60
#define Q_PAN_DEFAULT 15
#define Q_VOLUME_DEFAULT    10000

//
// Flags for channel functions.
//
#define QMIX_ALL              0x0001    // apply to all channels
#define QMIX_NOREMIX          0x0002    // don't remix
#define QMIX_CONTROL_NOREMIX  0x0004    // don't remix


typedef
struct QSVECTOR {
    float x;                // meters
    float y;                // meters
    float z;                // meters
} QSVECTOR, *LPQSVECTOR;


typedef
struct QSPOLAR {
    float azimuth;          // degrees
    float range;            // meters
    float elevation;        // degrees
} QSPOLAR, *LPQSPOLAR;


#ifdef  __cplusplus
extern "C" {
#endif


typedef
struct QMIXINFO {
    DWORD dwSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
}
QMIXINFO, FAR* LPQMIXINFO;

_QMIXIMP
DWORD
WINAPI
QSWaveMixGetInfoEx(
    LPQMIXINFO lpInfo
    );


//
// Error handling functions.
//

typedef UINT QMIX_RESULT;

_QMIXIMP
QMIX_RESULT
WINAPI
QSWaveMixGetLastError(
    void
    );

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixGetErrorText(
    QMIX_RESULT qmrError,
    LPSTR pszText,
    UINT cchText
    );


//
// Initialization functions.
//

_QMIXIMP
HQMIXER
WINAPI
QSWaveMixInit(
    void
    );

#define QMIX_CONFIG_USEPRIMARY      0x0001
#define QMIX_CONFIG_NOTHREAD        0x0002
#define QMIX_CONFIG_PLAYSILENCE     0x0004
#define QMIX_CONFIG_LEFTCOORDS      0x0008
#define QMIX_CONFIG_RIGHTCOORDS     0x0000
#define QMIX_CONFIG_GLOBALROLLOFF	0x0010

#define QMIX_CONFIG_OUTPUT_DETECT       0
#define QMIX_CONFIG_OUTPUT_WAVEOUT      1
#define QMIX_CONFIG_OUTPUT_DIRECTSOUND  2

#define QMIX_CONFIG_MATH_DETECT         0
#define QMIX_CONFIG_MATH_INTEGER        1
#define QMIX_CONFIG_MATH_FLOAT          2
#define QMIX_CONFIG_MATH_MMX            3

typedef
struct QMIXCONFIG {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwSamplingRate;   // 11025, 22050, or 44100 Hz; if 0 uses 22050
    LPVOID lpIDirectSound;
    const GUID FAR* lpGuid;
    int iChannels;          // if 0, uses default of 8
    int iOutput;            // if 0, uses best output device
    int iLatency;           // (in ms) if 0, uses default for output device
    int iMath;              // style of math
    HWND hwnd;              // handle used when initializing DirectSound
                            // (if 0, QMixer will find main window)
    int iChannels3dHw;      // number of 3d hardware channels
    int iChannels3dQS;      // number of 3d QSound software channels
    int iChannels3dSw;      // number of 3d software channels
    int iChannels2dHw;      // number of 2d hardware channels
    int iChannels2dQS;      // number of 2d QSound software channels
    int iChannels2dSw;      // number of 2d software channels
}
QMIXCONFIG, FAR* LPQMIXCONFIG;


_QMIXIMP
HQMIXER
WINAPI
QSWaveMixInitEx(
    LPQMIXCONFIG lpConfig
    );


//
// Choose between speakers or headphones.
//
#define QMIX_SPEAKERS_STEREO     0
#define QMIX_SPEAKERS_HEADPHONES 1
#define QMIX_SPEAKERS_MONO       2
#define QMIX_SPEAKERS_QUAD       3

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixSetSpeakerPlacement(
    HQMIXER hQMixer,
    int placement
    );


_QMIXIMP
MMRESULT
WINAPI
QSWaveMixGetSpeakerPlacement(
    HQMIXER hQMixer,
    LPDWORD lpdwPlacement
    );


//
// The following function turns on and off various options
// in the mixer.  Turning off features can save CPU cycles.
//
#define QMIX_OPTIONS_REAR       1       // improved rear effect
#define QMIX_OPTIONS_CROSSOVER  2       // improved frequency response
#define QMIX_OPTIONS_FLIPOUTPUT 4       // flip output channels
#define QMIX_OPTIONS_DOPPLER    8       // calculate listener velocity using position updates
#define QMIX_OPTIONS_ALL 0xffffffff

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixSetOptions(
    HQMIXER hQMixer,
    DWORD dwFlags,
    DWORD dwMask
    );

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixGetOptions(
    HQMIXER hQMixer,
    LPDWORD lpdwFlags
    );


_QMIXIMP
MMRESULT
WINAPI
QSWaveMixSetMixerOption(
    HQMIXER hQMixer,
	DWORD dwFlags,
	LPVOID lpvOptions
	);


//
// QSWaveMixOpenChannel flags
//
#define QMIX_OPENSINGLE    0    // open the single channel specified by iChannel
#define QMIX_OPENALL       1    // opens all the channels, iChannel ignored
#define QMIX_OPENCOUNT     2    // open iChannel Channels (eg. if iChannel = 4 will create channels 0-3)
#define QMIX_OPENAVAILABLE 3    // open the first unopened channel, and return channel number

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixOpenChannel(
    HQMIXER hQMixer,
    int iChannel,
    DWORD dwFlags
    );


//
// Comprehensive channel parameters.  Used in the functions
// - QSWaveMixGetChannelParams
// - QSWaveMixSetChannelParams
// - QSWaveMixCalculateChannelParams
// - QSWaveMixFindChannel
// - QSWaveMixPlayEx
//
// The first set of parameters shouldn't be modified, the second set
// can be altered for QSWaveMixSetChannelParams, and the third set
// are calculated by QMixer.
//
typedef
struct QMIX_CHANNEL_PARAMS {
    DWORD dwSize;                   // size of this structure
    DWORD dwChannelType;            // combination of QMIX_CHANNELTYPE_ flags
    LPWAVEFORMATEX lpwfxFormat;     // channel format (overrides default mixer output)
    DWORD dwMixerFlags;             // reserved

    QSVECTOR vPosition;             // 3d position (Cartesian coordinates)
    QSVECTOR vVelocity;             // 3d velocity (Cartesian coordinates)
    QSVECTOR vOrientation;          // 3d orientation (Cartesian coordinates)
    float flInnerConeAngle;         // inner cone angle (degrees)
    float flOuterConeAngle;         // outer cone angle (degrees)
    float flOuterConeAtten;         // attenuation outside cone (db)
    float flMinDistance;            // minimum distance (m)
    float flMaxDistance;            // maximum distance (m)
    float flRolloff;                // rolloff factor (0..10, default=1.0)
    float flVolume;                 // scale
    LONG lFrequency;                // playback frequency
    LONG lPanRate;                  // time in which a change is completed (ms)
    LONG lPriority;                 // user-defined priority
    DWORD dwModes;                  // combination of QSWaveMixEnableChannel flags
    long unused[16];                // reserved for future expansion (set to zero)

    LONG lTimePlaying;              // current time playing current sound (ms)
    LONG lTimeRemaining;            // current time left to play (ms)
    float flDistance;               // current distance to listener (m)
    float flAzimuth;                // current azimuth (degrees)
    float flElevation;              // current elevation (degrees)
    float flConeScale;              // amount that volume is scaled for cone effect
    float flRolloffScale;           // amount that volume is scaled for rolloff effect
    float flAppliedVolume;          // volume applied to sound, including rolloff & cone effects (scale)
    float flDopplerSpeedup;         // speedup due to Doppler shift
    QSVECTOR vHeadPosition;         // position relative to listener (left-handed coordinates)
    QSVECTOR vHeadVelocity;         // velocity relative to listener (left-handed coordinates)
} QMIX_CHANNEL_PARAMS;


_QMIXIMP
MMRESULT
WINAPI
QSWaveMixGetChannelParams(
    HQMIXER hQMixer,
    int iChannel,
    QMIX_CHANNEL_PARAMS* params
    );


_QMIXIMP MMRESULT WINAPI
QSWaveMixSetChannelParams(
    HQMIXER hQMixer,
    int iChannel,
    DWORD dwFlags,
    const QMIX_CHANNEL_PARAMS* params
    );


_QMIXIMP MMRESULT WINAPI
QSWaveMixCalculateChannelParams(
    HQMIXER hQMixer,
    QMIX_CHANNEL_PARAMS* params,
    DWORD dwFlags
    );


//
// Channel types.  These flags are used in:
//    QSWaveMixConfigureChannel,
//    QSWaveMixFindChannel.
// They also appear in the dwChannelType field in QMIX_CHANNEL_PARAMS.
//
#define QMIX_CHANNELTYPE_QSOUND     0x00010000  // uses QSound software mixer
#define QMIX_CHANNELTYPE_STREAMING  0x00020000  // uses hardware streaming DirectSoundBuffer
#define QMIX_CHANNELTYPE_STATIC     0x00040000  // uses hardware static DirectSoundBuffer
#define QMIX_CHANNELTYPE_SOFTWARE   0x00080000  // uses software DirectSoundBuffer
#define QMIX_CHANNELTYPE_DSOUND \
		(QMIX_CHANNELTYPE_STREAMING|QMIX_CHANNELTYPE_STATIC|QMIX_CHANNELTYPE_SOFTWARE)
#define QMIX_CHANNELTYPE_ANYMIXER   0x00ff0000

#define QMIX_CHANNELTYPE_2D         0x01000000  // stereo only
#define QMIX_CHANNELTYPE_3D         0x02000000  // supports full 3d features
#define QMIX_CHANNELTYPE_ANYFEATURE 0x7f000000

#define QMIX_CHANNELTYPE_ALL        (QMIX_CHANNELTYPE_ANYMIXER|QMIX_CHANNELTYPE_ANYFEATURE)


_QMIXIMP
MMRESULT
WINAPI
QSWaveMixConfigureChannel(
    HQMIXER hQMixer,
    int iChannel,
    DWORD dwFlags,                  // type of channel
    LPCWAVEFORMATEX format,         // format of channel (optional)
    LPVOID lpvOptions               // options (reserved, set to zero)
    );


//
// Find a channel using one of the QMIX_FINDCHANNEL_ priority schemes.
//
// The type of channel can be restricted to one of the types given by
// a combination of the QMIX_CHANNELTYPE_ flags.
//
// Normally the channel will be excluded from subsequent calls to
// QSWaveMixFindChannel until a sound is played on the channel.  This is
// to allow the setting up of several sounds to be played at the same
// instant.  This reservation can be disabled by setting the
// QMIX_FINDCHANNEL_NORESERVE flag.
//

#define QMIX_FINDCHANNEL_SORTMETHOD 0x0000000f  // mask for sort method
#define QMIX_FINDCHANNEL_BEGINTIME  0x00000000  // sort by time from beginning (least-recently used)
#define QMIX_FINDCHANNEL_ENDTIME    0x00000001  // sort by time to end (most-recently available)
#define QMIX_FINDCHANNEL_VOLUME     0x00000002  // sort by applied volume
#define QMIX_FINDCHANNEL_DISTANCE   0x00000003  // sort by distance from listener
#define QMIX_FINDCHANNEL_PRIORITY   0x00000004  // sort by user priority

#define QMIX_FINDCHANNEL_TOPLAY     0x00000000  // pick a playing or ready (idle) channel for playing another
#define QMIX_FINDCHANNEL_TOPAUSE    0x00000010  // pick a playing channel for pausing
#define QMIX_FINDCHANNEL_TORESTART  0x00000020  // pick a paused channel for restarting
#define QMIX_FINDCHANNEL_FORMAT     0x00000080  // channel must match lpParams->lpwfxFormat
#define QMIX_FINDCHANNEL_NORESERVE  0x80000000  // don't exclude from subsequent searches

#define QMIX_FINDCHANNEL_LRU        QMIX_FINDCHANNEL_BEGINTIME
#define QMIX_FINDCHANNEL_MRA        QMIX_FINDCHANNEL_ENDTIME

_QMIXIMP
int
WINAPI
QSWaveMixFindChannel(
    HQMIXER hQMixer,
    DWORD dwFlags,
    const QMIX_CHANNEL_PARAMS* lpParams         // compare with these parameters
    );


//
// Turn on QMixer's dynamic scheduling.  The lpChannelTypes array gives the
// channel type (one of QMIX_CHANNELTYPE_ANYMIXER and one of
// QMIX_CHANNELTYPE_ANYFEATURE) and the maximum number of desired channels of
// that type that should play at any time.  A count of -1 channels specifies
// that as many channels as possible should be played.  An entry with
// dwChannelType ends the list.  If lpChannelTypes is null, the current type
// settings are used.  If QMIX_PRIORITIZE_NONE is used, scheduling is turned
// off.
//
typedef
struct QMIX_CHANNELTYPES {
    DWORD dwChannelType;            // type of channel
    int iMaxChannels;               // maximum number of channels of that type to play
} QMIX_CHANNELTYPES, *LPQMIX_CHANNELTYPES;


#define QMIX_PRIORITIZE_NONE        0x0000000f

_QMIXIMP MMRESULT WINAPI
QSWaveMixPrioritizeChannels(
    HQMIXER hQMixer,
    LPQMIX_CHANNELTYPES lpChannelTypes,
    DWORD dwFlags,
    LPVOID lpvParams
    );


//
// Structure describing wave data; returned by QSWaveMixOpenWave and
// QSWaveMixOpenWaveEx.
//
#define MAXFILENAME 15 

typedef struct MIXWAVE {
    PCMWAVEFORMAT pcm;          // data format
    WAVEHDR wh;                 // location and length of data
    char szWaveFilename[MAXFILENAME+1];     // truncated version of name
    LPWAVEFORMATEX pwf;         // data format; needed for MSACM wave files
#if defined(MACINTOSH)
    CmpSoundHeader format;      // Mac version of sound info
#endif
}
MIXWAVE, FAR* LPMIXWAVE;
typedef const MIXWAVE FAR* LPCMIXWAVE;

// Input streaming callback function.  Return TRUE to continue to play, return
// FALSE to stop playing.
typedef BOOL (CALLBACK *LPQMIXINPUTCALLBACK)(void* buffer, DWORD bytes, void* user);

//
// QSWaveMixOpenWave and QSWaveMixOpenWaveEx flags.
//
#define QMIX_FILE                   0x0001  // load from file
#define QMIX_RESOURCE               0x0002  // load from resource
#define QMIX_MEMORY                 0x0004  // load using MMIOINFO supplied by user
#define QMIX_RESIDENT               0x0008  // use memory-resident data
#define QMIX_FILESTREAM             0x0010  // stream from file
#define QMIX_INPUTSTREAM            0x0011  // stream from input buffer
#define QMIX_FILESPEC               0x0012  // Macintosh file specification

typedef union {
    const char* FileName;       // Name of RIFF file (e.g. "*.wav") with wave.
    struct {                    // Wave stored in resource.
        const char* Name;           // Name of resource.
        HINSTANCE hInstance;        // Handle of module where resource is located.
    } Resource;
    LPMMIOINFO lpMmioInfo;      // Read from file already opened by mmioOpen.
    struct {                    // Memory resident wave.
        LPCWAVEFORMATEX Format;     // Format of wave; can be compressed.
        HPSTR Data;                 // Pointer to memory resident audio samples.
        DWORD Bytes;                // Length of Data, in bytes.
        DWORD Samples;              // Number of wave samples; needed if compressed.
        DWORD Tag;                  // User-defined id.
    } Resident;
    struct {                    // Input streaming using user callback.
        LPCWAVEFORMATEX Format; // Format of data (must be uncompressed).
        DWORD BlockBytes;           // Preferred length of block.
        LPQMIXINPUTCALLBACK Callback; // User-defined callback function.
        LPVOID User;                // Data to be provided to callback.
        DWORD Tag;                  // User-defined id.
    } Stream;
#if defined(MACINTOSH)
    FSSpec* FileSpec;           // Macintosh file specifier.
#endif
} QMIXWAVEPARAMS, *LPQMIXWAVEPARAMS;


_QMIXIMP
LPMIXWAVE
WINAPI
QSWaveMixOpenWaveEx(
    HQMIXER hQMixer,
    LPQMIXWAVEPARAMS lpWaveParams,
    DWORD dwFlags
    );


_QMIXIMP
MMRESULT
WINAPI
QSWaveMixActivate(
    HQMIXER hQMixer,
    BOOL fActivate
    );


//
// Options for dwFlags parameter in QSWaveMixPlayEx.
//
// Note:  the QMIX_USELRUCHANNEL flag has two functions.  When QMIX_CLEARQUEUE
// is also set, the channel that has been playing the longest (least-recently-
// used) is cleared and the buffer played.  When QMIX_QUEUEWAVE is set, the
// channel that will first finish playing will be selected and the buffer
// queued to play.  Of course, if an unused channel is found, it will be
// selected instead.  If QMIX_WAIT has not been specified, then the channel
// number will be returned in the iChannel field.
//
#define QMIX_QUEUEWAVE          0x0000  // queue on channel
#define QMIX_CLEARQUEUE         0x0001  // clear queue on channel
#define QMIX_USELRUCHANNEL      0x0002  // see note above
#define QMIX_HIPRIORITY         0x0004
#define QMIX_HIGHPRIORITY QMIX_HIPRIORITY
#define QMIX_WAIT               0x0008  // queue to be played with other sounds
#define QMIX_IMMEDIATE          0x0020  // apply volume/pan changes without interpolation

#define QMIX_PLAY_SETEVENT      0x0100  // call SetEvent((HANDLE)callback) on completion
#define QMIX_PLAY_PULSEEVENT    0x0200  // call PulseEvent((HANDLE)callback) on completion
#define QMIX_PLAY_NOTIFYSTOP    0x0400  // do callback even when stopping or flushing sound

typedef void (CALLBACK *LPQMIXDONECALLBACK)(int iChannel, LPMIXWAVE lpWave, DWORD dwUser);

typedef struct QMIXPLAYPARAMS {
    DWORD dwSize;           // this must be set
    LPMIXWAVE lpImage;      // additional preprocessed audio for high performance
    HWND hwndNotify;        // if set, WOM_OPEN and WOM_DONE notification messages sent here
    LPQMIXDONECALLBACK callback;
    DWORD dwUser;           // user data accompanying callback
    LONG lStart;
    LONG lStartLoop;
    LONG lEndLoop;
    LONG lEnd;
    const QMIX_CHANNEL_PARAMS* lpChannelParams;     // initialize with these parameters
}
QMIXPLAYPARAMS, *LPQMIXPLAYPARAMS;

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixPlayEx(
    HQMIXER hQMixer,            // the mixer handle
    int iChannel,               // channel number to be played on
    DWORD dwFlags,
    LPMIXWAVE lpMixWave,        // sound to be played
    int iLoops,                 // number of loops to be played (-1 = forever)
    LPQMIXPLAYPARAMS lpParams   // if NULL, defaults will be used
    );


//
// Functions for controlling channels.
//
// The flag QMIX_WAIT can be used in the dwFlags argument of the
// channel parameter functions to defer the use of the new parameter
// until the next sound is played with QSWaveMixPlayEx.  This allows
// us synchronize the changing of channel parameters with the beginning
// of a sound.
//
// The flag QMIX_USEONCE can be used to use the parameter only for the
// remainder of the current sound; afterwards the parameter reverts
// back to the channel's defaults.  If no sound is playing, the
// parameter is used for the entire duration of the next sound.
// If both QMIX_USEONCE and QMIX_WAIT are used, then the property takes
// effect for the entire duration of the following sound.
//
// These flags can be used with the following functions:
//   - QSWaveMixEnableChannel
//   - QSWaveMixSetChannelParams
//   - QSWaveMixSetChannelPriority
//   - QSWaveMixSetDistanceMapping
//   - QSWaveMixSetFrequency
//   - QSWaveMixSetPan
//   - QSWaveMixSetPanRate
//   - QSWaveMixSetPolarPosition
//   - QSWaveMixSetPosition
//   - QSWaveMixSetSourceCone
//   - QSWaveMixSetSourceCone2
//   - QSWaveMixSetSourcePosition
//   - QSWaveMixSetSourceVelocity
//   - QSWaveMixSetVolume
//
#define QMIX_USEONCE        0x10    // settings are temporary


//
// Set the amount of time, in milliseconds, to effect a change in
// a channel property (e.g. volume, position).  Non-zero values smooth
// out changes.
//
// Use QSWaveMixSetPanRate(hQMixer, iChannel, QMIX_WAIT|QMIX_USEONCE, 0)
// to implement sudden changes for the next sound without losing the
// smoothing function.  Using QMIX_IMMEDIATE with QSWaveMixPlayEx also
// causes immediate changes while preserving subsequent pan interpolations.
//
_QMIXIMP
MMRESULT
WINAPI
QSWaveMixSetPanRate(
    HQMIXER hQMixer,
    int iChannel,
    DWORD dwFlags,
    DWORD dwRate
    );

//
// The pan value can be set to values from 0 to 60:
//    0 = full left,
//   15 = straight ahead (mono),
//   30 = full right,
//   45 = behind.
//
_QMIXIMP
MMRESULT
WINAPI
QSWaveMixSetPan(
    HQMIXER hQMixer,
    int iChannel,
    DWORD dwFlags,
    int pan
    );

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixSetVolume(
    HQMIXER hQMixer,
    int iChannel,
    DWORD dwFlags,
    int volume
    );

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixSetPolarPosition(
    HQMIXER hQMixer,
    int iChannel,
    DWORD dwFlags,
    const QSPOLAR* lpQPosition
    );

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixSetFrequency(
    HQMIXER hQMixer,
    int iChannel,
    DWORD dwFlags,
    DWORD dwFrequency
    );

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixPauseChannel(
    HQMIXER hQMixer,
    int iChannel,
    DWORD dwFlags
    );

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixRestartChannel(
    HQMIXER hQMixer,
    int iChannel,
    DWORD dwFlags
    );


#define QMIX_STOP_FINISHLASTLOOP 0x1000

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixStopChannel(
    HQMIXER hQMixer,
    int iChannel,
    DWORD dwFlags
    );


//
// QSWaveMixEnableChannel:  if mode==0, use conventional, high-performance
// stereo mixer.  Non-zero modes imply some form of additional processing.
//
#define QMIX_CHANNEL_STEREO             0x0000  // perform stereo mixing
#define QMIX_CHANNEL_QSOUND             0x0001  // perform QSound localization (default)
#define QMIX_CHANNEL_DOPPLER            0x0002  // calculate velocity using position updates
#define QMIX_CHANNEL_RANGE              0x0004  // do range effects
#define QMIX_CHANNEL_ELEVATION          0x0008  // do elevation effects
#define QMIX_CHANNEL_NODOPPLERPITCH     0x0010  // disable Doppler pitch shift for this channel
#define QMIX_CHANNEL_PITCH_COPY         0x0000  // pitch shifting using copying (fastest)
#define QMIX_CHANNEL_PITCH_LINEAR       0x0100  // pitch shifting using linear interpolation (better but slower)
#define QMIX_CHANNEL_PITCH_SPLINE       0x0200  // pitch shifting using spline interpolation (better yet, but much slower)
#define QMIX_CHANNEL_PITCH_FILTER       0x0300  // pitch shifting using FIR filter (best, but slowest)
#define QMIX_CHANNEL_PITCH_MASK         0x0700  // bits reserved for pitch types

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixEnableChannel(
    HQMIXER hQMixer,
    int iChannel,
    DWORD dwFlags,
    int mode
    );


//
// QSWaveMixEnable is equivalent to
//   QSWaveMixEnableChannel(hQMixer, 0, QMIX_ALL, mode).
//
_QMIXIMP
MMRESULT
WINAPI
QSWaveMixEnable(
    HQMIXER hQMixer,
    int mode
    );


_QMIXIMP
MMRESULT
WINAPI
QSWaveMixSetChannelPriority(
    HQMIXER hQMixer,
    int iChannel,
    LONG lPriority,
    DWORD dwFlags
    );


//
// Status functions.
//

//
// Return a pointer to the internal DirectSound object.  Handy for creating
// additional secondary buffers (but only if QMIX_CONFIG_USEPRIMARY was not
// set in QSWaveMixConfigureInit).
//
_QMIXIMP
MMRESULT
WINAPI
QSWaveMixGetDirectSound(
    HQMIXER hQMixer,
    LPVOID FAR* lplpIDirectSound
    );


//
// Return a pointer to the internal DirectSoundBuffer object.
//
_QMIXIMP
MMRESULT
WINAPI
QSWaveMixGetDirectSoundBuffer(
    HQMIXER hQMixer,
    int iChannel,
    LPVOID FAR* lplpIDirectSoundBuffer
);


//
// Return TRUE if there are no more buffers playing or queued on the channel.
// This is an alternative to waiting for a WOM_DONE message to be sent to a
// window.
//
_QMIXIMP
BOOL
WINAPI
QSWaveMixIsChannelDone(
    HQMIXER hQMixer,
    int iChannel
    );


#define QMIX_CHANNELSTATUS_OPEN     0x01    // channel is open
#define QMIX_CHANNELSTATUS_QUEUED   0x02    // channel has a play request queued on it
#define QMIX_CHANNELSTATUS_PAUSED   0x04    // channel is paused
#define QMIX_CHANNELSTATUS_PLAYING  0x08    // channel is playing a sound
#define QMIX_CHANNELSTATUS_RESERVED 0x10    // channel is reserved from FindChannel for playing

_QMIXIMP DWORD WINAPI
QSWaveMixGetChannelStatus(
    HQMIXER hQMixer,
    int iChannel
    );


#define QMIX_POSITION_BYTES    0
#define QMIX_POSITION_SAMPLES  1
#define QMIX_POSITION_MS       2

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixGetPlayPosition(
    HQMIXER hQMixer,
    int iChannel,
    LPMIXWAVE* lplpWave,
    LPDWORD lpdwPosition,
    DWORD dwFlags
    );


_QMIXIMP
MMRESULT
WINAPI
QSWaveMixGetPan(
    HQMIXER hQMixer,
    int iChannel,
    LPDWORD lpdwPan
    );

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixGetVolume(
    HQMIXER hQMixer,
    int iChannel,
    LPDWORD lpdwVolume
    );

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixGetPanRate(
    HQMIXER hQMixer,
    int iChannel,
    LPDWORD lpdwRate
    );

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixGetFrequency(
    HQMIXER hQMixer,
    int iChannel,
    LPDWORD lpdwFrequency
    );

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixGetPolarPosition(
    HQMIXER hQMixer,
    int iChannel,
    QSPOLAR* lpQPosition
    );


//**********************************************************************
// Interface for 3d Cartesian Coordinate system.
//
// All measurements are in S.I. units, e.g. coordinates and room size are
// meters, velocity and speed of sound in m/s.
//

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixSetRoomSize(
    HQMIXER hQMixer,
    float roomSize,
    DWORD dwFlags
    );

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixSetSpeedOfSound(
    HQMIXER hQMixer,
    float speed,
    DWORD dwFlags
    );


typedef struct QMIX_DISTANCES {
    UINT cbSize;            // size of structure (needed for future versions)
    float minDistance;      // sounds are at full volume if closer than this
    float maxDistance;      // sounds are muted if further away than this
    float scale;            // relative amount to adjust rolloff by
} QMIX_DISTANCES, *LPQMIX_DISTANCES;

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixSetDistanceMapping(
    HQMIXER hQMixer,
    int iChannel,
    DWORD dwFlags,
    const LPQMIX_DISTANCES lpDistances
    );


_QMIXIMP
MMRESULT
WINAPI
QSWaveMixGetRoomSize(
    HQMIXER hQMixer,
    float* pfRoomSize
    );

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixGetSpeedOfSound(
    HQMIXER hQMixer,
    float* pfSpeed
    );

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixGetDistanceMapping(
    HQMIXER hQMixer,
    int iChannel,
    LPQMIX_DISTANCES lpDistances
    );

//
// Listener parameters.
//
_QMIXIMP
MMRESULT
WINAPI
QSWaveMixSetListenerPosition(
    HQMIXER hQMixer,
    const QSVECTOR* position,
    DWORD dwFlags
    );

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixSetListenerOrientation(
    HQMIXER hQMixer,
    const QSVECTOR* direction,
    const QSVECTOR* up,
    DWORD dwFlags
    );

//
// Velocity is used solely to set Doppler shift.
//
_QMIXIMP
MMRESULT
WINAPI
QSWaveMixSetListenerVelocity(
    HQMIXER hQMixer,
    const QSVECTOR* velocity,
    DWORD dwFlags
    );


_QMIXIMP
MMRESULT
WINAPI
QSWaveMixSetListenerRolloff(
	HQMIXER hQMixer,
	float rolloff,
	DWORD dwFlags
	);


_QMIXIMP
MMRESULT
WINAPI
QSWaveMixGetListenerPosition(
    HQMIXER hQMixer,
    QSVECTOR* position
    );

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixGetListenerOrientation(
    HQMIXER hQMixer,
    QSVECTOR* direction,
    QSVECTOR* up
    );

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixGetListenerVelocity(
    HQMIXER hQMixer,
    QSVECTOR* velocity
    );


_QMIXIMP
MMRESULT
WINAPI
QSWaveMixSetSourcePosition(
    HQMIXER hQMixer,
    int iChannel,
    DWORD dwFlags,
    const QSVECTOR* position
    );

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixSetSourceVelocity(
    HQMIXER hQMixer,
    int iChannel,
    DWORD dwFlags,
    const QSVECTOR* velocity
    );

//
// Listener will hear full volume if inside source's cone, given by
// orientation and angle.  If outside, source will be attenuated by
// additional attenuation.
//
_QMIXIMP
MMRESULT
WINAPI
QSWaveMixSetSourceCone(
    HQMIXER hQMixer,
    int iChannel,
    DWORD dwFlags,
    const QSVECTOR* orientation,
    float coneAngle,
    float ambient_db
    );

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixSetSourceCone2(
    HQMIXER hQMixer,
    int iChannel,
    DWORD dwFlags,
    const QSVECTOR* orientation,
    float innerConeAngle,
    float outerConeAngle,
    float ambient_db
    );

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixGetSourcePosition(
    HQMIXER hQMixer,
    int iChannel,
    QSVECTOR* position
    );

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixGetSourceVelocity(
    HQMIXER hQMixer,
    int iChannel,
    QSVECTOR* velocity
    );

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixGetSourceCone(
    HQMIXER hQMixer,
    int iChannel,
    QSVECTOR* orientation,
    float* pfAngle,
    float* ambient_db
    );

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixGetSourceCone2(
    HQMIXER hQMixer,
    int iChannel,
    QSVECTOR* orientation,
    float* pfInnerAngle,
    float* pfOuterAngle,
    float* ambient_db
    );


//**********************************************************************
// Profiling functions.
//
typedef struct QMIX_PROFILE {
    DWORD dwCalls;          // number of times mixer thread was called
    DWORD dwMixing;         // time spent mixing
    DWORD dwAudioDriver;    // time spent calling audio driver (e.g. DirectSound)
    DWORD dwTotal;          // total time in mixer thread
    DWORD dwDuration;       // time during test
} QMIX_PROFILE, *LPQMIX_PROFILE;

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixStartProfile(
    HQMIXER hQMixer
    );

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixStopProfile(
    HQMIXER hQMixer,
    LPQMIX_PROFILE lpProfile,
    UINT cbProfile
    );


//**********************************************************************
// Call QSWaveMixPump to force more mixing.  Essential if mixer was
// configured with QMIX_CONFIG_NOTHREAD.
//
_QMIXIMP
void
WINAPI
QSWaveMixPump(
    void);


//**********************************************************************
// Functions to shut down mixer.
//
// Note that, in versions 2.10 and later, that QSWaveMixFreeWave can be
// called with a null hQMixer if QSWaveMixCloseSession has been called.
//
_QMIXIMP
MMRESULT
WINAPI
QSWaveMixFlushChannel(
    HQMIXER hQMixer,
    int iChannel,
    DWORD dwFlags
    );

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixCloseChannel(
    HQMIXER hQMixer,
    int iChannel,
    DWORD dwFlags
    );

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixFreeWave(
    HQMIXER hQMixer,
    LPMIXWAVE lpMixWave
    );

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixCloseSession(
    HQMIXER hQMixer
    );


//
// Diagnostic codes available from QSWaveMixGetLastError.  Equivalent strings
// can be recalled using QSWaveMixGetErrorText.
//
#define ERROR_QMIX_OK                   0  // OK, no error.
#define ERROR_QMIX_CONFIGMISSING        1  // Can't find configuration (.ini) file.
#define ERROR_QMIX_MIXERHANDLE          2  // Mixer handle is null or invalid.
#define ERROR_QMIX_RESULTUNKNOWN        3  // Unknown error code.
#define ERROR_QMIX_MEMORY               4  // Out of memory.
#define ERROR_QMIX_EXCEPTIONUNKNOWN     5  // Unknown error.
#define ERROR_QMIX_WAVEOUTNONE          6  // No wave output devices.
#define ERROR_QMIX_DLLNAME              7  // Name of QMixer library is unavailable or invalid.
#define ERROR_QMIX_WAVEOUTINCOMPATIBLE  8  // The wave output device is incompatible with QMixer.
#define ERROR_QMIX_ACTIVATED            9  // Can't activate two QMixer sessions.
#define ERROR_QMIX_CHANNELINVALID      10  // Channel number is invalid.
#define ERROR_QMIX_OPENCHANNELFLAG     11  // Invalid flag for opening channel.
#define ERROR_QMIX_PANINVALID          12  // Invalid pan value.
#define ERROR_QMIX_NOTACTIVATED        13  // Mixer has not been activated.
#define ERROR_QMIX_CHANNELUNAVAILABLE  14  // No channels are available for playing this sound.
#define ERROR_QMIX_WAVERESOURCE        15  // Couldn't find or load resource containing wave data.
#define ERROR_QMIX_WAVEOPEN            16  // Couldn't open wave file.
#define ERROR_QMIX_WAVEFORMAT          17  // Format of wave data is corrupted.
#define ERROR_QMIX_ACMINVALID          18  // The ACM wave format couldn't be translated.
#define ERROR_QMIX_DSOUNDMISSING       19  // The DirectSound library dsound.dll could not be found.
#define ERROR_QMIX_DSOUNDCREATEADDR    20  // Could not find DirectSoundCreate function in dsound.dll.
#define ERROR_QMIX_DSOUNDCREATE        21  // Couldn't create DirectSound object.
#define ERROR_QMIX_DSOUNDCOOPERATIVE   22  // The cooperative level for DirectSound could not be set.
#define ERROR_QMIX_DSOUNDCAPS          23  // Couldn't determine DirectSound capabilities.
#define ERROR_QMIX_DSOUNDPRIMARY       24  // Couldn't create DirectSound primary buffer.
#define ERROR_QMIX_DSOUNDPRIMARYCAPS   25  // Couldn't determine DirectSound primary buffer capabilities.
#define ERROR_QMIX_DSOUNDFORMAT        26  // Couldn't set format of DirectSound output.
#define ERROR_QMIX_DSOUNDSECONDARY     27  // Couldn't create DirectSound secondary buffer.
#define ERROR_QMIX_DSOUNDPLAY          28  // Couldn't play DirectSound buffer.
#define ERROR_QMIX_WAVEOUTOPEN         29  // Wave output error while opening device.
#define ERROR_QMIX_WAVEOUTWRITE        30  // Wave output error while writing to device.
#define ERROR_QMIX_WAVEOUTCLOSE        31  // Wave output error while closing device.
#define ERROR_QMIX_PARAMPTR            32  // Parameter points to invalid location.
#define ERROR_QMIX_NOTIMPLEMENTED      33  // Feature is not implemented.
#define ERROR_QMIX_DSOUNDNODRIVER      34  // No DirectSound driver is available for use.
#define ERROR_QMIX_SMALLPRIMARY        35  // DirectSound primary buffer is too small.
#define ERROR_QMIX_UNKNOWNFORMAT       36  // Unknown sound file format.
#define ERROR_QMIX_DRIVENOTREADY       37  // Drive is not ready.
#define ERROR_QMIX_READERROR           38  // Error reading sound file.
#define ERROR_QMIX_UNKNOWNAIFC         39  // Unknown format of AIFF-C file.
#define ERROR_QMIX_MACRESOURCE         40  // Unable to obtain sampled sound data from resource.
#define ERROR_QMIX_SCINVALID           41  // The sound data format couldn't be translated.
#define ERROR_QMIX_MACSOUNDCAPS        42  // Couldn't retrieve Macintosh sound output capabilities.
#define ERROR_QMIX_MACCHANNEL          43  // Couldn't create sound channel.
#define ERROR_QMIX_MACCLEAR            44  // Couldn't clear sound channel.
#define ERROR_QMIX_MACPAUSE            45  // Couldn't pause sound channel.
#define ERROR_QMIX_MACQUEUE            46  // Couldn't queue data on sound channel.
#define ERROR_QMIX_MACRESUME           47  // Couldn't resume paused sound channel.
#define ERROR_QMIX_CATCH               48  // Caught unknown error.
#define ERROR_QMIX_HOLD                49  // Unable to prevent virtual memory from being paged.
#define ERROR_QMIX_INVALIDPARAM        50  // Invalid parameter.
#define ERROR_QMIX_DSB_STATUS          51  // Couldn't get DirectSound buffer status.
#define ERROR_QMIX_DSB_POSITION        52  // Couldn't get DirectSound buffer position.
#define ERROR_QMIX_DSOUNDMIXPRIMARY    53  // Can't mix into primary buffer (DirectSound driver is probably emulated).


/* set to byte packing so Win16 and Win32 structures will be the same */
#pragma pack(1)

typedef struct WAVEMIXINFO {
    WORD wSize;
    BYTE bVersionMajor;
    BYTE bVersionMinor;
    char szDate[12]; /* Mmm dd yyyy */
    DWORD dwFormats; /* see waveOutGetDevCaps (wavemix requires synchronous device) */
}
WAVEMIXINFO, *PWAVEMIXINFO, FAR * LPWAVEMIXINFO;

_QMIXIMP
WORD
WINAPI
QSWaveMixGetInfo(
    LPWAVEMIXINFO lpWaveMixInfo
    );


//
// Constants, structure, and declaration for obsolete configuration function.
//

#define WMIX_CONFIG_CHANNELS       0x0001
#define WMIX_CONFIG_SAMPLINGRATE   0x0002
#define WMIX_CONFIG_DIRECTSOUND    0x0004
#define WMIX_CONFIG_USEPRIMARY     0x0008
#define WMIX_CONFIG_INPUTCHANNELS  0x0010
#define WMIX_CONFIG_OUTPUT         0x0020
#define WMIX_CONFIG_LATENCY        0x0040
#define WMIX_CONFIG_NOTHREAD       0x0080

#define WMIX_OUTPUT_WAVEOUT       0
#define WMIX_OUTPUT_DIRECTSOUND   1
#define WMIX_OUTPUT_NATIVEAUDIO   2     // not implemented

typedef struct MIXCONFIG {
    WORD wSize;
    DWORD dwFlags;
    WORD wChannels;       // 1=MONO, 2=STEREO
    WORD wSamplingRate;   // 11=11025, 22=22050, 44=44100 Hz
    LPVOID lpIDirectSound;
    WORD wInputChannels;
    WORD wOutput;
    WORD wLatency;
}
MIXCONFIG, *PMIXCONFIG, FAR * LPMIXCONFIG;

_QMIXIMP
HQMIXER
WINAPI
QSWaveMixConfigureInit(
    LPMIXCONFIG lpConfig
    );


//
// Obsolete wave opening function.
//

//
// Structure for opening waves that are already residing in memory.
// Note that the dwSamples parameter is a new field needed for using
// compressed data.  To keep backwards compatibility, it is only used
// when QSWaveMixOpenWave is used with the QMIX_RESIDENT_COMPRESSED
// parameter.
//
#define QMIX_RESIDENT_COMPRESSED    0x0009  // QMIX_RESIDENT with valid dwSample parameter

typedef struct QMEMRESIDENT {
    PCMWAVEFORMAT* pcm;     // format; LPWAVEFORMATEX works here too
    HPSTR lpData;           // pointer to memory resident audio samples
    DWORD dwDataSize;       // length of data, in bytes
    WORD tag;               // user-defined id (used in MIXWAVE.szWaveFilename)
    DWORD dwSamples;        // number of samples in compressed wave
}
QMEMRESIDENT, * PQMEMRESIDENT, FAR * LPQMEMRESIDENT;

//
// Structure for a wave that will get audio from user callback function.
//
#if defined(_WIN32)
    typedef void (__cdecl* LPQMIXSTREAMCALLBACK)(void* buffer, DWORD bytes, void* user);
#endif
#if defined(MACINTOSH)
    typedef void (* LPQMIXSTREAMCALLBACK)(void* buffer, DWORD bytes, void* user);
#endif
typedef struct QINPUTSTREAM {
    PCMWAVEFORMAT* pcm;     // format of input data:  must be in PCM
    DWORD dwBlockLength;    // preferred block length (bytes)
    LPQMIXSTREAMCALLBACK callback;
                            // callback called when more data needed
    void *user;             // user data sent to callback
    WORD tag;
}
QINPUTSTREAM, *PQINPUTSTREAM, FAR* LPQINPUTSTREAM;

//
// Open wave file.  The szWaveFilename parameter is one of the
// following:
//   - name of file (QMIX_FILE and QMIX_FILESTREAM flags),
//   - name of resource (also need to set hInst) (QMIX_RESOURCE flag),
//   - LPQMEMRESIDENT pointer cast to LPSTR (QMIX_RESIDENT flag),
//   - LPQINPUTSTREAM pointer cast to LPSTR (QMIX_INPUTSTREAM flag),
//   - LPMMIOINFO pointer cast to LPSTR (QMIX_MEMORY flag),
//   - FSSpec* pointer cast to LPSTR (on Macintosh) (QMIX_FILESPEC flag),
//
_QMIXIMP
LPMIXWAVE
WINAPI
QSWaveMixOpenWave(
    HQMIXER hQMixer,
    LPSTR szWaveFilename,   // see notes above
    HINSTANCE hInst,        // only required when QMIX_RESOURCE flag set
    DWORD dwFlags
    );


typedef struct QPOSITION {
    int azimuth;            // degrees; 0 is straight ahead, 90 is to the right
    int range;              // 0 to 32767; 0 is closest
    int elevation;          // -90 to 90 degrees
} QPOSITION, FAR* LPQPOSITION;


_QMIXIMP
MMRESULT
WINAPI
QSWaveMixSetPosition(
    HQMIXER hQMixer,
    int iChannel,
    DWORD dwFlags,
    const QPOSITION* lpQPosition
    );

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixGetPosition(
    HQMIXER hQMixer,
    int iChannel,
    LPQPOSITION lpQPosition
    );

//
// Obsolete play function.
//

#define QMIX_USEPOSITION    0x10    // use the MIXPLAYPARAMS.position field

typedef struct MIXPLAYPARAMS {
    WORD wSize;             // this must be set
    HQMIXER hMixSession;    // value returned by QSWaveMixInit or QSWaveMixConfigureInit
    int iChannel;           // channel number
    LPMIXWAVE lpMixWave;    // pointer returned by QSWaveMixOpenWave
    HWND hWndNotify;        // if set, WOM_OPEN and WOM_DONE notification messages sent here
    DWORD dwFlags;
    WORD wLoops;            // 0xFFFF means loop forever
    int qVolume;            // initial volume; set to -1 to use default
    int qPanPos;            // initial pan position; set to -1 to use default
    LPMIXWAVE lpImage;      // set to preprocessed data for high performance
    QPOSITION position;     // initial position (used only if QMIX_USEPOSITION set)
}
MIXPLAYPARAMS, * PMIXPLAYPARAM, FAR* LPMIXPLAYPARAMS;

_QMIXIMP
MMRESULT
WINAPI
QSWaveMixPlay(
    LPMIXPLAYPARAMS lpMixPlayParams
    );


/* return to default packing */
#pragma pack()

//
// Obsolete constants.
//
#define WMIX_ALL                   QMIX_ALL
#define WMIX_NOREMIX               QMIX_NOREMIX
#define WMIX_CONTROL_NOREMIX       QMIX_CONTROL_NOREMIX

#define WMIX_OPENSINGLE            QMIX_OPENSINGLE
#define WMIX_OPENALL               QMIX_OPENALL
#define WMIX_OPENCOUNT             QMIX_OPENCOUNT
#define WMIX_OPENAVAILABLE         QMIX_OPENAVAILABLE

#define WMIX_FILE                  QMIX_FILE
#define WMIX_RESOURCE              QMIX_RESOURCE
#define WMIX_MEMORY                QMIX_MEMORY
#define WMIX_RESIDENT              QMIX_RESIDENT
#define WMIX_FILESTREAM            QMIX_FILESTREAM
#define WMIX_INPUTSTREAM           QMIX_INPUTSTREAM

#define WMIX_QUEUEWAVE             QMIX_QUEUEWAVE
#define WMIX_CLEARQUEUE            QMIX_CLEARQUEUE
#define WMIX_USELRUCHANNEL         QMIX_USELRUCHANNEL
#define WMIX_HIPRIORITY            QMIX_HIPRIORITY
#define WMIX_HIGHPRIORITY          QMIX_HIGHPRIORITY
#define WMIX_WAIT                  QMIX_WAIT
#define WMIX_USEPOSITION           QMIX_USEPOSITION


#ifdef __cplusplus
}
#endif

#endif

