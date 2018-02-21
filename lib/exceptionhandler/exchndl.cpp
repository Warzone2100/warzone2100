/*
	This file is part of Warzone 2100.
	Copyright (C) 1997-XXXX  José Fonseca <j_r_fonseca@yahoo.co.uk>
	 * Originally based on Matt Pietrek's MSJEXHND.CPP in Microsoft Systems Journal, April 1997.
	Copyright (C) 2008  Giel van Schijndel
	Copyright (C) 2008-2017  Warzone 2100 Project

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
#if (_WIN32_WINNT < 0x0500)			// must force win 2k or higher
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#include "lib/framework/frame.h"
#include "dumpinfo.h"
#include "exchndl.h"

#include <assert.h>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <shlobj.h>
#include <shlwapi.h>

#if !defined(WZ_CC_MSVC)
#define HAVE_BFD	1
#endif

#define WSTRING(x) CONCAT(L, x)

// Declare the static variables
static wchar_t szLogFileName[MAX_PATH] = L"";
static LPTOP_LEVEL_EXCEPTION_FILTER prevExceptionFilter = NULL;
static HANDLE hReportFile;
static char *formattedVersionString = NULL;

static
int __cdecl rprintf(const TCHAR *format, ...)
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

	return VirtualQuery((LPCVOID) dwAddress, &Buffer, sizeof(Buffer)) ? (DWORD) Buffer.AllocationBase : 0;
}


#ifdef HAVE_BFD

#include <bfd.h>
extern "C"
{
#include "include/demangle.h"
// cross compiler does not like these...
#if !defined(WZ_CC_MINGW)
#include "include/coff/internal.h"
#include "include/libcoff.h"
#endif
}

// Read in the symbol table.
static bfd_boolean
slurp_symtab(bfd *abfd, asymbol ***syms, long *symcount)
{
	long storage;

	if ((bfd_get_file_flags(abfd) & HAS_SYMS) == 0)
	{
		return FALSE;
	}

	storage = bfd_get_symtab_upper_bound(abfd);
	if (storage < 0)
	{
		return FALSE;
	}

	*syms = (asymbol **) GlobalAlloc(GMEM_FIXED, storage);
	if (*syms == NULL)
	{
		return FALSE;
	}

	if ((*symcount = bfd_canonicalize_symtab(abfd, *syms)) < 0)
	{
		return FALSE;
	}

	return TRUE;
}

// This stucture is used to pass information between translate_addresses and find_address_in_section.
struct find_handle
{
	asymbol **syms;
	bfd_vma pc;
	const char *filename;
	const char *functionname;
	unsigned int line;
	bfd_boolean found;
};

// Look for an address in a section.  This is called via  bfd_map_over_sections.
static void find_address_in_section(bfd *abfd, asection *section, PTR data)
{
	struct find_handle *info = (struct find_handle *) data;
	bfd_vma vma;
	bfd_size_type size;

	if (info->found)
	{
		return;
	}

	if ((bfd_get_section_flags(abfd, section) & SEC_ALLOC) == 0)
	{
		return;
	}

	vma = bfd_get_section_vma(abfd, section);
	size = bfd_get_section_size(section);

	if (info->pc < (vma = bfd_get_section_vma(abfd, section)))
	{
		return;
	}

	if (info->pc >= vma + (size = bfd_get_section_size(section)))
	{
		return;
	}

	info->found = bfd_find_nearest_line(abfd, section, info->syms, info->pc - vma, &info->filename, &info->functionname, &info->line);
}

static
bool BfdDemangleSymName(LPCTSTR lpName, LPTSTR lpDemangledName, DWORD nSize)
{
	char *res;

	assert(lpName != NULL);

	if ((res = cplus_demangle(lpName, DMGL_ANSI /*| DMGL_PARAMS*/)) == NULL)
	{
		lstrcpyn(lpDemangledName, lpName, nSize);
		return FALSE;
	}
	else
	{
		lstrcpyn(lpDemangledName, res, nSize);
		free(res);
		return TRUE;
	}
}

static
bool BfdGetSymFromAddr(bfd *abfd, asymbol **syms, long symcount, DWORD dwAddress, LPTSTR lpSymName, DWORD nSize)
{
	HMODULE hModule;
	struct find_handle info;

	if (!(hModule = (HMODULE) GetModuleBase(dwAddress)))
	{
		return FALSE;
	}

	info.pc = dwAddress;

	if (!(bfd_get_file_flags(abfd) & HAS_SYMS) || !symcount)
	{
		return FALSE;
	}
	info.syms = syms;

	info.found = FALSE;
	bfd_map_over_sections(abfd, find_address_in_section, (PTR) &info);
	if (info.found == FALSE || info.line == 0)
	{
		return FALSE;
	}

	assert(lpSymName);

	if (info.functionname == NULL && *info.functionname == '\0')
	{
		return FALSE;
	}

	lstrcpyn(lpSymName, info.functionname, nSize);

	return TRUE;
}

