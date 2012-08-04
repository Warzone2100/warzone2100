#ifndef _NET_QUEUE_H_
#define _NET_QUEUE_H_

#include "lib/framework/frame.h"
#include <vector>
#include <list>
#include <deque>

// At game level:
// There should be a NetQueue representing each client.
// Clients should serialise messages to their own queue.
// Clients should call readRawData on their own queue, and send it over the network. (And popRawData when done.)
// Clients should receive data from remote clients, and call writeRawData on the corresponding queues.
// Clients should deserialise messages from all queues (including their own), such that all clients are processing the same data in the same way.

// At socket level:
// There should be a NetQueuePair per socket.


/// A NetMessage consists of a type (uint8_t) and some data, the meaning of which depends on the type.
class NetMessage
{
public:
	NetMessage(uint8_t type_ = 0xFF) : type(type_) {}
	uint8_t *rawDataDup() const;  ///< Returns data compatible with NetQueue::writeRawData(). Must be delete[]d.
	size_t rawLen() const;        ///< Returns the length of the return value of rawDataDup().
	uint8_t type;
	std::vector<uint8_t> data;
};

/// MessageWriter is used for serialising, using the same interface as MessageReader.
class MessageWriter
{
public:
	enum { Read, Write, Direction = Write };

	MessageWriter(NetMessage *m = NULL) : message(m) {}
	MessageWriter(NetMessage &m) : message(&m) {}
	void byte(uint8_t v) const { message->data.push_back(v); }
	bool valid() const { return true; }
	NetMessage *message;
};
/// MessageReader is used for deserialising, using the same interface as MessageWriter.
class MessageReader
{
public:
	enum { Read, Write, Direction = Read };

	MessageReader(const NetMessage *m = NULL) : message(m), index(0) {}
	MessageReader(const NetMessage &m) : message(&m), index(0) {}
	void byte(uint8_t &v) const { v = index >= message->data.size() ? 0x00 : message->data[index]; ++index; }
	bool valid() const { return index <= message->data.size(); }
	const NetMessage *message;
	mutable size_t index;
};

/// A NetQueue is a queue of NetMessages. A NetQueue can convert the messages into a stream of bytes, which can be sent over the network, and converted back into a queue of NetMessages by the NetQueue at the other end.
class NetQueue
{
public:
	NetQueue();

	// Network related, receiving
	void writeRawData(const uint8_t *netData, size_t netLen);          ///< Inserts data from the network into the NetQueue.
	// Network related, sending
	void setWillNeverGetMessagesForNet();                              ///< Marks that we will not be sending this data over the network.
	unsigned numMessagesForNet() const;                                ///< Checks that we didn't mark that we will not be sending this data over the network (returns 0), and returns the number of messages to be sent.
	const NetMessage &getMessageForNet() const;                        ///< Extracts data from the NetQueue to send over the network.
	void popMessageForNet();                                           ///< Pops the extracted data, so that future getMessageForNet calls do not return that data.

	// All game clients should check game messages from all queues, including their own, and only the net messages sent to them.
	// Message related, storing.
	void pushMessage(const NetMessage &message);                       ///< Adds a message to the queue.
	// Message related, extracting.
	void setWillNeverGetMessages();                                    ///< Marks that we will not be reading any of the messages (only sending over the network).
	bool haveMessage() const;                                          ///< Return true if we have a message ready to return.
	const NetMessage &getMessage() const;                              ///< Returns a message.
	void popMessage();                                                 ///< Pops the last returned message.

private:
	void popOldMessages();                                             ///< Pops any messages that are no longer needed.

	// Disable copy constructor and assignment operator.
	NetQueue(const NetQueue &);         // TODO When switching to C++0x, use "= delete" notation.
	void operator =(const NetQueue &);  // TODO When switching to C++0x, use "= delete" notation.

	bool canGetMessagesForNet;                                         ///< True if we will send the messages over the network, false if we don't.
	bool canGetMessages;                                               ///< True if we will get the messages, false if we don't use them ourselves.

	typedef std::list<NetMessage> List;
	List::iterator                dataPos;                             ///< Last message which was sent over the network.
	List::iterator                messagePos;                          ///< Last message which was popped.
	List                          messages;                            ///< List of messages. Messages are added to the front and read from the back.
	std::vector<uint8_t>          incompleteReceivedMessageData;       ///< Data from network which has not yet formed an entire message.
};

/// A NetQueuePair is used for talking to a socket. We insert NetMessages in the send NetQueue, which converts the messages into a stream of bytes for the
/// socket. We take incoming bytes from the socket and insert them in the receive NetQueue, which converts the bytes back into NetMessages.
class NetQueuePair
{
public:
	NetQueuePair() { send.setWillNeverGetMessages(); receive.setWillNeverGetMessagesForNet(); }

	NetQueue send;
	NetQueue receive;
};

/// Returns the number of bytes required to encode v.
unsigned encodedlength_uint32_t(uint32_t v);
/// Returns true iff there is another byte to be encoded.
/// Modifies v.
/// Input is v, output is b.
bool encode_uint32_t(uint8_t &b, uint32_t &v, unsigned n);
/// Returns true iff another byte is required to decode.
/// Must init v to 0, does not modify b.
/// Input is b, output is v.
bool decode_uint32_t(uint8_t b, uint32_t &v, unsigned n);

#endif //_NET_QUEUE_H_
