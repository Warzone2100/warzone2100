#include "netqueue.h"
#include "lib/framework/frame.h"

NetQueue::NetQueue()
	: canReadRawData(true)
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
		uint32_t len = buffer[used]<<24 | buffer[used + 1]<<16 | buffer[used + 2]<<8 | buffer[used + 3];
		uint8_t type = buffer[used + 4];
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

void NetQueue::willNeverReadRawData()
{
	canReadRawData = false;
}

void NetQueue::readRawData(const uint8_t **netData, size_t *netLen)
{
	ASSERT(canReadRawData, "Wrong NetQueue type for readRawData.");

	std::vector<uint8_t> &buffer = unsentMessageData;  // Short alias.

	// Turn the messages into raw data.
	while (dataPos != messages.begin())
	{
		--dataPos;
		buffer.push_back(dataPos->data.size()>>24);
		buffer.push_back(dataPos->data.size()>>16);
		buffer.push_back(dataPos->data.size()>> 8);
		buffer.push_back(dataPos->data.size()    );
		buffer.push_back(dataPos->type);
		buffer.insert(buffer.end(), dataPos->data.begin(), dataPos->data.end());
	}

	// Recycle old data.
	popOldMessages();

	// Return the data.
	*netData = &buffer[0];
	*netLen = buffer.size();
}

void NetQueue::popRawData(size_t netLen)
{
	ASSERT(canReadRawData, "Wrong NetQueue type for popRawData.");
	ASSERT(netLen <= unsentMessageData.size(), "Popped too much data!");

	// Pop the data.
	unsentMessageData.erase(unsentMessageData.begin(), unsentMessageData.begin() + netLen);
}

void NetQueue::pushMessage(const NetMessage &message)
{
	messages.push_front(message);
}

void NetQueue::willNeverGetMessages()
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
	if (!canReadRawData)
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
