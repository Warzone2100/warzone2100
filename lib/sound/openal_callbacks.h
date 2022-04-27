#ifndef _LIBSOUND_OPENAL_CALLBACKS_H_
#define _LIBSOUND_OPENAL_CALLBACKS_H_
#include "lib/framework/frame.h"
#include <physfs.h>
#include <limits>
#include "lib/framework/physfs_ext.h"
#include "codecs.h"

size_t wz_ogg_read(void *ptr, size_t size, size_t nmemb, void *datasource);
int wz_ogg_read2(void *_stream, unsigned char *_ptr, int _nbytes);
int wz_ogg_seek(void *datasource, int64_t offset, int whence);
int wz_ogg_seek2(void *datasource, int64_t offset, int whence);
int64_t wz_ogg_tell(void *datasource);

#endif