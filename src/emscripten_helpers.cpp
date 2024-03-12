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
#include <algorithm>
#include "lib/ivis_opengl/gfx_api.h"

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
static WzString emBottomRendererSystemInfoText;

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

void initWZEmscriptenHelpers_PostInit()
{
	// Generate bottom renderer + system info text
	emBottomRendererSystemInfoText.clear();
	if (gfx_api::context::isInitialized())
	{
		emBottomRendererSystemInfoText = WzString::fromUtf8(gfx_api::context::get().getFormattedRendererInfoString());
	}
	else
	{
		ASSERT(gfx_api::context::isInitialized(), "Function called before gfx context initialized");
		emBottomRendererSystemInfoText = "WebGL";
	}
	emBottomRendererSystemInfoText.append(" | Available Memory: ");
	emBottomRendererSystemInfoText.append(WzString::number(WZ_EmscriptenGetMaximumMemoryMiB()));
	emBottomRendererSystemInfoText.append(" MiB");
}

unsigned int WZ_EmscriptenGetMaximumMemoryMiB()
{
	int result = MAIN_THREAD_EM_ASM_INT({
		if (typeof wz_js_get_maximum_memory_mib !== "function") {
			return 0;
		}
		return wz_js_get_maximum_memory_mib();
	});
	return static_cast<unsigned int>(std::max<int>(result, 0));
}

const WzString& WZ_EmscriptenGetBottomRendererSysInfoString()
{
	return emBottomRendererSystemInfoText;
}

void WZ_EmscriptenSyncPersistFSChanges(bool isUserInitiatedSave)
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
			wz_js_save_config_dir_to_persistent_storage($0, () => {
				handleFinished();
			});
		}
		catch (error) {
			console.error(error);
			// Always resume the main loop on error
			handleFinished();
		}
	}, isUserInitiatedSave);
}

#endif
