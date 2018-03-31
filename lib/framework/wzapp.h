/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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

#ifndef __INCLUDED_WZAPP_C_H__
#define __INCLUDED_WZAPP_C_H__

#include "frame.h"
#include <vector>
#include <functional>

struct WZ_THREAD;
struct WZ_MUTEX;
struct WZ_SEMAPHORE;

class WZ_MAINTHREADEXEC
{
public:
	WZ_MAINTHREADEXEC() { }
	virtual ~WZ_MAINTHREADEXEC() { };

	// subclass should override this
	virtual void doExecOnMainThread() { };
};

class WZ_MAINTHREADEXECFUNC: public WZ_MAINTHREADEXEC
{
public:
	typedef std::function<void ()> execFuncType;
public:
	WZ_MAINTHREADEXECFUNC(const execFuncType &execFunc)
	: execFunc(execFunc)
	{ }
	virtual ~WZ_MAINTHREADEXECFUNC() { };

	void doExecOnMainThread()
	{
		execFunc();
	};
private:
	execFuncType execFunc;
};

struct screeninfo
{
	int width;
	int height;
	int refresh_rate;
	int screen;
};

void wzMain(int &argc, char **argv);
bool wzMainScreenSetup(int antialiasing = 0, bool fullscreen = false, bool vsync = true, bool highDPI = true);
void wzGetGameToRendererScaleFactor(float *horizScaleFactor, float *vertScaleFactor);
void wzMainEventLoop();
void wzQuit();              ///< Quit game
void wzShutdown();
void wzToggleFullscreen();
bool wzIsFullscreen();
void wzSetWindowIsResizable(bool resizable);
bool wzIsWindowResizable();
bool wzSupportsLiveResolutionChanges();
bool wzChangeDisplayScale(unsigned int displayScale);
bool wzChangeWindowResolution(int screen, unsigned int width, unsigned int height);
unsigned int wzGetMaximumDisplayScaleForWindowSize(unsigned int windowWidth, unsigned int windowHeight);
unsigned int wzGetCurrentDisplayScale();
void wzGetWindowResolution(int *screen, unsigned int *width, unsigned int *height);
void wzSetCursor(CURSOR index);
void wzScreenFlip();	///< Swap the graphics buffers
void wzShowMouse(bool visible); ///< Show the Mouse?
void wzGrabMouse();		///< Trap mouse cursor in application window
void wzReleaseMouse();	///< Undo the wzGrabMouse operation
bool wzActiveWindow();	///< Whether application currently has the mouse pointer over it
int wzGetTicks();		///< Milliseconds since start of game
WZ_DECL_NONNULL(1) void wzFatalDialog(const char *text);	///< Throw up a modal warning dialog

std::vector<screeninfo> wzAvailableResolutions();
std::vector<unsigned int> wzAvailableDisplayScales();
void wzSetSwapInterval(int swap);
int wzGetSwapInterval();
QString wzGetSelection();
QString wzGetCurrentText();
unsigned int wzGetCurrentKey();
void wzDelay(unsigned int delay);	//delay in ms
// unicode text support
void StartTextInput();
void StopTextInput();
// Thread related
WZ_THREAD *wzThreadCreate(int (*threadFunc)(void *), void *data);
WZ_DECL_NONNULL(1) int wzThreadJoin(WZ_THREAD *thread);
WZ_DECL_NONNULL(1) void wzThreadDetach(WZ_THREAD *thread);
WZ_DECL_NONNULL(1) void wzThreadStart(WZ_THREAD *thread);
void wzYieldCurrentThread();
WZ_MUTEX *wzMutexCreate();
WZ_DECL_NONNULL(1) void wzMutexDestroy(WZ_MUTEX *mutex);
WZ_DECL_NONNULL(1) void wzMutexLock(WZ_MUTEX *mutex);
WZ_DECL_NONNULL(1) void wzMutexUnlock(WZ_MUTEX *mutex);
WZ_SEMAPHORE *wzSemaphoreCreate(int startValue);
WZ_DECL_NONNULL(1) void wzSemaphoreDestroy(WZ_SEMAPHORE *semaphore);
WZ_DECL_NONNULL(1) void wzSemaphoreWait(WZ_SEMAPHORE *semaphore);
WZ_DECL_NONNULL(1) void wzSemaphorePost(WZ_SEMAPHORE *semaphore);
WZ_DECL_NONNULL(1) void wzAsyncExecOnMainThread(WZ_MAINTHREADEXEC *exec);

