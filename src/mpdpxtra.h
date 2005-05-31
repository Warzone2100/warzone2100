#ifndef __MPDPXTRA_H
#define __MPDPXTRA_H

// stuff by alex l 1998 to get compiling withoug devkit
#define	MPDPXTRAERR_OK						0
//#define mpcdecl cdecl
#define mpcdecl
#define REG_KEY_MPLAYER         "Software\\Mpath\\Mplayer"      // in HKEY_LOCAL_MACHINE
typedef unsigned long MPPLAYERID;

/*----------------------------------------------------------------------------

	MPLAYER DIRECTPLAY EXTRAS

	This library provides extra extensions for 	DirectPlay games that that
	run on the Mplayer network. 
  
	Current Features
	----------------

	Lobby Status Reporting	-	Allows games to report staus information
								to the game's Mplayer lobby. This inform-
								ation includes items such as changing the
								name of the game room, reporting game-in
								progress details, etc.

	Score Reporting			-	Allows games to report game statistics
								and scores to be saved in a database which
								can then be queried to generate rankings,
								ratings, and high score lists.

	Release Notes
	----------------

	1)	A new feature for DirectPlay in the DirectX 5.0 release is the ability
		an applications process to create multiple DirectPlay objects. The
		MPDPXTRA library will not work with applications that create more than
		one DirectPlay object. The MPDPXTRA library is compatibles with all
		pre-DirectX	5.0 applications as well as	DirectX 5.0 applications that
		create only one DirectPlay object.

	Using the Mplayer DirectPlay Extras Library
	-------------------------------------------

	1)	The Mplayer DirectPlay Extras Library consists of the following files:

		-	Header files including this one (MPDPXTRA.H) located in the Mplayer
		-	DirectPlay SDK include directory
		-	A dynamic library stub loader file (MPDPXTRA.C) that is linked with
			your application
		-	The library file (DPMPLAY.DLL) which exists in the Mplayer run-time
			system directory.

	2)	To use the library, make sure the Mplayer DirectPlay SDK include directory
		can be found in your compiler's path. Link the stub loader file, MPDPXTRA.C,
		with your application. This file is used to dynamically load DPMPLAY.DLL
		at run-time rather than statically linking it to your application. This
		allows you to ship your game without including DPMPLAY.DLL.	Instead,
		DPMPLAY.DLL will be installed and and automatically upgraded and maintained
		by Mplayer when users logon to the Mplayer network.

	3)	At run-time, your application calls MPDPXTRA_Init() to initialize the library.
		This function will attempt to locate and load DPMPLAY.DLL. The function may
		fail if your application is not being run over the Mplayer network because the
		loader does not know where DPMPLAY.DLL is located. When your application is
		launched on the Mplayer network, however, the loader will properly locate it
		in the Mplayer System Directory and load it. You may decide to gracefully
		accept a failed load of DPMPLAY.DLL. Subsequent calls to other functions in
		the library will simply not do anything. When your game is running on the
		Mplayer network, you still have the option of ignoring a failed	load although
		this should never happen. If the load does fail, the library will not actual
		perform any of its functions but players can still play your game. The game
		will not be able to change the appearance of the lobby or successfully report
		scores.

	4)	All MPDPXTRA library routines with the exception of MPDPXTRA_Init() should
		be called only after an application has successfully connected to a game
		session using the DirectPlay Open function. Calling library routines prior
		to successfully connecting to a game session will return an error code of
		MPDPXTRAERR_NOT_CONNECTED_TO_MPLAYER_NETWORK.

	Using the Lobby Status Reporting Features
	-----------------------------------------

	1)	Lobby Status Reporting allows your game to report staus information to the
		game's Mplayer lobby. There are 3 types of status that can be reported:

		- MP_LOBBYDATA_COMMENT
		- MP_LOBBYDATA_SHORT_STATUS
		- MP_LOBBYDATA_LONG_STATUS

		All status reporting to the lobby should be as infrequent as possible to
		prevent unneccessary bandwidth usage on the Mplayer network. You should never
		try to perform real-time status reporting. packets should be kept as small
		as possible.

	2) MP_LOBBYDATA_COMMENT

		This type of status reporting allows a game to change the comment underneath
		the room icon in the Mplayer lobby for this particular game session. See the
		description of the MPDPXTRA_PostLobbyData function for details.

	3) MP_LOBBYDATA_SHORT_STATUS

		These are small opaque blocks of data that can be sent to your game extension
		DLL. These data blocks will be sent to *EVERYONE* connected to your game's
		Mplayer lobby so it is imperative that they be kept extremely small. These
		types of data blocks can be used to send small pieces of information such
		as the score of the game. Your game extension DLL will be able to receive
		these data blocks and adjust the appearance of game's Mplayer lobby as well
		as the screen area inside the game room owned by the game extension DLL based
		on the contents.

	4) MP_LOBBYDATA_LONG_STATUS

		These data blocks are also opaque blocks of data that can be sent to your game
		extension DLL. They are only sent to users who have either selected this game
		session's icon in the game's Mplayer lobby or who are in the game session's
		room. Since these data blocks are sent to fewer people, they can be a bit
		larger than MP_LOBBYDATA_SHORT_STATUS data blocks to allow for more detailed
		status of a game. Once again, however, these data blocks should be kept as
		small as possible and be sent as infrequently as possible. These data blocks
		can be used to send expanded status to users interested in this particular
		game session. Your game extension DLL will be able to receive these data
		blocks and adjust the appearance of game's Mplayer lobby as well as the screen
		area inside the game room owned by the game extension DLL based	on the
		contents.
		
	Using Score Reporting Features
	------------------------------

	1)	Using the Score Rreporting features is very simple. Currently, Score
		Reporting consists of just two functions: MPDPXTRA_AddScoreResult and
		MPDPXTRA_SaveScoreResults.
		
	2)	MPDPXTRA_AddScoreResult allows you to accumulate scores on the local client
		based on four pieces of information: a source player, a destination player,
		a key, and a value. Multiple score results can be queued on the local client.
		Results that match all four parameters with a previous call	to
		MPDPXTRA_AddScoreResult will replace that result.
		
	3)	MPDPXTRA_SaveScoreResults sends all of the accumulated results to the Mplayer
		network where they are stored in a persistent database and saved for future
		score ranking and rating display. Mplayer will work with developers to
		generate algorithms that can process these results and display them accord-
		ingly.


	Written By Andy Riedel
	Copyright 1996-97. Mpath Interactive, Inc.

----------------------------------------------------------------------------*/



