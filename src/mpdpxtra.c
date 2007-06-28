/*----------------------------------------------------------------------------

	MPLAYER DIRECTPLAY EXTRAS

	This collection of modular components comprise extra extensions for 
	DirectPlay games that that run on the Mplayer network. 
  
	Current Components
	------------------
	Lobby Status Reporting	-	Allows games to report staus information
								to the game's Mplayer lobby. This inform-
								ation includes items such as changing the
								name of the game room, reporting game-in
								progress details, etc.

	Score Reporting			-	Allows games to report game statistics
								and scores to be saved in a database which
								can then be queried to generate rankings,
								ratings, and high score lists.
								
	Written By Andy Riedel
	Coptyright 1996-97. Mpath Interactive, Inc.

----------------------------------------------------------------------------*/

#include "frame.h"
#include "mpdpxtra.h"

/******** DEFINITIONS ********/

#define MPDPXTRA_DLL_NAME	"DPMPLAY.DLL"

/******** DYNAMIC FUNCTION PROTOTYPES ********/
typedef MPDPXTRAERR mpcdecl	(*MPDPXTRA_DPIDToMPPLAYERID_TYPE)			(DPID dpid,MPPLAYERID *mpPlayerID);

typedef MPDPXTRAERR	mpcdecl (*MPDPXTRA_AddScoreResult_TYPE)				(DPID srcId,DPID dstId,u_long key,long value);
typedef MPDPXTRAERR	mpcdecl (*MPDPXTRA_AddScoreResultEx_TYPE)			(MPPLAYERID srcId,MPPLAYERID dstId,u_long key,long value);
typedef MPDPXTRAERR	mpcdecl (*MPDPXTRA_FetchServerGameData_TYPE)		(UINT key,void *pData,UINT *pDataLen);
typedef MPDPXTRAERR	mpcdecl (*MPDPXTRA_PostLobbyData_TYPE)				(UINT objectID,void *pObjectData,UINT objectDataLen);
typedef MPDPXTRAERR	mpcdecl (*MPDPXTRA_PostServerGameData_TYPE)			(UINT key,void *pData,UINT dataLen);
typedef MPDPXTRAERR	mpcdecl (*MPDPXTRA_SaveScoreResults_TYPE)			(void);
typedef MPDPXTRAERR	mpcdecl (*MPDPXTRA_SubscribeServerGameData_TYPE)	(UINT key,SERVER_GAME_DATA_CBF *cbf,void *context);
typedef MPDPXTRAERR	mpcdecl (*MPDPXTRA_UnsubscribeServerGameData_TYPE)	(UINT key,SERVER_GAME_DATA_CBF *cbf,void *context);

/******** DYNAMIC FUNCTION POINTERS ********/
MPDPXTRA_DPIDToMPPLAYERID_TYPE			MPDPXTRA_DPIDToMPPLAYERID_InDLL				= NULL;

MPDPXTRA_AddScoreResult_TYPE			MPDPXTRA_AddScoreResult_InDLL				= NULL;
MPDPXTRA_AddScoreResultEx_TYPE			MPDPXTRA_AddScoreResultEx_InDLL				= NULL;
MPDPXTRA_FetchServerGameData_TYPE		MPDPXTRA_FetchServerGameData_InDLL			= NULL;
MPDPXTRA_PostLobbyData_TYPE				MPDPXTRA_PostLobbyData_InDLL				= NULL;
MPDPXTRA_PostServerGameData_TYPE		MPDPXTRA_PostServerGameData_InDLL			= NULL;
MPDPXTRA_SaveScoreResults_TYPE			MPDPXTRA_SaveScoreResults_InDLL				= NULL;
MPDPXTRA_SubscribeServerGameData_TYPE	MPDPXTRA_SubscribeServerGameData_InDLL		= NULL;
MPDPXTRA_UnsubscribeServerGameData_TYPE	MPDPXTRA_UnsubscribeServerGameData_InDLL	= NULL;

/******** GLOBAL VARIABLES ********/

