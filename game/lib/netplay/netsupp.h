/*
 * netsupp.h
 * 
 * Alex Lee, Pumpkin Studios, Nov97
 */

// ////////////////////////////////////////////////////////////////////////
// Prototypes.
extern HRESULT CreateDirectPlayInterface( LPDIRECTPLAY4A *lplpDirectPlay4A );
extern HRESULT DestroyDirectPlayInterface(HWND hWnd, LPDIRECTPLAY4A lpDirectPlay4A);
extern HRESULT HostSession	(LPDIRECTPLAY4A lpDirectPlay4A,LPSTR lpszSessionName, LPSTR lpszPlayerName,LPNETPLAY lpNetPlay,
							 DWORD one,DWORD two, DWORD three, DWORD four,UDWORD plyrs);
extern HRESULT JoinSession	(LPDIRECTPLAY4A lpDirectPlay4A,LPGUID lpguidSessionInstance,LPSTR lpszPlayerName,LPNETPLAY lpNetPlay);
				
