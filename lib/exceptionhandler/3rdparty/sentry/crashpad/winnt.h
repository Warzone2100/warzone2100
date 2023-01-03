// Copyright 2015 The Crashpad Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef CRASHPAD_COMPAT_MINGW_WINNT_H_
#define CRASHPAD_COMPAT_MINGW_WINNT_H_

#include_next <winnt.h>
#include <minwindef.h>

// https://msdn.microsoft.com/library/aa373184.aspx: "Note that this structure
// definition was accidentally omitted from WinNT.h."
struct PROCESSOR_POWER_INFORMATION {
  ULONG Number;
  ULONG MaxMhz;
  ULONG CurrentMhz;
  ULONG MhzLimit;
  ULONG MaxIdleState;
  ULONG CurrentIdleState;
};

// 10.0.10240.0 SDK

#ifndef PROCESSOR_ARCHITECTURE_ARM64
#define PROCESSOR_ARCHITECTURE_ARM64 12
#endif

#ifndef PF_ARM_V8_INSTRUCTIONS_AVAILABLE
#define PF_ARM_V8_INSTRUCTIONS_AVAILABLE 29
#endif

#ifndef PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE
#define PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE 30
#endif

#ifndef PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE
#define PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE 31
#endif

#ifndef PF_RDTSCP_INSTRUCTION_AVAILABLE
#define PF_RDTSCP_INSTRUCTION_AVAILABLE 32
#endif

// 10.0.14393.0 SDK

#ifndef PROCESSOR_ARCHITECTURE_ARM32_ON_WIN64
#define PROCESSOR_ARCHITECTURE_ARM32_ON_WIN64 13
#endif

#include <apisetcconv.h>

#ifdef __cplusplus
extern "C" {
#endif

WINBASEAPI HRESULT WINAPI GetThreadDescription(HANDLE,PWSTR *);

#ifdef __cplusplus
}
#endif

#endif  // CRASHPAD_COMPAT_MINGW_WINNT_H_
