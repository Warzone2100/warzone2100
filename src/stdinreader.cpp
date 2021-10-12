/*
	This file is part of Warzone 2100.
	Copyright (C) 2020-2021  Warzone 2100 Project

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
#include "lib/netplay/netplay.h"
#include "multiint.h"
#include "multilobbycommands.h"
#include "clparse.h"

#include <string>
#include <atomic>

#if defined(WZ_OS_UNIX)
# include <fcntl.h>
# include <sys/select.h>
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

static WZ_THREAD *stdinThread = nullptr;
static std::atomic<bool> stdinThreadQuit;
#if defined(HAVE_SYS_EVENTFD_H)
int quitSignalEventFd = -1;
#elif defined(HAVE_UNISTD_H)
int quitSignalPipeFds[2] = {-1, -1};
#endif

#if !defined(_WIN32) && (defined(HAVE_SYS_EVENTFD_H) || defined(HAVE_UNISTD_H))
# define WZ_STDIN_READER_SUPPORTED
#endif

#if defined(WZ_STDIN_READER_SUPPORTED)
enum class StdInReadyStatus
{
	Error,
	NotReady,
	Ready,
	Exit,
};
static StdInReadyStatus stdInHasDataToBeRead(int quitSignalFd, int msTimeout = 2000)
{
#if defined(_WIN32)
# error "Not supported on Windows"
#endif

#if defined(HAVE_WORKING_POLL)
	struct pollfd pfds[2];
	pfds[0].fd = STDIN_FILENO;
	pfds[0].events = POLLIN;
	pfds[0].revents = 0;
	if (quitSignalFd >= 0)
	{
		pfds[1].fd = quitSignalFd;
		pfds[1].events = POLLIN;
		pfds[1].revents = 0;
	}
	int timeoutValue = (quitSignalFd >= 0) ? -1 : msTimeout;

	int retval = poll(pfds, (quitSignalFd >= 0) ? 2 : 1, timeoutValue);
	if (retval < 0)
	{
		return StdInReadyStatus::Error;
	}
	if (retval > 0)
	{
		if (pfds[1].revents & POLLIN)
		{
			return StdInReadyStatus::Exit;
		}
		if (pfds[0].revents & POLLIN)
		{
			return StdInReadyStatus::Ready;
		}
	}
#else
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(STDIN_FILENO, &rfds);
	if (quitSignalFd >= 0)
	{
		FD_SET(quitSignalFd, &rfds);
	}
	struct timeval tv;
	tv.tv_sec = msTimeout / 1000;
	tv.tv_usec = (msTimeout % 1000) * 1000;

	int retval = select(std::max<int>(STDIN_FILENO, quitSignalFd)+1, &rfds, NULL, NULL, (quitSignalFd >= 0) ? NULL : &tv);
	if (retval == -1)
	{
		return StdInReadyStatus::Error;
	}
	if (retval > 0 && quitSignalFd >= 0 && FD_ISSET(quitSignalFd, &rfds))
	{
		return StdInReadyStatus::Exit;
	}
	if (retval > 0 && FD_ISSET(STDIN_FILENO, &rfds))
	{
		return StdInReadyStatus::Ready;
	}
#endif

	return StdInReadyStatus::NotReady;
}

int stdinThreadFunc(void *)
{
	fseek(stdin, 0, SEEK_END);
	errlog("WZCMD: stdinReadReady\n");
	bool inexit = false;
	int quitSignalFd = -1;
#if defined(HAVE_SYS_EVENTFD_H)
	quitSignalFd = quitSignalEventFd;
#elif defined(HAVE_UNISTD_H)
	quitSignalFd = quitSignalPipeFds[0];
#endif
	while (!inexit)
	{
		while (true)
		{
			auto result = stdInHasDataToBeRead(quitSignalFd);
			if (result == StdInReadyStatus::Exit)
			{
				// quit thread
				return 0;
			}
			else if (result == StdInReadyStatus::NotReady)
			{
				if (stdinThreadQuit.load())
				{
					// quit thread
					return 0;
				}
			}
			else if (result == StdInReadyStatus::Ready)
			{
				break;
			}
			else
			{
				errlog("WZCMD error: getline failed!\n");
				return 1;
			}
		}

		char *line = NULL;
		size_t len = 0;
		ssize_t lineSize = 0;
		lineSize = getline(&line, &len, stdin);
		if (lineSize == -1)
		{
			errlog("WZCMD error: getline failed!\n");
			inexit = true;
			continue;
		}

		if(!strncmpl(line, "exit"))
		{
			errlog("WZCMD info: exit command received - stdin reader will now stop procesing commands\n");
			inexit = true;
		}
		else if(!strncmpl(line, "admin add-hash "))
		{
			char newadmin[1024] = {0};
			int r = sscanf(line, "admin add-hash %1023[^\n]s", newadmin);
			if (r != 1)
			{
				errlog("WZCMD error: Failed to add room admin hash! (Expecting one parameter)\n");
			}
			else
			{
				std::string newAdminStrCopy(newadmin);
				wzAsyncExecOnMainThread([newAdminStrCopy]{
					errlog("WZCMD info: Room admin hash added: %s\n", newAdminStrCopy.c_str());
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
				errlog("WZCMD error: Failed to add room admin public key! (Expecting one parameter)\n");
			}
			else
			{
				std::string newAdminStrCopy(newadmin);
				wzAsyncExecOnMainThread([newAdminStrCopy]{
					errlog("WZCMD info: Room admin public key added: %s\n", newAdminStrCopy.c_str());
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
				errlog("WZCMD error: Failed to remove room admin! (Expecting one parameter)\n");
			}
			else
			{
				std::string newAdminStrCopy(newadmin);
				wzAsyncExecOnMainThread([newAdminStrCopy]{
					if (removeLobbyAdminPublicKey(newAdminStrCopy))
					{
						errlog("WZCMD info: Room admin public key removed: %s\n", newAdminStrCopy.c_str());
						auto roomAdminMessage = astringf("Room admin removed: %s", newAdminStrCopy.c_str());
						sendRoomSystemMessage(roomAdminMessage.c_str());
					}
					else if (removeLobbyAdminIdentityHash(newAdminStrCopy))
					{
						errlog("WZCMD info: Room admin hash removed: %s\n", newAdminStrCopy.c_str());
						auto roomAdminMessage = astringf("Room admin removed: %s", newAdminStrCopy.c_str());
						sendRoomSystemMessage(roomAdminMessage.c_str());
					}
					else
					{
						errlog("WZCMD info: Failed to remove room admin! (Provided parameter not found as either admin hash or public key)\n");
					}
				});
			}
		}
		else if(!strncmpl(line, "ban ip "))
		{
			char tobanip[1024] = {0};
			int r = sscanf(line, "ban ip %1023[^\n]s", tobanip);
			if (r != 1)
			{
				errlog("WZCMD error: Failed to get ban ip!\n");
			}
			else
			{
				std::string banIPStrCopy(tobanip);
				wzAsyncExecOnMainThread([banIPStrCopy] {
					for (int i = 0; i < MAX_CONNECTED_PLAYERS; i++)
					{
						auto player = NetPlay.players[i];
						if (!isHumanPlayer(i))
						{
							continue;
						}
						if (!strcmp(player.IPtextAddress, banIPStrCopy.c_str()))
						{
							kickPlayer(i, "You have been banned from joining by the administrator.", ERROR_INVALID);
							auto KickMessage = astringf("Player %s was banned by the administrator.", player.name);
							sendRoomSystemMessage(KickMessage.c_str());
						}
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
				errlog("WZCMD error: Failed to get bcast message!\n");
			}
			else
			{
				std::string chatmsgstr(chatmsg);
				wzAsyncExecOnMainThread([chatmsgstr] {
					if (!NetPlay.isHostAlive)
					{
						// can't send this message when the host isn't alive
						errlog("WZCMD error: Failed to send bcast message because host isn't yet hosting!\n");
					}
					sendRoomSystemMessage(chatmsgstr.c_str());
				});
			}
		}
		else if(!strncmpl(line, "shutdown now"))
		{
			errlog("WZCMD info: shutdown now command received - shutting down\n");
			wzQuit(0);
			inexit = true;
		}

		free(line);
	}
	return 0;
}

void stdInThreadInit()
{
	if (!stdinThread)
	{
		stdinThreadQuit.store(false);
#if defined(HAVE_SYS_EVENTFD_H)
		int flags = 0;
# if defined(EFD_CLOEXEC)
		flags = EFD_CLOEXEC;
# endif
		quitSignalEventFd = eventfd(0, flags);
#elif defined(HAVE_UNISTD_H)
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
#endif

		stdinThread = wzThreadCreate(stdinThreadFunc, nullptr);
		wzThreadStart(stdinThread);
	}
}

void stdInThreadShutdown()
{
	if (stdinThread)
	{
		// Signal the stdin thread to quit
		stdinThreadQuit.store(true);
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

		wzThreadJoin(stdinThread);
		stdinThread = nullptr;

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
}

#else // !defined(WZ_STDIN_READER_SUPPORTED)

// For unsupported platforms

void stdInThreadInit()
{
	if (!stdinThread)
	{
		debug(LOG_ERROR, "This platform does not support the stdin command reader");
		stdinThreadQuit.store(false);
	}
}

void stdInThreadShutdown()
{
	// no-op
}

#endif

void wz_command_interface_output(const char *str, ...)
{
	if (wz_command_interface() == WZ_Command_Interface::None)
	{
		return;
	}
	va_list ap;
	static char outputBuffer[2048];
	va_start(ap, str);
	vssprintf(outputBuffer, str, ap);
	va_end(ap);
	fwrite(outputBuffer, sizeof(char), strlen(outputBuffer), stderr);
	fflush(stderr);
}