HINSTANCE	gMPDPXTRALibHndl			= NULL;
int			gMPDPXTRALibUseCount		= 0;
BOOL		gMPDPXTRALibDisplayErrors	= FALSE;

/**************************************************************************************

							  MPDPXTRA UTILITY FUNCTIONS

**************************************************************************************/


FARPROC		MPDPXTRA_GetProcAddress(HMODULE hModule,LPCSTR lpProcName)
{
	FARPROC retVal = GetProcAddress(hModule,lpProcName);

	if (retVal == NULL && gMPDPXTRALibDisplayErrors)
	{
		char msg[255];

		wsprintf(msg,"Unable to load function %s from %s",lpProcName,MPDPXTRA_DLL_NAME);
		MessageBox(0,msg,"Warning",MB_OK);
	}

    return retVal;
}


HINSTANCE	MPDPXTRA_LoadLibrary()
{
	HINSTANCE	hInst;

	// Dynamically link to Mpdpxtra.dll and initialize all function pointers.
	//.......................................................................

    if (NULL != (hInst = LoadLibrary(MPDPXTRA_DLL_NAME)))
	{
		MPDPXTRA_DPIDToMPPLAYERID_InDLL			= (MPDPXTRA_DPIDToMPPLAYERID_TYPE)			MPDPXTRA_GetProcAddress(hInst,"MPDPXTRA_DPIDToMPPLAYERID");
		MPDPXTRA_AddScoreResult_InDLL			= (MPDPXTRA_AddScoreResult_TYPE)			MPDPXTRA_GetProcAddress(hInst,"MPDPXTRA_AddScoreResult");
		MPDPXTRA_AddScoreResultEx_InDLL			= (MPDPXTRA_AddScoreResultEx_TYPE)			MPDPXTRA_GetProcAddress(hInst,"MPDPXTRA_AddScoreResultEx");
		MPDPXTRA_FetchServerGameData_InDLL		= (MPDPXTRA_FetchServerGameData_TYPE)		MPDPXTRA_GetProcAddress(hInst,"MPDPXTRA_FetchServerGameData");
		MPDPXTRA_PostLobbyData_InDLL			= (MPDPXTRA_PostLobbyData_TYPE)				MPDPXTRA_GetProcAddress(hInst,"MPDPXTRA_PostLobbyData");
		MPDPXTRA_PostServerGameData_InDLL		= (MPDPXTRA_PostServerGameData_TYPE)		MPDPXTRA_GetProcAddress(hInst,"MPDPXTRA_PostServerGameData");
		MPDPXTRA_SaveScoreResults_InDLL			= (MPDPXTRA_SaveScoreResults_TYPE)			MPDPXTRA_GetProcAddress(hInst,"MPDPXTRA_SaveScoreResults");
		MPDPXTRA_SubscribeServerGameData_InDLL	= (MPDPXTRA_SubscribeServerGameData_TYPE)	MPDPXTRA_GetProcAddress(hInst,"MPDPXTRA_SubscribeServerGameData");
		MPDPXTRA_UnsubscribeServerGameData_InDLL= (MPDPXTRA_UnsubscribeServerGameData_TYPE)	MPDPXTRA_GetProcAddress(hInst,"MPDPXTRA_UnsubscribeServerGameData");
	}
	else if (gMPDPXTRALibDisplayErrors)
	{
		char msg[255];

		wsprintf(msg,"Unable to load %s",MPDPXTRA_DLL_NAME);
		MessageBox(0,msg, "Warning", MB_OK);
	}

    return (hInst);
}


void		MPDPXTRA_CheckDisplayErrors()
{
	HKEY		hKey;
	BYTE		value[256];
	DWORD		valueSize = sizeof(value);

	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,REG_KEY_MPLAYER"\\MAIN",0,KEY_QUERY_VALUE,&hKey))
	{
		if (ERROR_SUCCESS == RegQueryValueEx(hKey,"DEBUGDYNDLL",NULL,NULL,value,&valueSize))
		{
			if (atoi(value) >= 1)
				gMPDPXTRALibDisplayErrors = TRUE;
			else
				gMPDPXTRALibDisplayErrors = FALSE;
		}
		
		RegCloseKey(hKey);
	}
}



