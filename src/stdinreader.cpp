/*
	This file is part of Warzone 2100.
	Copyright (C) 2020-2024  Warzone 2100 Project

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

#include "stdinreader.h"

#include "lib/framework/wzglobal.h" // required for config.h
#include "lib/framework/wzapp.h"
#include "lib/netplay/netpermissions.h"
#include "multiint.h"
#include "multistat.h"
#include "multilobbycommands.h"
#include "clparse.h"
#include "main.h"

#include <string>
#include <atomic>
#include <functional>
#include <memory>
#include <limits>

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wcast-align"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wcast-align"
#endif

#include <3rdparty/readerwriterqueue/readerwriterqueue.h>

#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif

typedef std::vector<char> CmdInterfaceMessageBuffer;
static std::unique_ptr<moodycamel::BlockingReaderWriterQueue<CmdInterfaceMessageBuffer>> cmdInterfaceOutputQueue;
static CmdInterfaceMessageBuffer latestWriteBuffer;
constexpr size_t maxReserveMessageBufferSize = 2048;

#if defined(WZ_OS_UNIX)
# include <fcntl.h>
# include <sys/select.h>
# include <sys/socket.h>
# include <sys/un.h>
# include <sys/param.h>
#endif

#if defined(HAVE_POLL_H)
# include <poll.h>
#elif defined(HAVE_SYS_POLL_H)
# include <sys/poll.h>
#endif

#if (defined(HAVE_POLL_H) || defined(HAVE_SYS_POLL_H)) && !defined(__APPLE__) // Do not use poll on macOS - ref: https://daniel.haxx.se/blog/2016/10/11/poll-on-mac-10-12-is-broken/
# define HAVE_WORKING_POLL
#endif

#if defined(HAVE_SYS_EVENTFD_H)
# include <sys/eventfd.h>
#elif defined(HAVE_UNISTD_H)
# include <unistd.h>
#endif

#define strncmpl(a, b) strncmp(a, b, strlen(b))
#define errlog(...) do { fprintf(stderr, __VA_ARGS__); fflush(stderr); } while(0);

#define wz_command_interface_output_onmainthread(...) \
wzAsyncExecOnMainThread([]{ \
	wz_command_interface_output(__VA_ARGS__); \
});

static WZ_Command_Interface wz_cmd_interface = WZ_Command_Interface::None;
static std::string wz_cmd_interface_param;

WZ_Command_Interface wz_command_interface()
{
	return wz_cmd_interface;
}

static WZ_THREAD *cmdInputThread = nullptr;
static WZ_THREAD *cmdOutputThread = nullptr;
static std::atomic<bool> cmdInputThreadQuit;
static std::atomic<bool> cmdOutputThreadQuit;
int readFd = -1;
bool readFdIsSocket = false;
int writeFd = -1;
bool writeFdIsNonBlocking = false;
std::function<void()> cleanupIOFunc;
#if defined(HAVE_SYS_EVENTFD_H)
int quitSignalEventFd = -1;
#elif defined(HAVE_UNISTD_H)
int quitSignalPipeFds[2] = {-1, -1};
#endif

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__) && (defined(HAVE_SYS_EVENTFD_H) || defined(HAVE_UNISTD_H))
# define WZ_STDIN_READER_SUPPORTED
#endif

#if defined(WZ_STDIN_READER_SUPPORTED)

static std::vector<char> stdInReadBuffer;
constexpr size_t readChunkSize = 1024;
static size_t newlineSearchStart = 0;
static size_t actualAvailableBytes = 0;

enum class CmdIOReadyStatus
{
	Error,
	NotReady,
	ReadyRead,
	ReadyWrite,
	Exit,
};
static CmdIOReadyStatus cmdIOIsReady(optional<int> inputFd, optional<int> outputFd, int quitSignalFd, int msTimeout = 2000)
{
#if defined(_WIN32)
# error "Not supported on Windows"
#endif

#if defined(HAVE_WORKING_POLL)
	struct pollfd pfds[3];
	nfds_t nfds = 0;
	optional<size_t> readIdx;
	optional<size_t> writeIdx;
	optional<size_t> quitIdx;
	if (inputFd.has_value())
	{
		pfds[nfds].fd = inputFd.value();
		pfds[nfds].events = POLLIN;
		pfds[nfds].revents = 0;
		readIdx = nfds;
		nfds++;
	}
	if (outputFd.has_value())
	{
		pfds[nfds].fd = outputFd.value();
		pfds[nfds].events = POLLOUT;
		pfds[nfds].revents = 0;
		writeIdx = nfds;
		nfds++;
	}
	if (quitSignalFd >= 0)
	{
		pfds[nfds].fd = quitSignalFd;
		pfds[nfds].events = POLLIN;
		pfds[nfds].revents = 0;
		quitIdx = nfds;
		nfds++;
	}
	int timeoutValue = (quitSignalFd >= 0) ? -1 : msTimeout;

	int retval = poll(pfds, nfds, timeoutValue);
	if (retval < 0)
	{
		return CmdIOReadyStatus::Error;
	}
	if (retval > 0)
	{
		if (quitIdx.has_value())
		{
			if (pfds[quitIdx.value()].revents & POLLIN)
			{
				return CmdIOReadyStatus::Exit;
			}
		}
		if (readIdx.has_value())
		{
			if (pfds[readIdx.value()].revents & POLLIN)
			{
				return CmdIOReadyStatus::ReadyRead;
			}
		}
		if (writeIdx.has_value())
		{
			if (pfds[writeIdx.value()].revents & POLLOUT)
			{
				return CmdIOReadyStatus::ReadyWrite;
			}
		}
	}
#else
	fd_set readset;
	fd_set writeset;
	FD_ZERO(&readset);
	FD_ZERO(&writeset);
	if (inputFd.has_value())
	{
		FD_SET(inputFd.value(), &readset);
	}
	if (outputFd.has_value())
	{
		FD_SET(outputFd.value(), &writeset);
	}
	if (quitSignalFd >= 0)
	{
		FD_SET(quitSignalFd, &readset);
	}
	int maxFd = std::max<int>({inputFd.value_or(-1), outputFd.value_or(-1), quitSignalFd});
	struct timeval tv;
	tv.tv_sec = msTimeout / 1000;
	tv.tv_usec = (msTimeout % 1000) * 1000;

	int retval = select(maxFd+1, &readset, (outputFd.has_value()) ? &writeset : NULL, NULL, (quitSignalFd >= 0) ? NULL : &tv);
	if (retval == -1)
	{
		return CmdIOReadyStatus::Error;
	}
	if (retval > 0 && quitSignalFd >= 0 && FD_ISSET(quitSignalFd, &readset))
	{
		return CmdIOReadyStatus::Exit;
	}
	if (inputFd.has_value())
	{
		if (retval > 0 && FD_ISSET(inputFd.value(), &readset))
		{
			return CmdIOReadyStatus::ReadyRead;
		}
	}
	if (outputFd.has_value())
	{
		if (retval > 0 && FD_ISSET(outputFd.value(), &writeset))
		{
			return CmdIOReadyStatus::ReadyWrite;
		}
	}
#endif

	return CmdIOReadyStatus::NotReady;
}

optional<std::string> getNextLineFromBuffer()
{
	for (size_t idx = newlineSearchStart; idx < actualAvailableBytes; ++idx)
	{
		if (stdInReadBuffer[idx] == '\n')
		{
			std::string nextLine(stdInReadBuffer.data(), idx);
			std::vector<decltype(stdInReadBuffer)::value_type>(stdInReadBuffer.begin()+(idx+1), stdInReadBuffer.end()).swap(stdInReadBuffer);
			newlineSearchStart = 0;
			actualAvailableBytes -= (idx + 1);
			return nextLine;
		}
	}
	newlineSearchStart = actualAvailableBytes;
	return nullopt;
}

// Returns true if read was successful, sets nextLine to the next available line (if any)
// Returns false if there was an unrecoverable error reading (such as a socket peer closing the socket), and no more reads should be attempted
bool getInputLine(int fd, bool isSocketFd, optional<std::string> &nextLine)
{
	// read more data from STDIN
	stdInReadBuffer.resize((((stdInReadBuffer.size() + readChunkSize - 1) / readChunkSize) + 1) * readChunkSize);
	ssize_t bytesRead = -1;
	if (!isSocketFd)
	{
		bytesRead = read(fd, stdInReadBuffer.data() + actualAvailableBytes, readChunkSize);
	}
	else
	{
		do {
			bytesRead = recv(fd, stdInReadBuffer.data() + actualAvailableBytes, readChunkSize, 0);
		} while (bytesRead == -1 && errno == EINTR);
		if (bytesRead == 0)
		{
			// stream socket peers will cause a return of "0" when there is an orderly shutdown
			return false;
		}
	}
	if (bytesRead < 0)
	{
		int errno_cpy = errno;
		errlog("Failed to read/recv with errno: %d\n", errno_cpy);
		return false;
	}
	if (bytesRead == 0)
	{
		return true;
	}
	actualAvailableBytes += static_cast<size_t>(bytesRead);
	nextLine = getNextLineFromBuffer();
	return true;
}

static void convertEscapedNewlines(std::string& input)
{
	// convert \\n -> \n
	size_t index = input.find("\\n");
	while (index != std::string::npos)
	{
#if !defined(__clang__) && defined(__GNUC__) && __GNUC__ >= 12
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wrestrict"
#endif
		input.replace(index, 2, "\n");
#if !defined(__clang__) && defined(__GNUC__) && __GNUC__ >= 12
#pragma GCC diagnostic pop
#endif
		index = input.find("\\n", index + 1);
	}
}

int cmdOutputThreadFunc(void *)
{
	if (!cmdInterfaceOutputQueue)
	{
		errlog("WZCMD FAILURE: No output queue?\n");
		return 1;
	}

	int quitSignalFd = -1;
#if defined(HAVE_SYS_EVENTFD_H)
	quitSignalFd = quitSignalEventFd;
#elif defined(HAVE_UNISTD_H)
	quitSignalFd = quitSignalPipeFds[0];
#endif

#if defined(MSG_NOSIGNAL) && !defined(__APPLE__)
	int writeFlags = MSG_NOSIGNAL;
#else
	int writeFlags = 0;
#endif

	// Returns true if it wrote, false if it didn't (and the caller should wait until writing is possible)
	// Returns nullopt if an unrecoverable error occurred
	auto tryWrite = [](int writeFd, const CmdInterfaceMessageBuffer& msg, int writeFlags) -> optional<bool> {
		// Attempt to write
		ssize_t outputBytes = -1;
		do {
			outputBytes = send(writeFd, msg.data(), msg.size(), writeFlags);
		} while (outputBytes == -1 && errno == EINTR);

		if (outputBytes == -1)
		{
			switch (errno)
			{
				case EAGAIN:
				#if EWOULDBLOCK != EAGAIN
				case EWOULDBLOCK:
				#endif
					// Must wait to be ready to write
					return false;
				case EPIPE:
					// Other end died / closed connection
					errlog("WZCMD NOTICE: Other end closed connection\n");
					return nullopt;
				default:
					// Unrecoverable
					errlog("WZCMD FAILURE: Unrecoverable error trying to send data\n");
					return nullopt;
			}
		}
		else
		{
			// wrote it
			return true;
		}
	};

	CmdInterfaceMessageBuffer msg;
	while (true)
	{
		if (!cmdInterfaceOutputQueue->wait_dequeue_timed(msg, std::chrono::milliseconds(2000)))
		{
			// no messages to write - check if signaled to quit
			auto result = cmdIOIsReady(nullopt, writeFd, quitSignalFd);
			if (result == CmdIOReadyStatus::Exit)
			{
				// quit thread
				return 0;
			}
			else if (result == CmdIOReadyStatus::NotReady)
			{
				if (cmdOutputThreadQuit.load())
				{
					// quit thread
					return 0;
				}
			}
			// loop and check for a msg again
			continue;
		}

		// Have a message to write

		if (writeFdIsNonBlocking)
		{
			// Attempt to write
			auto writeResult = tryWrite(writeFd, msg, writeFlags);
			if (!writeResult.has_value())
			{
				// unrecoverable error
				return 1;
			}
			if (writeResult.value())
			{
				// write succeeded - continue
				continue;
			}
			else
			{
				// write failed - need to wait to be ready to write
				// fall-through
			}
		}

		bool successfullyWroteMsg = false;
		do {
			// Wait to be ready to write
			auto result = cmdIOIsReady(nullopt, writeFd, -1); // do not wait on the quit signal fd here - all messages must be sent!
			if (result == CmdIOReadyStatus::Exit)
			{
				// ignore, and retry
				continue;
			}
			else if (result == CmdIOReadyStatus::NotReady)
			{
				// retry
				continue;
			}
			else if (result == CmdIOReadyStatus::ReadyWrite)
			{
				// Attempt to write
				// Can still fail, so need to handle if it does
				auto writeResult = tryWrite(writeFd, msg, writeFlags);
				if (!writeResult.has_value())
				{
					// unrecoverable error
					return 1;
				}
				if (writeResult.value())
				{
					// write succeeded
					successfullyWroteMsg = true;
				}
				else
				{
					// write failed - need to wait to be ready to write
					// loop again
					continue;
				}
			}
			else
			{
				return 1;
			}
		} while (!successfullyWroteMsg);
	}
	return 0;
}

static bool applyToActivePlayerWithIdentity(const std::string& playerIdentityStrCopy, const std::function<void (uint32_t playerIdx)>& func)
{
	bool foundActivePlayer = false;
	for (int i = 0; i < MAX_CONNECTED_PLAYERS; i++)
	{
		if (!isHumanPlayer(i))
		{
			continue;
		}

		bool matchingPlayer = false;
		auto& identity = getMultiStats(i).identity;
		if (identity.empty())
		{
			if (playerIdentityStrCopy == "0") // special case for empty identity, in case that happens...
			{
				matchingPlayer = true;
			}
			else
			{
				continue;
			}
		}

		if (!matchingPlayer)
		{
			// Check playerIdentityStrCopy versus both the (b64) public key and the public hash
			std::string checkIdentityHash = identity.publicHashString();
			std::string checkPublicKeyB64 = base64Encode(identity.toBytes(EcKey::Public));
			if (playerIdentityStrCopy == checkPublicKeyB64 || playerIdentityStrCopy == checkIdentityHash)
			{
				matchingPlayer = true;
			}
		}

		if (matchingPlayer)
		{
			func(static_cast<uint32_t>(i));
			foundActivePlayer = true;
		}
	}

	return foundActivePlayer;
}

static bool changeHostChatPermissionsForActivePlayerWithIdentity(const std::string& playerIdentityStrCopy, bool freeChatEnabled)
{
	// special-cases:
	if (playerIdentityStrCopy == "all")
	{
		NETsetDefaultMPHostFreeChatPreference(freeChatEnabled);
		if (NetPlay.isHost && NetPlay.bComms)
		{
			// update all existing slots
			for (uint32_t player = 0; player < MAX_CONNECTED_PLAYERS; ++player)
			{
				ingame.hostChatPermissions[player] = NETgetDefaultMPHostFreeChatPreference();
			}
			sendHostConfig();
		}
		wz_command_interface_output("WZEVENT: hostChatPermissions=%s: all\n", (freeChatEnabled) ? "Y" : "N");
		return true;
	}
	else if (playerIdentityStrCopy == "newjoin")
	{
		NETsetDefaultMPHostFreeChatPreference(freeChatEnabled);
		wz_command_interface_output("WZEVENT: hostChatPermissions=%s: newjoin\n", (freeChatEnabled) ? "Y" : "N");
		return true;
	}
	else if (playerIdentityStrCopy == "host")
	{
		if (NetPlay.isHost && NetPlay.bComms)
		{
			// update host slot
			if (NetPlay.hostPlayer < MAX_CONNECTED_PLAYERS)
			{
				ingame.hostChatPermissions[NetPlay.hostPlayer] = freeChatEnabled;
				sendHostConfig();
				wz_command_interface_output("WZEVENT: hostChatPermissions=%s: host\n", (freeChatEnabled) ? "Y" : "N");
				return true;
			}
		}
		return false;
	}

	bool result = applyToActivePlayerWithIdentity(playerIdentityStrCopy, [freeChatEnabled](uint32_t i) {
		if (i == NetPlay.hostPlayer)
		{
			wz_command_interface_output("WZCMD error: Can't mute / unmute host!\n");
			return;
		}
		ingame.hostChatPermissions[i] = freeChatEnabled;

		// other clients will automatically display a notice of the change, but display one locally for the host as well
		const char *pPlayerName = getPlayerName(i);
		std::string playerNameStr = (pPlayerName) ? pPlayerName : (std::string("[p") + std::to_string(i) + "]");
		std::string msg;
		if (freeChatEnabled)
		{
			msg = astringf(_("Host: Free chat enabled for: %s"), playerNameStr.c_str());
		}
		else
		{
			msg = astringf(_("Host: Free chat muted for: %s"), playerNameStr.c_str());
		}
		displayRoomSystemMessage(msg.c_str());

		std::string playerPublicKeyB64 = base64Encode(getMultiStats(i).identity.toBytes(EcKey::Public));
		std::string playerIdentityHash = getMultiStats(i).identity.publicHashString();
		std::string playerVerifiedStatus = (ingame.VerifiedIdentity[i]) ? "V" : "?";
		std::string playerName = NetPlay.players[i].name;
		std::string playerNameB64 = base64Encode(std::vector<unsigned char>(playerName.begin(), playerName.end()));
		wz_command_interface_output("WZEVENT: hostChatPermissions=%s: %" PRIu32 " %" PRIu32 " %s %s %s %s %s\n", (freeChatEnabled) ? "Y" : "N", i, gameTime, playerPublicKeyB64.c_str(), playerIdentityHash.c_str(), playerVerifiedStatus.c_str(), playerNameB64.c_str(), NetPlay.players[i].IPtextAddress);
	});

	if (result)
	{
		sendHostConfig();
	}
	return result;
}

static bool kickActivePlayerWithIdentity(const std::string& playerIdentityStrCopy, const std::string& kickReasonStrCopy, bool banPlayer)
{
	return applyToActivePlayerWithIdentity(playerIdentityStrCopy, [&](uint32_t i) {
		if (i == NetPlay.hostPlayer)
		{
			wz_command_interface_output("WZCMD error: Can't kick host!\n");
			return;
		}
		const char *pPlayerName = getPlayerName(i);
		std::string playerNameStr = (pPlayerName) ? pPlayerName : (std::string("[p") + std::to_string(i) + "]");
		kickPlayer(i, kickReasonStrCopy.c_str(), ERROR_KICKED, banPlayer);
		auto KickMessage = astringf("Player %s was kicked by the administrator.", playerNameStr.c_str());
		sendRoomSystemMessage(KickMessage.c_str());
	});
}

int cmdInputThreadFunc(void *)
{
	fseek(stdin, 0, SEEK_END);
	wz_command_interface_output_onmainthread("WZCMD: stdinReadReady\n");
	bool inexit = false;
	int quitSignalFd = -1;
#if defined(HAVE_SYS_EVENTFD_H)
	quitSignalFd = quitSignalEventFd;
#elif defined(HAVE_UNISTD_H)
	quitSignalFd = quitSignalPipeFds[0];
#endif
	while (!inexit)
	{
		optional<std::string> nextLine = getNextLineFromBuffer();
		while (!nextLine.has_value())
		{
			auto result = cmdIOIsReady(readFd, nullopt, quitSignalFd);
			if (result == CmdIOReadyStatus::Exit)
			{
				// quit thread
				return 0;
			}
			else if (result == CmdIOReadyStatus::NotReady)
			{
				if (cmdInputThreadQuit.load())
				{
					// quit thread
					return 0;
				}
			}
			else if (result == CmdIOReadyStatus::ReadyRead)
			{
				if (!getInputLine(readFd, readFdIsSocket, nextLine))
				{
					errlog("WZCMD FAILURE: get input line failed! (did peer close the connection?)\n");
					return 1;
				}
				break;
			}
			else
			{
				errlog("WZCMD FAILURE: getline failed!\n");
				return 1;
			}
		}

		if (!nextLine.has_value())
		{
			// continue & wait until we read a full line
			continue;
		}
		const char *line = nextLine.value().c_str();

		if(!strncmpl(line, "exit"))
		{
			wz_command_interface_output_onmainthread("WZCMD info: exit command received - stdin reader will now stop procesing commands\n");
			inexit = true;
		}
		else if(!strncmpl(line, "admin add-hash "))
		{
			char newadmin[1024] = {0};
			int r = sscanf(line, "admin add-hash %1023[^\n]s", newadmin);
			if (r != 1)
			{
				wz_command_interface_output_onmainthread("WZCMD error: Failed to add room admin hash! (Expecting one parameter)\n");
			}
			else
			{
				std::string newAdminStrCopy(newadmin);
				wzAsyncExecOnMainThread([newAdminStrCopy]{
					wz_command_interface_output("WZCMD info: Room admin hash added: %s\n", newAdminStrCopy.c_str());
					addLobbyAdminIdentityHash(newAdminStrCopy);
					auto roomAdminMessage = astringf("Room admin assigned to: %s", newAdminStrCopy.c_str());
					sendRoomSystemMessage(roomAdminMessage.c_str());
				});
			}
		}
		else if(!strncmpl(line, "admin add-public-key "))
		{
			char newadmin[1024] = {0};
			int r = sscanf(line, "admin add-public-key %1023[^\n]s", newadmin);
			if (r != 1)
			{
				wz_command_interface_output_onmainthread("WZCMD error: Failed to add room admin public key! (Expecting one parameter)\n");
			}
			else
			{
				std::string newAdminStrCopy(newadmin);
				wzAsyncExecOnMainThread([newAdminStrCopy]{
					wz_command_interface_output("WZCMD info: Room admin public key added: %s\n", newAdminStrCopy.c_str());
					addLobbyAdminPublicKey(newAdminStrCopy);
					auto roomAdminMessage = astringf("Room admin assigned to: %s", newAdminStrCopy.c_str());
					sendRoomSystemMessage(roomAdminMessage.c_str());
				});
			}
		}
		else if(!strncmpl(line, "admin remove "))
		{
			char newadmin[1024] = {0};
			int r = sscanf(line, "admin remove %1023[^\n]s", newadmin);
			if (r != 1)
			{
				wz_command_interface_output_onmainthread("WZCMD error: Failed to remove room admin! (Expecting one parameter)\n");
			}
			else
			{
				std::string newAdminStrCopy(newadmin);
				wzAsyncExecOnMainThread([newAdminStrCopy]{
					if (removeLobbyAdminPublicKey(newAdminStrCopy))
					{
						wz_command_interface_output("WZCMD info: Room admin public key removed: %s\n", newAdminStrCopy.c_str());
						auto roomAdminMessage = astringf("Room admin removed: %s", newAdminStrCopy.c_str());
						sendRoomSystemMessage(roomAdminMessage.c_str());
					}
					else if (removeLobbyAdminIdentityHash(newAdminStrCopy))
					{
						wz_command_interface_output("WZCMD info: Room admin hash removed: %s\n", newAdminStrCopy.c_str());
						auto roomAdminMessage = astringf("Room admin removed: %s", newAdminStrCopy.c_str());
						sendRoomSystemMessage(roomAdminMessage.c_str());
					}
					else
					{
						wz_command_interface_output("WZCMD info: Failed to remove room admin! (Provided parameter not found as either admin hash or public key)\n");
					}
				});
			}
		}
		else if(!strncmpl(line, "kick identity "))
		{
			char playeridentitystring[1024] = {0};
			char kickreasonstr[1024] = {0};
			int r = sscanf(line, "kick identity %1023s %1023[^\n]s", playeridentitystring, kickreasonstr);
			if (r != 1 && r != 2)
			{
				wz_command_interface_output_onmainthread("WZCMD error: Failed to get player public key or hash!\n");
			}
			else
			{
				std::string playerIdentityStrCopy(playeridentitystring);
				std::string kickReasonStrCopy = (r >= 2) ? kickreasonstr : "You have been kicked by the administrator.";
				convertEscapedNewlines(kickReasonStrCopy);
				wzAsyncExecOnMainThread([playerIdentityStrCopy, kickReasonStrCopy] {
					if (NetPlay.hostPlayer < MAX_PLAYERS && ingame.TimeEveryoneIsInGame.has_value())
					{
						// host is not a spectator host
						wz_command_interface_output("WZCMD error: Failed to execute in-game kick command - not a spectator host\n");
						return;
					}
					bool foundActivePlayer = kickActivePlayerWithIdentity(playerIdentityStrCopy, kickReasonStrCopy, false);
					if (!foundActivePlayer)
					{
						wz_command_interface_output("WZCMD info: kick identity %s: failed to find currently-connected player with matching public key or hash\n", playerIdentityStrCopy.c_str());
					}
				});
			}
		}
		else if(!strncmpl(line, "permissions set connect:allow "))
		{
			char playeridentitystring[1024] = {0};
			int r = sscanf(line, "permissions set connect:allow %1023[^\n]s", playeridentitystring);
			if (r != 1 && r != 2)
			{
				wz_command_interface_output_onmainthread("WZCMD error: Failed to get player public key or hash!\n");
			}
			else
			{
				std::string playerIdentityStrCopy(playeridentitystring);
				wzAsyncExecOnMainThread([playerIdentityStrCopy] {
					netPermissionsSet_Connect(playerIdentityStrCopy, ConnectPermissions::Allowed);
				});
			}
		}
		else if(!strncmpl(line, "permissions set connect:block "))
		{
			char playeridentitystring[1024] = {0};
			int r = sscanf(line, "permissions set connect:block %1023[^\n]s", playeridentitystring);
			if (r != 1 && r != 2)
			{
				wz_command_interface_output_onmainthread("WZCMD error: Failed to get player public key or hash!\n");
			}
			else
			{
				std::string playerIdentityStrCopy(playeridentitystring);
				std::string kickReasonStrCopy = "You have been kicked by the administrator.";
				wzAsyncExecOnMainThread([playerIdentityStrCopy, kickReasonStrCopy] {
					netPermissionsSet_Connect(playerIdentityStrCopy, ConnectPermissions::Blocked);
					if (NetPlay.hostPlayer < MAX_PLAYERS)
					{
						// host is not a spectator host
						return;
					}
					kickActivePlayerWithIdentity(playerIdentityStrCopy, kickReasonStrCopy, true);
				});
			}
		}
		else if(!strncmpl(line, "permissions unset connect "))
		{
			char playeridentitystring[1024] = {0};
			int r = sscanf(line, "permissions unset connect %1023[^\n]s", playeridentitystring);
			if (r != 1 && r != 2)
			{
				wz_command_interface_output_onmainthread("WZCMD error: Failed to get player public key or hash!\n");
			}
			else
			{
				std::string playerIdentityStrCopy(playeridentitystring);
				wzAsyncExecOnMainThread([playerIdentityStrCopy] {
					netPermissionsUnset_Connect(playerIdentityStrCopy);
				});
			}
		}
		else if(!strncmpl(line, "set chat "))
		{
			char chatlevel[1024] = {0};
			char playeridentitystring[1024] = {0};
			int r = sscanf(line, "set chat %1023s %1023[^\n]s", chatlevel, playeridentitystring);
			if (r != 2)
			{
				wz_command_interface_output_onmainthread("WZCMD error: Failed to get chatlevel or identity!\n");
			}
			else
			{
				bool freeChatEnabled = true;
				if (strcmp(chatlevel, "allow") == 0)
				{
					freeChatEnabled = true;
				}
				else if ((strcmp(chatlevel, "quickchat") == 0) || (strcmp(chatlevel, "mute") == 0))
				{
					freeChatEnabled = false;
				}
				else
				{
					wz_command_interface_output_onmainthread("WZCMD error: Unsupported set chat chatlevel!\n");
					continue;
				}
				std::string playerIdentityStrCopy(playeridentitystring);
				std::string chatLevelStrCopy(chatlevel);
				wzAsyncExecOnMainThread([playerIdentityStrCopy, chatLevelStrCopy, freeChatEnabled] {
					bool foundActivePlayer = changeHostChatPermissionsForActivePlayerWithIdentity(playerIdentityStrCopy, freeChatEnabled);
					if (!foundActivePlayer)
					{
						wz_command_interface_output("WZCMD info: set chat %s %s: failed to find currently-connected player with matching public key or hash\n", chatLevelStrCopy.c_str(), playerIdentityStrCopy.c_str());
					}
				});
			}
		}
		else if(!strncmpl(line, "ban ip "))
		{
			char tobanip[1024] = {0};
			char banreasonstr[1024] = {0};
			int r = sscanf(line, "ban ip %1023s %1023[^\n]s", tobanip, banreasonstr);
			if (r != 1 && r != 2)
			{
				wz_command_interface_output_onmainthread("WZCMD error: Failed to get ban ip!\n");
			}
			else
			{
				std::string banIPStrCopy(tobanip);
				std::string banReasonStrCopy = (r >= 2) ? banreasonstr : "You have been banned from joining by the administrator.";
				convertEscapedNewlines(banReasonStrCopy);
				wzAsyncExecOnMainThread([banIPStrCopy, banReasonStrCopy] {
					if (NetPlay.hostPlayer < MAX_PLAYERS && ingame.TimeEveryoneIsInGame.has_value())
					{
						// host is not a spectator host
						wz_command_interface_output("WZCMD error: Failed to execute in-game ban command - not a spectator host\n");
						return;
					}
					bool foundActivePlayer = false;
					for (int i = 0; i < MAX_CONNECTED_PLAYERS; i++)
					{
						auto player = NetPlay.players[i];
						if (!isHumanPlayer(i))
						{
							continue;
						}
						if (!strcmp(player.IPtextAddress, banIPStrCopy.c_str()))
						{
							kickPlayer(i, banReasonStrCopy.c_str(), ERROR_INVALID, true);
							auto KickMessage = astringf("Player %s was banned by the administrator.", player.name);
							sendRoomSystemMessage(KickMessage.c_str());
							foundActivePlayer = true;
						}
					}
					if (!foundActivePlayer)
					{
						// add the IP to the ban list anyway
						addIPToBanList(banIPStrCopy.c_str(), "Banned player");
					}
				});
			}
		}
		else if(!strncmpl(line, "unban ip "))
		{
			char tounbanip[1024] = {0};
			int r = sscanf(line, "unban ip %1023[^\n]s", tounbanip);
			if (r != 1)
			{
				wz_command_interface_output_onmainthread("WZCMD error: Failed to get unban ip!\n");
			}
			else
			{
				std::string unbanIPStrCopy(tounbanip);
				wzAsyncExecOnMainThread([unbanIPStrCopy] {
					if (!removeIPFromBanList(unbanIPStrCopy.c_str()))
					{
						wz_command_interface_output("WZCMD error: IP was not on the ban list!\n");
					}
				});
			}
		}
		else if(!strncmpl(line, "chat bcast "))
		{
			char chatmsg[1024] = {0};
			int r = sscanf(line, "chat bcast %1023[^\n]s", chatmsg);
			if (r != 1)
			{
				wz_command_interface_output_onmainthread("WZCMD error: Failed to get bcast message!\n");
			}
			else
			{
				std::string chatmsgstr(chatmsg);
				wzAsyncExecOnMainThread([chatmsgstr] {
					if (!NetPlay.isHostAlive)
					{
						// can't send this message when the host isn't alive
						wz_command_interface_output("WZCMD error: Failed to send bcast message because host isn't yet hosting!\n");
					}
					sendRoomSystemMessage(chatmsgstr.c_str());
				});
			}
		}
		else if(!strncmpl(line, "chat direct "))
		{
			char playeridentitystring[1024] = {0};
			char chatmsg[1024] = {0};
			int r = sscanf(line, "chat direct %1023s %1023[^\n]s", playeridentitystring, chatmsg);
			if (r != 2)
			{
				wz_command_interface_output_onmainthread("WZCMD error: Failed to get chat receiver or message!\n");
			}
			else
			{
				std::string playerIdentityStrCopy(playeridentitystring);
				std::string chatmsgstr(chatmsg);
				wzAsyncExecOnMainThread([playerIdentityStrCopy, chatmsgstr] {
					if (!NetPlay.isHostAlive)
					{
						// can't send this message when the host isn't alive
						wz_command_interface_output("WZCMD error: Failed to send chat direct message because host isn't yet hosting!\n");
					}

					bool foundActivePlayer = false;
					for (uint32_t i = 0; i < MAX_CONNECTED_PLAYERS; i++)
					{
						auto player = NetPlay.players[i];
						if (!isHumanPlayer(i))
						{
							continue;
						}

						bool msgThisPlayer = false;
						auto& identity = getMultiStats(i).identity;
						if (identity.empty())
						{
							if (playerIdentityStrCopy == "0") // special case for empty identity, in case that happens...
							{
								msgThisPlayer = true;
							}
							else
							{
								continue;
							}
						}

						if (!msgThisPlayer)
						{
							// Check playerIdentityStrCopy versus both the (b64) public key and the public hash
							std::string checkIdentityHash = identity.publicHashString();
							std::string checkPublicKeyB64 = base64Encode(identity.toBytes(EcKey::Public));
							if (playerIdentityStrCopy == checkPublicKeyB64 || playerIdentityStrCopy == checkIdentityHash)
							{
								msgThisPlayer = true;
							}
						}

						if (msgThisPlayer)
						{
							sendRoomSystemMessageToSingleReceiver(chatmsgstr.c_str(), i);
							foundActivePlayer = true;
						}
					}
					if (!foundActivePlayer)
					{
						wz_command_interface_output("WZCMD info: chat direct %s: failed to find currently-connected player with matching public key or hash\n", playerIdentityStrCopy.c_str());
					}
				});
			}
		}
		else if(!strncmpl(line, "join "))
		{
			char action[1024] = {0};
			char uniqueJoinID[1024] = {0};
			char rejectionMessage[MAX_JOIN_REJECT_REASON] = {0};
			unsigned int rejectionReason = static_cast<unsigned int>(ERROR_NOERROR);
			int r = sscanf(line, "join %1023s %1023s %u %2047[^\n]s", action, uniqueJoinID, &rejectionReason, rejectionMessage);
			if (r < 2 || r > 4)
			{
				wz_command_interface_output_onmainthread("WZCMD error: Failed to parse join command!\n");
			}
			else
			{
				optional<AsyncJoinApprovalAction> approve = nullopt;
				if (strcmp(action, "approve") == 0)
				{
					approve = AsyncJoinApprovalAction::Approve;
				}
				else if (strcmp(action, "reject") == 0)
				{
					approve = AsyncJoinApprovalAction::Reject;
				}
				else if (strcmp(action, "approvespec") == 0)
				{
					approve = AsyncJoinApprovalAction::ApproveSpectators;
				}
				if (approve.has_value() && rejectionReason < static_cast<unsigned int>(std::numeric_limits<uint8_t>::max()))
				{
					auto approveValue = approve.value();
					std::string uniqueJoinIDCopy(uniqueJoinID);
					std::string rejectionMessageCopy(rejectionMessage);
					convertEscapedNewlines(rejectionMessageCopy);
					wzAsyncExecOnMainThread([uniqueJoinIDCopy, approveValue, rejectionReason, rejectionMessageCopy] {
						if (!NETsetAsyncJoinApprovalResult(uniqueJoinIDCopy, approveValue, static_cast<LOBBY_ERROR_TYPES>(rejectionReason), rejectionMessageCopy))
						{
							wz_command_interface_output("WZCMD info: Could not find currently-waiting join with specified uniqueJoinID\n");
						}
					});
				}
				else
				{
					wz_command_interface_output_onmainthread("WZCMD error: Invalid action or rejectionReason passed to join approve/reject command\n");
				}
			}
		}
		else if(!strncmpl(line, "autobalance"))
		{
			wzAsyncExecOnMainThread([] {
				if (autoBalancePlayersCmd())
				{
					wz_command_interface_output("WZCMD info: autobalanced players\n");
				}
				else
				{
					wz_command_interface_output("WZCMD error: autobalance failed\n");
				}
			});
		}
		else if(!strncmpl(line, "status"))
		{
			wzAsyncExecOnMainThread([] {
				wz_command_interface_output_room_status_json();
			});
		}
		else if(!strncmpl(line, "shutdown now"))
		{
			inexit = true;
			wzAsyncExecOnMainThread([] {
				wz_command_interface_output("WZCMD info: shutdown now command received - shutting down\n");
				wzQuit(0);
			});
		}
	}
	return 0;
}

static void sockBlockSIGPIPE(const int fd, bool block_sigpipe)
{
#if defined(SO_NOSIGPIPE)
	const int no_sigpipe = block_sigpipe ? 1 : 0;

	if (setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &no_sigpipe, sizeof(no_sigpipe)) != 0)
	{
		errlog("WZCMD INFO: Failed to set SO_NOSIGPIPE on socket, SIGPIPE might be raised when connection gets broken.\n");
	}
#else
	// Prevent warnings
	(void)fd;
	(void)block_sigpipe;
#endif
}

bool cmdInterfaceCreateUnixSocket(const std::string& local_path)
{
	if (local_path.empty())
	{
		errlog("WZCMD FAILURE: Requested socket path is empty - cancelling\n");
		return false;
	}
	int socket_type = SOCK_STREAM;
#if defined(SOCK_CLOEXEC)
	socket_type |= SOCK_CLOEXEC;
#endif
	int cmdSockFd = socket(AF_UNIX, socket_type, 0);
	if (cmdSockFd == -1)
	{
		int errno_cpy = errno;
		errlog("WZCMD FAILURE: socket() call failed with errno: %d\n", errno_cpy);
		return false;
	}
	sockBlockSIGPIPE(cmdSockFd, true);

	sockaddr_un addr;
	socklen_t sockLen = sizeof(addr);
	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", local_path.c_str());
	if (strlen(addr.sun_path) != local_path.length())
	{
		errlog("WZCMD FAILURE: Requested socket path %s would be truncated to %s - cancelling\n", local_path.c_str(), addr.sun_path);
		return false;
	}
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__) || defined(__OpenBSD__) || defined(__NetBSD__)
	addr.sun_len = SUN_LEN(&addr);
	sockLen = addr.sun_len;
#endif

	unlink(addr.sun_path);

	if (bind(cmdSockFd, reinterpret_cast<sockaddr*>(&addr), sockLen) != 0)
	{
		int errno_cpy = errno;
		errlog("WZCMD FAILURE: bind() failed with errno: %d\n", errno_cpy);
		return false;
	}
	auto listenResult = listen(cmdSockFd, 1);
	if (listenResult != 0)
	{
		int errno_cpy = errno;
		errlog("WZCMD FAILURE: listen() failed with errno: %d\n", errno_cpy);
		return false;
	}
	struct sockaddr_storage ss;
	socklen_t slen = sizeof(ss);
	int cmdPeerFd = -1;
	do {
#if defined(SOCK_CLOEXEC)
		cmdPeerFd = accept4(cmdSockFd,  (struct sockaddr*)&ss, &slen, SOCK_CLOEXEC);
#else
		cmdPeerFd = accept(cmdSockFd,  (struct sockaddr*)&ss, &slen);
#endif
	} while (cmdPeerFd == -1 && errno == EINTR);
	if (cmdPeerFd == -1)
	{
		int errno_cpy = errno;
		errlog("WZCMD FAILURE: accept() failed with errno: %d\n", errno_cpy);
		close(cmdSockFd);
		return false;
	}

	if (fcntl(cmdPeerFd, F_SETFL, O_NONBLOCK) == 0)
	{
		writeFdIsNonBlocking = true;
	}
	else
	{
		int errno_cpy = errno;
		errlog("WZCMD NOTICE: Attempt to set non-blocking flag failed with errno: %d\n", errno_cpy);
		writeFdIsNonBlocking = false;
	}
	sockBlockSIGPIPE(cmdPeerFd, true);

	readFd = cmdPeerFd;
	readFdIsSocket = true;
	writeFd = cmdPeerFd;

	cleanupIOFunc = [cmdPeerFd, cmdSockFd, addr](){
		close(cmdPeerFd);
		close(cmdSockFd);
		unlink(addr.sun_path);
	};

	return true;
}

void cmdInterfaceThreadInit()
{
	if (wz_command_interface() == WZ_Command_Interface::None)
	{
		return;
	}

#if defined(HAVE_SYS_EVENTFD_H)
	if (quitSignalEventFd == -1)
	{
		int flags = 0;
# if defined(EFD_CLOEXEC)
		flags = EFD_CLOEXEC;
# endif
		quitSignalEventFd = eventfd(0, flags);
	}
#elif defined(HAVE_UNISTD_H)
	if (quitSignalPipeFds[0] == -1 && quitSignalPipeFds[1] == -1)
	{
		int result = -1;
# if defined(HAVE_PIPE2) && defined(O_CLOEXEC)
		result = pipe2(quitSignalPipeFds, O_CLOEXEC);
# else
		result = pipe(quitSignalPipeFds);
# endif
		if (result == -1)
		{
			quitSignalPipeFds[0] = -1;
			quitSignalPipeFds[1] = -1;
		}
	}
#endif

	// initialize output queue (if used)
	if (wz_command_interface() != WZ_Command_Interface::StdIn_Interface)
	{
		cmdInterfaceOutputQueue = std::make_unique<moodycamel::BlockingReaderWriterQueue<CmdInterfaceMessageBuffer>>(1024);
		latestWriteBuffer.reserve(maxReserveMessageBufferSize);
	}

	switch (wz_command_interface())
	{
		case WZ_Command_Interface::StdIn_Interface:
			readFd = STDIN_FILENO;
			readFdIsSocket = false;
			writeFd = STDERR_FILENO;
			writeFdIsNonBlocking = false;
			break;
		case WZ_Command_Interface::Unix_Socket:
			// sets readFd, writeFd, etc as appropriate
			if (!cmdInterfaceCreateUnixSocket(wz_cmd_interface_param))
			{
				// failed (and logged a failure to stderr)
				return;
			}
			break;
		default:
			return;
	}

	if (!cmdInputThread)
	{
		cmdInputThreadQuit.store(false);
		cmdInputThread = wzThreadCreate(cmdInputThreadFunc, nullptr, "wzCmdInterfaceInputThread");
		wzThreadStart(cmdInputThread);
	}

	if (!cmdOutputThread)
	{
		// Only in non-stdin case (for now)
		if (wz_command_interface() != WZ_Command_Interface::StdIn_Interface)
		{
			cmdOutputThreadQuit.store(false);
			cmdOutputThread = wzThreadCreate(cmdOutputThreadFunc, nullptr, "wzCmdInterfaceOutputThread");
			wzThreadStart(cmdOutputThread);
		}
	}
}

void cmdInterfaceThreadShutdown()
{
	if (cmdInputThread || cmdOutputThread)
	{
		// Signal the threads to quit
		cmdInputThreadQuit.store(true);
		cmdOutputThreadQuit.store(true);
		int quitSignalFd = -1;
#if defined(HAVE_SYS_EVENTFD_H)
		quitSignalFd = quitSignalEventFd;
#elif defined(HAVE_UNISTD_H)
		quitSignalFd = quitSignalPipeFds[1];
#endif
		if (quitSignalFd != -1)
		{
			uint64_t writeBytes = 1;
			if (write(quitSignalFd, &writeBytes, sizeof(uint64_t)) != sizeof(uint64_t))
			{
				debug(LOG_ERROR, "Failed to write to quitSignal fd??");
			}
		}

		if (cmdInputThread)
		{
			wzThreadJoin(cmdInputThread);
			cmdInputThread = nullptr;
		}
		if (cmdOutputThread)
		{
			wzThreadJoin(cmdOutputThread);
			cmdOutputThread = nullptr;
		}
	}

	if (cleanupIOFunc)
	{
		cleanupIOFunc();
		cleanupIOFunc = nullptr;
	}

#if defined(HAVE_SYS_EVENTFD_H)
	if (quitSignalEventFd != -1)
	{
		close(quitSignalEventFd);
		quitSignalEventFd = -1;
	}
#elif defined(HAVE_UNISTD_H)
	if (quitSignalPipeFds[0] != -1)
	{
		close(quitSignalPipeFds[0]);
		quitSignalPipeFds[0] = -1;
	}
	if (quitSignalPipeFds[1] != -1)
	{
		close(quitSignalPipeFds[1]);
		quitSignalPipeFds[1] = -1;
	}
#endif
}

#else // !defined(WZ_STDIN_READER_SUPPORTED)

// For unsupported platforms

void cmdInterfaceThreadInit()
{
	if (wz_command_interface() == WZ_Command_Interface::None)
	{
		return;
	}

	if (!cmdInputThread && !cmdOutputThread)
	{
		debug(LOG_ERROR, "This platform does not support the command interface");
		cmdInputThreadQuit.store(false);
		cmdOutputThreadQuit.store(true);
	}
}

void cmdInterfaceThreadShutdown()
{
	// no-op
}

#endif

bool wz_command_interface_enabled()
{
	return wz_command_interface() != WZ_Command_Interface::None;
}

void wz_command_interface_output_str(const char *str)
{
	if (wz_command_interface() == WZ_Command_Interface::None)
	{
		return;
	}

	size_t bufferLen = strlen(str);
	if (bufferLen == 0)
	{
		return;
	}

	if (wz_command_interface() == WZ_Command_Interface::StdIn_Interface)
	{
		fwrite(str, sizeof(char), bufferLen, stderr);
		fflush(stderr);
	}
	else
	{
		latestWriteBuffer.insert(latestWriteBuffer.end(), str, str + bufferLen);
		cmdInterfaceOutputQueue->enqueue(std::move(latestWriteBuffer));
		latestWriteBuffer = std::vector<char>();
		latestWriteBuffer.reserve(maxReserveMessageBufferSize);
	}
}

void wz_command_interface_output(const char *str, ...)
{
	if (wz_command_interface() == WZ_Command_Interface::None)
	{
		return;
	}
	va_list ap;
	static char outputBuffer[maxReserveMessageBufferSize];
	int ret = -1;
	va_start(ap, str);
	ret = vssprintf(outputBuffer, str, ap);
	va_end(ap);
	if (ret >= 0 && ret < maxReserveMessageBufferSize)
	{
		// string was completely written
		wz_command_interface_output_str(outputBuffer);
	}
	else
	{
		// string was truncated - try again but use a heap-allocated string
		if (ret < 0)
		{
			// some ancient implementations of vsnprintf return -1 instead of the needed buffer length...
			errlog("WZCMD ERROR: Failed to output truncated string - check vsnprintf implementation\n");
			return;
		}
		size_t neededBufferSize = static_cast<size_t>(ret);
		char* tmpBuffer = (char *)malloc(neededBufferSize + 1);
		va_start(ap, str);
		ret = vsnprintf(tmpBuffer, neededBufferSize + 1, str, ap);
		va_end(ap);
		if (ret < 0 || ret >= neededBufferSize + 1)
		{
			errlog("WZCMD ERROR: Failed to output truncated string\n");
			free(tmpBuffer);
			return;
		}
		if (tmpBuffer)
		{
			wz_command_interface_output_str(tmpBuffer);
			free(tmpBuffer);
		}
	}
}

void configSetCmdInterface(WZ_Command_Interface mode, std::string value)
{
	if (cmdInputThread || cmdOutputThread)
	{
		return;
	}

	wz_cmd_interface = mode;
	if (mode == WZ_Command_Interface::Unix_Socket && value.empty())
	{
#if defined(WZ_OS_UNIX)
		char cwdBuff[PATH_MAX] = {0};
		if (getcwd(cwdBuff, PATH_MAX) != nullptr)
		{
			errlog("WZCMD INFO: No unix socket path specified - will create wz2100.cmd.sock in: %s\n", cwdBuff);
		}
		else
		{
			errlog("WZCMD INFO: No unix socket path specified - will create wz2100.cmd.sock in the working directory\n");
		}
#else
		errlog("WZCMD INFO: No unix socket path specified - will create ./wz2100.cmd.sock\n");
#endif
		value = "./wz2100.cmd.sock";
	}
	wz_cmd_interface_param = value;
}

// MARK: - Output Room Status JSON

static void WzCmdInterfaceDumpHumanPlayerVarsImpl(uint32_t player, bool gameHasFiredUp, nlohmann::ordered_json& j)
{
	PLAYER const &p = NetPlay.players[player];

	j["name"] = p.name;

	if (!gameHasFiredUp)
	{
		// in lobby, output "ready" status
		j["ready"] = static_cast<int>(p.ready);
	}
	else
	{
		// once game has fired up, output loading / connection status
		if (p.allocated)
		{
			if (ingame.JoiningInProgress[player])
			{
				j["status"] = "loading";
			}
			else
			{
				j["status"] = "active";
			}
			if (ingame.PendingDisconnect[player])
			{
				j["status"] = "pendingleave";
			}
		}
		else
		{
			j["status"] = "left";
		}
	}

	const auto& identity = getMultiStats(player).identity;
	if (!identity.empty())
	{
		j["pk"] = base64Encode(identity.toBytes(EcKey::Public));
	}
	else
	{
		j["pk"] = "";
	}
	j["ip"] = NetPlay.players[player].IPtextAddress;

	if (ingame.PingTimes[player] != PING_LIMIT)
	{
		j["ping"] = ingame.PingTimes[player];
	}
	else
	{
		j["ping"] = -1; // for "infinite" ping
	}

	j["admin"] = static_cast<int>(NetPlay.players[player].isAdmin || (player == NetPlay.hostPlayer));

	if (player == NetPlay.hostPlayer)
	{
		j["host"] = 1;
	}
}

void wz_command_interface_output_room_status_json()
{
	if (!wz_command_interface_enabled())
	{
		return;
	}

	bool gameHasFiredUp = (GetGameMode() == GS_NORMAL);

	auto root = nlohmann::ordered_json::object();
	root["ver"] = 1;

	auto data = nlohmann::ordered_json::object();
	if (gameHasFiredUp)
	{
		if (ingame.TimeEveryoneIsInGame.has_value())
		{
			data["state"] = "started";
		}
		else
		{
			data["state"] = "starting";
		}
	}
	else
	{
		data["state"] = "lobby";
	}
	if (NetPlay.isHost)
	{
		auto lobbyGameId = NET_getCurrentHostedLobbyGameId();
		if (lobbyGameId != 0)
		{
			data["lobbyid"] = lobbyGameId;
		}
	}
	data["map"] = game.map;

	root["data"] = std::move(data);

	if (NetPlay.isHost)
	{
		auto players = nlohmann::ordered_json::array();
		for (uint8_t player = 0; player < game.maxPlayers; ++player)
		{
			PLAYER const &p = NetPlay.players[player];
			auto j = nlohmann::ordered_json::object();

			j["pos"] = p.position;
			j["team"] = p.team;
			j["col"] = p.colour;
			j["fact"] = static_cast<int32_t>(p.faction);

			if (p.ai == AI_CLOSED)
			{
				// closed slot
				j["type"] = "closed";
			}
			else if (p.ai == AI_OPEN)
			{
				if (!gameHasFiredUp && !p.allocated)
				{
					// available / open slot (in lobby)
					j["type"] = "open";
				}
				else
				{
					if (!p.allocated)
					{
						// if game has fired up and this slot is no longer allocated, skip it entirely if it wasn't initially a human player
						if (p.difficulty != AIDifficulty::HUMAN)
						{
							continue;
						}
					}

					// human (or host) slot
					j["type"] = (p.isSpectator) ? "spec" : "player";

					WzCmdInterfaceDumpHumanPlayerVarsImpl(player, gameHasFiredUp, j);
				}
			}
			else
			{
				// bot player
				j["type"] = "bot";

				j["name"] = getAIName(player);
				j["difficulty"] = static_cast<int>(NetPlay.players[player].difficulty);
			}

			players.push_back(std::move(j));
		}
		root["players"] = std::move(players);

		auto spectators = nlohmann::ordered_json::array();
		for (uint32_t i = MAX_PLAYER_SLOTS; i < MAX_CONNECTED_PLAYERS; ++i)
		{
			PLAYER const &p = NetPlay.players[i];
			if (p.ai == AI_CLOSED)
			{
				continue;
			}

			auto j = nlohmann::ordered_json::object();
			if (!p.allocated)
			{
				if (!gameHasFiredUp)
				{
					// available / open spectator slot
					j["type"] = "open";
				}
				else
				{
					// no spectator connected to this slot - skip
					continue;
				}
			}
			else
			{
				// human (or host) slot
				j["type"] = (p.isSpectator) ? "spec" : "player";

				WzCmdInterfaceDumpHumanPlayerVarsImpl(i, gameHasFiredUp, j);
			}

			spectators.push_back(std::move(j));
		}
		root["specs"] = std::move(spectators);
	}

	std::string statusJSONStr = std::string("__WZROOMSTATUS__") + root.dump(-1, ' ', false, nlohmann::ordered_json::error_handler_t::replace) + "__ENDWZROOMSTATUS__";
	statusJSONStr.append("\n");
	wz_command_interface_output_str(statusJSONStr.c_str());
}
