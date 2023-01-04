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
 * @file netqueue.cpp
 *
 * Basic netqueue.
 */
#include "lib/framework/frame.h"
#include "netqueue.h"
#include "netplay.h"

#include <limits>
#include <cstdint>

// See comments in netqueue.h.


// Byte n is the final byte, iff it is less than 256-a[n].

// Some possible values of table_uint32_t_a[n], such that the maximum encodable value is 0xFFFFFFFF, satisfying the assertion below:
// ASSERT(0xFFFFFFFF == 255*(1 + a[0]*(1 + a[1]*(1 + a[2]*(1 + a[3]*(1 + a[4]))))), "Maximum encodable value not 0xFFFFFFFF.");

//static const unsigned table_uint32_t_a[5] = {14, 127, 74, 127, 0}, table_uint32_t_m[5] = {1, 14, 1778, 131572, 16709644};  // <242: 1 byte, <2048: 2 bytes, <325644: 3 bytes, <17298432: 4 bytes, <4294967296: 5 bytes
static const unsigned table_uint32_t_a[5] = {78, 95, 32, 70, 0}, table_uint32_t_m[5] = {1, 78, 7410, 237120, 16598400};  // <178: 1 byte, <12736: 2 bytes, <1672576: 3 bytes, <45776896: 4 bytes, <4294967296: 5 bytes
//static const unsigned table_uint32_t_a[5] = {78, 95, 71, 31, 0}, table_uint32_t_m[5] = {1, 78, 7410, 526110, 16309410};  // <178: 1 byte, <12736: 2 bytes, <1383586: 3 bytes, <119758336: 4 bytes, <4294967296: 5 bytes
//static const unsigned table_uint32_t_a[5] = {104, 71, 19, 119, 0}, table_uint32_t_m[5] = {1, 104, 7384, 140296, 16695224};  // <152: 1 byte, <19392: 2 bytes, <1769400: 3 bytes, <20989952: 4 bytes, <4294967296: 5 bytes
//static const unsigned table_uint32_t_a[5] = {104, 71, 20, 113, 0}, table_uint32_t_m[5] = {1, 104, 7384, 147680, 16687840};  // <152: 1 byte, <19392: 2 bytes, <1762016: 3 bytes, <22880256: 4 bytes, <4294967296: 5 bytes
//static const unsigned table_uint32_t_a[5] = {104, 71, 24, 94, 0}, table_uint32_t_m[5] = {1, 104, 7384, 177216, 16658304};  // <152: 1 byte, <19392: 2 bytes, <1732480: 3 bytes, <30441472: 4 bytes, <4294967296: 5 bytes
//static const unsigned table_uint32_t_a[5] = {104, 71, 30, 75, 0}, table_uint32_t_m[5] = {1, 104, 7384, 221520, 16614000};  // <152: 1 byte, <19392: 2 bytes, <1688176: 3 bytes, <41783296: 4 bytes, <4294967296: 5 bytes
//static const unsigned table_uint32_t_a[5] = {104, 71, 38, 59, 0}, table_uint32_t_m[5] = {1, 104, 7384, 280592, 16554928};  // <152: 1 byte, <19392: 2 bytes, <1629104: 3 bytes, <56905728: 4 bytes, <4294967296: 5 bytes
//static const unsigned table_uint32_t_a[5] = {104, 71, 40, 56, 0}, table_uint32_t_m[5] = {1, 104, 7384, 295360, 16540160};  // <152: 1 byte, <19392: 2 bytes, <1614336: 3 bytes, <60686336: 4 bytes, <4294967296: 5 bytes
//static const unsigned table_uint32_t_a[5] = {104, 71, 57, 39, 0}, table_uint32_t_m[5] = {1, 104, 7384, 420888, 16414632};  // <152: 1 byte, <19392: 2 bytes, <1488808: 3 bytes, <92821504: 4 bytes, <4294967296: 5 bytes
//static const unsigned table_uint32_t_a[5] = {104, 71, 60, 37, 0}, table_uint32_t_m[5] = {1, 104, 7384, 443040, 16392480};  // <152: 1 byte, <19392: 2 bytes, <1466656: 3 bytes, <98492416: 4 bytes, <4294967296: 5 bytes
//static const unsigned table_uint32_t_a[5] = {104, 71, 76, 29, 0}, table_uint32_t_m[5] = {1, 104, 7384, 561184, 16274336};  // <152: 1 byte, <19392: 2 bytes, <1348512: 3 bytes, <128737280: 4 bytes, <4294967296: 5 bytes
//static const unsigned table_uint32_t_a[5] = {104, 71, 95, 23, 0}, table_uint32_t_m[5] = {1, 104, 7384, 701480, 16134040};  // <152: 1 byte, <19392: 2 bytes, <1208216: 3 bytes, <164653056: 4 bytes, <4294967296: 5 bytes
//static const unsigned table_uint32_t_a[5] = {104, 71, 114, 19, 0}, table_uint32_t_m[5] = {1, 104, 7384, 841776, 15993744};  // <152: 1 byte, <19392: 2 bytes, <1067920: 3 bytes, <200568832: 4 bytes, <4294967296: 5 bytes
//static const unsigned table_uint32_t_a[5] = {104, 71, 120, 18, 0}, table_uint32_t_m[5] = {1, 104, 7384, 886080, 15949440};  // <152: 1 byte, <19392: 2 bytes, <1023616: 3 bytes, <211910656: 4 bytes, <4294967296: 5 bytes