/**************************************************************************************

							 MPDPXTRA DYNAMIC FUNCTION STUBS

**************************************************************************************/

/*-----------------------------------------------------------------------------------

	MPDPXTRA_Init

	Initializes the Mplayer DirectPlay Extras Library. This function must be called
	before using any of the other functions available in the library. If called,
	multiple time within a single process, the library will only be loaded once and
	a reference count will be incremented for each call after the first. Each call
	to MPDPXTRA_Init must have a matching call to MPDPXTRA_Destroy which decrements
	the reference count. The library will be unloaded when the refence count reaches
	zero.

	Inputs:

		None.

	Outputs:

		None.

	Return Codes:

	MPDPXTRAERR_OK
	MPDPXTRAERR_UNABLE_TO_LOAD_MPDPXTRA_LIBRARY

-----------------------------------------------------------------------------------*/

MPDPXTRAERR		MPDPXTRA_Init()
{	
	MPDPXTRAERR		retVal = MPDPXTRAERR_OK;

	if (gMPDPXTRALibHndl == NULL)
	{
		MPDPXTRA_CheckDisplayErrors();

		if (NULL != (gMPDPXTRALibHndl = MPDPXTRA_LoadLibrary()))
			gMPDPXTRALibUseCount++;
		else
			retVal = MPDPXTRAERR_UNABLE_TO_LOAD_MPDPXTRA_LIBRARY;
	}
	else
		gMPDPXTRALibUseCount++;

	return (retVal);
}


/*-----------------------------------------------------------------------------------

	MPDPXTRA_Destroy

	Decrements the reference count for the Mplayer DirectPlay Extras Library and
	releases the library and all associate resources if it reaches zero.

	Inputs:

		None.

	Outputs:

		None.

	Return Codes:

	MPDPXTRAERR_OK
	MPDPXTRAERR_MPDPXTRA_LIBRARY_NOT_LOADED
	MPDPXTRAERR_MPDPXTRA_LIBRARY_REFERENCE_COUNT_NOT_ZERO

-----------------------------------------------------------------------------------*/

MPDPXTRAERR		MPDPXTRA_Destroy()
{
	MPDPXTRAERR		retVal = MPDPXTRAERR_OK;

	if (gMPDPXTRALibHndl != NULL)
	{
		if (gMPDPXTRALibUseCount == 1)
		{
			FreeLibrary(gMPDPXTRALibHndl);
			gMPDPXTRALibHndl = NULL;
		}
		else
			retVal = MPDPXTRAERR_MPDPXTRA_LIBRARY_REFERENCE_COUNT_NOT_ZERO;

		if (gMPDPXTRALibUseCount > 0)
			gMPDPXTRALibUseCount--;
	}
	else
		retVal = MPDPXTRAERR_MPDPXTRA_LIBRARY_NOT_LOADED;

	return (retVal);
}


/*-----------------------------------------------------------------------------------
	
  MPDPXTRA_DPIDToMPPLAYERID

	The Mplayer DirectPlay Extras Library may need to translate DPID's to
	MPPLAYERID's for use with the score reporting API. This function
	performs a translation by requesting the MPPLAYERID from the local
	player data for the specified DPID.
	
	Inputs:

		dpid		-	The DPID of the player in question.
		
		mpPlayerID	-	A pointer to the variable to recieve the MPPLAYERID.

	Outputs:

		None.

	Return Codes:

		MPDPXTRAERR_OK
		MPDPXTRAERR_DLL_NOT_LOADED
		MPDPXTRAERR_UNABLE_TO_LOCATE_FUNCTION_IN_MPDPXTRA_DLL
		MPDPXTRAERR_NOT_CONNECTED_TO_MPLAYER_NETWORK
		MPDPXTRAERR_SERIOUS_MPLAYER_ERROR

-----------------------------------------------------------------------------------*/

