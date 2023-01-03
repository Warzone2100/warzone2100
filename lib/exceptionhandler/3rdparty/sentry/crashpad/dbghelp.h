// Copyright 2014 The Crashpad Authors. All rights reserved.
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

#ifndef CRASHPAD_COMPAT_MINGW_DBGHELP_H_
#define CRASHPAD_COMPAT_MINGW_DBGHELP_H_

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-include-next"

#include_next <dbghelp.h>
#include <stdint.h>
#include <timezoneapi.h>
#include <winnt.h>
#include <winver.h>

//! \file

//! \brief Information about XSAVE-managed state stored within CPU-specific
//!     context structures.
struct __attribute__((packed, aligned(4))) XSTATE_CONFIG_FEATURE_MSC_INFO {
  //! \brief The size of this structure, in bytes. This value is
  //!     `sizeof(XSTATE_CONFIG_FEATURE_MSC_INFO)`.
  uint32_t SizeOfInfo;

  //! \brief The size of a CPU-specific context structure carrying all XSAVE
  //!     state components described by this structure.
  //!
  //! Equivalent to the value returned by `InitializeContext()` in \a
  //! ContextLength.
  uint32_t ContextSize;

  //! \brief The XSAVE state-component bitmap, XSAVE_BV.
  //!
  //! See Intel Software Developer’s Manual, Volume 1: Basic Architecture
  //! (253665-060), 13.4.2 “XSAVE Header”.
  uint64_t EnabledFeatures;

  //! \brief The location of each state component within a CPU-specific context
  //!     structure.
  //!
  //! This array is indexed by bit position numbers used in #EnabledFeatures.
  XSTATE_FEATURE Features[MAXIMUM_XSTATE_FEATURES];
};

//! \anchor MINIDUMP_MISCx
//! \name MINIDUMP_MISC*
//!
//! \brief Field validity flag values for MINIDUMP_MISC_INFO::Flags1.
//! \{

//! \brief MINIDUMP_MISC_INFO::ProcessId is valid.
#define MINIDUMP_MISC1_PROCESS_ID 0x00000001

//! \brief The time-related fields in MINIDUMP_MISC_INFO are valid.
//!
//! The following fields are valid:
//!  - MINIDUMP_MISC_INFO::ProcessCreateTime
//!  - MINIDUMP_MISC_INFO::ProcessUserTime
//!  - MINIDUMP_MISC_INFO::ProcessKernelTime
#define MINIDUMP_MISC1_PROCESS_TIMES 0x00000002

//! \brief The CPU-related fields in MINIDUMP_MISC_INFO_2 are valid.
//!
//! The following fields are valid:
//!  - MINIDUMP_MISC_INFO_2::ProcessorMaxMhz
//!  - MINIDUMP_MISC_INFO_2::ProcessorCurrentMhz
//!  - MINIDUMP_MISC_INFO_2::ProcessorMhzLimit
//!  - MINIDUMP_MISC_INFO_2::ProcessorMaxIdleState
//!  - MINIDUMP_MISC_INFO_2::ProcessorCurrentIdleState
//!
//! \note This macro should likely have been named
//!     MINIDUMP_MISC2_PROCESSOR_POWER_INFO.
#define MINIDUMP_MISC1_PROCESSOR_POWER_INFO 0x00000004

//! \brief MINIDUMP_MISC_INFO_3::ProcessIntegrityLevel is valid.
#define MINIDUMP_MISC3_PROCESS_INTEGRITY 0x00000010

//! \brief MINIDUMP_MISC_INFO_3::ProcessExecuteFlags is valid.
#define MINIDUMP_MISC3_PROCESS_EXECUTE_FLAGS 0x00000020

//! \brief The time zone-related fields in MINIDUMP_MISC_INFO_3 are valid.
//!
//! The following fields are valid:
//!  - MINIDUMP_MISC_INFO_3::TimeZoneId
//!  - MINIDUMP_MISC_INFO_3::TimeZone
#define MINIDUMP_MISC3_TIMEZONE 0x00000040

//! \brief MINIDUMP_MISC_INFO_3::ProtectedProcess is valid.
#define MINIDUMP_MISC3_PROTECTED_PROCESS 0x00000080

//! \brief The build string-related fields in MINIDUMP_MISC_INFO_4 are valid.
//!
//! The following fields are valid:
//!  - MINIDUMP_MISC_INFO_4::BuildString
//!  - MINIDUMP_MISC_INFO_4::DbgBldStr
#define MINIDUMP_MISC4_BUILDSTRING 0x00000100

//! \brief MINIDUMP_MISC_INFO_5::ProcessCookie is valid.
#define MINIDUMP_MISC5_PROCESS_COOKIE 0x00000200

//! \}

#ifdef __cplusplus