//static const unsigned table_uint32_t_m[5] = {1, a[0], a[0]*a[1], a[0]*a[1]*a[2], a[0]*a[1]*a[2]*a[3]};

unsigned encodedlength_uint32_t(uint32_t v)
{
	for (unsigned n = 0;; ++n)
	{
		const unsigned a = table_uint32_t_a[n];
		const unsigned m = table_uint32_t_m[n];
		const unsigned possibleValues = (256 - a) * m; // Number of values which encode to n + 1 bytes.
		if (v < possibleValues)
		{
			return n + 1;
		}
		v -= possibleValues;
	}
}

bool encode_uint32_t(uint8_t &b, uint32_t &v, unsigned n)
{
	const unsigned a = table_uint32_t_a[n];

	bool isLastByte = v < 256 - a;
	if (isLastByte)
	{
		b = v;
	}
	else
	{
		v -= 256 - a;
		b = 255 - v % a;
		v /= a;
	}
	return !isLastByte;
}

bool decode_uint32_t(uint8_t b, uint32_t &v, unsigned n)
{
	const unsigned a = table_uint32_t_a[n];
	const unsigned m = table_uint32_t_m[n];

	bool isLastByte = b < 256 - a;
	if (isLastByte)
	{
		v += b * m;
	}
	else
	{
		v += (256 - a + 255 - b) * m;
	}
	return !isLastByte;
}

uint8_t *NetMessage::rawDataDup() const
{
#if SIZE_MAX > UINT32_MAX
	ASSERT_OR_RETURN(nullptr, data.size() <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()), "Trying to duplicate a very large packet (%zu bytes). (Message type: %" PRIu8 ")", data.size(), type);
#endif
	uint32_t dataSizeU32 = static_cast<uint32_t>(data.size());
	unsigned encodedLengthOfSize = encodedlength_uint32_t(dataSizeU32);

	uint8_t *ret = new uint8_t[1 + encodedLengthOfSize + dataSizeU32];

	ret[0] = type;
	uint32_t len = dataSizeU32;
	for (unsigned n = 0; n < encodedLengthOfSize; ++n)
	{
		encode_uint32_t(ret[n + 1], len, n);
	}

	std::copy(data.begin(), data.begin() + dataSizeU32, ret + 1 + encodedLengthOfSize);
	return ret;
}

void NetMessage::rawDataAppendToVector(std::vector<uint8_t> &output) const
{
#if SIZE_MAX > UINT32_MAX
	ASSERT(data.size() <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()), "Trying to duplicate a very large packet (%zu bytes).", data.size());
#endif
	uint32_t dataSizeU32 = static_cast<uint32_t>(data.size());

	unsigned encodedLengthOfSize = encodedlength_uint32_t(dataSizeU32);

	output.push_back(type);
	uint32_t len = dataSizeU32;
	for (unsigned n = 0; n < encodedLengthOfSize; ++n)
	{
		uint8_t b;
		encode_uint32_t(b, len, n);
		output.push_back(b);
	}

	output.insert(output.end(), data.begin(), data.end());
}

size_t NetMessage::rawLen() const
{
	return 1 + static_cast<size_t>(encodedlength_uint32_t(static_cast<uint32_t>(data.size()))) + data.size();
}

NetQueue::NetQueue()
	: canGetMessagesForNet(true)
	, canGetMessages(true)
	, pendingGameTimeUpdateMessages(0)
{
	dataPos = messages.end();
	messagePos = messages.end();
}

