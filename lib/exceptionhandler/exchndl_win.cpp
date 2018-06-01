/*
	This file is part of Warzone 2100.
	Portions derived from: exchndl_mingw.cpp
	Copyright (C) 2008-2018  Warzone 2100 Project

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
#if (_WIN32_WINNT < 0x0501)			// must force Windows XP or higher
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#include "lib/framework/frame.h"
#include "dumpinfo.h"
#include "exchndl.h"
#include "3rdparty/StackWalker.h"

#include <assert.h>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <shlobj.h>
#include <shlwapi.h>

#include <vector>

// Declare the static variables
static wchar_t szLogFileName[MAX_PATH] = L"";
static wchar_t szMinidumpFileName[MAX_PATH] = L"";
static LPTOP_LEVEL_EXCEPTION_FILTER prevExceptionFilter = NULL;
static char *formattedVersionString = NULL;

static void MyStrCpy(char* szDest, size_t nMaxDestSize, const char* szSrc)
{
	if (nMaxDestSize <= 0) return;
	strncpy_s(szDest, nMaxDestSize, szSrc, _TRUNCATE);
}  // MyStrCpy

class WzFormattedStackWalker : public StackWalker
{
public:
	WzFormattedStackWalker()
		: StackWalker()
	{}
protected:

	virtual void OnDbgHelpErr(LPCSTR szFuncName, DWORD gle, DWORD64 addr)
	{
		if ((strcmp(szFuncName, "SymGetLineFromAddr64") == 0) && (gle == 487))
		{
			// ignore SymGetLineFromAddr64, error 487
			return;
		}
		StackWalker::OnDbgHelpErr(szFuncName, gle, addr);
	}

	virtual void BeforeLoadModules()
	{
		OnOutput("\r\nLoaded Modules:\r\n");
	}

	virtual void BeforeStackTrace()
	{
		OnOutput("\r\nCall Stack:\r\n");
	}

	virtual void OnLoadModule(LPCSTR img, LPCSTR mod, DWORD64 baseAddr, DWORD size, DWORD result, LPCSTR symType, LPCSTR pdbName, ULONGLONG fileVersion)
	{
		CHAR buffer[STACKWALK_MAX_NAMELEN];
		size_t maxLen = STACKWALK_MAX_NAMELEN;
#if _MSC_VER >= 1400
		maxLen = _TRUNCATE;
#endif
		if (fileVersion == 0)
			_snprintf_s(buffer, maxLen, "%s:%s (%p), size: %d (result: %d), SymType: '%s', PDB: '%s'\r\n", img, mod, (LPVOID)baseAddr, size, result, symType, pdbName);
		else
		{
			DWORD v4 = (DWORD)(fileVersion & 0xFFFF);
			DWORD v3 = (DWORD)((fileVersion >> 16) & 0xFFFF);
			DWORD v2 = (DWORD)((fileVersion >> 32) & 0xFFFF);
			DWORD v1 = (DWORD)((fileVersion >> 48) & 0xFFFF);
			_snprintf_s(buffer, maxLen, "%s:%s (%p), size: %d (result: %d), SymType: '%s', PDB: '%s', fileVersion: %d.%d.%d.%d\r\n", img, mod, (LPVOID)baseAddr, size, result, symType, pdbName, v1, v2, v3, v4);
		}
		buffer[STACKWALK_MAX_NAMELEN - 1] = 0;  // be sure it is NUL terminated
		OnOutput(buffer);
	}

	virtual void OnCallstackEntry(CallstackEntryType eType, CallstackEntry &entry)
	{
		CHAR buffer[STACKWALK_MAX_NAMELEN];
		size_t maxLen = STACKWALK_MAX_NAMELEN;
#if _MSC_VER >= 1400
		maxLen = _TRUNCATE;
#endif
		if ((eType != lastEntry) && (entry.offset != 0))
		{
			if (entry.name[0] == 0)
				MyStrCpy(entry.name, STACKWALK_MAX_NAMELEN, "(function-name not available)");
			if (entry.undName[0] != 0)
				MyStrCpy(entry.name, STACKWALK_MAX_NAMELEN, entry.undName);
			if (entry.undFullName[0] != 0)
				MyStrCpy(entry.name, STACKWALK_MAX_NAMELEN, entry.undFullName);
			if (entry.lineFileName[0] == 0)
			{
				MyStrCpy(entry.lineFileName, STACKWALK_MAX_NAMELEN, "(filename not available)");
				if (entry.moduleName[0] == 0)
					MyStrCpy(entry.moduleName, STACKWALK_MAX_NAMELEN, "(module-name not available)");
				_snprintf_s(buffer, maxLen, "%p  %s:%p  %s\r\n", (LPVOID)entry.offset, entry.loadedImageName, (LPVOID)entry.offset, entry.name);
			}
			else
				_snprintf_s(buffer, maxLen, "%p  %s  %s  %s:%d\r\n", (LPVOID)entry.offset, entry.loadedImageName, entry.name, entry.lineFileName, entry.lineNumber);
			buffer[STACKWALK_MAX_NAMELEN - 1] = 0;
			OnOutput(buffer);
		}
	}
};

class WzExceptionStackWalker : public WzFormattedStackWalker
{
public:
	WzExceptionStackWalker(HANDLE outputFile)
		: hOutputFile(outputFile)
		, WzFormattedStackWalker()
	{}
protected:
	virtual void OnOutput(LPCSTR szText)
	{
		DWORD cbWritten = 0;
		size_t len = strlen(szText);
		WriteFile(hOutputFile, szText, len * sizeof(char), &cbWritten, 0);
	}

private:
	HANDLE hOutputFile = INVALID_HANDLE_VALUE;
};

static
int __cdecl rprintf(HANDLE hReportFile, const TCHAR *format, ...)
{
	TCHAR szBuff[4096];
	int retValue;
	DWORD cbWritten;
	va_list argptr;

	va_start(argptr, format);
	retValue = wvsprintf(szBuff, format, argptr);
	va_end(argptr);

	WriteFile(hReportFile, szBuff, retValue * sizeof(TCHAR), &cbWritten, 0);

	return retValue;
}

// The GetModuleBase function retrieves the base address of the module that contains the specified address.
static
DWORD GetModuleBase(DWORD dwAddress)
{
	MEMORY_BASIC_INFORMATION Buffer;

	return VirtualQuery((LPCVOID)dwAddress, &Buffer, sizeof(Buffer)) ? (DWORD)Buffer.AllocationBase : 0;
}

// Generate a MiniDump

# include "dbghelp.h"
static bool OutputMiniDump(LPCWSTR miniDumpPath, PEXCEPTION_POINTERS pExceptionInfo)
{
	bool bSuccess = false;
	HANDLE miniDumpFile = CreateFileW(miniDumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (miniDumpFile != INVALID_HANDLE_VALUE)
	{
		size_t package_version_len = strlen(PACKAGE_VERSION);
		ULONG package_version_len_ulong = (package_version_len <= ULONG_MAX) ? (ULONG)package_version_len : 0;
		MINIDUMP_USER_STREAM uStream = { LastReservedStream + 1, package_version_len_ulong, PACKAGE_VERSION };
		MINIDUMP_USER_STREAM_INFORMATION uInfo = { 1, &uStream };
		MINIDUMP_EXCEPTION_INFORMATION eInfo = { GetCurrentThreadId(), pExceptionInfo, false };

		if (MiniDumpWriteDump(
			GetCurrentProcess(),
			GetCurrentProcessId(),
			miniDumpFile,
			MiniDumpNormal,
			pExceptionInfo ? &eInfo : NULL,
			&uInfo,
			NULL))
		{
			// success
			bSuccess = true;
		}
		else
		{
			bSuccess = false;
		}

		CloseHandle(miniDumpFile);
	}
	else
	{
		bSuccess = false;
	}

	return bSuccess;
}

static
void GenerateExceptionReport(HANDLE hReportFile, PEXCEPTION_POINTERS pExceptionInfo)
{
	PEXCEPTION_RECORD pExceptionRecord = pExceptionInfo->ExceptionRecord;
	TCHAR szModule[MAX_PATH];
	HMODULE hModule;
	PCONTEXT pContext;

	// Start out with a banner
	rprintf(hReportFile, _T("-------------------\r\n\r\n"));

	{
		const TCHAR *lpDayOfWeek[] =
		{
			_T("Sunday"),
			_T("Monday"),
			_T("Tuesday"),
			_T("Wednesday"),
			_T("Thursday"),
			_T("Friday"),
			_T("Saturday")
		};
		const TCHAR *lpMonth[] =
		{
			NULL,
			_T("January"),
			_T("February"),
			_T("March"),
			_T("April"),
			_T("May"),
			_T("June"),
			_T("July"),
			_T("August"),
			_T("September"),
			_T("October"),
			_T("November"),
			_T("December")
		};
		SYSTEMTIME SystemTime;

		GetLocalTime(&SystemTime);
		rprintf(hReportFile, _T("Error occurred on %s, %s %i, %i at %02i:%02i:%02i.\r\n\r\n"),
		        lpDayOfWeek[SystemTime.wDayOfWeek],
		        lpMonth[SystemTime.wMonth],
		        SystemTime.wDay,
		        SystemTime.wYear,
		        SystemTime.wHour,
		        SystemTime.wMinute,
		        SystemTime.wSecond
		       );
	}

	// Dump a generic info header
	dbgDumpHeader(hReportFile);

	// First print information about the type of fault
	rprintf(hReportFile, _T("\r\n%s caused "),  GetModuleFileName(NULL, szModule, MAX_PATH) ? szModule : _T("Application"));
	switch (pExceptionRecord->ExceptionCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		rprintf(hReportFile, _T("an Access Violation"));
		break;

	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		rprintf(hReportFile, _T("an Array Bound Exceeded"));
		break;

	case EXCEPTION_BREAKPOINT:
		rprintf(hReportFile, _T("a Breakpoint"));
		break;

	case EXCEPTION_DATATYPE_MISALIGNMENT:
		rprintf(hReportFile, _T("a Datatype Misalignment"));
		break;

	case EXCEPTION_FLT_DENORMAL_OPERAND:
		rprintf(hReportFile, _T("a Float Denormal Operand"));
		break;

	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		rprintf(hReportFile, _T("a Float Divide By Zero"));
		break;

	case EXCEPTION_FLT_INEXACT_RESULT:
		rprintf(hReportFile, _T("a Float Inexact Result"));
		break;

	case EXCEPTION_FLT_INVALID_OPERATION:
		rprintf(hReportFile, _T("a Float Invalid Operation"));
		break;

	case EXCEPTION_FLT_OVERFLOW:
		rprintf(hReportFile, _T("a Float Overflow"));
		break;

	case EXCEPTION_FLT_STACK_CHECK:
		rprintf(hReportFile, _T("a Float Stack Check"));
		break;

	case EXCEPTION_FLT_UNDERFLOW:
		rprintf(hReportFile, _T("a Float Underflow"));
		break;

	case EXCEPTION_GUARD_PAGE:
		rprintf(hReportFile, _T("a Guard Page"));
		break;

	case EXCEPTION_ILLEGAL_INSTRUCTION:
		rprintf(hReportFile, _T("an Illegal Instruction"));
		break;

	case EXCEPTION_IN_PAGE_ERROR:
		rprintf(hReportFile, _T("an In Page Error"));
		break;

	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		rprintf(hReportFile, _T("an Integer Divide By Zero"));
		break;

	case EXCEPTION_INT_OVERFLOW:
		rprintf(hReportFile, _T("an Integer Overflow"));
		break;

	case EXCEPTION_INVALID_DISPOSITION:
		rprintf(hReportFile, _T("an Invalid Disposition"));
		break;

	case EXCEPTION_INVALID_HANDLE:
		rprintf(hReportFile, _T("an Invalid Handle"));
		break;

	case EXCEPTION_NONCONTINUABLE_EXCEPTION:
		rprintf(hReportFile, _T("a Noncontinuable Exception"));
		break;

	case EXCEPTION_PRIV_INSTRUCTION:
		rprintf(hReportFile, _T("a Privileged Instruction"));
		break;

	case EXCEPTION_SINGLE_STEP:
		rprintf(hReportFile, _T("a Single Step"));
		break;

	case EXCEPTION_STACK_OVERFLOW:
		rprintf(hReportFile, _T("a Stack Overflow"));
		break;

	case DBG_CONTROL_C:
		rprintf(hReportFile, _T("a Control+C"));
		break;

	case DBG_CONTROL_BREAK:
		rprintf(hReportFile, _T("a Control+Break"));
		break;

	case DBG_TERMINATE_THREAD:
		rprintf(hReportFile, _T("a Terminate Thread"));
		break;

	case DBG_TERMINATE_PROCESS:
		rprintf(hReportFile, _T("a Terminate Process"));
		break;

	case RPC_S_UNKNOWN_IF:
		rprintf(hReportFile, _T("an Unknown Interface"));
		break;

	case RPC_S_SERVER_UNAVAILABLE:
		rprintf(hReportFile, _T("a Server Unavailable"));
		break;

	default:
		/*
		static TCHAR szBuffer[512] = { 0 };

		// If not one of the "known" exceptions, try to get the string
		// from NTDLL.DLL's message table.

		FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE,
						GetModuleHandle(_T("NTDLL.DLL")),
						dwCode, 0, szBuffer, sizeof(szBuffer), 0);
		*/

		rprintf(hReportFile, _T("an Unknown [0x%lX] Exception"), pExceptionRecord->ExceptionCode);
		break;
	}

	// Now print information about where the fault occurred
	rprintf(hReportFile, _T(" at location %08x"), (DWORD) pExceptionRecord->ExceptionAddress);
	if ((hModule = (HMODULE) GetModuleBase((DWORD) pExceptionRecord->ExceptionAddress)) && GetModuleFileName(hModule, szModule, sizeof(szModule)))
	{
		rprintf(hReportFile, _T(" in module %s"), szModule);
	}

	// If the exception was an access violation, print out some additional information, to the error log and the debugger.
	if (pExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && pExceptionRecord->NumberParameters >= 2)
	{
		rprintf(hReportFile, _T(" %s location %08x"), pExceptionRecord->ExceptionInformation[0] ? "Writing to" : "Reading from", pExceptionRecord->ExceptionInformation[1]);
	}

	rprintf(hReportFile, _T(".\r\n\r\n"));

	dbgDumpLog(hReportFile);

	pContext = pExceptionInfo->ContextRecord;