static
bool BfdGetLineFromAddr(bfd *abfd, asymbol **syms, long symcount, DWORD dwAddress,  LPTSTR lpFileName, DWORD nSize, LPDWORD lpLineNumber)
{
	HMODULE hModule;
	struct find_handle info;

	if (!(hModule = (HMODULE) GetModuleBase(dwAddress)))
	{
		return FALSE;
	}

	info.pc = dwAddress;

	if (!(bfd_get_file_flags(abfd) & HAS_SYMS) || !symcount)
	{
		return FALSE;
	}

	info.syms = syms;

	info.found = FALSE;
	bfd_map_over_sections(abfd, find_address_in_section, (PTR) &info);
	if (info.found == FALSE || info.line == 0)
	{
		return FALSE;
	}

	assert(lpFileName && lpLineNumber);

	lstrcpyn(lpFileName, info.filename, nSize);
	*lpLineNumber = info.line;

	return TRUE;
}

#endif /* HAVE_BFD */

#include <imagehlp.h>

static bool bSymInitialized = FALSE;

static HMODULE hModule_Imagehlp = NULL;

typedef bool (WINAPI *PFNSYMINITIALIZE)(HANDLE, LPSTR, bool);
static PFNSYMINITIALIZE pfnSymInitialize = NULL;

static
bool WINAPI j_SymInitialize(HANDLE hProcess, PSTR UserSearchPath, bool fInvadeProcess)
{
	if (
	    (hModule_Imagehlp || (hModule_Imagehlp = LoadLibrary(_T("IMAGEHLP.DLL")))) &&
	    (pfnSymInitialize || (pfnSymInitialize = (PFNSYMINITIALIZE) GetProcAddress(hModule_Imagehlp, "SymInitialize")))
	)
	{
		return pfnSymInitialize(hProcess, UserSearchPath, fInvadeProcess);
	}
	else
	{
		return FALSE;
	}
}

typedef bool (WINAPI *PFNSYMCLEANUP)(HANDLE);
static PFNSYMCLEANUP pfnSymCleanup = NULL;

static
bool WINAPI j_SymCleanup(HANDLE hProcess)
{
	if (
	    (hModule_Imagehlp || (hModule_Imagehlp = LoadLibrary(_T("IMAGEHLP.DLL")))) &&
	    (pfnSymCleanup || (pfnSymCleanup = (PFNSYMCLEANUP) GetProcAddress(hModule_Imagehlp, "SymCleanup")))
	)
	{
		return pfnSymCleanup(hProcess);
	}
	else
	{
		return FALSE;
	}
}

typedef DWORD (WINAPI *PFNSYMSETOPTIONS)(DWORD);
static PFNSYMSETOPTIONS pfnSymSetOptions = NULL;

static
DWORD WINAPI j_SymSetOptions(DWORD SymOptions)
{
	if (
	    (hModule_Imagehlp || (hModule_Imagehlp = LoadLibrary(_T("IMAGEHLP.DLL")))) &&
	    (pfnSymSetOptions || (pfnSymSetOptions = (PFNSYMSETOPTIONS) GetProcAddress(hModule_Imagehlp, "SymSetOptions")))
	)
	{
		return pfnSymSetOptions(SymOptions);
	}
	else
	{
		return FALSE;
	}
}

typedef bool (WINAPI *PFNSYMUNDNAME)(PIMAGEHLP_SYMBOL, PSTR, DWORD);
static PFNSYMUNDNAME pfnSymUnDName = NULL;

static
bool WINAPI j_SymUnDName(PIMAGEHLP_SYMBOL Symbol, PSTR UnDecName, DWORD UnDecNameLength)
{
	if (
	    (hModule_Imagehlp || (hModule_Imagehlp = LoadLibrary(_T("IMAGEHLP.DLL")))) &&
	    (pfnSymUnDName || (pfnSymUnDName = (PFNSYMUNDNAME) GetProcAddress(hModule_Imagehlp, "SymUnDName")))
	)
	{
		return pfnSymUnDName(Symbol, UnDecName, UnDecNameLength);
	}
	else
	{
		return FALSE;
	}
}

typedef PFUNCTION_TABLE_ACCESS_ROUTINE PFNSYMFUNCTIONTABLEACCESS;
static PFNSYMFUNCTIONTABLEACCESS pfnSymFunctionTableAccess = NULL;

static
PVOID WINAPI j_SymFunctionTableAccess(HANDLE hProcess, DWORD AddrBase)
{
	if (
	    (hModule_Imagehlp || (hModule_Imagehlp = LoadLibrary(_T("IMAGEHLP.DLL")))) &&
	    (pfnSymFunctionTableAccess || (pfnSymFunctionTableAccess = (PFNSYMFUNCTIONTABLEACCESS) GetProcAddress(hModule_Imagehlp, "SymFunctionTableAccess")))
	)
	{
		return pfnSymFunctionTableAccess(hProcess, AddrBase);
	}
	else
	{
		return NULL;
	}
}

typedef PGET_MODULE_BASE_ROUTINE PFNSYMGETMODULEBASE;
static PFNSYMGETMODULEBASE pfnSymGetModuleBase = NULL;

static
DWORD WINAPI j_SymGetModuleBase(HANDLE hProcess, DWORD dwAddr)
{
	if (
	    (hModule_Imagehlp || (hModule_Imagehlp = LoadLibrary(_T("IMAGEHLP.DLL")))) &&
	    (pfnSymGetModuleBase || (pfnSymGetModuleBase = (PFNSYMGETMODULEBASE) GetProcAddress(hModule_Imagehlp, "SymGetModuleBase")))
	)
	{
		return pfnSymGetModuleBase(hProcess, dwAddr);
	}
	else
	{
		return 0;
	}
}