void NetQueue::writeRawData(const uint8_t *netData, size_t netLen)
{
	size_t used = 0;
	std::vector<uint8_t> &buffer = incompleteReceivedMessageData;  // Short alias.

	// Insert the data.
	buffer.insert(buffer.end(), netData, netData + netLen);

	// Extract the messages.
	while (buffer.size() - used > 1)
	{
		uint8_t type = buffer[used];

		uint32_t len = 0;
		bool moreBytes = true;
		unsigned n;
		for (n = 0; moreBytes && buffer.size() - used > 1 + n; ++n)
		{
			moreBytes = decode_uint32_t(buffer[used + 1 + n], len, n);
		}
		unsigned headerLen = 1 + n;

		ASSERT(len < 40000000, "Trying to write a very large packet (%u bytes) to the queue.", len);
		if (buffer.size() - used - headerLen < len)
		{
			break;  // Don't have a whole message ready yet.
		}

		messages.push_front(NetMessage(type));
		messages.front().data.assign(buffer.begin() + used + headerLen, buffer.begin() + used + headerLen + len);
		if (type == GAME_GAME_TIME)
		{
			++pendingGameTimeUpdateMessages;
		}
		used += headerLen + len;
	}

	// Recycle old data.
	buffer.erase(buffer.begin(), buffer.begin() + used);
}

void NetQueue::setWillNeverGetMessagesForNet()
{
	canGetMessagesForNet = false;
}

unsigned NetQueue::numMessagesForNet() const
{
	unsigned count = 0;
	if (canGetMessagesForNet)
	{
		for (List::iterator i = dataPos; i != messages.begin(); --i)
		{
			++count;
		}
	}

	return count;
}

const NetMessage &NetQueue::getMessageForNet() const
{
	ASSERT(canGetMessagesForNet, "Wrong NetQueue type for getMessageForNet.");
	ASSERT(dataPos != messages.begin(), "No message to get!");

	// Return the message.
	return internal_getMessageForNet();
}

void NetQueue::popMessageForNet()
{
	ASSERT(canGetMessagesForNet, "Wrong NetQueue type for popMessageForNet.");
	ASSERT(dataPos != messages.begin(), "No message to pop!");

	if (messagePos != messages.begin() && internal_getMessageForNet().type == GAME_GAME_TIME)
	{
		if (pendingGameTimeUpdateMessages > 0)
		{
			--pendingGameTimeUpdateMessages;
		}
	}

	// Pop the message.
	--dataPos;

	// Recycle old data.
	popOldMessages();
}

void NetQueue::pushMessage(const NetMessage &message)
{
	if (message.type == GAME_GAME_TIME)
	{
		++pendingGameTimeUpdateMessages;
	}
	messages.push_front(message);
}

void NetQueue::setWillNeverGetMessages()
{
	canGetMessages = false;
}

bool NetQueue::haveMessage() const
{
	ASSERT(canGetMessages, "Wrong NetQueue type for haveMessage.");
	return messagePos != messages.begin();
}

const NetMessage &NetQueue::getMessage() const
{
	ASSERT(canGetMessages, "Wrong NetQueue type for getMessage.");
	ASSERT(messagePos != messages.begin(), "No message to get!");

	// Return the message.
	return internal_getMessage();
}

void NetQueue::popMessage()
{
	ASSERT(canGetMessages, "Wrong NetQueue type for popMessage.");
	ASSERT(messagePos != messages.begin(), "No message to pop!");

	if (messagePos != messages.begin() && internal_getMessage().type == GAME_GAME_TIME)
	{
		if (pendingGameTimeUpdateMessages > 0)
		{
			--pendingGameTimeUpdateMessages;
		}
	}

	// Pop the message.
	--messagePos;

	// Recycle old data.
	popOldMessages();
}

void NetQueue::popOldMessages()
{
	if (!canGetMessagesForNet)
	{
		dataPos = messages.begin();
	}
	if (!canGetMessages)
	{
		messagePos = messages.begin();
	}

	List::iterator i = messages.end();
	while (i != dataPos && i != messagePos)
	{
		--i;
	}

	if (i == dataPos)
	{
		dataPos = messages.end();  // Old iterator will become invalid.
	}
	if (i == messagePos)
	{
		messagePos = messages.end();  // Old iterator will become invalid.
	}

	messages.erase(i, messages.end());
}