MPDPXTRAERR		MPDPXTRA_DPIDToMPPLAYERID(DPID dpid,MPPLAYERID *mpPlayerID)
{
	MPDPXTRAERR		retVal = MPDPXTRAERR_OK;

	if (gMPDPXTRALibHndl != NULL)
	{
		if (MPDPXTRA_DPIDToMPPLAYERID_InDLL != NULL)
			retVal = MPDPXTRA_DPIDToMPPLAYERID_InDLL(dpid,mpPlayerID);
		else
			retVal = MPDPXTRAERR_UNABLE_TO_LOCATE_FUNCTION_IN_MPDPXTRA_LIBRARY;
	}
	else
		retVal = MPDPXTRAERR_MPDPXTRA_LIBRARY_NOT_LOADED;

	return (retVal);
}

/*-----------------------------------------------------------------------------------
	
	MPDPXTRA_PostLobbyData
 
	Send an opaque chunk of data to clients in the lobby.  Their Game Extension DLLs
	will then interpret the object and display relevant information about the running
	game instance.

	The general purpose of this API is to allow a running game instance to report
	significant events to the lobby, for the general edification and entertainment
	of people outside the game.

	The actual objects sent should be small, and infrequent.

	Valid LobbyData objects ID's are:


	Inputs:

		objectID	-	A LobbyData object ID. Valid LobbyData objects ID's are:

						MP_LOBBYDATA_COMMENT			0	// Replace room Comment
						MP_LOBBYDATA_SHORT_STATUS		1	// Opaque data for GED (simple score)
						MP_LOBBYDATA_LONG_STATUS		2	// Opaque data for GED (game state)
		
		pObjectData	-	A pointer to the object data. Object data should be comprised
						of the following information based on the object ID:
		
						MP_LOBBYDATA_COMMENT		- A pointer to the string to replace the 
													  lobby's room comment with.
						MP_LOBBYDATA_SHORT_STATUS	- Opaque data for the game's extension DLL
													  representing small and simple updates such as
													  simple scores.
						MP_LOBBYDATA_LONG_STATUS	- Opaque data for the game's extension DLL
													  representing larger updates such as a game's 
													  current state

		objectDataLen - The length of the data block. For string's make sure that this value is
						one greater than the value returned by strlen so that the end of string
						character is included.

	Outputs:

		None.

	Return Codes:

		MPDPXTRAERR_OK
		MPDPXTRAERR_DLL_NOT_LOADED
		MPDPXTRAERR_UNABLE_TO_LOCATE_FUNCTION_IN_MPDPXTRA_DLL
		MPDPXTRAERR_NOT_CONNECTED_TO_MPLAYER_NETWORK
		MPDPXTRAERR_SERIOUS_MPLAYER_ERROR
		MPDPXTRAERR_BAD_LOBBY_DATA_OBJECT_ID
		MPDPXTRAERR_UNABLE_TO_SEND_LOBBY_DATA

-----------------------------------------------------------------------------------*/

MPDPXTRAERR		MPDPXTRA_PostLobbyData(UINT objectID,void *pObjectData,UINT objectDataLen)
{
	MPDPXTRAERR		retVal = MPDPXTRAERR_OK;

	if (gMPDPXTRALibHndl != NULL)
	{
		if (MPDPXTRA_PostLobbyData_InDLL != NULL)
			retVal = MPDPXTRA_PostLobbyData_InDLL(objectID,pObjectData,objectDataLen);
		else
			retVal = MPDPXTRAERR_UNABLE_TO_LOCATE_FUNCTION_IN_MPDPXTRA_LIBRARY;
	}
	else
		retVal = MPDPXTRAERR_MPDPXTRA_LIBRARY_NOT_LOADED;

	return (retVal);
}