#ifdef __BORLANDC
#pragma option -a8					// Pack on 8 byte boundaries for Borland
#endif

#ifndef MP_LOAD_DLL_DYNAMICALLY		// Indicate that the Mplayer DLL is dynamically loaded
#define MP_LOAD_DLL_DYNAMICALLY
#endif

//#include "mplayer.h"

#ifdef __BORLANDC
#pragma option -a.					// Pack on compiler defined byte boundaries (like pack pop!)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/****** MPDPXTRA TYPEDEFS ******/

typedef void SERVER_GAME_DATA_CBF(int	reserved,		// Reserved for Mplayer use
								  UINT	key,			// Identifies the opaque server data block
								  void	*context,		// Context passed by the subscriber 
								  void	*pData,			// Pointer to the opaque server data block
								  UINT	dataLen);		// Length of opaque server data block received

/****** MPDPXTRA ERROR CODES ******/

typedef	int	MPDPXTRAERR;

/********** MPDPXTRA INITIALIZATION ERRORS **********/

// Indicates success
#define	MPDPXTRAERR_OK												0
// The Mplayer DirectPlay Extras Library contained in DPMPLAY.DLL could
// not be loaded. Either Dpmplay.dll could not be located or one of the
// DLL's that it makes use of could not be loaded.
#define	MPDPXTRAERR_UNABLE_TO_LOAD_MPDPXTRA_LIBRARY					-1
// The Mplayer DirectPlay Extras Library contained in DPMPLAY.DLL is not
// currently loaded. This return code may occur if the library was not
// loaded as indicated by an error returned by MPDPXTRA_Init or a previous
// call to MPDPXTRA_Destroy already decremented the reference count to zero
// and released	the library.
#define	MPDPXTRAERR_MPDPXTRA_LIBRARY_NOT_LOADED						-2
// Multiple calls have been made to MPDPXTRA_Init and the reference count for
// the library has not yet reached zero .
#define MPDPXTRAERR_MPDPXTRA_LIBRARY_REFERENCE_COUNT_NOT_ZERO		-3
// The library function called could not be dynamically linked. If MPDPXTRA_Init
// returns a successful code of MPDPXTRAERR_OK, this error should never occur. If
// it does, there is a serious error with the Windows system such as an extremely
// low memory condition.
#define MPDPXTRAERR_UNABLE_TO_LOCATE_FUNCTION_IN_MPDPXTRA_LIBRARY	-4

