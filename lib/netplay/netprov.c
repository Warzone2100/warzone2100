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
// Prototypes.
BOOL NETsetupGeneric	(LPDPCOMPOUNDADDRESSELEMENT LPelements, DWORD count, LPVOID * LPaddr);
BOOL NETsetupIPX		(LPVOID *addr);
BOOL NETsetupTCPIP		(LPVOID *addr, char *machine);
BOOL NETsetupSerial	(LPVOID *addr,DWORD ComPort,DWORD BaudRate, DWORD StopBits, DWORD Parity, DWORD FlowControl);

BOOL NETsetupModem		(LPVOID *addr, char *Phoneno, UDWORD modemToUse );
BOOL NETchooseModem		(UBYTE mod);


// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// Setup functions to avoid DPLAY dialog boxes when connecting.

// Called by the other Setup functions.
BOOL NETsetupGeneric(LPDPCOMPOUNDADDRESSELEMENT LPelements, DWORD count, LPVOID * LPaddr)				
{
	HRESULT hr;
	DWORD	addrsize;
	LPVOID  addr;

	// see how much room is needed to store this address
	hr = IDirectPlayLobby_CreateCompoundAddress(glpDPL3,	LPelements,	count,NULL,	&addrsize); 				

	if (hr != DPERR_BUFFERTOOSMALL)
	{
		DBERROR(("NETPLAY: Buffer failure"));
		return (FALSE);
	}

	addr = MALLOC(addrsize);	// allocate space
	if (addr == NULL)
	{
		hr = DPERR_NOMEMORY;
		DBERROR(("NETPLAY: Can't alloc memory for address"));
		return FALSE;
	}

	// create the address
	hr = IDirectPlayLobby_CreateCompoundAddress(glpDPL3,LPelements,	count, addr,	&addrsize);
									
	if (hr != DP_OK)
	{
		DBERROR(("NETPLAY: Failed to create a proper DP address"));
		return (FALSE);
	}


	// return the address info
	*LPaddr = addr;
	
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////
// IPX stuff
// IPX doesn't require anything at all!

BOOL NETsetupIPX(LPVOID *addr)
{
	DPCOMPOUNDADDRESSELEMENT elements[2];
	GUID	type = DPSPGUID_IPX;
	UDWORD	count=0;

	// all DPADDRESS's must have a service provider chunk
	elements[count].guidDataType = DPAID_ServiceProvider;
	elements[count].dwDataSize = sizeof(GUID);
	elements[count].lpData = &type;
	count++;

	return NETsetupGeneric( ((DPCOMPOUNDADDRESSELEMENT *)&elements),0, addr);
	
}

// ////////////////////////////////////////////////////////////////////////
// Internet Stuff
// pass an ip address to this in order to avoid the popup dialog on the client.
BOOL NETsetupTCPIP(LPVOID *addr, char *machine)
{
	DPCOMPOUNDADDRESSELEMENT elements[2];
	DWORD	count=0;
	GUID	type = DPSPGUID_TCPIP;

	// all DPADDRESS's must have a service provider chunk
	elements[count].guidDataType = DPAID_ServiceProvider;
	elements[count].dwDataSize = sizeof(GUID);
	elements[count].lpData = &type;
	count++;

	// and the destination address is.....
	elements[count].guidDataType = DPAID_INet;
	elements[count].dwDataSize = lstrlen(machine) + 1;
	elements[count].lpData = machine;
	count++;

	return NETsetupGeneric( ((DPCOMPOUNDADDRESSELEMENT *)&elements),count, addr);
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
	DPCOMPOUNDADDRESSELEMENT elements[2];
	DWORD					count=0;
	GUID					type = DPSPGUID_SERIAL;
	DPCOMPORTADDRESS		comp;

	// all DPADDRESS's must have a service provider chunk
	elements[count].guidDataType = DPAID_ServiceProvider;
	elements[count].dwDataSize = sizeof(GUID);
	elements[count].lpData = &type;
	count++;

	comp.dwComPort = ComPort;
    comp.dwBaudRate = BaudRate;
    comp.dwStopBits = StopBits;
    comp.dwParity = Parity;
    comp.dwFlowControl = FlowControl;

	// comport settings.
	elements[count].guidDataType = DPAID_ComPort;
	elements[count].dwDataSize = sizeof(DPCOMPORTADDRESS);	// +1 for terminal
	elements[count].lpData = &comp;
	count++;

	return NETsetupGeneric(((DPCOMPOUNDADDRESSELEMENT *)&elements),count,addr);
}

// ////////////////////////////////////////////////////////////////////////
// Modem stuff. Call with NULL for the server. 
// Call with the phone number for the client.

static UDWORD elementModemCount;
static LPVOID elementModemData;
static DWORD  elementModemSize;

static BOOL FAR PASCAL enumModemAddress(REFGUID guidDataType,  DWORD dwDataSize,  LPCVOID lpData,  LPVOID lpContext  )
{
 	if(IsEqualGUID(guidDataType,&DPAID_Modem))
	{
		if(elementModemCount ==0)
		{
			DBPRINTF(("\nNETPLAY: using modem %s\n",lpData));
			elementModemData = lpData;
			elementModemSize = dwDataSize;
			return FALSE;
		}
		DBPRINTF(("\nNETPLAY: not using modem %s\n",lpData));
		elementModemCount--;
	}
	return TRUE;
}

BOOL NETsetupModem(LPVOID *addr, char *Phoneno, UDWORD modemToUse )
{
	DPCOMPOUNDADDRESSELEMENT elements[3];
	DWORD	count=0;
	GUID	type = DPSPGUID_MODEM;
	HRESULT	hr;
	BOOL	result = FALSE;

	LPDIRECTPLAY		lpDPlay1 = NULL;
	LPDIRECTPLAY4A		lpDPlay4A = NULL;
	LPVOID				lpAddress = NULL;
	DWORD				dwAddressSize = 0;
	GUID				guidServiceProvider = DPSPGUID_MODEM;

	// default stuff
	// all DPADDRESS's must have a service provider chunk
	elements[count].guidDataType = DPAID_ServiceProvider;
	elements[count].dwDataSize = sizeof(GUID);
	elements[count].lpData = &type;
	count++;

	// Modem chunk . This is the worst bit of code i ever saw. borrowed from msdevkit.

	// Use the obsolete DirectPlayCreate() as quick way to load a specific
	// service provider.  Trade off using DirectPlayCreate() and
	// QueryInterface() vs using CoCreateInitialize() and building a
	// DirectPlay Address for InitializeConnection().
	hr = DirectPlayCreate(&type, &lpDPlay1, NULL);
	if FAILED(hr)
		goto FAILURE;

	// query for an ANSI DirectPlay4 interface
	hr = lpDPlay1->lpVtbl->QueryInterface(lpDPlay1, &IID_IDirectPlay4A, (LPVOID *) &lpDPlay4A);
	if FAILED(hr)
		goto FAILURE;

	// get size of player address for player zero
	hr = lpDPlay4A->lpVtbl->GetPlayerAddress(lpDPlay4A, DPID_ALLPLAYERS, NULL, &dwAddressSize);
	if (hr != DPERR_BUFFERTOOSMALL)
		goto FAILURE;

	// make room for it
	lpAddress = MALLOC(dwAddressSize);
	if (lpAddress == NULL)
	{
		hr = DPERR_NOMEMORY;
		goto FAILURE;
	}

	// get the address
	hr = lpDPlay4A->lpVtbl->GetPlayerAddress(lpDPlay4A, DPID_ALLPLAYERS, lpAddress, &dwAddressSize);
	if FAILED(hr)
		goto FAILURE;

	//pick correctmodem. using enumplayeraddress
	elementModemCount=modemToUse;
	elementModemData =NULL;
	elementModemSize =0 ;

	// get modem strings from address
	hr = IDirectPlayLobby_EnumAddress(glpDPL3, enumModemAddress, lpAddress, dwAddressSize, NULL);
	if FAILED(hr)
		goto FAILURE;

	// set the modem.
	elements[count].guidDataType = DPAID_Modem;
	elements[count].dwDataSize = elementModemSize;
	elements[count].lpData = elementModemData;
	count++;

	// Phone Number
	elements[count].guidDataType = DPAID_Phone;
	elements[count].dwDataSize = lstrlen(Phoneno) + 1;				// +1 for terminal
	elements[count].lpData = Phoneno;
	count++;

	result = NETsetupGeneric(((DPCOMPOUNDADDRESSELEMENT *)&elements),count,addr);

FAILURE:
	if (lpDPlay1)
		lpDPlay1->lpVtbl->Release(lpDPlay1);
	if (lpDPlay4A)
		lpDPlay4A->lpVtbl->Release(lpDPlay4A);
	if (lpAddress)
		FREE(lpAddress);

	return result;
} 


