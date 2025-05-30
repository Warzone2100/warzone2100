/*
	This file is part of Warzone 2100.
	Copyright (C) 2010-2020  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/**
 * @file netqueue.h
 *
 * Basic netqueue.
 */
#ifndef _NET_QUEUE_H_
#define _NET_QUEUE_H_

#include "lib/framework/frame.h"
#include <vector>
#include <list>
#include <deque>
#include <unordered_map>

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
	static bool tryFromRawData(const uint8_t* buffer, size_t bufferLen, NetMessage& output);
	uint8_t *rawDataDup() const;  ///< Returns data compatible with NetQueue::writeRawData(). Must be delete[]d.
	void rawDataAppendToVector(std::vector<uint8_t> &output) const;  ///< Appends data compatible with NetQueue::writeRawData() to the input vector.
	size_t rawLen() const;        ///< Returns the length of the return value of rawDataDup().
	uint8_t type;
	std::vector<uint8_t> data;
};
constexpr uint32_t NetMessageHeaderMaxBytes = 1 + 5; // 1 byte for type, 5 bytes max for length (see: encode_uint32_t)

struct NETQUEUE
{
	void* queue;  ///< Is either a (NetQueuePair **) or a (NetQueue *). (Note different numbers of *.)
	bool isPair;
	uint8_t index;
	uint8_t queueType;
	uint8_t exclude;
};

/// MessageWriter is used for serialising, using the same interface as MessageReader.
class MessageWriter
{
public:

	// This should allocate a message owned by this instance
	explicit MessageWriter(NETQUEUE queue, uint8_t messageType)
		: queueInfo(queue), message(messageType)
	{}

	MessageWriter(MessageWriter&&) = default;
	MessageWriter(const MessageWriter&) = default;
	MessageWriter& operator=(const MessageWriter&) = default;

	void byte(uint8_t v)
	{
		message.data.push_back(v);
	}
	void bytes(uint8_t *pIn, size_t numBytes)
	{
		message.data.insert(message.data.end(), pIn, pIn + numBytes);
	}
	void bytes(const uint8_t* pIn, size_t numBytes)
	{
		message.data.insert(message.data.end(), pIn, pIn + numBytes);
	}
	void bytesVector(std::vector<uint8_t> &vIn, size_t numBytes)
	{
		message.data.insert(message.data.end(), vIn.begin(), vIn.begin() + std::min(numBytes, vIn.size()));
	}
	bool valid() const
	{
		return true;
	}

	NETQUEUE queueInfo;
	bool bSecretMessageWrap = false;
	NetMessage message;
};

/// MessageReader is used for deserialising, using the same interface as MessageWriter.
class MessageReader
{
public:

	explicit MessageReader(NETQUEUE queue, const NetMessage& m)
		: queueInfo(queue), message(&m), index(0)
	{}

	MessageReader(MessageReader&&) = default;
	MessageReader(const MessageReader&) = default;
	MessageReader& operator=(const MessageReader&) = default;

	void byte(uint8_t &v) const
	{
		v = index >= message->data.size() ? 0x00 : message->data[index];
		++index;
	}
	void bytes(uint8_t *pOut, size_t numBytes) const
	{
		size_t numCopyBytes = (index >= message->data.size()) ? 0 : std::min<size_t>(message->data.size() - index, numBytes);
		if (numCopyBytes > 0)
		{
			memcpy(pOut, &(message->data[index]), numCopyBytes);
		}
		if (numCopyBytes < numBytes)
		{
			memset(pOut + numCopyBytes, 0, numBytes - numCopyBytes);
		}
		index += numCopyBytes;
	}
	void bytesVector(std::vector<uint8_t> &vOut, size_t desiredBytes) const
	{
		size_t numCopyBytes = (index >= message->data.size()) ? 0 : std::min<size_t>(message->data.size() - index, desiredBytes);
		if (numCopyBytes > 0)
		{
			size_t startIdx = vOut.size();
			vOut.resize(vOut.size() + numCopyBytes);
			memcpy(&(vOut[startIdx]), &(message->data[index]), numCopyBytes);
		}
		index += numCopyBytes;
	}
	bool valid() const
	{
		return index <= message->data.size();
	}

	NETQUEUE queueInfo;
	const NetMessage* message;
	mutable size_t index;
};

/// A NetQueue is a queue of NetMessages. A NetQueue can convert the messages into a stream of bytes, which can be sent over the network, and converted back into a queue of NetMessages by the NetQueue at the other end.
class NetQueue
{
public:
	NetQueue();

	// Disable copy constructor and assignment operator.
	NetQueue(const NetQueue &) = delete;
	void operator =(const NetQueue &) = delete;

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
	bool currentMessageWasDecrypted() const;
	bool replaceCurrentWithDecrypted(NetMessage &&decryptedMessage);
	void popMessage();                                                 ///< Pops the last returned message.

	size_t currentIncompleteDataBuffered() const;

	inline size_t numPendingGameTimeUpdateMessages() const
	{
		return pendingGameTimeUpdateMessages;
	}

private:
	void popOldMessages();                                             ///< Pops any messages that are no longer needed.

	bool canGetMessagesForNet;                                         ///< True if we will send the messages over the network, false if we don't.
	bool canGetMessages;                                               ///< True if we will get the messages, false if we don't use them ourselves.

	inline const NetMessage &internal_getMessageForNet() const
	{
		// Return the message.
		List::iterator i = dataPos;
		--i;
		return *i;
	};

	inline const NetMessage &internal_getConstMessage() const
	{
		// Return the message.
		List::iterator i = messagePos;
		--i;
		return *i;
	};

	inline NetMessage &internal_getMessage()
	{
		// Return the message.
		List::iterator i = messagePos;
		--i;
		return *i;
	};

	using List = std::list<NetMessage>;
	List::iterator                dataPos;                             ///< Last message which was sent over the network.
	List::iterator                messagePos;                          ///< Last message which was popped.
	List                          messages;                            ///< List of messages. Messages are added to the front and read from the back.
	std::vector<uint8_t>          incompleteReceivedMessageData;       ///< Data from network which has not yet formed an entire message.
	size_t                        pendingGameTimeUpdateMessages;       ///< Pending GAME_GAME_TIME messages added to this queue
	bool						  bCurrentMessageWasDecrypted;
};

/// A NetQueuePair is used for talking to a socket. We insert NetMessages in the send NetQueue, which converts the messages into a stream of bytes for the
/// socket. We take incoming bytes from the socket and insert them in the receive NetQueue, which converts the bytes back into NetMessages.
class NetQueuePair
{
public:
	NetQueuePair()
	{
		send.setWillNeverGetMessages();
		receive.setWillNeverGetMessagesForNet();
	}

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
