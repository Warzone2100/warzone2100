#ifndef _LIBSOUND_CODECS_H_
#define _LIBSOUND_CODECS_H_

#include "lib/framework/frame.h"
#include <physfs.h>

class WZDecoder
{
public:
  WZDecoder() {};
  WZDecoder(const WZDecoder&)                 = delete;
  WZDecoder(WZDecoder&&)                      = delete;
  WZDecoder &operator=(const WZDecoder &)     = delete;
  WZDecoder &operator=(WZDecoder &&)          = delete;

  /** Decode all the data into a buffer.
   * Note that the API is different from oggvorbis: opus takes in, and gives out *samples* not bytes 
   * and we have to convert to little endian ourselves
   * 
   * \param [out] buffer: preallocated buffer to hold decoded data, little-endian. Will *not* be freed, even on error.
   * \param [in] bufferSize: takes buffer size in *bytes* as input, to be consistent with oggvorbis, 
   * \returns how much bytes have been placed into buffer, or negative number if error */
  virtual int     decode(uint8_t*, size_t) = 0;
  virtual int64_t totalTime()       const = 0;
  virtual int     channels()        const = 0;

  /** Sampling rate of the bitstream */
  virtual size_t  frequency()       const = 0;
  virtual ~WZDecoder()                      = default;

};
#endif
