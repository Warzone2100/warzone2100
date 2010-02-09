#include "netqueue.h"
#include "lib/framework/frame.h"
#ifdef USE_ZLIB
#include "zlib.h"
#endif //USE_ZLIB

NetQueue::NetQueue(UsagePattern p)
	: deserialiseUnderflow(false)
	, isCompressed(false)
	, readOffset(0)
	, readSuccessOffset(0)
	, netOffset(0)
#ifdef USE_ZLIB
	, stream(NULL)
#endif //USE_ZLIB
{
	switch (p)
	{
		case Unused:      canSerialise = false; canDeserialise = false; canWriteRaw = false; canReadRaw = false; canCompress = false; break;

		case NetSend:     canSerialise = true;  canDeserialise = false; canWriteRaw = false; canReadRaw = true;  canCompress = true;  break;
		case NetReceive:  canSerialise = false; canDeserialise = true;  canWriteRaw = true;  canReadRaw = false; canCompress = true;  break;

		case GameSend:    canSerialise = true;  canDeserialise = true;  canWriteRaw = false; canReadRaw = true;  canCompress = false; break;
		case GameReceive: canSerialise = false; canDeserialise = true;  canWriteRaw = true;  canReadRaw = false; canCompress = false; break;
		case GameSolo:    canSerialise = true;  canDeserialise = true;  canWriteRaw = false; canReadRaw = false; canCompress = false; break;
	}
	ASSERT(!canSerialise || !canWriteRaw, "Can't insert both objects and raw data into the same NetQueue.");
	ASSERT(!canWriteRaw || canDeserialise, "No point being able to write data into the NetQueue if we can't deserialise it.");
}

NetQueue::~NetQueue()
{
#ifdef USE_ZLIB
	if (stream != NULL)
	{
		if (canWriteRaw)
		{
			// Was writing compressed data into this NetQueue.
			inflateEnd(stream);
		}
		else
		{
			// Was reading compressed data from this NetQueue.
			deflateEnd(stream);
		}
		delete stream;
	}
#endif //USE_ZLIB
}

void NetQueue::writeRawData(const uint8_t *netData, size_t netLen)
{
	ASSERT(canWriteRaw, "Wrong NetQueue type for writeRawData.");

#ifdef USE_ZLIB
	if (stream != NULL)
	{
		compressedData.insert(compressedData.end(), netData, netData + netLen);
		stream->next_in = &compressedData[0];
		stream->avail_in = compressedData.size();
		do
		{
			uint8_t tmp[1024];
			memset(tmp, 0x00, sizeof(tmp));
			stream->next_out = tmp;
			stream->avail_out = sizeof(tmp);
			int ret = inflate(stream, Z_NO_FLUSH);
			ASSERT(ret == Z_OK || ret == Z_STREAM_END || ret == Z_BUF_ERROR, "Bad compressed data stream, zlib error, or out of memory.");

			// Insert the data.
			data.insert(data.end(), tmp, tmp + sizeof(tmp) - stream->avail_out);
		} while (stream->avail_out == 0);

		return;
	}
#endif //USE_ZLIB

	// Insert the data.
	data.insert(data.end(), netData, netData + netLen);
}

void NetQueue::readRawData(const uint8_t **netData, size_t *netLen)
{
	ASSERT(canReadRaw, "Wrong NetQueue type for readRawData.");

	// Return the data. (If not compressing.)
	*netData = &data[netOffset];
	*netLen = data.size() - netOffset;

#ifdef USE_ZLIB
	if (stream != NULL)
	{
		stream->next_in = const_cast<uint8_t *>(*netData);
		stream->avail_in = *netLen;
		do
		{
			uint8_t tmp[1024];
			stream->next_out = tmp;
			stream->avail_out = sizeof(tmp);
			int ret = deflate(stream, Z_SYNC_FLUSH);  // Z_SYNC_FLUSH = squeeze out every last byte of output immediately, even if it hurts compression.
			ASSERT(ret == Z_OK || ret == Z_STREAM_END || ret == Z_BUF_ERROR, "Bad compressed data stream, zlib error, or out of memory.");

			compressedData.insert(compressedData.end(), tmp, tmp + sizeof(tmp) - stream->avail_out);
		} while (stream->avail_out == 0);

		netOffset += *netLen;
		popOldData();

		// Return the compressed data instead of the uncompressed data.
		*netData = &compressedData[0];
		*netLen = compressedData.size();
	}
#endif //USE_ZLIB
}