typedef bool (WINAPI *PFNSTACKWALK)(DWORD, HANDLE, HANDLE, LPSTACKFRAME, LPVOID, PREAD_PROCESS_MEMORY_ROUTINE, PFUNCTION_TABLE_ACCESS_ROUTINE, PGET_MODULE_BASE_ROUTINE, PTRANSLATE_ADDRESS_ROUTINE);
static PFNSTACKWALK pfnStackWalk = NULL;

static
bool WINAPI j_StackWalk(
    DWORD MachineType,
    HANDLE hProcess,
    HANDLE hThread,
    LPSTACKFRAME StackFrame,
    PVOID ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
    PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine,
    PTRANSLATE_ADDRESS_ROUTINE TranslateAddress
)
{
	if (
	    (hModule_Imagehlp || (hModule_Imagehlp = LoadLibrary(_T("IMAGEHLP.DLL")))) &&
	    (pfnStackWalk || (pfnStackWalk = (PFNSTACKWALK) GetProcAddress(hModule_Imagehlp, "StackWalk")))
	)
		return pfnStackWalk(
		           MachineType,
		           hProcess,
		           hThread,
		           StackFrame,
		           ContextRecord,
		           ReadMemoryRoutine,
		           FunctionTableAccessRoutine,
		           GetModuleBaseRoutine,
		           TranslateAddress
		       );
	else
	{
		return FALSE;
	}
}

typedef bool (WINAPI *PFNSYMGETSYMFROMADDR)(HANDLE, DWORD, LPDWORD, PIMAGEHLP_SYMBOL);
static PFNSYMGETSYMFROMADDR pfnSymGetSymFromAddr = NULL;

static
bool WINAPI j_SymGetSymFromAddr(HANDLE hProcess, DWORD Address, PDWORD Displacement, PIMAGEHLP_SYMBOL Symbol)
{
	if (
	    (hModule_Imagehlp || (hModule_Imagehlp = LoadLibrary(_T("IMAGEHLP.DLL")))) &&
	    (pfnSymGetSymFromAddr || (pfnSymGetSymFromAddr = (PFNSYMGETSYMFROMADDR) GetProcAddress(hModule_Imagehlp, "SymGetSymFromAddr")))
	)
	{
		return pfnSymGetSymFromAddr(hProcess, Address, Displacement, Symbol);
	}
	else
	{
		return FALSE;
	}
}

typedef bool (WINAPI *PFNSYMGETLINEFROMADDR)(HANDLE, DWORD, LPDWORD, PIMAGEHLP_LINE);
static PFNSYMGETLINEFROMADDR pfnSymGetLineFromAddr = NULL;

static
bool WINAPI j_SymGetLineFromAddr(HANDLE hProcess, DWORD dwAddr, PDWORD pdwDisplacement, PIMAGEHLP_LINE Line)
{
	if (
	    (hModule_Imagehlp || (hModule_Imagehlp = LoadLibrary(_T("IMAGEHLP.DLL")))) &&
	    (pfnSymGetLineFromAddr || (pfnSymGetLineFromAddr = (PFNSYMGETLINEFROMADDR) GetProcAddress(hModule_Imagehlp, "SymGetLineFromAddr")))
	)
	{
		return pfnSymGetLineFromAddr(hProcess, dwAddr, pdwDisplacement, Line);
	}
	else
	{
		return FALSE;
	}
}

static
bool ImagehlpDemangleSymName(LPCTSTR lpName, LPTSTR lpDemangledName, DWORD nSize)
{
	BYTE symbolBuffer[sizeof(IMAGEHLP_SYMBOL) + 512];
	PIMAGEHLP_SYMBOL pSymbol = (PIMAGEHLP_SYMBOL) symbolBuffer;

	memset(symbolBuffer, 0, sizeof(symbolBuffer));

	pSymbol->SizeOfStruct = sizeof(symbolBuffer);
	pSymbol->MaxNameLength = 512;

	lstrcpyn((LPTSTR)pSymbol->Name, lpName, pSymbol->MaxNameLength);

	if (!j_SymUnDName(pSymbol, (PSTR)lpDemangledName, nSize))
	{
		return FALSE;
	}

	return TRUE;
}

static
bool ImagehlpGetSymFromAddr(HANDLE hProcess, DWORD dwAddress, LPTSTR lpSymName, DWORD nSize)
{
	// IMAGEHLP is wacky, and requires you to pass in a pointer to a
	// IMAGEHLP_SYMBOL structure.  The problem is that this structure is
	// variable length.  That is, you determine how big the structure is
	// at runtime.  This means that you can't use sizeof(struct).
	// So...make a buffer that's big enough, and make a pointer
	// to the buffer.  We also need to initialize not one, but TWO
	// members of the structure before it can be used.

	BYTE symbolBuffer[sizeof(IMAGEHLP_SYMBOL) + 512];
	PIMAGEHLP_SYMBOL pSymbol = (PIMAGEHLP_SYMBOL) symbolBuffer;
	DWORD dwDisplacement = 0;  // Displacement of the input address, relative to the start of the symbol

	pSymbol->SizeOfStruct = sizeof(symbolBuffer);
	pSymbol->MaxNameLength = 512;

	assert(bSymInitialized);

	if (!j_SymGetSymFromAddr(hProcess, dwAddress, &dwDisplacement, pSymbol))
	{
		return FALSE;
	}

	lstrcpyn(lpSymName, (LPCTSTR)pSymbol->Name, nSize);

	return TRUE;
}

