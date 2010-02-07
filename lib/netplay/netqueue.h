#ifndef _NET_QUEUE_H_
#define _NET_QUEUE_H_

#include "lib/framework/types.h"

// TODO Add zlib to build scripts and use it.
//#define USE_ZLIB

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
		NetSend,      ///< For use with outgoing sockets. Will serialise and read raw data.
		NetReceive,   ///< For use with incoming sockets. Will write raw data and deserialise.

		GameSend,     ///< For use as a game order queue. Will serialise, deserialise and read raw data.
		GameReceive,  ///< For use as a game order queue. Will deserialise and write raw data.
		GameSolo      ///< For use as a game order queue. Will serialise and deserialise.
	};

	class Writer
	{
	public:
		enum { Read, Write, Direction = Write };

		Writer(NetQueue &q) : queue(q) {}
		void byte(uint8_t v) const { queue.serialise(v); }
		NetQueue &queue;
	};
	class Reader
	{
	public:
		enum { Read, Write, Direction = Read };

		Reader(NetQueue &q) : queue(q) {}
		void byte(uint8_t &v) const { queue.deserialise(v); }
		NetQueue &queue;
	};

	NetQueue(UsagePattern p = NetReceive);
	~NetQueue();

	// Network related
	void writeRawData(const uint8_t *netData, size_t netLen);          ///< Inserts data from the network into the NetQueue.
	void readRawData(const uint8_t **netData, size_t *netLen);         ///< Extracts data from the NetQueue to send over the network.
	void popRawData(size_t netLen);                                    ///< Pops the extracted data, so that future readRawData calls do not return that data.

	// Serialise/deserialise related. All game clients should deserialise all queues, including their own.
	Writer beginSerialise() { return Writer(*this); }                  ///< No matching endSerialise.
	Reader beginDeserialise() { return Reader(*this); }
	bool endDeserialise();                                             ///< Returns true if deserialise succeeded, data has been consumed. Returns false if deserialise failed, due to not enough data yet.
	bool isDeserialiseError() const;                                   ///< Returns true if deserialise has failed.
	void serialise(uint8_t v);                                         ///< Serialise a byte.
	void deserialise(uint8_t &v);                                      ///< Deserialise a byte.
#ifdef USE_ZLIB
	void setCompression();                                             ///< Enable compression. Should call after serialising or deserialising an appropriate message.
#endif //USE_ZLIB

private:
	void popOldData();                                                 ///< Pops any data that is no longer needed.

	bool canSerialise;                                                 ///< True if we are the producer of data for this NetQueue.
	bool canDeserialise;                                               ///< True if we are the producer of data for this NetQueue.
	bool canWriteRaw;                                                  ///< True if we will call getDataForNetwork.
	bool canReadRaw;                                                   ///< True if we will call getDataForNetwork.
	bool deserialiseUnderflow;                                         ///< We ran out of data when deserialising.
	bool canCompress;                                                  ///< True if this is a NetQueue for a socket.
	unsigned readOffset;                                               ///< Offset in data for reading.
	unsigned readSuccessOffset;                                        ///< Offset in data for reading, when success was last called.
	unsigned netOffset;                                                ///< Offset in data for writing to network.
	std::vector<uint8_t> data;                                          ///< Decompressed serialised data.

#ifdef USE_ZLIB
	std::vector<uint8_t> compressedData;                                ///< Compressed data.
	struct z_stream_s *stream;                                         ///< Non-null iff we are using compression.
#endif //USE_ZLIB
};

extern "C"
{
#endif //__cplusplus

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_NET_QUEUE_H_