/********** MPDPXTRA NETWORK ERRORS **********/

// The Mplayer network could not be initialized. This error can occur when the
// Mplayer network is unavailable due to technical difficulties.
#define MPDPXTRAERR_UNABLE_TO_INITIALIZE_MPLAYER_NETWORK			-10
// A connection to the Mplayer network could not be established. This error
// can occur when the Mplayer network is unavailable due to technical difficulties.
#define MPDPXTRAERR_UNABLE_TO_CONNECT_TO_MPLAYER_NETWORK			-11
// A connection to the Mplayer network has not been established.
#define MPDPXTRAERR_NOT_CONNECTED_TO_MPLAYER_NETWORK				-12
// A serious internal Mplayer network error has occurred. The current connection to
// Mplayer can no longer be deemed reliable and the Mplayer DirectPlay Extras Library
// can no longer be used reliably. The library should be shut down with a call to
// MPDPXTRA_Destroy. This error can occur when the Mplayer network has experienced
// a network outage.
#define MPDPXTRAERR_SERIOUS_MPLAYER_ERROR							-13

/********** MPDPXTRA LOBBY STATUS REPORTING ERRORS **********/

// The lobby data sent via MPDPXTRA_PostLobbyData could not be sent because the object
// ID passed was invalid.
#define MPDPXTRAERR_BAD_LOBBY_DATA_OBJECT_ID						-20 
// The lobby data sent via MPDPXTRA_PostLobbyData could not be sent due to an internal
// Mplayer network error. This error can occur when the Mplayer network is flooded with
// too much outgoing data.
#define MPDPXTRAERR_UNABLE_TO_SEND_LOBBY_DATA						-21

/********** MPDPXTRA SCORE REPORTING ERRORS **********/

// Unable to allocate memory needed to queue the score result.
#define MPDPXTRAERR_UNABLE_ALLOCATE_MEMORY_FOR_RESULT				-30
// The lobby data sent via MPDPXTRA_SendResults could not be sent due to an internal
// Mplayer network error. This error can occur when the Mplayer network is flooded with
// too much outgoing data or the Mplayer network is unavailable due to technical dif-
// ficulties.
#define MPDPXTRAERR_UNABLE_TO_SEND_SCORE_RESULTS					-31
// An invalid player DPID was passed to the MPDPXTRA_AddScoreResult function.
#define MPDPXTRAERR_INVALID_PLAYER_ID								-32

/********** MPDPXTRA SERVER GAME DATA ERRORS **********/