static
bool ImagehlpGetLineFromAddr(HANDLE hProcess, DWORD dwAddress,  LPTSTR lpFileName, DWORD nSize, LPDWORD lpLineNumber)
{
	IMAGEHLP_LINE Line;
	DWORD dwDisplacement = 0;  // Displacement of the input address, relative to the start of the symbol

	// Do the source and line lookup.
	memset(&Line, 0, sizeof(IMAGEHLP_LINE));
	Line.SizeOfStruct = sizeof(IMAGEHLP_LINE);

	assert(bSymInitialized);

#if 1
	{
		// The problem is that the symbol engine only finds those source
		//  line addresses (after the first lookup) that fall exactly on
		//  a zero displacement.  I will walk backwards 100 bytes to
		//  find the line and return the proper displacement.
		DWORD dwTempDisp = 0 ;
		while (dwTempDisp < 100 && !j_SymGetLineFromAddr(hProcess, dwAddress - dwTempDisp, &dwDisplacement, &Line))
		{
			++dwTempDisp;
		}

		if (dwTempDisp >= 100)
		{
			return FALSE;
		}

		// It was found and the source line information is correct so
		//  change the displacement if it was looked up multiple times.
		if (dwTempDisp < 100 && dwTempDisp != 0)
		{
			dwDisplacement = dwTempDisp;
		}
	}
#else
	if (!j_SymGetLineFromAddr(hProcess, dwAddress, &dwDisplacement, &Line))
	{
		return FALSE;
	}
#endif

	assert(lpFileName && lpLineNumber);

	lstrcpyn(lpFileName, (LPCTSTR)Line.FileName, nSize);
	*lpLineNumber = Line.LineNumber;

	return TRUE;
}

static
bool PEGetSymFromAddr(HANDLE hProcess, DWORD dwAddress, LPTSTR lpSymName, DWORD nSize)
{
	HMODULE hModule;
	PIMAGE_NT_HEADERS pNtHdr;
	IMAGE_NT_HEADERS NtHdr;
	PIMAGE_SECTION_HEADER pSection;
	DWORD dwNearestAddress = 0, dwNearestName;
	int i;

	if (!(hModule = (HMODULE) GetModuleBase(dwAddress)))
	{
		return FALSE;
	}

	{
		PIMAGE_DOS_HEADER pDosHdr;
		LONG e_lfanew;

		// Point to the DOS header in memory
		pDosHdr = (PIMAGE_DOS_HEADER)hModule;

		// From the DOS header, find the NT (PE) header
		if (!ReadProcessMemory(hProcess, &pDosHdr->e_lfanew, &e_lfanew, sizeof(e_lfanew), NULL))
		{
			return FALSE;
		}

		pNtHdr = (PIMAGE_NT_HEADERS)((DWORD)hModule + (DWORD)e_lfanew);

		if (!ReadProcessMemory(hProcess, pNtHdr, &NtHdr, sizeof(IMAGE_NT_HEADERS), NULL))
		{
			return FALSE;
		}
	}

	pSection = (PIMAGE_SECTION_HEADER)((DWORD)pNtHdr + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + NtHdr.FileHeader.SizeOfOptionalHeader);

	// Look for export section
	for (i = 0; i < NtHdr.FileHeader.NumberOfSections; i++, pSection++)
	{
		IMAGE_SECTION_HEADER Section;
		PIMAGE_EXPORT_DIRECTORY pExportDir = NULL;
		BYTE ExportSectionName[IMAGE_SIZEOF_SHORT_NAME] = {'.', 'e', 'd', 'a', 't', 'a', '\0', '\0'};

		if (!ReadProcessMemory(hProcess, pSection, &Section, sizeof(IMAGE_SECTION_HEADER), NULL))
		{
			return FALSE;
		}

		if (memcmp(Section.Name, ExportSectionName, IMAGE_SIZEOF_SHORT_NAME) == 0)
		{
			pExportDir = (PIMAGE_EXPORT_DIRECTORY) Section.VirtualAddress;
		}
		else if ((NtHdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress >= Section.VirtualAddress) && (NtHdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress < (Section.VirtualAddress + Section.SizeOfRawData)))
		{
			pExportDir = (PIMAGE_EXPORT_DIRECTORY) NtHdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
		}

		if (pExportDir)
		{
			IMAGE_EXPORT_DIRECTORY ExportDir;

			if (!ReadProcessMemory(hProcess, (PVOID)((DWORD)hModule + (DWORD)pExportDir), &ExportDir, sizeof(IMAGE_EXPORT_DIRECTORY), NULL))
			{
				return FALSE;
			}

			{
				PDWORD *AddressOfFunctions = (PDWORD *)alloca(ExportDir.NumberOfFunctions * sizeof(PDWORD));
				int j;

				if (!ReadProcessMemory(hProcess, (PVOID)((DWORD)hModule + (DWORD)ExportDir.AddressOfFunctions), AddressOfFunctions, ExportDir.NumberOfFunctions * sizeof(PDWORD), NULL))
				{
					return FALSE;
				}

				for (j = 0; j < ExportDir.NumberOfNames; ++j)
				{
					DWORD pFunction = (DWORD)hModule + (DWORD)AddressOfFunctions[j];
					//ReadProcessMemory(hProcess, (DWORD) hModule + (DWORD) (&ExportDir.AddressOfFunctions[j]), &pFunction, sizeof(pFunction), NULL);

					if (pFunction <= dwAddress && pFunction > dwNearestAddress)
					{
						dwNearestAddress = pFunction;

						if (!ReadProcessMemory(hProcess, (PVOID)((DWORD)hModule + (DWORD)(&ExportDir.AddressOfNames)[j]), &dwNearestName, sizeof(dwNearestName), NULL))
						{
							return FALSE;
						}

						dwNearestName = (DWORD) hModule + dwNearestName;
					}
				}
			}
		}
	}

	if (!dwNearestAddress)
	{
		return FALSE;
	}

	if (!ReadProcessMemory(hProcess, (PVOID)dwNearestName, lpSymName, nSize, NULL))
	{
		return FALSE;
	}
	lpSymName[nSize - 1] = 0;

	return TRUE;
}

