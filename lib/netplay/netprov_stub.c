/* 
 * NetProv.c 
 * Builds info for direct play providers..
 * 
 * Alex Lee Sep 97
 * Allows you to override all the MS dialogs.
 */

// ////////////////////////////////////////////////////////////////////////
// Includes
#include "frame.h"
#include "netplay.h"

// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// Setup functions to avoid DPLAY dialog boxes when connecting.

// ////////////////////////////////////////////////////////////////////////
// IPX stuff
// IPX doesn't require anything at all!

BOOL NETsetupIPX(LPVOID *addr)
{
	return FALSE;
	
}

// ////////////////////////////////////////////////////////////////////////
// Internet Stuff
// pass an ip address to this in order to avoid the popup dialog on the client.
BOOL NETsetupTCPIP(LPVOID *addr, char *machine)
{
	return FALSE;
}

// ////////////////////////////////////////////////////////////////////////
// Direct Cable Stuff
// (all defined in Winbase.h)
// dwComPort can be
//  1, 2, 3, or 4. 
//
// BaudRate 
//  CBR_110		CBR_300		CBR_600		CBR_1200	CBR_2400	CBR_4800	CBR_9600	CBR_14400
//  CBR_19200	CBR_38400	CBR_56000	CBR_57600	CBR_115200	CBR_128000	CBR_256000 
//
// StopBits can be
//  ONESTOPBIT, ONE5STOPBITS, or TWOSTOPBITS. 
//
// Parity can be
//  NOPARITY, ODDPARITY, EVENPARITY, or MARKPARITY. 
//
// FlowControl  can be:
//  DPCPA_DTRFLOW 		Indicates hardware flow control with DTR. 
//  DPCPA_NOFLOW 		Indicates no flow control. 
//  DPCPA_RTSDTRFLOW 	Indicates hardware flow control with RTS and DTR. 
//  DPCPA_RTSFLOW 		Indicates hardware flow control with RTS. 
//  DPCPA_XONXOFFFLOW 	Indicates software flow control (xon/xoff). 
BOOL NETsetupSerial(LPVOID *addr,DWORD ComPort,DWORD BaudRate, DWORD StopBits, DWORD Parity, DWORD FlowControl)
{
	return FALSE;
}

BOOL NETsetupModem(LPVOID *addr, char *Phoneno, UDWORD modemToUse )
{
	return FALSE;
} 