// An invalid server game data key was specified. Valid values are 0-0x7FFFFFFF.
#define MPDPXTRAERR_SERVER_GAME_DATA_INVALID_KEY					-40
// The pointer specified to post server game data was NULL.
#define MPDPXTRAERR_SERVER_GAME_DATA_INVALID_POINTER				-41
// The callback specified to was NULL.
#define MPDPXTRAERR_SERVER_GAME_DATA_INVALID_CALLBACK				-42
// The data length specified to post server game data was too big. The maximum
// size if 256K.
#define MPDPXTRAERR_SERVER_GAME_DATA_SIZE_TOO_BIG					-43
// The size of the buffer specified when fetching server game daya is too small.
#define MPDPXTRAERR_SERVER_GAME_DATA_SIZE_TOO_SMALL					-44
// A network error occurred attempting to post server game data.
#define MPDPXTRAERR_UNABLE_TO_SEND_SERVER_GAME_DATA					-45
// Unable to allocate memory required for a server game data operation.
#define MPDPXTRAERR_UNABLE_TO_ALLOCATE_MEMORY_FOR_SERVER_GAME_DATA	-46
// No data has been posted on the server for the specified key.
#define MPDPXTRAERR_NO_SERVER_GAME_DATA_ASSOCIATED_WITH_KEY			-47

/****** MPDPXTRA FUNCTION PROTOTYPES ******/

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
	MPDPXTRAERR_UNABLE_TO_LOAD_MPDPXTRA_DLL
	MPDPXTRAERR_UNABLE_TO_LOCATE_FUNCTION_IN_MPDPXTRA_DLL
	MPDPXTRAERR_UNABLE_TO_LOAD_MPLAYER_DLL
	MPDPXTRAERR_UNABLE_TO_LOCATE_FUNCTION_IN_MPLAYER_DLL
	MPDPXTRAERR_UNABLE_TO_INITIALIZE_MPLAYER_NETWORK
	MPDPXTRAERR_UNABLE_TO_CONNECT_TO_MPLAYER_NETWORK

-----------------------------------------------------------------------------------*/

MPDPXTRAERR		MPDPXTRA_Init(void);

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
	MPDPXTRAERR_DLL_NOT_LOADED
	MPDPXTRAERR_UNABLE_TO_LOCATE_FUNCTION_IN_MPDPXTRA_DLL
	MPDPXTRAERR_DLL_REFERENCE_COUNT_NOT_ZERO

-----------------------------------------------------------------------------------*/

MPDPXTRAERR		MPDPXTRA_Destroy(void);

/*-----------------------------------------------------------------------------------

  MPDPXTRA_DPIDToMPPLAYERID

	The Mplayer DirectPlay Extras Library may need to translate DPID's to
	MPPLAYERID's for use with the score reporting API. This function
	performss a translation by requesting the MPPLAYERID from the local
	player data for the specified DPID.
	
-----------------------------------------------------------------------------------*/

MPDPXTRAERR		MPDPXTRA_DPIDToMPPLAYERID(DPID dpid,MPPLAYERID *mpPlayerID);

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

MPDPXTRAERR		MPDPXTRA_PostLobbyData(UINT	objectID,void *pObjectData,UINT	objectDataLen);

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

MPDPXTRAERR		MPDPXTRA_AddScoreResult(DPID srcId,DPID dstId,u_long key,long value);

/*-----------------------------------------------------------------------------------
	
	MPDPXTRA_AddScoreResultEx

	Queues a score result on the local client to be uploaded to the Mplayer network
	at a later time using MPDPXTRA_SaveScoreResults.

	Inputs:

		srcId -			Originating player's MPLAYERID
		dstId -			Target player's MPLAYERID, if any (use 0 for none)
		key -			Key value for the score result
		value -			Value of the score result

	Outputs:

		None.

	Return Codes:

		MPERR_ADDRESULT_OK
		MPDPXTRAERR_NOT_CONNECTED_TO_MPLAYER_NETWORK
		MPDPXTRAERR_SERIOUS_MPLAYER_ERROR
		MPDPXTRAERR_UNABLE_ALLOCATE_MEMORY_FOR_RESULT
		MPDPXTRAERR_INVALID_PLAYER_ID

-----------------------------------------------------------------------------------*/

MPDPXTRAERR		MPDPXTRA_AddScoreResultEx(MPPLAYERID srcId,MPPLAYERID dstId,u_long key,long value);

