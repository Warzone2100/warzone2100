/* 
 * netusers.c
 * functions regarding specific players
 */

#include "frame.h"
#include "netplay.h"

// call to disable/enable ALL comms. Absolute arse of a thing, be very careful!
BOOL NETuseNetwork(BOOL val)
{
	return FALSE;
}


// ////////////////////////////////////////////////////////////////////////
 BOOL NETgetLocalPlayerData(DPID dpid,VOID *pData, DWORD *pSize)
 {
	return FALSE;
 }

 // ////////////////////////////////////////////////////////////////////////
 BOOL NETgetGlobalPlayerData(DPID dpid,VOID *pData, DWORD *pSize)
 {
	return FALSE;
 }
// ////////////////////////////////////////////////////////////////////////
 BOOL NETsetLocalPlayerData(DPID dpid,VOID *pData, DWORD size)
 {
	return FALSE;
 }

 // ////////////////////////////////////////////////////////////////////////
 BOOL NETsetGlobalPlayerData(DPID dpid,VOID *pData, DWORD size)
 {
	return FALSE;
 }

// ////////////////////////////////////////////////////////////////////////
// count players. call with null to enumerate the game already joined.
UDWORD NETplayerInfo(LPGUID guidinstance)
{
	return 0;
}

// ////////////////////////////////////////////////////////////////////////
// rename the local player
//
// dont call this a lot, since it uses a guaranteed msg.
BOOL NETchangePlayerName(UDWORD dpid, char *newName)
{
	return FALSE;
}


// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// Functions for spectators.

BOOL NETspectate(GUID guidSessionInstance)
{
	return FALSE;
}

// ////////////////////////////////////////////////////////////////////////
BOOL NETisSpectator(DPID dpid)
{
	return FALSE;
}