// Cross platform compatibility.
// If you change this, make sure it doesn't break the cross compile or the native compile.
// Don't ask why this is needed, have no idea.
struct ItDoesntMatterIfItsADWORDOrAVoidPointer_JustCompileTheDamnThing
{
	ItDoesntMatterIfItsADWORDOrAVoidPointer_JustCompileTheDamnThing(void *whatever) : whatever(whatever) {}
	operator void *() const
	{
		return whatever;    // Oooh, look compiler1, I'm a void *, just what you wanted!
	}
	operator DWORD() const
	{
		return (DWORD)whatever;    // Oooh, look compiler2, I'm a DWORD, just what you wanted!
	}
	void *whatever;
};

static
bool WINAPI IntelStackWalk(
    DWORD MachineType,
    HANDLE hProcess,
    WZ_DECL_UNUSED HANDLE hThread,
    LPSTACKFRAME StackFrame,
    PCONTEXT ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE ReadMemoryRoutine,
    WZ_DECL_UNUSED PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
    WZ_DECL_UNUSED PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine,
    WZ_DECL_UNUSED PTRANSLATE_ADDRESS_ROUTINE TranslateAddress
)
{
	assert(MachineType == IMAGE_FILE_MACHINE_I386);

	if (ReadMemoryRoutine == NULL)
	{
		ReadMemoryRoutine = (PREAD_PROCESS_MEMORY_ROUTINE)ReadProcessMemory;
	}

	if (!StackFrame->Reserved[0])
	{
		StackFrame->Reserved[0] = 1;

		StackFrame->AddrPC.Mode = AddrModeFlat;
		StackFrame->AddrPC.Offset = ContextRecord->Eip;
		StackFrame->AddrStack.Mode = AddrModeFlat;
		StackFrame->AddrStack.Offset = ContextRecord->Esp;
		StackFrame->AddrFrame.Mode = AddrModeFlat;
		StackFrame->AddrFrame.Offset = ContextRecord->Ebp;

		StackFrame->AddrReturn.Mode = AddrModeFlat;
// Error   26      error C2664: 'bool (HANDLE,DWORD,PVOID,DWORD,PDWORD)' :
// cannot convert parameter 2 from 'void *' to 'DWORD'
// c:\warzone\lib\exceptionhandler\exchndl.cpp     599
// ../../../../lib/exceptionhandler/exchndl.cpp: In function ‘bool IntelStackWalk(DWORD, void*, void*, _tagSTACKFRAME*, CONTEXT*, bool (*)(void*, const void*, void*, DWORD, DWORD*), void* (*)(void*, DWORD), DWORD (*)(void*, DWORD), DWORD (*)(void*, void*, _tagADDRESS*))’:
// ../../../../lib/exceptionhandler/exchndl.cpp:599: error: invalid conversion from ‘long unsigned int’ to ‘const void*’
		if (!ReadMemoryRoutine((HANDLE)hProcess, ItDoesntMatterIfItsADWORDOrAVoidPointer_JustCompileTheDamnThing((void *)(StackFrame->AddrFrame.Offset + sizeof(DWORD))), (void *)&StackFrame->AddrReturn.Offset, sizeof(DWORD), NULL))
		{
			return FALSE;
		}
	}
	else
	{
		StackFrame->AddrPC.Offset = StackFrame->AddrReturn.Offset;
		//AddrStack = AddrFrame + 2*sizeof(DWORD);
		if (!ReadMemoryRoutine((HANDLE)hProcess, ItDoesntMatterIfItsADWORDOrAVoidPointer_JustCompileTheDamnThing((void *) StackFrame->AddrFrame.Offset), (void *)&StackFrame->AddrFrame.Offset, sizeof(DWORD), NULL))
		{
			return FALSE;
		}
		if (!ReadMemoryRoutine((HANDLE)hProcess, ItDoesntMatterIfItsADWORDOrAVoidPointer_JustCompileTheDamnThing((void *)(StackFrame->AddrFrame.Offset + sizeof(DWORD))), (void *)&StackFrame->AddrReturn.Offset, sizeof(DWORD), NULL))
		{
			return FALSE;
		}
	}

	ReadMemoryRoutine((HANDLE)hProcess, ItDoesntMatterIfItsADWORDOrAVoidPointer_JustCompileTheDamnThing((void *)(StackFrame->AddrFrame.Offset + 2 * sizeof(DWORD))), (void *)StackFrame->Params, sizeof(StackFrame->Params), NULL);

	return TRUE;
}

