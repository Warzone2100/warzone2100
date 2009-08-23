/*
	Copyright (C) 2006  Angus Lees <gus@inodes.org>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

/*
 * Convert ARMovie/Eidos' Replay (.RPL) file to uncompressed AVI
 * GPL - Angus Lees <gus@inodes.org>  April 2006
 */

/* There seems to be a bug somewhere in the streamer adpcm decode code
   (at least under wine).  It throws a whole bunch of '-1's into the
   decoded audio stream for long files, which sounds like lots of
   static.  Work around this by doing the decoding ourselves. */
#define USE_MY_CODE 1

#define WIN32_LEAN_AND_MEAN	/* heh */
#include <windows.h>
#include <vfw.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "streamer.h"

#define STREAMER_AUDIOOUT_BITSIZE  16 /* streamer always outputs SLE16 */

#ifdef __WINE__
/* This throws a stub warning on wine at present (Wine 0.9.9) */
#define AVIFileExit() do {} while (0)
#endif


#ifdef __WINE__
/* HACK HACK HACK: We need to increase cbIdxRecords to work around an
 * index size limitation in current (0.9.12) wine versions */

#define MAX_AVISTREAMS 8

typedef struct _IAVIFileImpl IAVIFileImpl;

typedef struct _EXTRACHUNKS {
  LPVOID lp;
  DWORD  cb;
} EXTRACHUNKS, *LPEXTRACHUNKS;

typedef struct _IPersistFileImpl {
  /* IUnknown stuff */
  const void *lpVtbl;

  /* IPersistFile stuff */
  void     *paf;
} IPersistFileImpl;

struct _IAVIFileImpl {
  /* IUnknown stuff */
  const void     *lpVtbl;
  LONG              ref;

  /* IAVIFile stuff... */
  IPersistFileImpl  iPersistFile;

  AVIFILEINFOW      fInfo;
  void   *ppStreams[MAX_AVISTREAMS];

  EXTRACHUNKS       fileextra;

  DWORD             dwMoviChunkPos;  /* some stuff for saving ... */
  DWORD             dwIdxChunkPos;
  DWORD             dwNextFramePos;
  DWORD             dwInitialFrames;

  MMCKINFO          ckLastRecord;
  AVIINDEXENTRY    *idxRecords;      /* won't be updated while loading */
  DWORD             nIdxRecords;     /* current fill level */
  DWORD             cbIdxRecords;    /* size of idxRecords */

  /* IPersistFile stuff ... */
  HMMIO             hmmio;
  LPWSTR            szFileName;
  UINT              uMode;
  BOOL              fDirty;
};
#endif


static const char *avierror(HRESULT err) {
  switch (err) {
  case AVIERR_OK: return "OK";
  case AVIERR_UNSUPPORTED: return "UNSUPPORTED";
  case AVIERR_BADFORMAT: return "BADFORMAT";
  case AVIERR_MEMORY: return "MEMORY";
  case AVIERR_INTERNAL: return "INTERNAL";
  case AVIERR_BADFLAGS: return "BADFLAGS";
  case AVIERR_BADPARAM: return "BADPARAM";
  case AVIERR_BADSIZE: return "BADSIZE";
  case AVIERR_BADHANDLE: return "BADHANDLE";
  case AVIERR_FILEREAD: return "FILEREAD";
  case AVIERR_FILEWRITE: return "FILEWRITE";
  case AVIERR_FILEOPEN: return "FILEOPEN";
  case AVIERR_COMPRESSOR: return "COMPRESSOR";
  case AVIERR_NOCOMPRESSOR: return "NOCOMPRESSOR";
  case AVIERR_READONLY: return "READONLY";
  case AVIERR_NODATA: return "NODATA";
  case AVIERR_BUFFERTOOSMALL: return "BUFFERTOOSMALL";
  case AVIERR_CANTCOMPRESS: return "CANTCOMPRESS";
  case AVIERR_USERABORT: return "USERABORT";
  case AVIERR_ERROR: return "ERROR";
  default: return  "Unknown?";
  }
}

