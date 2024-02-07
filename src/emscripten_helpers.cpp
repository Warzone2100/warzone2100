/*
	This file is part of Warzone 2100.
	Copyright (C) 2022-2024  Warzone 2100 Project

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

#if defined(__EMSCRIPTEN__)

#include "emscripten_helpers.h"
#include <emscripten.h>
#include <emscripten/eventloop.h>
#include <cstdlib>

/* Older Emscriptens don't have this, but it's needed for wasm64 compatibility. */
#ifndef MAIN_THREAD_EM_ASM_PTR
	#ifdef __wasm64__
		#error You need to upgrade your Emscripten compiler to support wasm64
	#else
		#define MAIN_THREAD_EM_ASM_PTR MAIN_THREAD_EM_ASM_INT
	#endif
#endif

EM_JS_DEPS(wz2100emhelpers, "$stringToUTF8,$UTF8ToString");

static std::string windowLocationURL;

std::string WZ_GetEmscriptenWindowLocationURL()
{
	return windowLocationURL;
}

void initWZEmscriptenHelpers()
{
	// Get window location URL
	char *str = (char*)MAIN_THREAD_EM_ASM_PTR({
	  let jsString = window.location.href;
	  let lengthBytes = lengthBytesUTF8(jsString) + 1;
	  let stringOnWasmHeap = _malloc(lengthBytes);
	  stringToUTF8(jsString, stringOnWasmHeap, lengthBytes);
	  return stringOnWasmHeap;
	});
	windowLocationURL = (str) ? str : "";
	free(str); // Each call to _malloc() must be paired with free(), or heap memory will leak!
}

void WZ_EmscriptenSyncPersistFSChanges()
{
	emscripten_runtime_keepalive_push(); // Must be used so that onExit handlers aren't called
	emscripten_pause_main_loop();

	// Note: Module.resumeMainLoop() is equivalent to emscripten_resume_main_loop()
	MAIN_THREAD_EM_ASM({
		if (typeof wz_js_display_saving_indicator === "function") {
			wz_js_display_saving_indicator(true);
		}
		let handleFinished = function() {
			if (typeof wz_js_display_saving_indicator === "function") {
				wz_js_display_saving_indicator(false);
			}
			Module.resumeMainLoop();
			runtimeKeepalivePop();
		};
		try {
			Module.wzSaveConfigDirToPersistentStore(() => {
				handleFinished();
			});
		}
		catch (error) {
			console.error(error);
			// Always resume the main loop on error
			handleFinished();
		}
	});
}

#endif