void NetQueue::popRawData(size_t netLen)
{
	ASSERT(canReadRaw, "Wrong NetQueue type for popRawData.");

#ifdef USE_ZLIB
	if (stream != NULL)
	{
		ASSERT(netLen <= compressedData.size(), "Popped too much data!");
		compressedData.erase(compressedData.begin(), compressedData.begin() + netLen);

		return;
	}
#endif //USE_ZLIB

	netOffset += netLen;
	ASSERT(netOffset <= data.size(), "Popped too much data!");

	popOldData();
}

void NetQueue::endSerialiseLength()
{
	uint32_t len = data.size() - beginSerialiseOffset - 4;
	data[beginSerialiseOffset  ] = len>>24 & 0xFF;
	data[beginSerialiseOffset+1] = len>>16 & 0xFF;
	data[beginSerialiseOffset+2] = len>> 8 & 0xFF;
	data[beginSerialiseOffset+3] = len     & 0xFF;
}

bool NetQueue::endDeserialise()
{
	if (!deserialiseUnderflow)
	{
		// Success.
		readSuccessOffset = readOffset;  // Advance readSuccessOffset.
		popOldData();
		return true;
	}
	else
	{
		// Failure.
		readOffset = readSuccessOffset;  // Reset readOffset.
		deserialiseUnderflow = false;    // Reset deserialiseUnderflow.
		return false;
	}
}

bool NetQueue::isDeserialiseError() const
{
	return deserialiseUnderflow;
}

void NetQueue::serialiseLength()
{
	beginSerialiseOffset = data.size();

	// Reserve room for length.
	data.resize(data.size() + 4);
}

bool NetQueue::deserialiseHaveLength()
{
	if (readOffset + 4 > data.size())
	{
		return false;
	}
	uint32_t len = data[readOffset]<<24 | data[readOffset+1]<<16 | data[readOffset+2]<<8 | data[readOffset+3];
	return len < data.size() && readOffset + 4 <= data.size() - len;
}

uint8_t NetQueue::deserialiseGetType()
{
	return readOffset + 5 > data.size() ? 0 : data[readOffset+4];
}

void NetQueue::popOldData()
{
	if (!canDeserialise)
	{
		readSuccessOffset = data.size();
	}
	if (!canReadRaw)
	{
		netOffset = data.size();
	}

	size_t pop = std::min(readSuccessOffset, netOffset);
	if (pop <= data.size()/2)
	{
		return;  // Wait until we have more old data (or less new data), and pop all at once, since popping from the beginning is an O(data.size() - pop) operation.
	}
	data.erase(data.begin(), data.begin() + pop);
	readOffset        -= pop;  // readOffset        unused if !canDeserialise.
	readSuccessOffset -= pop;  // readSuccessOffset unused if !canDeserialise.
	netOffset         -= pop;  // netOffset         unused if !canReadRaw.
}

void NetQueue::serialise(uint8_t v)
{
	ASSERT(canSerialise, "Wrong NetQueue type for serialise.");

	// Serialise.
	data.push_back(v);
}

void NetQueue::deserialise(uint8_t &v)
{
	ASSERT(canDeserialise, "Wrong NetQueue type for deserialise.");

	if (canReadRaw && readOffset > netOffset)
	{
		debug(LOG_ERROR, "Deserialising data before it has been sent over the network, and that the other clients therefore haven't received yet.");
	}

	// Deserialise.
	if (readOffset + 1 > data.size())
	{
		deserialiseUnderflow = true;
		return;  // Not enough data.
	}

	v = data[readOffset++];
}