static const char *winstr_error(LONG err) {
  switch (err) {
  case STREAMER_OK: return "OK";
  case STREAMER_NO_SOUND_DRIVER: return "STREAMER_NO_SOUND_DRIVER";
  case STREAMER_NO_SOUND_IN_RPL: return "STREAMER_NO_SOUND_IN_RPL";
  case STREAMER_BAD_ALLOC_SOUND_HANDLE: return "STREAMER_BAD_ALLOC_SOUND_HANDLE";
  case STREAMER_BAD_ALLOC_DBUFFERS: return "STREAMER_BAD_ALLOC_DBUFFERS";
  case STREAMER_BAD_ALLOC_SAMPLE_HANDLE: return "STREAMER_BAD_ALLOC_SAMPLE_HANDLE";
  case STREAMER_BAD_ALLOC_TIMER: return "STREAMER_BAD_ALLOC_TIMER";
  case STREAMER_BAD_ALLOC_TIMER_HANDLE: return "STREAMER_BAD_ALLOC_TIMER_HANDLE";
  case STREAMER_BAD_ALLOC_VIDEO_BUFFER: return "STREAMER_BAD_ALLOC_VIDEO_BUFFER";
  case STREAMER_BAD_ALLOC_GLCW_LUT: return "STREAMER_BAD_ALLOC_GLCW_LUT";
  case STREAMER_BAD_ALLOC_LSCW_LUT: return "STREAMER_BAD_ALLOC_LSCW_LUT";
  case STREAMER_BAD_ALLOC_GSCW_LUT: return "STREAMER_BAD_ALLOC_GSCW_LUT";
  case STREAMER_BAD_ALLOC_VIDEO_HANDLE: return "STREAMER_BAD_ALLOC_VIDEO_HANDLE";
  case STREAMER_VESA_ERROR: return "STREAMER_VESA_ERROR";
  case STREAMER_BAD_ALLOC_SB_OFFSETS: return "STREAMER_BAD_ALLOC_SB_OFFSETS";
  case STREAMER_BAD_ALLOC_MOVIE_HANDLE: return "STREAMER_BAD_ALLOC_MOVIE_HANDLE";
  case STREAMER_BAD_ALLOC_MOVIE_INDEX: return "STREAMER_BAD_ALLOC_MOVIE_INDEX";
  case STREAMER_FILE_OPEN_ERROR: return "STREAMER_FILE_OPEN_ERROR";
  case STREAMER_BAD_PLAYBACK_STATE: return "STREAMER_BAD_PLAYBACK_STATE";
  case STREAMER_FILE_OPEN_ERROR_HIGH: return "STREAMER_FILE_OPEN_ERROR_HIGH";
  case STREAMER_BAD_MOVIE_HEADER: return "STREAMER_BAD_MOVIE_HEADER";
  case STREAMER_CHUNK_OUT_FILE_ERROR: return "STREAMER_CHUNK_OUT_FILE_ERROR";
  case STREAMER_BAD_ALLOC_OPTIONS: return "STREAMER_BAD_ALLOC_OPTIONS";
  case STREAMER_BAD_ALIGN_SEEK: return "STREAMER_BAD_ALIGN_SEEK";
  case STREAMER_PRELOAD_KEY_PRESS: return "STREAMER_PRELOAD_KEY_PRESS";
  case STREAMER_FILE_READ_ERROR: return "STREAMER_FILE_READ_ERROR";
  case STREAMER_BAD_ALLOC_SCREEN_BUFFER: return "STREAMER_BAD_ALLOC_SCREEN_BUFFER";
  case STREAMER_BAD_SOUND_CALLBACK: return "STREAMER_BAD_SOUND_CALLBACK";
  case STREAMER_BADTHREADCREATE: return "STREAMER_BADTHREADCREATE";
  case STREAMER_BADDATARATE: return "STREAMER_BADDATARATE";
  case STREAMER_BADMOVIEHANDLE: return "STREAMER_BADMOVIEHANDLE";
  case STREAMER_NOEFFECTS: return "STREAMER_NOEFFECTS";
  case STREAMER_FINISHEDVIDEO: return "STREAMER_FINISHEDVIDEO";
  case STREAMER_FINISHEDAUDIO: return "STREAMER_FINISHEDAUDIO";
  case STREAMER_BAD_ALLOC_SOUND_BUFFER: return "STREAMER_BAD_ALLOC_SOUND_BUFFER";
  case STREAMER_NOTINGRAPHICSMODE: return "STREAMER_NOTINGRAPHICSMODE";
  case STREAMER_BAD_MOVIEHANDLE: return "STREAMER_BAD_MOVIEHANDLE";
  case STREAMER_BAD_VIDEOHANDLE: return "STREAMER_BAD_VIDEOHANDLE";
  case STREAMER_BAD_SOUNDHANDLE: return "STREAMER_BAD_SOUNDHANDLE";
  case STREAMER_BADSOUNDBUFFERNO: return "STREAMER_BADSOUNDBUFFERNO";
  case STREAMER_NODATADATASTREAM: return "STREAMER_NODATADATASTREAM";
  case STREAMER_BADMOVIEFORMAT: return "STREAMER_BADMOVIEFORMAT";
  case STREAMER_130CODECOPENERROR: return "STREAMER_130CODECOPENERROR";
  default: return "Unknown?";
  }
}