/*-----------------------------------------------------------------------------------
	
	MPDPXTRA_SaveScoreResults

	Sends all results accumulated with MPDPXTRA_AddScoreResult to the Mplayer
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

MPDPXTRAERR		MPDPXTRA_SaveScoreResults(void);

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
		MPDPXTRAERR_NOT_CONNECTED_TO_MPLAYER_NETWORK
		MPDPXTRAERR_SERIOUS_MPLAYER_ERROR
		MPDPXTRAERR_SERVER_GAME_DATA_INVALID_KEY
		MPDPXTRAERR_SERVER_GAME_DATA_INVALID_POINTER
		MPDPXTRAERR_SERVER_GAME_DATA_SIZE_TOO_BIG
		MPDPXTRAERR_UNABLE_TO_ALLOCATE_MEMORY_FOR_SERVER_GAME_DATA
		MPDPXTRAERR_UNABLE_TO_SEND_SERVER_GAME_DATA

-----------------------------------------------------------------------------------*/

MPDPXTRAERR		MPDPXTRA_PostServerGameData(UINT key,void *pData,UINT dataLen);

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
	correct size. A buffer can then be allocated and the function can be called again
	with the buffer passed as the pData variable.

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
		MPDPXTRAERR_NOT_CONNECTED_TO_MPLAYER_NETWORK
		MPDPXTRAERR_SERIOUS_MPLAYER_ERROR
		MPDPXTRAERR_SERVER_GAME_DATA_INVALID_KEY
		MPDPXTRAERR_SERVER_GAME_DATA_SIZE_TOO_SMALL
		MPDPXTRAERR_UNABLE_TO_ALLOCATE_MEMORY_FOR_SERVER_GAME_DATA
		MPDPXTRAERR_NO_SERVER_GAME_DATA_ASSOCIATED_WITH_KEY

-----------------------------------------------------------------------------------*/

MPDPXTRAERR		MPDPXTRA_FetchServerGameData(UINT key,void *pData,UINT *pDataLen);

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

	Inputs:

		key -			Key value for the opaque data block between 0 and 0x7FFFFFFF.
						This key is game specific so developers are free to choose any
						key they wish.
		cbf -			A pointer to a callback function of type SERVER_GAME_DATA_CBF.
		context -		A game specified pointer to a context that will be passed to the
						callback function.

	Outputs:

		None.

	Return Codes:

		MPDPXTRAERR_OK
		MPDPXTRAERR_NOT_CONNECTED_TO_MPLAYER_NETWORK
		MPDPXTRAERR_SERIOUS_MPLAYER_ERROR
		MPDPXTRAERR_INVALID_SERVER_GAME_DATA_KEY
		MPDPXTRAERR_INVALID_SERVER_GAME_DATA_CALLBACK
		MPDPXTRAERR_UNABLE_TO_SUBSCRIBE_TO_SERVER_GAME_DATA

-----------------------------------------------------------------------------------*/

MPDPXTRAERR		MPDPXTRA_SubscribeServerGameData(UINT key,SERVER_GAME_DATA_CBF *cbf,void *context);

/*-----------------------------------------------------------------------------------
	
	MPDPXTRA_UnsubscribeServerGameData

	Unsubscribes from updates of server game data blocks associated with the specified
	key, callback function, and context.

	Inputs:

		key -			Key value for the opaque data block between 0 and 0x7FFFFFFF.
						This key is game specific so developers are free to choose any
						key they wish.
		cbf -			A pointer to a callback function of type SERVER_GAME_DATA_CBF.
		context -		A game specified pointer to a context that will be passed to the
						callback function.

	Outputs:

		None.

	Return Codes:

		MPDPXTRAERR_OK
		MPDPXTRAERR_NOT_CONNECTED_TO_MPLAYER_NETWORK
		MPDPXTRAERR_SERIOUS_MPLAYER_ERROR
		MPDPXTRAERR_INVALID_SERVER_GAME_DATA_KEY
		MPDPXTRAERR_INVALID_SERVER_GAME_DATA_CALLBACK

-----------------------------------------------------------------------------------*/

MPDPXTRAERR	mpcdecl MPDPXTRA_UnsubscribeServerGameData(UINT key,SERVER_GAME_DATA_CBF *cbf,void *context);

#ifdef __cplusplus
}
#endif

#endif
