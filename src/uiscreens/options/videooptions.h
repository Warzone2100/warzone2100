#pragma once

#include <string>

#include "../common.h"

void startVideoOptionsMenu();
bool runVideoOptionsMenu();
void refreshCurrentVideoOptionsValues();

// Video options, shared for in-game options menu use
char const* videoOptionsDisplayScaleLabel();
char const* videoOptionsVsyncString();
const char* videoOptionsResolutionLabel();
const char* videoOptionsResolutionGetReadOnlyTooltip();
std::string videoOptionsDisplayScaleString();
std::vector<unsigned int> availableDisplayScalesSorted();
void seqDisplayScale();
void seqVsyncMode();

std::shared_ptr<WIDGET> makeResolutionDropdown();
void videoOptionsDisableResolutionButtons();
void videoOptionsEnableResolutionButtons();
std::string videoOptionsAntialiasingString();
const char* videoOptionsWindowModeString();
const char* videoOptionsWindowModeLabel();
std::string videoOptionsTextureSizeString();
std::string videoOptionsGfxBackendString();
gfx_api::context::swap_interval_mode getCurrentSwapMode();
void saveCurrentSwapMode(gfx_api::context::swap_interval_mode mode);