static signed char index_adjust[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };

static short step_size[89] = {
	7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
	19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
	50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
	130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
	337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
	876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
	2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
	5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
	15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

struct adpcm_state {
  int pred_val, step_idx;
};

/* This code is "borrowed" from the ALSA library
   http://www.alsa-project.org */
static unsigned short
adpcm_decode_sample(struct adpcm_state *state, char code) {
	short pred_diff;	/* Predicted difference to next sample */
	short step;		/* holds previous step_size value */
	char sign;

	int i;

	/* Separate sign and magnitude */
	sign = code & 0x8;
	code &= 0x7;

	/*
	 * Computes pred_diff = (code + 0.5) * step / 4,
	 * but see comment in adpcm_coder.
	 */

	step = step_size[state->step_idx];

	/* Compute difference and new predicted value */
	pred_diff = step >> 3;
	for (i = 0x4; i; i >>= 1, step >>= 1) {
		if (code & i) {
			pred_diff += step;
		}
	}
	state->pred_val += (sign) ? -pred_diff : pred_diff;

	/* Clamp output value */
	if (state->pred_val > 32767) {
		state->pred_val = 32767;
	} else if (state->pred_val < -32768) {
		state->pred_val = -32768;
	}

	/* Find new step_size index value */
	state->step_idx += index_adjust[(int) code];

	if (state->step_idx < 0) {
		state->step_idx = 0;
	} else if (state->step_idx > 88) {
		state->step_idx = 88;
	}
	return state->pred_val;
}

void adpcm_init(struct adpcm_state *state) {
	state->pred_val = 0;
	state->step_idx = 0;
}

void adpcm_decode(struct adpcm_state *state,
		  unsigned char* input, unsigned int input_size,
		  unsigned short* output) {
	unsigned int i;

	for (i = 0; i < input_size; ++i) {
		unsigned char two_samples = input[i];
		*(output++) = adpcm_decode_sample(state, two_samples >> 4);
		*(output++) = adpcm_decode_sample(state, two_samples & 15);
	}
}

static long last_updated;
static PAVISTREAM audio;
static size_t audiobuffersize;
static size_t audiosamplesize;

static void sound_callback(LPSOUNDHANDLE shandle) {
  HRESULT res;
  long state;

  /* printf("sound_callback called!\n"); */

  state = Streamer_GetSoundDecodeMode(shandle);

#ifndef USE_MY_CODE
  res = AVIStreamWrite(audio, -1, audiobuffersize / audiosamplesize,
		       Streamer_GetSoundBuffer(shandle, state),
		       audiobuffersize, 0, 0, 0);
  if (FAILED(res)) {
    fprintf(stderr, "AVIStreamWrite(audio) returned %s\n", avierror(res));
    return;
  }
#endif

  Streamer_SetSoundDecodeMode(shandle, SSDM_IDLE);
  last_updated = state;
}

#ifdef __WINE__
static char *convert_path(const char *path) {
  WCHAR *dospath;
  char *newpath;

  dospath = wine_get_dos_file_name(path);
  if (!dospath) {
    fprintf(stderr, "Wine can't get a DOS path for %s\n", path);
    exit(1);
  }

  newpath = malloc(MAX_PATH);	/* leak */
  newpath[0] = '\0';
  WideCharToMultiByte(CP_UNIXCP, 0, dospath, -1, newpath, MAX_PATH, NULL, NULL);

  return newpath;
}
#else
#define convert_path(p) (p)
#endif


struct RPL {
  FILE* f;
  int loop;
  size_t nchunks, current_chunk;
  int width, height, bpp;
  float fps;
  int frames_per_chunk;
  int even_chunk_size, odd_chunk_size;
  int channels;

  long chunk_catalog_off, sprite_off, key_frame_off;

  struct rpl_chunk {
    unsigned long offset;
    size_t video_size, audio_size;
  } *chunks;

  int sound_format, sound_bits, audio_freq;
  unsigned char *audio_buffer;
  unsigned short *audio_decode_buffer;
} rpl;


static char *downcase_string(char *str)
{
  size_t i;
  for (i = 0; str[i]; i++) {
    if (isupper(str[i]))
      str[i] = tolower(str[i]);
  }
  return str;
}

static char *readline(FILE *f, char *linebuf, size_t len) {
	char c;
	size_t i = 0;

	for (i = 0; i < len-1; i++) {
	  if (fread(&c, 1, 1, f) != 1) {
	    perror("Error reading from sequence file");
	    break;
	  }
	  if (c == '\n' || !isprint(c))
	    break;
	  linebuf[i] = c;
	}

	linebuf[i] = '\0';

	return linebuf;
}

static int readint(FILE *f, char *linebuf, size_t len, char **rest) {
  int num, numread;
	readline(f, linebuf, len);
	if (sscanf(linebuf, "%u %n", &num, &numread) < 1) {
	  if (rest) *rest = "";
	  num = 0;
	}
	else if (rest)
	  *rest = downcase_string(&linebuf[numread]);

	return num;
}

static float readfloat(FILE *f, char *linebuf, size_t len, char **rest) {
	float num;
	int numread;
	readline(f, linebuf, len);
	if (sscanf(linebuf, "%f %n", &num, &numread) < 1) {
	  if (rest) *rest = "";
	  num = 0.0;
	} else if (rest)
	  *rest = downcase_string(&linebuf[numread]);

	return num;
}

static void init_rpl(const char* path) {
  char buf[80], *comment;
  int tmp;
  size_t i, len = sizeof(buf);
  size_t max_audio_size = 0;
  FILE* f = fopen(path, "rb");
  rpl.f = f;
  if (!f) {
    perror("Unable to open rpl file");
    exit(1);
  }

  rpl.chunks = NULL;
  rpl.current_chunk = 0;
  rpl.loop = 0;
  rpl.audio_buffer = NULL;
  rpl.audio_decode_buffer = NULL;

  if (strcmp(readline(f, buf, len), "ARMovie") != 0)
    fprintf(stderr, "Missing RPL magic number\n");

  readline(f, buf, len);
  /* fprintf(stderr, "Header filename: %s\n", buf); */
  readline(f, buf, len);
  /* fprintf(stderr, "Copyright: %s\n", buf); */
  readline(f, buf, len);
  /* fprintf(stderr, "Author: %s\n", buf); */

  tmp = readint(f, buf, len, NULL);
  switch (tmp) {
  case 130:
    /* init_dec130(&rpl->dec130_state);
       rpl->decode_video = rpl_dec130_decode; */
    break;
  case 13: /* mpeg? */
  case 18: /* h263? */
  default:
    fprintf(stderr, "Unknown video format %i\n", tmp);
    break;
  }

  rpl.width = readint(f, buf, len, NULL);
  rpl.height = readint(f, buf, len, NULL);
  rpl.bpp = readint(f, buf, len, &comment);
#if 0
  /* Eidos RPL files lie about this part of the 'standard' */
  if (!strstr(comment, "yuv"))
    debug(LOG_VIDEO, "%s: Header doesn't declare this as YUV."
	  " We only support YUV at present..\n", m->filename);
#endif

  rpl.fps = readfloat(f, buf, len, NULL);

  rpl.sound_format = readint(f, buf, len, &comment);
  rpl.audio_freq = readint(f, buf, len, NULL);
  rpl.channels = readint(f, buf, len, NULL);
  rpl.sound_bits = readint(f, buf, len, NULL);

#if 0
  switch (sound_format) {
  case 1: /* PCM */
    if (sound_bits == 8)
      m->audio_format =
	rpl->channels == 1 ? AL_FORMAT_MONO8 : AL_FORMAT_STEREO8;
    else if (sound_bits == 16)
      m->audio_format =
	rpl->channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
    else
      break;
    rpl->decode_audio = rpl_passthru_audio;
    break;
  case 101:
    if (al_supports_adpcm) {
      debug(LOG_VIDEO, "Using native OpenAL adpcm support (channels=%d)\n",
	    rpl->channels);
      m->audio_format =	rpl->channels == 1
	? AL_FORMAT_IMA_ADPCM_MONO16_EXT
	: AL_FORMAT_IMA_ADPCM_STEREO16_EXT;
      rpl->decode_audio = rpl_passthru_audio;
    }
    else {
      m->audio_format =	rpl->channels == 1
	? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
      rpl->decode_audio = rpl_adpcm_decode;
    }
    break;
  }

  if (sound_format && !rpl->decode_audio)
    debug(LOG_VIDEO, "Unknown RPL sound format %i (bits=%d, channels=%d)\n",
	  sound_format, sound_bits, rpl->channels);
#endif

  rpl.frames_per_chunk = readint(f, buf, len, NULL);
  rpl.nchunks = readint(f, buf, len, &comment);
  if (strstr(comment, "forever")) rpl.loop = 1;

  rpl.even_chunk_size = readint(f, buf, len, NULL);
  rpl.odd_chunk_size = readint(f, buf, len, NULL);
  rpl.chunk_catalog_off = readint(f, buf, len, NULL);
  rpl.sprite_off = readint(f, buf, len, NULL);
  rpl.key_frame_off = readint(f, buf, len, NULL);

  rpl.chunks = calloc(rpl.nchunks+1, sizeof(struct rpl_chunk));

  fseek(f, rpl.chunk_catalog_off, SEEK_SET);
  for (i = 0; i < rpl.nchunks+1; i++) {
    struct rpl_chunk *c = &(rpl.chunks[i]);
    readline(f, buf, len);
    if (sscanf(buf, "%lu , %zu ; %zu",
	       &c->offset, &c->video_size, &c->audio_size) != 3)
      fprintf(stderr, "Bad catalog line: %s\n", buf);

    if (c->audio_size > max_audio_size)
      max_audio_size = c->audio_size;
  }

  rpl.audio_buffer = malloc(max_audio_size);
  rpl.audio_decode_buffer = malloc(4*max_audio_size);
}

int main(int argc, char **argv) {
  char *inputfilename, *outputfilename, *unixinfile, *unixoutfile,
    *metafilename = NULL;
  LONG ret;
  LPMOVIEHANDLE mhandle;
  LPVIDEOHANDLE vhandle;
  LPSOUNDHANDLE shandle = NULL;
  LONG buffer_pixel_width, buffer_pixel_height;	/* what are these for?? */
  BYTE ap, ac, rp, rc, gp, gc, bp, bc;
  LPBYTE videobuffer, flipbuffer, soundbuffer;
  BOOL audio_finished;
  PAVIFILE avi;
  PAVISTREAM video;
  AVISTREAMINFO info;
  BITMAPINFOHEADER bi;
  HRESULT res;
  int pixelsize;
  size_t rowsize;
  struct adpcm_state adpcm_state;
  int arg;

  if (argc >= 3 && strcmp(argv[1], "--meta")==0) {
    metafilename = argv[2];
    arg = 3;
  }
  else
    arg = 1;

  if (argc - arg != 2) {
    fprintf(stderr, "Usage: %s [--meta FILE] <in.rpl> <out.avi>\n", argv[0]);
    exit(1);
  }

  unixinfile = argv[arg];
  unixoutfile = argv[arg+1];

  inputfilename = convert_path(unixinfile);
  outputfilename = convert_path(unixoutfile);


  ret = Streamer_InitMovie(&mhandle, NULL, 0, inputfilename,
			   STREAMER_BUFFERSIZE, SIM_NONE);

  if (ret != STREAMER_OK) {
    fprintf(stderr, "Streamer_InitMovie(%s) returned %s\n",
	    inputfilename, winstr_error(ret));
    exit(1);
  }

  printf("Name: %s\n", Movie_GetName(mhandle));
  printf("Author: %s\n", Movie_GetAuthor(mhandle));
  printf("Copyright: %s\n", Movie_GetCopyright(mhandle));
  if (metafilename) {
    FILE *f = fopen(metafilename, "w");
    if (!f) {
      perror(metafilename);
      exit(1);
    }
    fprintf(f, "--title='%s'\n", Movie_GetName(mhandle));
    fprintf(f, "--artist='%s'\n", Movie_GetAuthor(mhandle));
    fprintf(f, "--copyright='%s'\n", Movie_GetCopyright(mhandle));
    fclose(f);
  }

  AVIFileInit();

  res = AVIFileOpen(&avi, outputfilename, OF_WRITE|OF_CREATE, 0);
  if (FAILED(res)) {
    AVIFileExit();
    fprintf(stderr, "Error writing %s: %s\n", outputfilename, avierror(res));
    exit(1);
  }


  ret = Streamer_InitVideo(&vhandle, mhandle,
			   Movie_GetXSize(mhandle), Movie_GetYSize(mhandle),
			   0, 0, 0, 0, 0, 0,
			   DFLAG_INVIEWPORT,
			   &buffer_pixel_width, &buffer_pixel_height);
  if (ret != STREAMER_OK) {
    fprintf(stderr, "Streamer_InitVideo() returned %s\n", winstr_error(ret));
    exit(1);
  }
  Streamer_SetVideoPitch(vhandle,
			 Movie_GetXSize(mhandle), Movie_GetYSize(mhandle));


  switch (Movie_GetColourDepth(mhandle)) {
  default:
    fprintf(stderr, "Don't cope with native depth %ld yet, using 24bit..\n",
	    Movie_GetColourDepth(mhandle));
    /* fall through */
  case 32:			/* top byte unused, so really 24bit.. */
  case 24:
    /* BGR */
    pixelsize = 32;
    ap = 24;
    ac = 0;
    rp = 16;
    rc = 8;
    gp = 8;
    gc = 8;
    bp = 0;
    bc = 8;
    break;
  case 16:
    /* 555 RGB */
    pixelsize = 16;
    ap = 15;
    ac = 1;
    rp = 10;
    rc = 5;
    gp = 5;
    gc = 5;
    bp = 0;
    bc = 5;
    break;
  }

  printf("Found %ldx%ldx%ldbpp %gfps video stream\n",
	 Movie_GetXSize(mhandle), Movie_GetYSize(mhandle),
	 Movie_GetColourDepth(mhandle), Movie_GetFrameRate(mhandle));

  ret = Streamer_SetPixelFormat(vhandle, Movie_GetColourDepth(mhandle),
				ap, ac, rp, rc, gp, gc, bp, bc);
  if (ret != STREAMER_OK) {
    fprintf(stderr, "Streamer_SetPixelFormat returned %s\n", winstr_error(ret));
    exit(1);
  }


  memset(&info, 0, sizeof(info));
  info.fccType = streamtypeVIDEO;
  info.dwScale = 10000;
  info.dwRate = Movie_GetFrameRate(mhandle) * 10000;
  info.dwSuggestedBufferSize =
    Movie_GetXSize(mhandle) * Movie_GetYSize(mhandle) * pixelsize / 8;
  SetRect(&info.rcFrame, 0, 0,
	  Movie_GetXSize(mhandle), Movie_GetYSize(mhandle));
  res = AVIFileCreateStream(avi, &video, &info);
  if (FAILED(res)) {
    fprintf(stderr, "AVIFileCreateStream(video) returned %s\n", avierror(res));
    exit(1);
  }

  memset(&bi, 0, sizeof(bi));
  bi.biSize = sizeof(bi);
  bi.biWidth = Movie_GetXSize(mhandle);
  bi.biHeight = Movie_GetYSize(mhandle);
  bi.biBitCount = pixelsize;
  bi.biSizeImage = bi.biWidth * bi.biHeight * bi.biBitCount / 8;
  bi.biPlanes = 1;
  bi.biCompression = BI_RGB;
  res = AVIStreamSetFormat(video, 0, &bi, bi.biSize);
  if (FAILED(res)) {
    fprintf(stderr, "AVIStreamSetFormat(video) returned %s\n", avierror(res));
    exit(1);
  }



  if (Movie_GetSoundChannels(mhandle) &&
      Movie_GetSoundPrecision(mhandle) &&
      Movie_GetSoundRate(mhandle)) {
    long compression;
    BOOL pcm;
    long precision = Movie_GetSoundPrecision(mhandle);
    long channels = Movie_GetSoundChannels(mhandle);
    WAVEFORMATEX wf;

    if (precision == 4) {
      compression = 4;
      pcm = FALSE;
    } else {
      compression = 1;
      pcm = TRUE;
    }

    printf("Found %ldbit %ldHz %s audio stream\n",
	   precision, Movie_GetSoundRate(mhandle),
	   channels == 1 ? "mono" : "stereo");

    audiosamplesize = channels * STREAMER_AUDIOOUT_BITSIZE / 8;

    /* Should be one video frame's worth of (uncompressed) audio, so
     * we get a nicely interleaved AVI file.. */
    audiobuffersize = Movie_GetSoundRate(mhandle) * audiosamplesize /
      Movie_GetFrameRate(mhandle);

    ret = Streamer_InitSound(sound_callback, &shandle,
			     audiobuffersize,
			     compression, pcm, 4096, channels);
    if (ret != STREAMER_OK) {
      fprintf(stderr, "Streamer_InitSound() returned %s\n", winstr_error(ret));
      exit(1);
    }

    memset(&info, 0, sizeof(info));
    info.fccType = streamtypeAUDIO;
    info.dwSampleSize = audiosamplesize;
    info.dwScale = 1;
    info.dwRate = Movie_GetSoundRate(mhandle);
    info.dwQuality = -1;

    res = AVIFileCreateStream(avi, &audio, &info);
    if (FAILED(res)) {
      fprintf(stderr, "AVIFileCreateStream(audio) returned %s\n", avierror(res));
      exit(1);
    }

    memset(&wf, 0, sizeof(wf));
    wf.wFormatTag = WAVE_FORMAT_PCM;
    wf.nChannels = channels;
    wf.nSamplesPerSec = Movie_GetSoundRate(mhandle);
    wf.wBitsPerSample = STREAMER_AUDIOOUT_BITSIZE;
    wf.nBlockAlign = audiosamplesize;
    wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
    wf.cbSize = 0;

#if USE_MY_CODE
    init_rpl(unixinfile);
    adpcm_init(&adpcm_state);
#endif

    AVIStreamSetFormat(audio, 0, &wf, sizeof(WAVEFORMATEX));

    soundbuffer = malloc(audiobuffersize * 2);
    if (!soundbuffer) {
      fprintf(stderr, "Malloc failed, out of memory\n");
      exit(1);
    }

    Streamer_SetSoundBuffer(shandle, 0, soundbuffer);
    Streamer_SetSoundBuffer(shandle, 1, soundbuffer + audiobuffersize);
  }

  if (shandle)
    Streamer_SetSoundDecodeMode(shandle, SSDM_IDLE);
  last_updated = SSDM_SECONDBUFFER;

  videobuffer = malloc(Movie_GetXSize(mhandle) * Movie_GetYSize(mhandle) *
		       pixelsize / 8);
  flipbuffer = malloc(Movie_GetXSize(mhandle) * Movie_GetYSize(mhandle) *
		      pixelsize / 8);
  if (!videobuffer || !flipbuffer) {
    fprintf(stderr, "Out of memory allocating video buffer\n");
    exit(1);
  }

  /* used during image copy below */
  rowsize = pixelsize * Movie_GetXSize(mhandle) / 8;

#ifdef __WINE__
  /* HACK HACK HACK - work around 1024 index entry limit in wine 0.9.12.
   * See http://bugs.winehq.com/show_bug.cgi?id=5137 */
  if (Movie_GetTotalFrames(mhandle) > 1024)
    ((IAVIFileImpl*)avi)->cbIdxRecords =
      Movie_GetTotalFrames(mhandle) * sizeof(AVIINDEXENTRY);
#endif


  /* kicks the file into interleaved mode */
  AVIFileEndRecord(avi);


  /* This prefills audio buffer, etc */
  ret = Streamer_InitStreaming(mhandle, vhandle, shandle);
  if (ret != STREAMER_OK) {
    fprintf(stderr, "Streamer_InitStreaming returned %s\n", winstr_error(ret));
    exit(1);
  }


  audio_finished = (shandle == NULL);

  /* main loop */
  while (1) {
    LONG i;

    ret = Streamer_Stream(mhandle, vhandle, shandle, NULL,
			  1, /* framestoplay */
			  videobuffer, NULL,
			  NULL, 0);
    if (ret == STREAMER_BADDATARATE) {
      /* couldn't read enough in one go */
      continue;
    }
    else if (ret == STREAMER_FINISHEDVIDEO) {
      break;
    } else if (ret == STREAMER_FINISHEDAUDIO) {
      audio_finished = 1;
    } else if (ret != STREAMER_OK) {
      fprintf(stderr, "Streamer_Stream returned %s\n", winstr_error(ret));
      exit(1);
    }

    /* Streamer calls the audio callback during the *next* _Stream()
     * call, so we need to write the rec marker here, between the call
     * to _Stream() and writing out the "next" video frame */
    AVIFileEndRecord(avi);

    printf("[%ld] ", Movie_GetCurrentFrame(mhandle));

    /* Flip video frame vertically for AVI file.
     * NB: Can't flip videobuffer in-place, since Streamer seems to
     * only make incremental changes to buffer, rather than redrawing
     * completely each time */
    for (i = 0; i < Movie_GetYSize(mhandle); i++) {
      memcpy(flipbuffer + (Movie_GetYSize(mhandle) - i - 1) * rowsize,
	     videobuffer + i * rowsize,
	     rowsize);
    }

    res = AVIStreamWrite(video, -1, 1, flipbuffer,
			 Movie_GetXSize(mhandle) * Movie_GetYSize(mhandle) *
			 pixelsize / 8,
			 AVIIF_KEYFRAME, 0, 0);
    if (FAILED(res))
      fprintf(stderr, "AVIStreamWrite(video) returned %s\n", avierror(res));

    if (!audio_finished) {
#ifdef USE_MY_CODE
      HRESULT res;
      struct rpl_chunk *c = &rpl.chunks[Movie_GetCurrentFrame(mhandle)-1];
      fseek(rpl.f, c->offset + c->video_size, SEEK_SET);
      fread(rpl.audio_buffer, c->audio_size, 1, rpl.f);
      if (rpl.sound_format == 101 && rpl.sound_bits == 4) { /* adpcm */
	adpcm_decode(&adpcm_state, rpl.audio_buffer, c->audio_size,
		     rpl.audio_decode_buffer);
	size_t outbuflen = c->audio_size * 4;
	res = AVIStreamWrite(audio, -1, outbuflen/audiosamplesize,
			     rpl.audio_decode_buffer, outbuflen,
			     0, 0, 0);
      } else if (rpl.sound_format == 1 && rpl.sound_bits == 16) { /* sle16 */
	res = AVIStreamWrite(audio, -1, c->audio_size/audiosamplesize,
			     rpl.audio_buffer, c->audio_size,
			     0, 0, 0);
      } else {
	fprintf(stderr, "Unknown sound codec (fmt=%d bits=%d)\n",
		rpl.sound_format, rpl.sound_bits);
	exit(1);
      }
      if (FAILED(res)) {
	fprintf(stderr, "AVIStreamWrite(audio) returned %s\n", avierror(res));
	exit(1);
      }
#endif
      if (Streamer_GetSoundDecodeMode(shandle) == SSDM_IDLE) {
	/* No need for double buffering, since we're synchronously
	 * writing out the entire buffer to the AVI file.  Left here,
	 * since it shows the general mechanism Streamer expects to
	 * deal with. */
	LONG mode = last_updated == SSDM_FIRSTBUFFER ?
	  SSDM_SECONDBUFFER : SSDM_FIRSTBUFFER;
	Streamer_SetSoundDecodeMode(shandle, mode);
      }
    }
  }

  AVIFileEndRecord(avi);

  printf("Done.\n");

  AVIStreamRelease(video);
  if (shandle) AVIStreamRelease(audio);
  AVIFileRelease(avi);

  Streamer_ShutDownSound(&shandle);
  Streamer_ShutDownVideo(&vhandle);
  Streamer_ShutDownMovie(&mhandle);

  AVIFileExit();

  return 0;
}
