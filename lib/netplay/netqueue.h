#ifndef _NET_QUEUE_H_
#define _NET_QUEUE_H_

#include "lib/framework/types.h"

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

class NetMessage
{
public:
	NetMessage(uint8_t type_ = 0xFF) : type(type_) {}
	uint8_t type;
	std::vector<uint8_t> data;
};

class MessageWriter
{
public:
	enum { Read, Write, Direction = Write };

	MessageWriter(NetMessage *m = NULL) : message(m) {}
	MessageWriter(NetMessage &m) : message(&m) {}
	void byte(uint8_t v) const { message->data.push_back(v); }
	NetMessage *message;
};
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

class NetQueue
{
public:
	NetQueue();

	// Network related, receiving
	void writeRawData(const uint8_t *netData, size_t netLen);          ///< Inserts data from the network into the NetQueue.
	// Network related, sending
	void willNeverReadRawData();                                       ///< Marks that we will not be sending this data over the network.
	void readRawData(const uint8_t **netData, size_t *netLen);         ///< Extracts data from the NetQueue to send over the network.
	void popRawData(size_t netLen);                                    ///< Pops the extracted data, so that future readRawData calls do not return that data.

	// All game clients should check game messages from all queues, including their own, and only the net messages sent to them.
	// Message related, storing.
	void pushMessage(const NetMessage &message);                       ///< Adds a message to the queue.
	// Message related, extracting.
	void willNeverGetMessages();                                       ///< Marks that we will not be reading any of the messages (only sending over the network).
	bool haveMessage() const;                                          ///< Return true if we have a message ready to return.
	const NetMessage &getMessage() const;                              ///< Returns a message.
	void popMessage();                                                 ///< Pops the last returned message.

private:
	void popOldMessages();                                             ///< Pops any messages that are no longer needed.

	// Disable copy constructor and assignment operator.
	NetQueue(const NetQueue &);
	void operator =(const NetQueue &);

	bool canReadRawData;                                               ///< True if we will send the messages over the network, false if we don't.
	bool canGetMessages;                                               ///< True if we will get the messages, false if we don't use them ourselves.

	typedef std::list<NetMessage> List;
	List::iterator                dataPos;                             ///< Last message which was sent over the network.
	List::iterator                messagePos;                          ///< Last message which was popped.
	List                          messages;                            ///< List of messages. Messages are added to the front and read from the back.
	std::vector<uint8_t>          incompleteReceivedMessageData;       ///< Data from network which has not yet formed an entire message.
	std::vector<uint8_t>          unsentMessageData;                   ///< Data for network which has been requested but not yet popped.
};

class NetQueuePair
{
public:
	NetQueuePair() { send.willNeverGetMessages(); receive.willNeverReadRawData(); }

	NetQueue send;
	NetQueue receive;
};

#endif //_NET_QUEUE_H_