/*-----------------------------------------------------------------------------------
	
	MPDPXTRA_AddScoreResult

	Queues a score result on the local client to be uploaded to the Mplayer network
	at a later time using MPDPXTRA_SaveScoreResults.

	Inputs:

		srcId -			Originating player's DPID
		dstId -			Target player's DPID, if any (use 0 for none)
		key -			Key value for the score result
		value -			Value of the score result

	Outputs:

		None.

	Return Codes:

		MPDPXTRAERR_OK
		MPDPXTRAERR_MPDPXTRA_DLL_NOT_LOADED
		MPDPXTRAERR_UNABLE_TO_LOCATE_FUNCTION_IN_MPDPXTRA_DLL
		MPDPXTRAERR_NOT_CONNECTED_TO_MPLAYER_NETWORK
		MPDPXTRAERR_SERIOUS_MPLAYER_ERROR
		MPDPXTRAERR_UNABLE_ALLOCATE_MEMORY_FOR_RESULT
		MPDPXTRAERR_INVALID_PLAYER_ID

-----------------------------------------------------------------------------------*/

MPDPXTRAERR		MPDPXTRA_AddScoreResult(DPID srcId,DPID dstId,u_long key,long value)
{
	MPDPXTRAERR		retVal = MPDPXTRAERR_OK;

	if (gMPDPXTRALibHndl != NULL)
	{
		if (MPDPXTRA_AddScoreResult_InDLL != NULL)
			retVal = MPDPXTRA_AddScoreResult_InDLL(srcId,dstId,key,value);
		else
			retVal = MPDPXTRAERR_UNABLE_TO_LOCATE_FUNCTION_IN_MPDPXTRA_LIBRARY;
	}
	else
		retVal = MPDPXTRAERR_MPDPXTRA_LIBRARY_NOT_LOADED;

	return (retVal);
}

/*-----------------------------------------------------------------------------------
	
	MPDPXTRA_AddScoreResultEx

	Queues a score result on the local client to be uploaded to the Mplayer network
	at a later time using MPDPXTRA_SaveScoreResults.

	Inputs:

		srcId -			Originating player's MPPLAYERID
		dstId -			Target player's MPPLAYERID, if any (use 0 for none)
		key -			Key value for the score result
		value -			Value of the score result

	Outputs:

		None.

	Return Codes:

		MPDPXTRAERR_OK
		MPDPXTRAERR_MPDPXTRA_DLL_NOT_LOADED
		MPDPXTRAERR_UNABLE_TO_LOCATE_FUNCTION_IN_MPDPXTRA_DLL
		MPDPXTRAERR_NOT_CONNECTED_TO_MPLAYER_NETWORK
		MPDPXTRAERR_SERIOUS_MPLAYER_ERROR
		MPDPXTRAERR_UNABLE_ALLOCATE_MEMORY_FOR_RESULT
		MPDPXTRAERR_INVALID_PLAYER_ID

-----------------------------------------------------------------------------------*/

MPDPXTRAERR		MPDPXTRA_AddScoreResultEx(MPPLAYERID srcId,MPPLAYERID dstId,unsigned long key,long value)
{
	MPDPXTRAERR		retVal = MPDPXTRAERR_OK;

	if (gMPDPXTRALibHndl != NULL)
	{
		if (MPDPXTRA_AddScoreResultEx_InDLL != NULL)
			retVal = MPDPXTRA_AddScoreResultEx_InDLL(srcId,dstId,key,value);
		else
			retVal = MPDPXTRAERR_UNABLE_TO_LOCATE_FUNCTION_IN_MPDPXTRA_LIBRARY;
	}
	else
		retVal = MPDPXTRAERR_MPDPXTRA_LIBRARY_NOT_LOADED;

	return (retVal);
}