static
bool StackBackTrace(HANDLE hProcess, HANDLE hThread, PCONTEXT pContext)
{
	STACKFRAME StackFrame;

	HMODULE hModule = NULL;
	TCHAR szModule[MAX_PATH];

#ifdef HAVE_BFD
	bfd *abfd = NULL;
	asymbol **syms = NULL;	// The symbol table.
	long symcount = 0;	// Number of symbols in `syms'.
#endif /* HAVE_BFD */

	assert(!bSymInitialized);

	j_SymSetOptions(/* SYMOPT_UNDNAME | */ SYMOPT_LOAD_LINES);
	if (j_SymInitialize(hProcess, NULL, TRUE))
	{
		bSymInitialized = TRUE;
	}

	memset(&StackFrame, 0, sizeof(StackFrame));

	// Initialize the STACKFRAME structure for the first call.  This is only
	// necessary for Intel CPUs, and isn't mentioned in the documentation.
	StackFrame.AddrPC.Offset = pContext->Eip;
	StackFrame.AddrPC.Mode = AddrModeFlat;
	StackFrame.AddrStack.Offset = pContext->Esp;
	StackFrame.AddrStack.Mode = AddrModeFlat;
	StackFrame.AddrFrame.Offset = pContext->Ebp;
	StackFrame.AddrFrame.Mode = AddrModeFlat;

	rprintf(_T("Call stack:\r\n"));

	if (0)
	{
		rprintf(_T("AddrPC     AddrReturn AddrFrame  AddrStack\r\n"));
	}

	while (1)
	{
		bool bSuccess = FALSE;
#ifdef HAVE_BFD
		const HMODULE hPrevModule = hModule;
#endif /* HAVE_BFD */
		TCHAR szSymName[512] = _T("");
		TCHAR szFileName[MAX_PATH] = _T("");
		DWORD LineNumber = 0;

		if (bSymInitialized)
		{
			if (!j_StackWalk(
			        IMAGE_FILE_MACHINE_I386,
			        hProcess,
			        hThread,
			        &StackFrame,
			        pContext,
			        NULL,
			        j_SymFunctionTableAccess,
			        j_SymGetModuleBase,
			        NULL
			    )
			   )
			{
				break;
			}
		}
		else
		{
			if (!IntelStackWalk(
			        IMAGE_FILE_MACHINE_I386,
			        hProcess,
			        hThread,
			        &StackFrame,
			        pContext,
			        NULL,
			        NULL,
			        NULL,
			        NULL
			    )
			   )
			{
				break;
			}
		}

		// Basic sanity check to make sure  the frame is OK.  Bail if not.
		if (0 == StackFrame.AddrFrame.Offset)
		{
			break;
		}

		if (0)
		{
			rprintf(
			    _T("%08lX   %08lX   %08lX   %08lX\r\n"),
			    StackFrame.AddrPC.Offset,
			    StackFrame.AddrReturn.Offset,
			    StackFrame.AddrFrame.Offset,
			    StackFrame.AddrStack.Offset
			);
			rprintf(
			    _T("%08lX   %08lX   %08lX   %08lX\r\n"),
			    StackFrame.Params[0],
			    StackFrame.Params[1],
			    StackFrame.Params[2],
			    StackFrame.Params[3]
			);
		}

		rprintf(_T("%08lX"), StackFrame.AddrPC.Offset);

		if ((hModule = (HMODULE) GetModuleBase(StackFrame.AddrPC.Offset)) && GetModuleFileName(hModule, szModule, sizeof(szModule)))
		{
#ifndef HAVE_BFD
			rprintf(_T("  %s:ModulBase %08lX"), szModule, hModule);
#else /* HAVE_BFD */
			rprintf(_T("  %s:%08lX"), szModule, StackFrame.AddrPC.Offset);

			if (hModule != hPrevModule)
			{
				if (syms)
				{
					GlobalFree(syms);
					syms = NULL;
					symcount = 0;
				}

				if (abfd)
				{
					bfd_close(abfd);
				}

				if ((abfd = bfd_openr(szModule, NULL)))
					if (bfd_check_format(abfd, bfd_object))
						if (bfd_get_file_flags(abfd) & HAS_SYMS)
							/* Read in the symbol table.  */
						{
							slurp_symtab(abfd, &syms, &symcount);
						}
			}

			if (!bSuccess && abfd && syms && symcount)
				if ((bSuccess = BfdGetSymFromAddr(abfd, syms, symcount, StackFrame.AddrPC.Offset, szSymName, 512)))
				{
					/*
					framepointer = StackFrame.AddrFrame.Offset;
					hprocess = hProcess;
					*/

					BfdDemangleSymName(szSymName, szSymName, 512);

					rprintf(_T("  %s"), szSymName);

					if (BfdGetLineFromAddr(abfd, syms, symcount, StackFrame.AddrPC.Offset, szFileName, MAX_PATH, &LineNumber))
					{
						rprintf(_T("  %s:%ld"), szFileName, LineNumber);
					}
				}
#endif /* HAVE_BFD */

			if (!bSuccess && bSymInitialized)
				if ((bSuccess = ImagehlpGetSymFromAddr(hProcess, StackFrame.AddrPC.Offset, szSymName, 512)))
				{
					rprintf(_T("  %s"), szSymName);

					ImagehlpDemangleSymName(szSymName, szSymName, 512);

					if (ImagehlpGetLineFromAddr(hProcess, StackFrame.AddrPC.Offset, szFileName, MAX_PATH, &LineNumber))
					{
						rprintf(_T("  %s:%ld"), szFileName, LineNumber);
					}
				}

			if (!bSuccess)
				if ((bSuccess = PEGetSymFromAddr(hProcess, StackFrame.AddrPC.Offset, szSymName, 512)))
				{
					rprintf(_T("  %s"), szSymName);
				}
		}

		rprintf(_T("\r\n"));
	}

#ifdef HAVE_BFD
	if (syms)
	{
		GlobalFree(syms);
		syms = NULL;
		symcount = 0;
	}

	if (abfd)
	{
		bfd_close(abfd);
	}
#endif /* HAVE_BFD */

	if (bSymInitialized)
	{
		if (!j_SymCleanup(hProcess))
		{
			assert(0);
		}

		bSymInitialized = FALSE;
	}

	return TRUE;
}