#ifdef _M_IX86	// Intel Only!

	// Show the registers
	rprintf(hReportFile, _T("Registers:\r\n"));
	if (pContext->ContextFlags & CONTEXT_INTEGER)
		rprintf(
			hReportFile,
		    _T("eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx\r\n"),
		    pContext->Eax,
		    pContext->Ebx,
		    pContext->Ecx,
		    pContext->Edx,
		    pContext->Esi,
		    pContext->Edi
		);
	if (pContext->ContextFlags & CONTEXT_CONTROL)
		rprintf(
			hReportFile,
		    _T("eip=%08lx esp=%08lx ebp=%08lx iopl=%1lx %s %s %s %s %s %s %s %s %s %s\r\n"),
		    pContext->Eip,
		    pContext->Esp,
		    pContext->Ebp,
		    (pContext->EFlags >> 12) & 3,	//  IOPL level value
		    pContext->EFlags & 0x00100000 ? "vip" : "   ",	//  VIP (virtual interrupt pending)
		    pContext->EFlags & 0x00080000 ? "vif" : "   ",	//  VIF (virtual interrupt flag)
		    pContext->EFlags & 0x00000800 ? "ov" : "nv",	//  VIF (virtual interrupt flag)
		    pContext->EFlags & 0x00000400 ? "dn" : "up",	//  OF (overflow flag)
		    pContext->EFlags & 0x00000200 ? "ei" : "di",	//  IF (interrupt enable flag)
		    pContext->EFlags & 0x00000080 ? "ng" : "pl",	//  SF (sign flag)
		    pContext->EFlags & 0x00000040 ? "zr" : "nz",	//  ZF (zero flag)
		    pContext->EFlags & 0x00000010 ? "ac" : "na",	//  AF (aux carry flag)
		    pContext->EFlags & 0x00000004 ? "po" : "pe",	//  PF (parity flag)
		    pContext->EFlags & 0x00000001 ? "cy" : "nc"	//  CF (carry flag)
		);
	if (pContext->ContextFlags & CONTEXT_SEGMENTS)
	{
		rprintf(
			hReportFile,
		    _T("cs=%04lx  ss=%04lx  ds=%04lx  es=%04lx  fs=%04lx  gs=%04lx"),
		    pContext->SegCs,
		    pContext->SegSs,
		    pContext->SegDs,
		    pContext->SegEs,
		    pContext->SegFs,
		    pContext->SegGs,
		    pContext->EFlags
		);
		if (pContext->ContextFlags & CONTEXT_CONTROL)
			rprintf(
				hReportFile,
			    _T("             efl=%08lx"),
			    pContext->EFlags
			);
	}
	else if (pContext->ContextFlags & CONTEXT_CONTROL)
		rprintf(
			hReportFile,
		    _T("                                                                       efl=%08lx"),
		    pContext->EFlags
		);
	rprintf(hReportFile, _T("\r\n\r\n"));