void NetQueue::setCompression(uint32_t compressionMask)
{
	ASSERT(canCompress, "Wrong NetQueue type for setCompression.");

#ifdef USE_ZLIB
	if (!isCompressed && (compressionMask & CompressionZlib) != 0)
	{
		setCompressionZlib();
		isCompressed = true;
	}
#endif //USE_ZLIB
}

#ifdef USE_ZLIB
void NetQueue::setCompressionZlib()
{
	stream = new z_stream;
	stream->zalloc = NULL;
	stream->zfree = NULL;
	stream->opaque = NULL;
	if (canWriteRaw)
	{
		// Reading compressed data from this NetQueue.
		stream->avail_in = 0;
		stream->next_in = NULL;
		int ret = inflateInit(stream);
		ASSERT(ret == Z_OK, "inflateInit failed. Bad zlib version?");

		ASSERT(readSuccessOffset == readOffset, "Called setCompression() after beginDeserialise() without endDeserialise()");
		// Reinsert the remaining data, now that we know that it's compressed.
		std::vector<uint8_t> tmp(data.begin() + readSuccessOffset, data.end());
		data.resize(readSuccessOffset);
		writeRawData(&tmp[0], tmp.size());
	}
	else
	{
		// Writing compressed data into this NetQueue.
		int ret = deflateInit(stream, Z_BEST_COMPRESSION);
		ASSERT(ret == Z_OK, "deflateInit failed. Bad zlib version?");

		// Don't compress data from before setCompression was called, since clients must agree on exactly when compression starts.
		compressedData.assign(data.begin() + netOffset, data.end());
		netOffset = data.size();
	}
}
#endif //USE_ZLIB

#if 0
extern "C" void testNetQueue()
{
	printf("Hello world!\n");
	{
		NetQueue read(NetQueue::NetReceive), write(NetQueue::NetSend);

		// Generate the data.
		uint8_t testString[] = "Hello world, this is a string which will be compressed. Compressed strings are good, because they use less space. Compressed strings use a lot less space, if they repeat things a lot, which is very good. As an example of repetition, remember the sentence 'Compressed strings use a lot less space, if they repeat things a lot, which is very good.'. You have seen it before, which means that 'Compressed strings use a lot less space, if they repeat things a lot, which is very good.' was repeated. We can even repeat 'Compressed strings are good, because they use less space.' if we want, and that will be compressed too. Because of repeating things like 'Compressed strings use a lot less space, if they repeat things a lot, which is very good.' and 'Compressed strings are good, because they use less space.' a lot, the compressed string is much shorter than the uncompressed string.";
		printf("Test string size = %d\n", int(strlen((char *)testString) + 1));
		for (int i = 0; true; ++i)
		{
			queue(write.beginSerialise(), testString[i]);
			if (testString[i] == 0x00)
			{
				break;
			}
			if (testString[i] == ' ')
			{
				write.setCompression(CompressionMask);
			}
			printf("%c", testString[i]);
		}
		printf("\n");

		// Send the data over the network.
		const uint8_t *rawData;
		size_t rawLen;
		write.readRawData(&rawData, &rawLen);
		printf("Raw data size = %d\n", int(rawLen));
		for (size_t i = 0; i != rawLen; ++i)
		{
			printf("%02X ", rawData[i]);  // Send this byte over the network.
		}
		printf("\n");
		read.writeRawData(rawData, rawLen);
		write.popRawData(rawLen);

		// Parse the data.
		while (true)
		{
			uint8_t result;
			queue(read.beginDeserialise(), result);
			read.endDeserialise();
			if (result == 0x00)
			{
				break;
			}
			if (result == ' ')
			{
				read.setCompression(CompressionMask);
			}
			printf("%c", result);
		}
		printf("\n");
	}
	printf("End hello world!\n");
}
#endif