/*-----------------------------------------------------------------------------------
	
	MPDPXTRA_SaveScoreResults

	Sends all results accumulated with MPDPXTRA_SaveScoreResults to the Mplayer
	network.

	Inputs:

		None.

	Outputs:

		None.

	Return Codes:

		MPDPXTRAERR_OK
		MPDPXTRAERR_MPDPXTRA_DLL_NOT_LOADED
		MPDPXTRAERR_UNABLE_TO_LOCATE_FUNCTION_IN_MPDPXTRA_DLL
		MPDPXTRAERR_NOT_CONNECTED_TO_MPLAYER_NETWORK
		MPDPXTRAERR_SERIOUS_MPLAYER_ERROR
		MPDPXTRAERR_UNABLE_ALLOCATE_MEMORY_FOR_RESULT
		MPDPXTRAERR_UNABLE_TO_SEND_SCORE_RESULTS

-----------------------------------------------------------------------------------*/

MPDPXTRAERR		MPDPXTRA_SaveScoreResults(void)
{
	MPDPXTRAERR		retVal = MPDPXTRAERR_OK;

	if (gMPDPXTRALibHndl != NULL)
	{
		if (MPDPXTRA_SaveScoreResults_InDLL != NULL)
			retVal = MPDPXTRA_SaveScoreResults_InDLL();
		else
			retVal = MPDPXTRAERR_UNABLE_TO_LOCATE_FUNCTION_IN_MPDPXTRA_LIBRARY;
	}
	else
		retVal = MPDPXTRAERR_MPDPXTRA_LIBRARY_NOT_LOADED;

	return (retVal);
}


/******************************************************************************

					       MPDPXTRA SERVER GAME DATA

******************************************************************************/


/*-----------------------------------------------------------------------------------
	
	MPDPXTRA_PostServerGameData

	Posts an opaque data block up to 256K bytes in size to the game server
	so that it can be shared with others attached to the same game session.
	The data can be retrieved from the server either synchronously via the blocking
	function MPDPXTRA_FetchSharedGameData or asynchronously using the non-blocking
	function MPDPXTRA_SubscribeSharedGameData.

	Note that this post function is non-blocking.

	Inputs:

		key -			Key value for the opaque data block between 0 and 0x7FFFFFFF.
						This key is game specific so developers are free to choose any
						key they wish.
		pData -			A pointer to opaque data block to be saved on the game server.
		dataLen -		The size of the opaque data block.

	Outputs:

		None.

	Return Codes:

		MPDPXTRAERR_OK
		MPDPXTRAERR_MPDPXTRA_DLL_NOT_LOADED
		MPDPXTRAERR_UNABLE_TO_LOCATE_FUNCTION_IN_MPDPXTRA_DLL
		MPDPXTRAERR_NOT_CONNECTED_TO_MPLAYER_NETWORK
		MPDPXTRAERR_SERIOUS_MPLAYER_ERROR
		MPDPXTRAERR_SERVER_GAME_DATA_INVALID_KEY
		MPDPXTRAERR_SERVER_GAME_DATA_INVALID_POINTER
		MPDPXTRAERR_SERVER_GAME_DATA_SIZE_TOO_BIG
		MPDPXTRAERR_UNABLE_TO_ALLOCATE_MEMORY_FOR_SERVER_GAME_DATA
		MPDPXTRAERR_UNABLE_TO_SEND_SERVER_GAME_DATA

-----------------------------------------------------------------------------------*/

MPDPXTRAERR		MPDPXTRA_PostServerGameData(UINT key,void *pData,UINT dataLen)
{
	MPDPXTRAERR		retVal = MPDPXTRAERR_OK;

	if (gMPDPXTRALibHndl != NULL)
	{
		if (MPDPXTRA_PostServerGameData_InDLL != NULL)
			retVal = MPDPXTRA_PostServerGameData_InDLL(key,pData,dataLen);
		else
			retVal = MPDPXTRAERR_UNABLE_TO_LOCATE_FUNCTION_IN_MPDPXTRA_LIBRARY;
	}
	else
		retVal = MPDPXTRAERR_MPDPXTRA_LIBRARY_NOT_LOADED;

	return (retVal);
}