#endif
	
	// Print a StackTrace
	WzExceptionStackWalker stackWalker(hReportFile);
	stackWalker.ShowCallstack(GetCurrentThread(), pContext);

	rprintf(hReportFile, _T("\r\n\r\n"));
}

#include <stdio.h>
#include <fcntl.h>
#include <io.h>

// Entry point where control comes on an unhandled exception
static
LONG WINAPI TopLevelExceptionFilter(PEXCEPTION_POINTERS pExceptionInfo)
{
	static bool bBeenHere = FALSE;

	if (!bBeenHere)
	{
		UINT fuOldErrorMode;

		bBeenHere = TRUE;

		fuOldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);

		HANDLE hReportFile = CreateFileW(
		                  szLogFileName,
		                  GENERIC_WRITE,
		                  0,
		                  0,
		                  OPEN_ALWAYS,
		                  FILE_FLAG_WRITE_THROUGH,
		                  0
		              );
		if (hReportFile == INVALID_HANDLE_VALUE)
		{
			// Retrieve the system error message for the last-error code

			LPVOID lpMsgBuf;
			DWORD dw = GetLastError();
			TCHAR szBuffer[4196];

			FormatMessage(
			    FORMAT_MESSAGE_ALLOCATE_BUFFER |
			    FORMAT_MESSAGE_FROM_SYSTEM |
			    FORMAT_MESSAGE_IGNORE_INSERTS,
			    NULL,
			    dw,
			    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			    (LPTSTR) &lpMsgBuf,
			    0, NULL);

			wsprintf(szBuffer, _T("Exception handler failed with error %d: %s\n"), dw, lpMsgBuf);
			MessageBox((HWND)MB_ICONEXCLAMATION, szBuffer, _T("Error"), MB_OK);

			LocalFree(lpMsgBuf);
			debug(LOG_ERROR, "Exception handler failed to create file!");
		}

		if (hReportFile)
		{
			wchar_t szBuffer[4196];
			int err;

			SetFilePointer(hReportFile, 0, 0, FILE_END);

			GenerateExceptionReport(hReportFile, pExceptionInfo);
			CloseHandle(hReportFile);

			bool savedMiniDump = false;
			if (MessageBoxW(NULL, L"Warzone crashed unexpectedly, would you like to save a \"mini-dump\" diagnostic file?\r\n(A standard report log .RPT will always be saved.)", L"Warzone 2100", MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
			{
				savedMiniDump = OutputMiniDump(szMinidumpFileName, pExceptionInfo);
			}

			if (savedMiniDump)
			{
				wsprintfW(szBuffer, L"Warzone has crashed. See the following files for more details: \r\n- %s\r\n(The crash report file - please attach this to your bug report)\r\n- %s\r\n(The \"mini-dump\" - for developers only)", szLogFileName, szMinidumpFileName);
			}
			else
			{
				wsprintfW(szBuffer, L"Warzone has crashed.\r\nSee %s for more details\r\n", szLogFileName);
			}
			err = MessageBoxW((HWND)MB_ICONERROR, szBuffer, L"Warzone Crashed!", MB_OK | MB_ICONERROR);
			if (err == 0)
			{
				LPVOID lpMsgBuf;
				DWORD dw = GetLastError();
				wchar_t szBuffer[4196];

				FormatMessageW(
				    FORMAT_MESSAGE_ALLOCATE_BUFFER |
				    FORMAT_MESSAGE_FROM_SYSTEM |
				    FORMAT_MESSAGE_IGNORE_INSERTS,
				    NULL,
				    dw,
				    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				    (LPWSTR) &lpMsgBuf,
				    0, NULL);

				wsprintfW(szBuffer, L"Exception handler failed with error %d: %s\n", dw, lpMsgBuf);
				MessageBoxW((HWND)MB_ICONEXCLAMATION, szBuffer, L"Error", MB_OK);

				LocalFree(lpMsgBuf);
				debug(LOG_ERROR, "Exception handler failed to create file!");
			}
			hReportFile = 0;
		}
		SetErrorMode(fuOldErrorMode);
	}

	if (prevExceptionFilter)
	{
		return prevExceptionFilter(pExceptionInfo);
	}
	else
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}
}