//! \brief Contains the state of an individual system handle at the time the
//!     snapshot was taken. This structure is Windows-specific.
//!
//! \sa MINIDUMP_HANDLE_DESCRIPTOR
struct __attribute__((packed, aligned(4))) MINIDUMP_HANDLE_DESCRIPTOR_2
    : public MINIDUMP_HANDLE_DESCRIPTOR {
  //! \brief An RVA to a MINIDUMP_HANDLE_OBJECT_INFORMATION structure that
  //!     specifies object-specific information. This member can be zero if
  //!     there is no extra information.
  RVA ObjectInfoRva;

  //! \brief Must be zero.
  uint32_t Reserved0;
};

//! \brief Information about the process that the minidump file contains a
//!     snapshot of, as well as the system that hosted that process.
//!
//! This structure variant is used on Windows 7 (NT 6.1) and later.
//!
//! \sa \ref MINIDUMP_MISCx "MINIDUMP_MISC*"
//! \sa MINIDUMP_MISC_INFO
//! \sa MINIDUMP_MISC_INFO_2
//! \sa MINIDUMP_MISC_INFO_4
//! \sa MINIDUMP_MISC_INFO_5
//! \sa MINIDUMP_MISC_INFO_N
struct __attribute__((packed, aligned(4))) MINIDUMP_MISC_INFO_3
    : public MINIDUMP_MISC_INFO_2 {
  //! \brief The process’ integrity level.
  //!
  //! Windows typically uses `SECURITY_MANDATORY_MEDIUM_RID` (0x2000) for
  //! processes belonging to normal authenticated users and
  //! `SECURITY_MANDATORY_HIGH_RID` (0x3000) for elevated processes.
  //!
  //! This field is Windows-specific, and has no meaning on other operating
  //! systems.
  uint32_t ProcessIntegrityLevel;

  //! \brief The process’ execute flags.
  //!
  //! On Windows, this appears to be returned by `NtQueryInformationProcess()`
  //! with an argument of `ProcessExecuteFlags` (34).
  //!
  //! This field is Windows-specific, and has no meaning on other operating
  //! systems.
  uint32_t ProcessExecuteFlags;

  //! \brief Whether the process is protected.
  //!
  //! This field is Windows-specific, and has no meaning on other operating
  //! systems.
  uint32_t ProtectedProcess;

  //! \brief Whether daylight saving time was being observed in the system’s
  //!     location at the time of the snapshot.
  //!
  //! This field can contain the following values:
  //!  - `0` if the location does not observe daylight saving time at all. The
  //!    TIME_ZONE_INFORMATION::StandardName field of #TimeZoneId contains the
  //!    time zone name.
  //!  - `1` if the location observes daylight saving time, but standard time
  //!    was in effect at the time of the snapshot. The
  //!    TIME_ZONE_INFORMATION::StandardName field of #TimeZoneId contains the
  //!    time zone name.
  //!  - `2` if the location observes daylight saving time, and it was in effect
  //!    at the time of the snapshot. The TIME_ZONE_INFORMATION::DaylightName
  //!    field of #TimeZoneId contains the time zone name.
  //!
  //! \sa #TimeZone
  uint32_t TimeZoneId;

  //! \brief Information about the time zone at the system’s location.
  //!
  //! \sa #TimeZoneId
  TIME_ZONE_INFORMATION TimeZone;
};

//! \brief Information about the process that the minidump file contains a
//!     snapshot of, as well as the system that hosted that process.
//!
//! This structure variant is used on Windows 8 (NT 6.2) and later.
//!
//! \sa \ref MINIDUMP_MISCx "MINIDUMP_MISC*"
//! \sa MINIDUMP_MISC_INFO
//! \sa MINIDUMP_MISC_INFO_2
//! \sa MINIDUMP_MISC_INFO_3
//! \sa MINIDUMP_MISC_INFO_5
//! \sa MINIDUMP_MISC_INFO_N
struct __attribute__((packed, aligned(4))) MINIDUMP_MISC_INFO_4
    : public MINIDUMP_MISC_INFO_3 {
  //! \brief The operating system’s “build string”, a string identifying a
  //!     specific build of the operating system.
  //!
  //! This string is UTF-16-encoded and terminated by a UTF-16 `NUL` code unit.
  //!
  //! On Windows 8.1 (NT 6.3), this is “6.3.9600.17031
  //! (winblue_gdr.140221-1952)”.
  wchar_t BuildString[260];

  //! \brief The minidump producer’s “build string”, a string identifying the
  //!     module that produced a minidump file.
  //!
  //! This string is UTF-16-encoded and terminated by a UTF-16 `NUL` code unit.
  //!
  //! On Windows 8.1 (NT 6.3), this may be “dbghelp.i386,6.3.9600.16520” or
  //! “dbghelp.amd64,6.3.9600.16520” depending on CPU architecture.
  wchar_t DbgBldStr[40];
};