// Asynchronously executes execFunc() on the main thread.
// This function must be thread-safe, and may be safely called from any thread.
//
// No guarantees are made about when execFunc() will be called relative to the
// calling of this function - this function may return before, during, or after
// execFunc()'s execution on the main thread.
inline void wzAsyncExecOnMainThread(const std::function<void ()> &execFunc)
{
	wzAsyncExecOnMainThread(new WZ_MAINTHREADEXECFUNC(execFunc));
	// receiver handles deleting the parameter on the main thread after doExecOnMainThread() has been called
}

#if !defined(WZ_CC_MINGW)

#include <mutex>
#include <future>

namespace wz
{
	using mutex = std::mutex;
	template <typename R>
	using future = std::future<R>;
	template <typename RA>
	using packaged_task = std::packaged_task<RA>;
	using thread = std::thread;
}

#else  // Workaround for cross-compiler without std::mutex.

#include <memory>
#include <functional>

namespace wz
{
	class mutex
	{
	public:
		mutex() : handle(wzMutexCreate()) {}
		~mutex() { wzMutexDestroy(handle); }

		mutex(mutex const &) = delete;
		mutex &operator =(mutex const &) = delete;

		void lock() { wzMutexLock(handle); }
		//bool try_lock();
		void unlock() { wzMutexUnlock(handle); }

	private:
		WZ_MUTEX *handle;
	};

	template <typename R>
	class future
	{
	public:
		future() = default;
		future(future &&other) : internal(std::move(other.internal)) {}
		future(future const &) = delete;
		future &operator =(future &&other) = default;
		future &operator =(future const &) = delete;
		//std::shared_future<T> share();
		R get() { auto &data = *internal; wzSemaphoreWait(data.sem); return std::move(data.ret); }
		//valid(), wait*();

		struct Internal  // Should really be a promise.
		{
			Internal() : sem(wzSemaphoreCreate(0)) {}
			~Internal() { wzSemaphoreDestroy(sem); }
			R ret;
			WZ_SEMAPHORE *sem;
		};

		std::shared_ptr<Internal> internal;
	};

	template <typename RA>
	class packaged_task;

	template <typename R, typename... A>
	class packaged_task<R (A...)>
	{
	public:
		packaged_task() = default;
		template <typename F>
		explicit packaged_task(F &&f) { function = std::move(f); internal = std::make_shared<typename future<R>::Internal>(); }
		packaged_task(packaged_task &&) = default;
		packaged_task(packaged_task const &) = delete;

		future<R> get_future() { future<R> future; future.internal = internal; return std::move(future); }
		void operator ()(A &&... args) { auto &data = *internal; data.ret = function(std::forward<A>(args)...); wzSemaphorePost(data.sem); }

	private:
		std::function<R (A...)> function;
		std::shared_ptr<typename future<R>::Internal> internal;
	};

	class thread
	{
	public:
		thread() : internal(nullptr) {}
		thread(thread &&other) : internal(other.internal) { other.internal = nullptr; }
		template <typename Function, typename... Args>
		explicit thread(Function &&f, Args &&...args) : internal(wzThreadCreate([](void *vf) { auto F = (std::function<void ()> *)vf; (*F)(); delete F; return 0; }, new std::function<void ()>(std::bind(std::forward<Function>(f), std::forward<Args>(args)...)))) { wzThreadStart(internal); }
		thread(thread const &) = delete;
		~thread() { if (internal) { std::terminate(); } }
		thread &operator =(thread &&other) { std::swap(internal, other.internal); return *this; }
		void join() { if (!internal) { std::terminate(); } wzThreadJoin(internal); internal = nullptr; }
		void detach() { if (!internal) { std::terminate(); } wzThreadDetach(internal); internal = nullptr; }

	private:
		WZ_THREAD *internal;
	};
}

#endif

#endif