/*-----------------------------------------------------------------------------------
	
	MPDPXTRA_FetchServerGameData

	Fetches an opaque data block up to 256K bytes in size from the game server
	in a synchronous, blocking manner (i.e. the function won't return until
	the complete block has been received or an error has occurred).

	This function should really only be used for VERY small blocks (< 4K or so)
	of data where the user won't be aware that data transfer is occurring. The
	reason for this is that the game's user interface might block waiting for this
	call. For large blocks, it is recommended that developers use the
	MPDPXTRA_SubscribeServerGameData function.

	In order to get the size of the server game data block, 0 can be specified for
	for the dataLen. The function will return an error of
	MPDPXTRAERR_SERVER_GAME_DATA_SIZE_TOO_SMALL but the dataLen will be set to the
	correct size. A buffer can then be allocated, pData can be sent to point to it
	and this function can be called again.

	Inputs:

		key -			Key value for the opaque data block between 0 and 0x7FFFFFFF.
						This key is game specific so developers are free to choose any
						key they wish.
		pData -			A pointer to a buffer to be filled with the opaque server game
						data block.
		pDataLen -		A pointer to the size of the opaque data block which will be
						filled in.

	Outputs:

		None.

	Return Codes:

		MPDPXTRAERR_OK
		MPDPXTRAERR_MPDPXTRA_DLL_NOT_LOADED
		MPDPXTRAERR_UNABLE_TO_LOCATE_FUNCTION_IN_MPDPXTRA_DLL
		MPDPXTRAERR_NOT_CONNECTED_TO_MPLAYER_NETWORK
		MPDPXTRAERR_SERIOUS_MPLAYER_ERROR
		MPDPXTRAERR_SERVER_GAME_DATA_INVALID_KEY
		MPDPXTRAERR_SERVER_GAME_DATA_SIZE_TOO_SMALL
		MPDPXTRAERR_UNABLE_TO_ALLOCATE_MEMORY_FOR_SERVER_GAME_DATA
		MPDPXTRAERR_NO_SERVER_GAME_DATA_ASSOCIATED_WITH_KEY

-----------------------------------------------------------------------------------*/

MPDPXTRAERR		MPDPXTRA_FetchServerGameData(UINT key,void *pData,UINT *pDataLen)
{
	MPDPXTRAERR		retVal = MPDPXTRAERR_OK;

	if (gMPDPXTRALibHndl != NULL)
	{
		if (MPDPXTRA_FetchServerGameData_InDLL != NULL)
			retVal = MPDPXTRA_FetchServerGameData_InDLL(key,pData,pDataLen);
		else
			retVal = MPDPXTRAERR_UNABLE_TO_LOCATE_FUNCTION_IN_MPDPXTRA_LIBRARY;
	}
	else
		retVal = MPDPXTRAERR_MPDPXTRA_LIBRARY_NOT_LOADED;

	return (retVal);
}


/*-----------------------------------------------------------------------------------
	
	MPDPXTRA_SubscribeServerGameData

	Subscribes to receive updates of game data blocks stored on the game server.
	Updates will be received in an asynchronous fashion with the passed in
	callback function getting called when a complete update has been received.

	The first time this function is called, the server will be checked for the
	specified key. If there is an opqaue data block associated with the specified
	key stored on the server, an asynchronous transfer will begin and the callback
	function will be called when the complete block as been received. Subsequent
	updates to the opaque data block on the server via the MPDPXTRA_PostServerGameData
	function will cause the subscribed block to be asynchronously transmitted to
	all current subscribers with the callback function getting called when the
	complete block has been received by the subscriber.

	It is possible to subscribe to a data block with the same key but with different
	callback functions and different contexts. Each unique pair of callback function
	pointer and context will be treated a a separate "subscriber."

	***** WARNING *****

	Note that these callback functions are called on the Mplayer network's transmission
	thread. For this reason, it is not safe to do very much becuase any activity that
	occurs inside the callback will be blocking network transmission. It is suggested
	that a minimal amount of activity take place such as copying the returned buffer,
	posting a message or event, and immediately returning.


	Inputs:

		key -			Key value for the opaque data block between 0 and 0x7FFFFFFF.
						This key is game specific so developers are free to choose any
						key they wish.
		cbf -			A pointer to a callback function of type SERVER_GAME_DATA_CBF.
		cbf_context -	A game specified pointer to a context that will be passed to the
						callback function.

	Outputs:

		None.

	Return Codes:

		MPDPXTRAERR_OK
		MPDPXTRAERR_MPDPXTRA_DLL_NOT_LOADED
		MPDPXTRAERR_UNABLE_TO_LOCATE_FUNCTION_IN_MPDPXTRA_DLL
		MPDPXTRAERR_NOT_CONNECTED_TO_MPLAYER_NETWORK
		MPDPXTRAERR_SERIOUS_MPLAYER_ERROR
		MPDPXTRAERR_INVALID_SERVER_GAME_DATA_KEY
		MPDPXTRAERR_INVALID_SERVER_GAME_DATA_CALLBACK
		MPDPXTRAERR_UNABLE_TO_SUBSCRIBE_TO_SERVER_GAME_DATA

-----------------------------------------------------------------------------------*/