void ExchndlSetup(const char *packageVersion, const std::string &writeDir, bool portable_mode)
{
	wchar_t miniDumpPath[PATH_MAX] = {'\0'};
	DWORD dwRetVal = 0;

	// Install the unhandled exception filter function
	prevExceptionFilter = SetUnhandledExceptionFilter(TopLevelExceptionFilter);

	// Retrieve the current version
	formattedVersionString = strdup(packageVersion);

	// Convert the writeDir to WSTRING (UTF-16)
	int wcharsRequired = MultiByteToWideChar(CP_UTF8, 0, writeDir.c_str(), -1, NULL, 0);
	if (wcharsRequired == 0)
	{
		MessageBox((HWND)MB_ICONEXCLAMATION, "Failed to convert writeDir string to wide chars!\nProgram will now exit." , _T("Error"), MB_OK);
		exit(1);
	}
	std::vector<wchar_t> wchar_writeDir(wcharsRequired);
	if(MultiByteToWideChar(CP_UTF8, 0, writeDir.c_str(), -1, &wchar_writeDir[0], wcharsRequired) == 0)
	{
		MessageBox((HWND)MB_ICONEXCLAMATION, "Failed to convert writeDir string to wide chars! (2)\nProgram will now exit." , _T("Error"), MB_OK);
		exit(1);
	}

	// Because of UAC on vista / win7 we use this to write our dumps to (unless we override it via OverrideRPTDirectory())
	// NOTE: CSIDL_PERSONAL =  C:\Users\user name\Documents
	if (!portable_mode && SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, miniDumpPath)))
	{
		PathAppendW(miniDumpPath, wchar_writeDir.data());
		PathAppendW(miniDumpPath, L"\\logs");

		if (!PathFileExistsW(miniDumpPath))
		{
			if (ERROR_SUCCESS != SHCreateDirectoryExW(NULL, miniDumpPath, NULL))
			{
				//last attempt to get a path
				dwRetVal = GetTempPathW(PATH_MAX, miniDumpPath);
				if (dwRetVal > MAX_PATH || (dwRetVal == 0))
				{
					MessageBox((HWND)MB_ICONEXCLAMATION, "Could not created a temporary directory!\nProgram will now exit." , _T("Error"), MB_OK);
					exit(1);
				}
			}
		}
	}
	// in portable mode, we use where they installed it on, since this is a removeable drive (most likely), we will use where the program is located in.
	else if (portable_mode && ((dwRetVal = GetCurrentDirectoryW(MAX_PATH, miniDumpPath))))
	{
		if (dwRetVal > MAX_PATH)
		{
			MessageBox((HWND)MB_ICONEXCLAMATION, _T("Buffer exceeds maximum path to create directory.  Exiting."), _T("Error"), MB_OK);
			exit(1);
		}
		PathAppendW(miniDumpPath, wchar_writeDir.data());
		PathAppendW(miniDumpPath, L"\\logs");
	}
	else
	{
		// should never fail, but if it does, we fall back to this
		dwRetVal = GetTempPathW(PATH_MAX, miniDumpPath);
		if (dwRetVal > MAX_PATH || (dwRetVal == 0))
		{
			MessageBox((HWND)MB_ICONEXCLAMATION, _T("Could not created a temporary directory!\nProgram will now exit.") , _T("Error!"), MB_OK);
			exit(1);
		}
	}

	wcsncpy_s(szLogFileName, L"Warzone2100.RPT", _TRUNCATE);
	wcsncpy_s(szMinidumpFileName, L"Warzone2100.latest.mdmp", _TRUNCATE);

	wchar_t tmpPath[PATH_MAX] = {'\0'};
	if (wcsncpy_s(tmpPath, miniDumpPath, _TRUNCATE) != 0) { MessageBox((HWND)MB_ICONEXCLAMATION, _T("Buffer exceeds maximum path length (B-1).  Exiting."), _T("Error"), MB_OK); exit(1); }
	if (wcsncat_s(tmpPath, L"\\Warzone2100.RPT", _TRUNCATE) != 0) { MessageBox((HWND)MB_ICONEXCLAMATION, _T("Buffer exceeds maximum path length (C-1).  Exiting."), _T("Error"), MB_OK); exit(1); }
	if (wcsncpy_s(szLogFileName, tmpPath, _TRUNCATE) != 0) { MessageBox((HWND)MB_ICONEXCLAMATION, _T("Buffer exceeds maximum path length (D-1).  Exiting."), _T("Error"), MB_OK); exit(1); }

	if (wcsncpy_s(tmpPath, miniDumpPath, _TRUNCATE) != 0) { MessageBox((HWND)MB_ICONEXCLAMATION, _T("Buffer exceeds maximum path length (B-2).  Exiting."), _T("Error"), MB_OK); exit(1); }
	if (wcsncat_s(tmpPath, L"\\Warzone2100.latest.mdmp", _TRUNCATE) != 0) { MessageBox((HWND)MB_ICONEXCLAMATION, _T("Buffer exceeds maximum path length (C-2).  Exiting."), _T("Error"), MB_OK); exit(1); }
	if (wcsncpy_s(szMinidumpFileName, tmpPath, _TRUNCATE) != 0) { MessageBox((HWND)MB_ICONEXCLAMATION, _T("Buffer exceeds maximum path length (D-2).  Exiting."), _T("Error"), MB_OK); exit(1); }

	atexit(ExchndlShutdown);
}
void ResetRPTDirectory(wchar_t *newPath)
{
	wchar_t miniDumpPath[PATH_MAX] = { '\0' };
	if (wcsncpy_s(miniDumpPath, newPath, _TRUNCATE) != 0) { MessageBox((HWND)MB_ICONEXCLAMATION, _T("Buffer exceeds maximum path length (E).  Exiting."), _T("Error"), MB_OK); exit(1); }
	if (wcsncat_s(miniDumpPath, L"\\logs\\Warzone2100.RPT", _TRUNCATE) != 0) { MessageBox((HWND)MB_ICONEXCLAMATION, _T("Buffer exceeds maximum path length (F).  Exiting."), _T("Error"), MB_OK); exit(1); }
	if (wcsncpy_s(szLogFileName, miniDumpPath, _TRUNCATE) != 0) { MessageBox((HWND)MB_ICONEXCLAMATION, _T("Buffer exceeds maximum path length (G).  Exiting."), _T("Error"), MB_OK); exit(1); }

	if (wcsncpy_s(miniDumpPath, newPath, _TRUNCATE) != 0) { MessageBox((HWND)MB_ICONEXCLAMATION, _T("Buffer exceeds maximum path length (H).  Exiting."), _T("Error"), MB_OK); exit(1); }
	if (wcsncat_s(miniDumpPath, L"\\logs\\Warzone2100.latest.mdmp", _TRUNCATE) != 0) { MessageBox((HWND)MB_ICONEXCLAMATION, _T("Buffer exceeds maximum path length (I).  Exiting."), _T("Error"), MB_OK); exit(1); }
	if (wcsncpy_s(szMinidumpFileName, miniDumpPath, _TRUNCATE) != 0) { MessageBox((HWND)MB_ICONEXCLAMATION, _T("Buffer exceeds maximum path length (J).  Exiting."), _T("Error"), MB_OK); exit(1); }
}
void ExchndlShutdown(void)
{
	if (prevExceptionFilter)
	{
		SetUnhandledExceptionFilter(prevExceptionFilter);
	}

	prevExceptionFilter = NULL;
	free(formattedVersionString);
	formattedVersionString = NULL;
}
