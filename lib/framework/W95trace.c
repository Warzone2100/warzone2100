
/*
    Implementation of Win95 tracing facility to mimic that of NT
*/
#pragma warning (disable : 4201 4214 4115 4514)
#include <Windows.h>
#pragma warning (default : 4201 4214 4115)
#include <stdio.h>
#include <stdarg.h>
#include <process.h>

/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE

#include "w95trace.h"

void OutputDebugStringW95( LPCTSTR lpOutputString, ...)
{

    HANDLE heventDBWIN;  /* DBWIN32 synchronization object */
    HANDLE heventData;   /* data passing synch object */
    HANDLE hSharedFile;  /* memory mapped file shared data */
    LPSTR lpszSharedMem;
    char achBuffer[500];

    /* create the output buffer */
    va_list args;
    va_start(args, lpOutputString);
    vsprintf(achBuffer, lpOutputString, args);
    va_end(args);

    /* 
        Do a regular OutputDebugString so that the output is 
        still seen in the debugger window if it exists.

        This ifdef is necessary to avoid infinite recursion 
        from the inclusion of W95TRACE.H
    */
#ifdef _UNICODE
    OutputDebugStringW(achBuffer);
#else
    OutputDebugStringA(achBuffer);
#endif

    /* bail if it's not Win95 */
    {
        OSVERSIONINFO VerInfo;
        VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&VerInfo);
        if ( VerInfo.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS )
            return;
    }

    /* make sure DBWIN is open and waiting */
    heventDBWIN = OpenEvent(EVENT_MODIFY_STATE, FALSE, "DBWIN_BUFFER_READY");
    if ( !heventDBWIN )
    {
        //MessageBox(NULL, "DBWIN_BUFFER_READY nonexistent", NULL, MB_OK);
        return;            
    }

    /* get a handle to the data synch object */
    heventData = OpenEvent(EVENT_MODIFY_STATE, FALSE, "DBWIN_DATA_READY");
    if ( !heventData )
    {
        // MessageBox(NULL, "DBWIN_DATA_READY nonexistent", NULL, MB_OK);
        CloseHandle(heventDBWIN);
        return;            
    }
    
    hSharedFile = CreateFileMapping((HANDLE)-1, NULL, PAGE_READWRITE, 0, 4096, "DBWIN_BUFFER");
    if (!hSharedFile) 
    {
        //MessageBox(NULL, "DebugTrace: Unable to create file mapping object DBWIN_BUFFER", "Error", MB_OK);
        CloseHandle(heventDBWIN);
        CloseHandle(heventData);
        return;
    }

    lpszSharedMem = (LPSTR)MapViewOfFile(hSharedFile, FILE_MAP_WRITE, 0, 0, 512);
    if (!lpszSharedMem) 
    {
        //MessageBox(NULL, "DebugTrace: Unable to map shared memory", "Error", MB_OK);
        CloseHandle(heventDBWIN);
        CloseHandle(heventData);
        return;
    }

    /* wait for buffer event */
    WaitForSingleObject(heventDBWIN, INFINITE);

    /* write it to the shared memory */
    *((LPDWORD)lpszSharedMem) = _getpid();
    wsprintf(lpszSharedMem + sizeof(DWORD), "%s", achBuffer);

    /* signal data ready event */
    SetEvent(heventData);

    /* clean up handles */
    CloseHandle(hSharedFile);
    CloseHandle(heventData);
    CloseHandle(heventDBWIN);

    return;
}