MPDPXTRAERR		MPDPXTRA_SubscribeServerGameData(UINT key,SERVER_GAME_DATA_CBF *cbf,void *context)
{
	MPDPXTRAERR		retVal = MPDPXTRAERR_OK;

	if (gMPDPXTRALibHndl != NULL)
	{
		if (MPDPXTRA_SubscribeServerGameData_InDLL != NULL)
			retVal = MPDPXTRA_SubscribeServerGameData_InDLL(key,cbf,context);
		else
			retVal = MPDPXTRAERR_UNABLE_TO_LOCATE_FUNCTION_IN_MPDPXTRA_LIBRARY;
	}
	else
		retVal = MPDPXTRAERR_MPDPXTRA_LIBRARY_NOT_LOADED;

	return (retVal);
}


/*-----------------------------------------------------------------------------------
	
	MPDPXTRA_UnsubscribeServerGameData

	Unsubscribes from updates of server game data blocks associated with the specified
	key, callback function, and context.

	Inputs:

		key -			Key value for the opaque data block between 0 and 0x7FFFFFFF.
						This key is game specific so developers are free to choose any
						key they wish.
		cbf -			A pointer to a callback function of type SERVER_GAME_DATA_CBF.
		cbf_context -	A game specified pointer to a context that will be passed to the
						callback function.

	Outputs:

		None.

	Return Codes:

		MPDPXTRAERR_OK
		MPDPXTRAERR_MPDPXTRA_DLL_NOT_LOADED
		MPDPXTRAERR_UNABLE_TO_LOCATE_FUNCTION_IN_MPDPXTRA_DLL
		MPDPXTRAERR_NOT_CONNECTED_TO_MPLAYER_NETWORK
		MPDPXTRAERR_SERIOUS_MPLAYER_ERROR
		MPDPXTRAERR_INVALID_SERVER_GAME_DATA_KEY
		MPDPXTRAERR_INVALID_SERVER_GAME_DATA_CALLBACK

-----------------------------------------------------------------------------------*/

MPDPXTRAERR	mpcdecl MPDPXTRA_UnsubscribeServerGameData(UINT key,SERVER_GAME_DATA_CBF *cbf,void *context)
{
	MPDPXTRAERR		retVal = MPDPXTRAERR_OK;

	if (gMPDPXTRALibHndl != NULL)
	{
		if (MPDPXTRA_UnsubscribeServerGameData_InDLL != NULL)
			retVal = MPDPXTRA_UnsubscribeServerGameData_InDLL(key,cbf,context);
		else
			retVal = MPDPXTRAERR_UNABLE_TO_LOCATE_FUNCTION_IN_MPDPXTRA_LIBRARY;
	}
	else
		retVal = MPDPXTRAERR_MPDPXTRA_LIBRARY_NOT_LOADED;

	return (retVal);
}