static
void GenerateExceptionReport(PEXCEPTION_POINTERS pExceptionInfo)
{
	PEXCEPTION_RECORD pExceptionRecord = pExceptionInfo->ExceptionRecord;
	TCHAR szModule[MAX_PATH];
	HMODULE hModule;
	PCONTEXT pContext;

	// Start out with a banner
	rprintf(_T("-------------------\r\n\r\n"));

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
		rprintf(_T("Error occurred on %s, %s %i, %i at %02i:%02i:%02i.\r\n\r\n"),
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
	rprintf(_T("\r\n%s caused "),  GetModuleFileName(NULL, szModule, MAX_PATH) ? szModule : _T("Application"));
	switch (pExceptionRecord->ExceptionCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		rprintf(_T("an Access Violation"));
		break;

	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		rprintf(_T("an Array Bound Exceeded"));
		break;

	case EXCEPTION_BREAKPOINT:
		rprintf(_T("a Breakpoint"));
		break;

	case EXCEPTION_DATATYPE_MISALIGNMENT:
		rprintf(_T("a Datatype Misalignment"));
		break;

	case EXCEPTION_FLT_DENORMAL_OPERAND:
		rprintf(_T("a Float Denormal Operand"));
		break;

	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		rprintf(_T("a Float Divide By Zero"));
		break;

	case EXCEPTION_FLT_INEXACT_RESULT:
		rprintf(_T("a Float Inexact Result"));
		break;

	case EXCEPTION_FLT_INVALID_OPERATION:
		rprintf(_T("a Float Invalid Operation"));
		break;

	case EXCEPTION_FLT_OVERFLOW:
		rprintf(_T("a Float Overflow"));
		break;

	case EXCEPTION_FLT_STACK_CHECK:
		rprintf(_T("a Float Stack Check"));
		break;

	case EXCEPTION_FLT_UNDERFLOW:
		rprintf(_T("a Float Underflow"));
		break;

	case EXCEPTION_GUARD_PAGE:
		rprintf(_T("a Guard Page"));
		break;

	case EXCEPTION_ILLEGAL_INSTRUCTION:
		rprintf(_T("an Illegal Instruction"));
		break;

	case EXCEPTION_IN_PAGE_ERROR:
		rprintf(_T("an In Page Error"));
		break;

	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		rprintf(_T("an Integer Divide By Zero"));
		break;

	case EXCEPTION_INT_OVERFLOW:
		rprintf(_T("an Integer Overflow"));
		break;

	case EXCEPTION_INVALID_DISPOSITION:
		rprintf(_T("an Invalid Disposition"));
		break;

	case EXCEPTION_INVALID_HANDLE:
		rprintf(_T("an Invalid Handle"));
		break;

	case EXCEPTION_NONCONTINUABLE_EXCEPTION:
		rprintf(_T("a Noncontinuable Exception"));
		break;

	case EXCEPTION_PRIV_INSTRUCTION:
		rprintf(_T("a Privileged Instruction"));
		break;

	case EXCEPTION_SINGLE_STEP:
		rprintf(_T("a Single Step"));
		break;

	case EXCEPTION_STACK_OVERFLOW:
		rprintf(_T("a Stack Overflow"));
		break;

	case DBG_CONTROL_C:
		rprintf(_T("a Control+C"));
		break;

	case DBG_CONTROL_BREAK:
		rprintf(_T("a Control+Break"));
		break;

	case DBG_TERMINATE_THREAD:
		rprintf(_T("a Terminate Thread"));
		break;

	case DBG_TERMINATE_PROCESS:
		rprintf(_T("a Terminate Process"));
		break;

	case RPC_S_UNKNOWN_IF:
		rprintf(_T("an Unknown Interface"));
		break;

	case RPC_S_SERVER_UNAVAILABLE:
		rprintf(_T("a Server Unavailable"));
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

		rprintf(_T("an Unknown [0x%lX] Exception"), pExceptionRecord->ExceptionCode);
		break;
	}

	// Now print information about where the fault occurred
	rprintf(_T(" at location %08x"), (DWORD) pExceptionRecord->ExceptionAddress);
	if ((hModule = (HMODULE) GetModuleBase((DWORD) pExceptionRecord->ExceptionAddress)) && GetModuleFileName(hModule, szModule, sizeof(szModule)))
	{
		rprintf(_T(" in module %s"), szModule);
	}

	// If the exception was an access violation, print out some additional information, to the error log and the debugger.
	if (pExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && pExceptionRecord->NumberParameters >= 2)
	{
		rprintf(_T(" %s location %08x"), pExceptionRecord->ExceptionInformation[0] ? "Writing to" : "Reading from", pExceptionRecord->ExceptionInformation[1]);
	}

	rprintf(_T(".\r\n\r\n"));

	dbgDumpLog(hReportFile);

	pContext = pExceptionInfo->ContextRecord;

#ifdef _M_IX86	// Intel Only!

	// Show the registers
	rprintf(_T("Registers:\r\n"));
	if (pContext->ContextFlags & CONTEXT_INTEGER)
		rprintf(
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
			    _T("             efl=%08lx"),
			    pContext->EFlags
			);
	}
	else if (pContext->ContextFlags & CONTEXT_CONTROL)
		rprintf(
		    _T("                                                                       efl=%08lx"),
		    pContext->EFlags
		);
	rprintf(_T("\r\n\r\n"));

#endif
	// FIXME: We *never* return from the below call!
	StackBackTrace(GetCurrentProcess(), GetCurrentThread(), pContext);

	rprintf(_T("\r\n\r\n"));
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

		hReportFile = CreateFileW(
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

#ifdef HAVE_BFD
		bfd_set_error_handler((bfd_error_handler_type) rprintf);
#endif /* HAVE_BFD */

		if (hReportFile)
		{
			wchar_t szBuffer[4196];
			int err;

			SetFilePointer(hReportFile, 0, 0, FILE_END);

			// FIXME: We don't return from the below function call
			GenerateExceptionReport(pExceptionInfo);
			CloseHandle(hReportFile);

			wsprintfW(szBuffer, L"Warzone has crashed.\r\nSee %s for more details\r\n", szLogFileName);
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

void ExchndlSetup(const char *packageVersion)
{
# if defined(WZ_CC_MINGW)
	wchar_t miniDumpPath[PATH_MAX] = {'\0'};
	DWORD dwRetVal = 0;
#ifdef HAVE_BFD
	bfd_init();
#endif /* HAVE_BFD */

	// Install the unhandled exception filter function
	prevExceptionFilter = SetUnhandledExceptionFilter(TopLevelExceptionFilter);

	// Retrieve the current version
	formattedVersionString = strdup(packageVersion);
#ifndef WZ_PORTABLE
	// Because of UAC on vista / win7 we use this to write our dumps to (unless we override it via OverrideRPTDirectory())
	// NOTE: CSIDL_PERSONAL =  C:\Users\user name\Documents
	if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, miniDumpPath)))
	{
		PathAppendW(miniDumpPath, WSTRING(WZ_WRITEDIR));
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
#else
	// we use where they installed it on, since this is a removeable drive (most likely), we will use where the program is located in.
	if ((dwRetVal = GetCurrentDirectoryW(MAX_PATH, miniDumpPath)))
	{
		if (dwRetVal > MAX_PATH)
		{
			MessageBox((HWND)MB_ICONEXCLAMATION, "Buffer exceeds maximum path to create directory.  Exiting.", _T("Error"), MB_OK);
			exit(1);
		}
		PathAppendW(miniDumpPath, WSTRING(WZ_WRITEDIR));
		PathAppendW(miniDumpPath, L"\\logs");
	}
#endif
	else
	{
		// should never fail, but if it does, we fall back to this
		dwRetVal = GetTempPathW(PATH_MAX, miniDumpPath);
		if (dwRetVal > MAX_PATH || (dwRetVal == 0))
		{
			MessageBox((HWND)MB_ICONEXCLAMATION, "Could not created a temporary directory!\nProgram will now exit." , _T("Error!"), MB_OK);
			exit(1);
		}
	}

	wcscat(szLogFileName, L"Warzone2100.RPT");
	wcscat(miniDumpPath, L"\\");
	wcscat(miniDumpPath, szLogFileName);
	wcscpy(szLogFileName, miniDumpPath);

	atexit(ExchndlShutdown);
#endif
}
void ResetRPTDirectory(wchar_t *newPath)
{
	wcscpy(szLogFileName, newPath);
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
