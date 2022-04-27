#include "lib/framework/frame.h"
#include "oggopus.h"
#include "openal_callbacks.h"

// https://github.com/xiph/opusfile
// https://opus-codec.org/docs/opusfile_api-0.12/index.html
const OpusFileCallbacks wz_oggOpus_callbacks =
{
	wz_ogg_read2,
	// this one must return 0 on success, unlike its libvorbis counterpart
	wz_ogg_seek2,
	wz_ogg_tell,
	nullptr,
};

WZOpusDecoder* WZOpusDecoder::fromFilename(const char* fileName)
{
	PHYSFS_file *fileHandle = PHYSFS_openRead(fileName);
	debug(LOG_WZ, "Reading...[directory: %s] %s", WZ_PHYSFS_getRealDir_String(fileName).c_str(), fileName);
	if (fileHandle == nullptr)
	{
		debug(LOG_ERROR, "sound_LoadTrackFromFile: PHYSFS_openRead(\"%s\") failed with error: %s\n", fileName, WZ_PHYSFS_getLastError());
		PHYSFS_close(fileHandle);
		return nullptr;
	}
	int error = 0;
	OggOpusFile *of = op_open_callbacks(fileHandle, &wz_oggOpus_callbacks, nullptr, 0, &error);
	switch (error)
	{
	case OP_EREAD:
		debug(LOG_ERROR, "An underlying read, seek, or tell operation failed when it should have succeeded, or we failed to find data in the stream we had seen before.");
		return nullptr;
	case OP_EFAULT:
		debug(LOG_ERROR, "There was a memory allocation failure, or an internal library error.");
		return nullptr;
	case OP_EIMPL:
		debug(LOG_ERROR, "The stream used a feature that is not implemented, such as an unsupported channel family.");
		return nullptr;
	case OP_EINVAL:
		debug(LOG_ERROR, "seek() was implemented and succeeded on this source, but tell() did not, or the starting position indicator was not equal to _initial_bytes.");
		return nullptr;
	case OP_ENOTFORMAT:
		debug(LOG_ERROR, "The stream contained a link that did not have any logical Opus streams in it.");
		return nullptr;
	case OP_EBADHEADER:
		debug(LOG_ERROR, "A required header packet was not properly formatted, contained illegal values, or was missing altogether.");
		return nullptr;
	case OP_EVERSION:
		debug(LOG_ERROR, "An ID header contained an unrecognized version number.");
		return nullptr;
	case OP_EBADLINK:
		debug(LOG_ERROR, "We failed to find data we had seen before after seeking.");
		return nullptr;
	case OP_EBADTIMESTAMP:
		debug(LOG_ERROR, "The first or last timestamp in a link failed basic validity checks.");
		return nullptr;
	default:
		break;
	}
	ASSERT(error >= 0, "Unexpected libopusfile error %i", error);

	const OpusHead* head = op_head(of, -1);
	if (head == nullptr)
  {
		debug(LOG_ERROR, "OP failed to read header");
		PHYSFS_close(fileHandle);
    return nullptr;
  }
	WZOpusDecoder *decoder = new WZOpusDecoder(fileHandle, of, head);
	// Use a negative number to get the ID header information of the current link
	if (decoder == nullptr)
	{
		debug(LOG_ERROR, "OP failed to create decoder for %s", fileName);
		op_free(of);
		PHYSFS_close(fileHandle);
		return nullptr;
	}
	return decoder;
}

int WZOpusDecoder::decode(uint8_t* buffer, size_t bufferSize)
{

	// we need a small buffer to convert from big to little endian
	// sample_rate (48000/sec) * depth (2 bytes) * 120 milliseconds
	static const unsigned long TMP_BUF = 48 * 2 * 120; 
	opus_int16 pcm[TMP_BUF];

	size_t		bufferOffset = 0;
	int		    samples_per_chan = 0;
	m_bufferSize = bufferSize;
	// Decode PCM data into the buffer until there is nothing to decode left OR untill we hit bufferSize limit
	do
	{
		unsigned long spaceLeft = bufferSize - bufferOffset;
		unsigned long toRead = std::min(TMP_BUF, spaceLeft);

		// Note: the return value is the number of *samples per channel*, not *bytes* !!
		samples_per_chan = op_read_stereo(m_of, pcm, toRead);
		switch (samples_per_chan)
		{
		case OP_HOLE:
			debug(LOG_ERROR, "There was a hole in the data, and some samples may have been skipped. Call this function again to continue decoding past the hole.");
			return -1;
		case OP_EREAD:
			debug(LOG_ERROR, "An underlying read operation failed. This may signal a truncation attack from an <https:> source.");
			return -1;
		case OP_EFAULT:
			debug(LOG_ERROR, "An internal memory allocation failed.");
			return -1;
		case OP_EIMPL:
			debug(LOG_ERROR, "An unseekable stream encountered a new link that used a feature that is not implemented, such as an unsupported channel family.");
			return -1;
		case OP_EINVAL:
			debug(LOG_ERROR, "The stream was only partially open.");
			return -1;
		case OP_ENOTFORMAT:
			debug(LOG_ERROR, "An unseekable stream encountered a new link that did not have any logical Opus streams in it. ");
			return -1;
		case OP_EBADHEADER:
			debug(LOG_ERROR, "An unseekable stream encountered a new link with a required header packet that was not properly formatted, contained illegal values, or was missing altogether.");
			return -1;
		case OP_EVERSION:
			debug(LOG_ERROR, "An unseekable stream encountered a new link with an ID header that contained an unrecognized version number.");
			return -1;
		case OP_EBADPACKET:
			debug(LOG_ERROR, "Failed to properly decode the next packet.");
			return -1;
		case OP_EBADLINK:
			debug(LOG_ERROR, "We failed to find data we had seen before.");
			return -1;
		case OP_EBADTIMESTAMP:
			debug(LOG_ERROR, "An unseekable stream encountered a new link with a starting timestamp that failed basic validity checks.");
			return -1;
		default:
			break;
		}
		ASSERT(samples_per_chan > 0, "Unexpected Opus error %i", samples_per_chan);
		// convert to little endian
		for (int si = 0; si < m_nchannels * samples_per_chan; si++) 
		{
			buffer[bufferOffset + 0] = (uint8_t)(pcm[si] & 0xFF);
			buffer[bufferOffset + 1] = (uint8_t)(pcm[si] >> 8 & 0xFF);
			bufferOffset +=2;
		}
	} while ((samples_per_chan != 0 && bufferOffset < std::min(bufferSize, TMP_BUF)));
	return bufferOffset;
}
