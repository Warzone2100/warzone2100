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
#include "lib/framework/pool_allocator.h"
#include "lib/netplay/byteorder_funcs_wrapper.h"
#include <vector>
#include <list>

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

using MsgDataAllocator = PoolAllocator<uint8_t, MemoryPool>;
using NetMsgDataVector = std::vector<uint8_t, MsgDataAllocator>;

// At game level:
// There should be a NetQueue representing each client.
// Clients should serialise messages to their own queue.
// Clients should call readRawData on their own queue, and send it over the network. (And popRawData when done.)
// Clients should receive data from remote clients, and call writeRawData on the corresponding queues.
// Clients should deserialise messages from all queues (including their own), such that all clients are processing the same data in the same way.

// At socket level:
// There should be a NetQueuePair per socket.

/// <summary>
/// Represents a network message with a type (uint8_t) and payload, the meaning of which depends on the type.
///
/// Instances of this class are immutable after creation, and should be created by using
/// the NetMessageBuilder class.
///
/// The binary layout of a NetMessage is as follows (stored internally as `std::vector<uint8_t>`):
///
/// | Type (1 byte) | Payload Length (2 bytes) | Payload (variable length) |
///
/// The first byte is the message type, described by a `uint8_t`, which should correspond to a value
/// from the `MESSAGE_TYPES` enumeration.
///
/// The payload length is the size of the payload in bytes, stored as a `uint16_t` in network byte order (big endian).
/// </summary>
class NetMessage
{
public:
	static constexpr size_t HEADER_LENGTH = sizeof(uint8_t) + sizeof(uint16_t); // (type) + (payload length)

	// Static factory method to create from raw data
	static optional<NetMessage> tryFromRawData(const uint8_t* buffer, size_t bufferLen);

	NetMessage(NetMessage&&) = default;
	NetMessage& operator=(NetMessage&&) = default;

	NetMessage(const NetMessage&) = default;
	NetMessage& operator=(const NetMessage&) = default;

	uint8_t type() const;
	const NetMsgDataVector& rawData() const;
	/// Returns a pointer to the payload data (offset by `HEADER_LENGTH` from the beginning of the raw data).
	const uint8_t* payload() const;
	size_t payloadSize() const;

	// Append the raw data of this message to the provided output vector.
	void rawDataAppendToVector(std::vector<uint8_t>& output) const;

private:

	// Meant to be executed only by NetMessageBuilder
	explicit NetMessage(NetMsgDataVector&& data);

	friend class NetMessageBuilder;

	NetMsgDataVector data_;
};

/// <summary>
/// This class is used to facilitate efficient creation of network messages.
/// It provides methods to append data (via `append()` calls) and to create a final message out of
/// this intermediate data vector (via `build()` call), effectively just moving the data into the `NetMessage`
/// that is being built.
///
/// NOTE: Once the `build()` method is called, the current `NetMessageBuilder` instance becomes invalid.
/// </summary>
class NetMessageBuilder
{
public:

	explicit NetMessageBuilder(uint8_t type, size_t reservedCapacity = 16);
	explicit NetMessageBuilder(NetMsgDataVector&& rawData);

	uint8_t type() const
	{
		return data_[0];
	}

	void append(uint8_t v)
	{
		data_.push_back(v);
	}

	void append(const uint8_t* src, size_t len)
	{
		auto resultLen = data_.size() + len;
		ASSERT_OR_RETURN(, resultLen <= UINT16_MAX, "Resulting message length exceeds uint16_t max: %zu", resultLen);
		data_.insert(data_.end(), src, src + len);
	}

	// Build the final message (invalidates NetMessageBuilder instance)
	NetMessage build()
	{
		// Update the length in the header
		auto len = data_.size() - NetMessage::HEADER_LENGTH;
		if (len > UINT16_MAX)
		{
			ASSERT(false, "Message payload length exceeds uint16_t max: %zu (message type: %u)", len, type());
			len = UINT16_MAX; // Clamp to max length, so we can still send the message and probably gracefully handle it on the other side.
		}
		const uint16_t payloadLen = static_cast<uint16_t>(len);
		// Store payload length in network byte order starting at the second byte of the data vector.
		wz_htons_store_unaligned(&data_[1], payloadLen);
		return NetMessage(std::move(data_));
	}

private:
	NetMsgDataVector data_;
};

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
		: queueInfo(queue), msgBuilder(messageType)
	{}

	MessageWriter(MessageWriter&&) = default;
	MessageWriter(const MessageWriter&) = default;
	MessageWriter& operator=(const MessageWriter&) = default;

	void byte(uint8_t v)
	{
		msgBuilder.append(v);
	}
	void bytes(uint8_t* pIn, size_t numBytes)
	{
		msgBuilder.append(pIn, numBytes);
	}
	void bytes(const uint8_t* pIn, size_t numBytes)
	{
		msgBuilder.append(pIn, numBytes);
	}
	void bytesVector(const std::vector<uint8_t>& vIn, size_t numBytes)
	{
		msgBuilder.append(vIn.data(), std::min(numBytes, vIn.size()));
	}
	bool valid() const
	{
		return true;
	}

	NETQUEUE queueInfo;
	bool bSecretMessageWrap = false;
	NetMessageBuilder msgBuilder;
};

/// MessageReader is used for deserialising, using the same interface as MessageWriter.
class MessageReader
{
public:

	explicit MessageReader(const NetMessage& m)
		: msgData(&m.rawData()), index(NetMessage::HEADER_LENGTH), msgType(m.type())
	{}

	MessageReader(MessageReader&&) = default;
	MessageReader(const MessageReader&) = default;
	MessageReader& operator=(const MessageReader&) = default;

	void byte(uint8_t &v) const
	{
		v = index >= msgData->size() ? 0x00 : (*msgData)[index];
		++index;
	}
	void bytes(uint8_t *pOut, size_t numBytes) const
	{
		size_t numCopyBytes = (index >= msgData->size()) ? 0 : std::min<size_t>(msgData->size() - index, numBytes);
		if (numCopyBytes > 0)
		{
			memcpy(pOut, &((*msgData)[index]), numCopyBytes);
		}
		if (numCopyBytes < numBytes)
		{
			memset(pOut + numCopyBytes, 0, numBytes - numCopyBytes);
		}
		index += numCopyBytes;
	}
	template <typename VecT>
	void bytesVector(VecT& vOut, size_t desiredBytes) const
	{
		static_assert(std::is_same<typename VecT::value_type, uint8_t>::value, "Expected vector<uint8_t> as the argument");

		size_t numCopyBytes = (index >= msgData->size()) ? 0 : std::min<size_t>(msgData->size() - index, desiredBytes);
		if (numCopyBytes > 0)
		{
			size_t startIdx = vOut.size();
			vOut.resize(vOut.size() + numCopyBytes);
			memcpy(&(vOut[startIdx]), &((*msgData)[index]), numCopyBytes);
		}
		index += numCopyBytes;
	}
	bool valid() const
	{
		return index <= msgData->size();
	}

	const NetMsgDataVector* msgData;
	mutable size_t index = NetMessage::HEADER_LENGTH;
	uint8_t msgType;
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
	void pushMessage(NetMessage&& message);                       ///< Adds a message to the queue.
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

	using MsgAllocator = PoolAllocator<NetMessage, MemoryPool>;
	using List = std::list<NetMessage, MsgAllocator>;
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
