#include "netqueue.h"
#include "lib/framework/frame.h"

// See comments in netqueue.h.

uint8_t *NetMessage::rawDataDup() const
{
	uint8_t *ret = new uint8_t[5 + data.size()];
	ret[0] = type;
	ret[1] = data.size()>>24;
	ret[2] = data.size()>>16;
	ret[3] = data.size()>> 8;
	ret[4] = data.size();
	std::copy(data.begin(), data.end(), ret + 5);
	return ret;
}

size_t NetMessage::rawLen() const
{
	return 5 + data.size();
}

NetQueue::NetQueue()
	: canGetMessagesForNet(true)
	, canGetMessages(true)
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
	while (buffer.size() - used >= 5)
	{
		uint8_t type = buffer[used];
		uint32_t len = buffer[used + 1]<<24 | buffer[used + 2]<<16 | buffer[used + 3]<<8 | buffer[used + 4];
		ASSERT(len < 40000000, "Trying to writing a very large packet (%u bytes) to the queue.", len);
		if (buffer.size() - used - 5 < len)
		{
			break;
		}
		messages.push_front(NetMessage(type));
		messages.front().data.assign(buffer.begin() + used + 5, buffer.begin() + used + 5 + len);
		used += 5 + len;
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
	List::iterator i = dataPos;
	--i;
	return *i;
}

void NetQueue::popMessageForNet()
{
	ASSERT(canGetMessagesForNet, "Wrong NetQueue type for popMessageForNet.");
	ASSERT(dataPos != messages.begin(), "No message to pop!");

	// Pop the message.
	--dataPos;

	// Recycle old data.
	popOldMessages();
}

void NetQueue::pushMessage(const NetMessage &message)
{
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
	List::iterator i = messagePos;
	--i;
	return *i;
}

void NetQueue::popMessage()
{
	ASSERT(canGetMessages, "Wrong NetQueue type for popMessage.");
	ASSERT(messagePos != messages.begin(), "No message to pop!");

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