//! \brief Information about the process that the minidump file contains a
//!     snapshot of, as well as the system that hosted that process.
//!
//! This structure variant is used on Windows 10 and later.
//!
//! \sa \ref MINIDUMP_MISCx "MINIDUMP_MISC*"
//! \sa MINIDUMP_MISC_INFO
//! \sa MINIDUMP_MISC_INFO_2
//! \sa MINIDUMP_MISC_INFO_3
//! \sa MINIDUMP_MISC_INFO_4
//! \sa MINIDUMP_MISC_INFO_N
struct __attribute__((packed, aligned(4))) MINIDUMP_MISC_INFO_5
    : public MINIDUMP_MISC_INFO_4 {
  //! \brief Information about XSAVE-managed state stored within CPU-specific
  //!     context structures.
  //!
  //! This information can be used to locate state components within
  //! CPU-specific context structures.
  XSTATE_CONFIG_FEATURE_MSC_INFO XStateData;

  uint32_t ProcessCookie;
};

//! \brief The latest known version of the MINIDUMP_MISC_INFO structure.
typedef MINIDUMP_MISC_INFO_5 MINIDUMP_MISC_INFO_N;

#else

struct MINIDUMP_HANDLE_DESCRIPTOR_2 {
  ULONG64 Handle;
  RVA TypeNameRva;
  RVA ObjectNameRva;
  ULONG32 Attributes;
  ULONG32 GrantedAccess;
  ULONG32 HandleCount;
  ULONG32 PointerCount;
  RVA ObjectInfoRva;
  uint32_t Reserved0;
};

struct MINIDUMP_MISC_INFO_3 {
  ULONG32 SizeOfInfo;
  ULONG32 Flags1;
  ULONG32 ProcessId;
  ULONG32 ProcessCreateTime;
  ULONG32 ProcessUserTime;
  ULONG32 ProcessKernelTime;
  ULONG32 ProcessorMaxMhz;
  ULONG32 ProcessorCurrentMhz;
  ULONG32 ProcessorMhzLimit;
  ULONG32 ProcessorMaxIdleState;
  ULONG32 ProcessorCurrentIdleState;
  uint32_t ProcessIntegrityLevel;
  uint32_t ProcessExecuteFlags;
  uint32_t ProtectedProcess;
  uint32_t TimeZoneId;
  TIME_ZONE_INFORMATION TimeZone;
};

struct MINIDUMP_MISC_INFO_4 {
  ULONG32 SizeOfInfo;
  ULONG32 Flags1;
  ULONG32 ProcessId;
  ULONG32 ProcessCreateTime;
  ULONG32 ProcessUserTime;
  ULONG32 ProcessKernelTime;
  ULONG32 ProcessorMaxMhz;
  ULONG32 ProcessorCurrentMhz;
  ULONG32 ProcessorMhzLimit;
  ULONG32 ProcessorMaxIdleState;
  ULONG32 ProcessorCurrentIdleState;
  uint32_t ProcessIntegrityLevel;
  uint32_t ProcessExecuteFlags;
  uint32_t ProtectedProcess;
  uint32_t TimeZoneId;
  TIME_ZONE_INFORMATION TimeZone;
  wchar_t BuildString[260];
  wchar_t DbgBldStr[40];
};

struct MINIDUMP_MISC_INFO_5 {
  ULONG32 SizeOfInfo;
  ULONG32 Flags1;
  ULONG32 ProcessId;
  ULONG32 ProcessCreateTime;
  ULONG32 ProcessUserTime;
  ULONG32 ProcessKernelTime;
  ULONG32 ProcessorMaxMhz;
  ULONG32 ProcessorCurrentMhz;
  ULONG32 ProcessorMhzLimit;
  ULONG32 ProcessorMaxIdleState;
  ULONG32 ProcessorCurrentIdleState;
  uint32_t ProcessIntegrityLevel;
  uint32_t ProcessExecuteFlags;
  uint32_t ProtectedProcess;
  uint32_t TimeZoneId;
  TIME_ZONE_INFORMATION TimeZone;
  wchar_t BuildString[260];
  wchar_t DbgBldStr[40];
  struct XSTATE_CONFIG_FEATURE_MSC_INFO XStateData;
  uint32_t ProcessCookie;
};

typedef struct MINIDUMP_MISC_INFO_5 MINIDUMP_MISC_INFO_N;

#endif

#ifdef __cplusplus
extern "C" {
#endif

//! \brief Contains the name of the thread with the given thread ID.
struct __attribute__((packed, aligned(4))) MINIDUMP_THREAD_NAME {
  //! \brief The identifier of the thread.
  uint32_t ThreadId;

  //! \brief RVA64 of a MINIDUMP_STRING containing the name of the thread.
  RVA64 RvaOfThreadName;
};

//! \brief Variable-sized struct which contains a list of MINIDUMP_THREAD_NAME
//! structs.
struct __attribute__((packed, aligned(4))) MINIDUMP_THREAD_NAME_LIST {
  //! \brief The number of MINIDUMP_THREAD_NAME structs following this field.
  uint32_t NumberOfThreadNames;

  //! \brief A variably-sized array containing zero of more
  //! MINIDUMP_THREAD_NAME.
  //!    The length of the array is indicated by the NumberOfThreadNames field
  //!    in this struct.
  struct MINIDUMP_THREAD_NAME ThreadNames[0];
};

#ifdef __cplusplus
}
#endif

#pragma clang diagnostic pop

#endif  // CRASHPAD_COMPAT_MINGW_DBGHELP_H_
