#ifndef _NET_QUEUE_H_
#define _NET_QUEUE_H_

#include "lib/framework/types.h"

// TODO Add zlib to build scripts and use it.
// TODO But first move it directly on top of the sockets, seems it didn't really belong here. NetQueue
// was originally intended to be just a stream of bytes, which would be synchronised among all players.
// It ended up being a stream of messages prefixed with message size. And thanks to the need for
// broadcasting, the message streams can get mixed together, which isn't good if the message streams are
// individually zlib-compressed. While it would be possible to enable compression if being careful which
// queues it's enabled on, the compression code doesn't belong here anymore. Leaving it here for now,
// since the compression code is tested, and works. At the time of writing, the compression code is the
// only part of the code which is currently known to work well. It should be moved to a CompressPipe
// class, or something similar.
//#don't define USE_ZLIB

#ifdef __cplusplus
#include <vector>

// At game level:
// There should be a NetQueue representing each client.
// Each client should set their own queue to GameSend, or GameSolo if there are no other clients.
// The other queues should be set to GameReceive, or GameSolo if the queue belongs to an AI.
// Clients should serialise messages to their own queue (the one set to GameSend).
// Clients should call readRawData on their own queue, and send it over the network. (And popRawData when done.)
// Clients should receive data from remote clients, and call writeRawData on the corresponding queues.
// Clients should deserialise messages from all queues (including their own), such that all clients are processing the same data in the same way.

// At socket level:
// There should be two NetQueues per socket, one set to NetSend, the other set to NetReceive.

class NetQueue
{
public:
	enum UsagePattern
	{
		Unused,       ///< NetQueue is not used for anything.

		NetSend,      ///< For use with outgoing sockets. Will serialise and read raw data.
		NetReceive,   ///< For use with incoming sockets. Will write raw data and deserialise.

		GameSend,     ///< For use as a game order queue. Will serialise, deserialise and read raw data.
		GameReceive,  ///< For use as a game order queue. Will deserialise and write raw data.
		GameSolo      ///< For use as a game order queue. Will serialise and deserialise.
	};
	enum Compression
	{
		CompressionZlib = 0x00000001,  ///< Zlib compression.
		// TODO? CompressionLzma = 0x00000002,  ///< Lzma compression.
		CompressionMask = 0x00000000   ///< All compression types we support.
#ifdef USE_ZLIB
		| CompressionZlib
#endif //USE_ZLIB
	};

	class Writer
	{
	public:
		enum { Read, Write, Direction = Write };

		Writer(NetQueue *q = NULL) : queue(q) {}
		Writer(NetQueue &q) : queue(&q) {}
		void byte(uint8_t v) const { queue->serialise(v); }
		NetQueue *queue;
	};
	class Reader
	{
	public:
		enum { Read, Write, Direction = Read };

		Reader(NetQueue *q = NULL) : queue(q) {}
		Reader(NetQueue &q) : queue(&q) {}
		void byte(uint8_t &v) const { queue->deserialise(v); }
		NetQueue *queue;
	};

	NetQueue(UsagePattern p = Unused);
	~NetQueue();

	// Network related
	void writeRawData(const uint8_t *netData, size_t netLen);          ///< Inserts data from the network into the NetQueue.
	void readRawData(const uint8_t **netData, size_t *netLen);         ///< Extracts data from the NetQueue to send over the network.
	void popRawData(size_t netLen);                                    ///< Pops the extracted data, so that future readRawData calls do not return that data.

	// Serialise/deserialise related. All game clients should deserialise all queues, including their own.
	void endSerialiseLength();                                         ///< Must call after serialiseLength, but not call otherwise.
	bool endDeserialise();                                             ///< Returns true if deserialise succeeded, data has been consumed. Returns false if deserialise failed, due to not enough data yet.
	bool isDeserialiseError() const;                                   ///< Returns true if deserialise has failed.
	void serialiseLength();                                            ///< Do not call readRawData or beginDeserialise before calling endSerialiseLength! Makes room for the length, which will actually be serialised there when calling endSerialise.
	bool deserialiseHaveLength();                                      ///< Checks the length, and returns true iff that much data is available. Length will still be returned by deserialise.
	uint8_t deserialiseGetType();                                      ///< Returns the byte immediately after the length.
	void serialise(uint8_t v);                                         ///< Serialise a byte.
	void deserialise(uint8_t &v);                                      ///< Deserialise a byte.
	void setCompression(uint32_t compressionMask);                     ///< Enable compression if possible. Should call after serialising or deserialising an appropriate message.

private:
#ifdef USE_ZLIB
	void setCompressionZlib();                                         ///< Enable zlib compression.
#endif //USE_ZLIB
	void popOldData();                                                 ///< Pops any data that is no longer needed.

	// Disable copy constructor and assignment operator.
	NetQueue(const NetQueue &);
	void operator =(const NetQueue &);

	bool canSerialise;                                                 ///< True if we are the producer of data for this NetQueue.
	bool canDeserialise;                                               ///< True if we are the producer of data for this NetQueue.
	bool canWriteRaw;                                                  ///< True if we will call getDataForNetwork.
	bool canReadRaw;                                                   ///< True if we will call getDataForNetwork.
	bool deserialiseUnderflow;                                         ///< We ran out of data when deserialising.
	bool canCompress;                                                  ///< True if this is a NetQueue for a socket.
	bool isCompressed;                                                 ///< True if we turned compression on already.
	unsigned readOffset;                                               ///< Offset in data for reading.
	unsigned readSuccessOffset;                                        ///< Offset in data for reading, when success was last called.
	unsigned netOffset;                                                ///< Offset in data for writing to network.
	unsigned beginSerialiseOffset;                                     ///< Offset in data when serialiseLength was called.
	std::vector<uint8_t> data;                                         ///< Decompressed serialised data.
	std::vector<uint8_t> compressedData;                               ///< Compressed data.

#ifdef USE_ZLIB
	struct z_stream_s *stream;                                         ///< Non-null iff we are using zlib compression.
#endif //USE_ZLIB
};

class NetQueuePair
{
public:
	NetQueuePair() : send(NetQueue::NetSend), receive(NetQueue::NetReceive) {}

	NetQueue send;
	NetQueue receive;
};

extern "C"
{
#else
#error There isn''t currently any C interface to this.
#endif //__cplusplus

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_NET_QUEUE_H_
