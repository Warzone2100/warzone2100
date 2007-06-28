/* Handles the two CD issue */
/* Alex McLean */

#include "frame.h"
#include "piedef.h"
#include "piefunc.h"
#include "piemode.h"
#include "piestate.h"
#include "text.h"
#include "displaydef.h"
#include "rendmode.h"
#include "hci.h"
#include "intdisplay.h"
#include "audio.h"
#include "cdaudio.h"
#include "cdspan.h"

// turn on/off checks
#if 1
#define	DONTTEST
#endif

// labels 
#define	LABEL1	"Wz2100"
#define	LABEL2	"Wz2100b"

// Declarations for the positioning of the box */
#define	CDSPAN_BOX_WIDTH		(300)
#define	CDSPAN_BOX_DEPTH		(60)
#define	CDLEFT					(D_W + (CDSPAN_BOX_WIDTH/2))
#define CDRIGHT					(DISP_WIDTH - D_W - (CDSPAN_BOX_WIDTH/2))
#define TOP						((DISP_HEIGHT - CDSPAN_BOX_DEPTH)/2)
#define BOTTOM					((DISP_HEIGHT + CDSPAN_BOX_DEPTH)/2)

#define	MAX_VOL_NAMES			5

// -----------------------------------------------------------------------
// Static variables
/* Which CD is required */
static CD_INDEX		g_CDrequired = DISC_INVALID;

/* CD validated callbacks */
static CDSPAN_CALLBACK	g_fpOKCallback = NULL;
static CDSPAN_CALLBACK	g_fpCancelCallback = NULL;

static BOOL			g_bBoxVisible = FALSE;
static W_SCREEN		*g_psCurWScreen = NULL;

static STRING		g_szCurDriveName[MAX_STR] = "";

// -----------------------------------------------------------------------
// Functions.
// -----------------------------------------------------------------------

BOOL
cdspan_GetDriveName( BOOL bCheckAny, STRING szDriveName[], ... )
{							 //I use WIN321 to disable these for now -Q
#ifdef WIN321		//Neither linux nor win32 uses this, but maybe sometime in future? --QAMLY 
	UINT		uiRet;
	STRING		szDrives[MAX_STR],
				szVolNameList[MAX_VOL_NAMES][MAX_STR],
				seps[] = " ,\t\n",
				*token = NULL;
	SDWORD		i, j, iLabels;
	BOOL		bFound = FALSE;
	va_list		vArgList;
	char		szVolName[MAX_STR], szSysName[MAX_STR];
	DWORD		dwVolSerialNum, dwFlags, dwMaxlen;

	/* get list of volume names */
	iLabels = -1;
	va_start( vArgList, szDriveName );
	do
	{
		iLabels++;
		strcpy( szVolNameList[iLabels], va_arg( vArgList, STRING * ) );
	}
	while ( strcmp( szVolNameList[iLabels], "" ) != 0 );
	va_end( vArgList );

	/* check no labels entered */
	if ( bCheckAny == FALSE && iLabels == 0 )
	{
		return FALSE;
	}

	/* check all drives for CDROMs with matching labels */
	if ( GetLogicalDriveStrings( MAX_STR, szDrives ) )
	{
		/* go through string and remove single NULLs */
		for ( i=0; i<MAX_STR; i++ )
		{
			if ( szDrives[i] == '\0' )
			{
				if ( szDrives[i+1] == '\0' )
				{
					break;
				}
				else
				{
					szDrives[i] = '\t';
				}
			}
		}

		/* strtok it */
		token = strtok( szDrives, seps );
		while( token != NULL && bFound == FALSE )
		{
			uiRet = GetDriveType( token );
			if ( uiRet == DRIVE_CDROM )
			{
				if ( bCheckAny == TRUE )
				{
					strcpy( g_szCurDriveName, token );
					strcpy( szDriveName, token );
					return TRUE;
				}

				if ( GetVolumeInformation( token, szVolName, MAX_STR,
						&dwVolSerialNum, &dwMaxlen, &dwFlags, szSysName, MAX_STR) )
				{
					for ( j=0; j<iLabels; j++ )
					{
						if ( stricmp( szVolName, szVolNameList[j] ) == 0 )
						{
							bFound = TRUE;
							break;
						}
					}
				}
			}
			
			/* Get next token: */
			if ( bFound == FALSE )
			{
				token = strtok( NULL, seps );
			}
		}
	}

	if ( bFound == TRUE )
	{
		strcpy( g_szCurDriveName, token );
		strcpy( szDriveName, token );
		return TRUE;
	}
	else
	{
		strcpy( szDriveName, "" );
		return FALSE;
	}
#else
	strcpy( szDriveName, "" );
	return FALSE;
#endif
}

// -----------------------------------------------------------------------

BOOL
cdspan_GetCDLetter( STRING szDriveName[], CD_INDEX index )
{
	switch ( index )
	{
		case DISC_ONE:
		case DISC_TWO:
			return cdspan_GetDriveName( FALSE, szDriveName,
										cdspan_GetDiskLabel(index), "" );
		case DISC_EITHER:
			return cdspan_GetDriveName( FALSE, szDriveName,
										cdspan_GetDiskLabel(DISC_ONE),
										cdspan_GetDiskLabel(DISC_TWO), "" );
		case DISC_INVALID:
			return cdspan_GetDriveName( TRUE, szDriveName, "" );
	}
	return FALSE;
}

// -----------------------------------------------------------------------

void
cdspan_PlayInGameAudio( STRING szFileName[], SDWORD iVol )
{
	STRING szStream[MAX_STR];//	szDrive[MAX_STR] = "",
	BOOL	bPlaying = FALSE;

	audio_StopAll();

	wsprintf( szStream, "%s%s", g_szCurDriveName, szFileName );
	bPlaying = audio_PlayStream( szStream, iVol, NULL );

	/* try playing from hard disk */
	if ( bPlaying == FALSE )
	{
		wsprintf( szStream, "audio\\%s", szFileName );
		bPlaying = audio_PlayStream( szStream, iVol, NULL );
	}
}

// -----------------------------------------------------------------------

static BOOL
cdspan_GetCDLabel( STRING *szLabel )
{
#ifdef WIN321		//Neither linux nor win32 uses this, but maybe sometime in future? --QAMLY
	char	szVolName[MAX_STR], szSysName[MAX_STR];
	DWORD	dwVolSerialNum, dwFlags, dwMaxlen;

	if ( GetVolumeInformation( g_szCurDriveName, szVolName, MAX_STR,
				&dwVolSerialNum, &dwMaxlen, &dwFlags, szSysName, MAX_STR ) )
	{
		strcpy( szLabel, szVolName );
		return TRUE;
	}
	else
	{
		strcpy( szLabel, "" );
		return FALSE;
	}

	return TRUE;
#else
	strcpy( szLabel, "" );
	return FALSE;
#endif
}

// -----------------------------------------------------------------------

static BOOL
cdspan_CheckForCD( STRING szVolumeLabel[] )
{
	STRING	szVolName[MAX_STR];

	if ( cdspan_GetCDLabel( szVolName ) &&
		 stricmp( szVolumeLabel, szVolName ) == 0 )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}

	return TRUE;
}

/***************************************************************************/

STRING *
cdspan_GetCDStrFromIndex( CD_INDEX index )
{
	if ( index == DISC_ONE )
	{
		return LABEL1;
	}
	else if ( index == DISC_TWO )
	{
		return LABEL2;
	}
	else
	{
		DBPRINTF( ("cdspan_GetCDStrFromIndex: invalid cd index\n") );
		return NULL;
	}
}

// -----------------------------------------------------------------------

CD_INDEX
cdspan_GetCDIndexFromStr( STRING *pStr )
{
	if ( stricmp( pStr, LABEL1 ) == 0 )
	{
		return DISC_ONE;
	}
	else if ( stricmp( pStr, LABEL2 ) == 0 )
	{
		return DISC_TWO;
	}
	else
	{
		DBPRINTF( ("cdspan_GetCDIndexFromStr: invalid cd string\n") );
		return DISC_INVALID;
	}
}

// -----------------------------------------------------------------------
/* 	Returns the presently active (in the drive) CD */
BOOL cdspan_CheckCDPresent( CD_INDEX index )
{
	STRING	szDriveName[MAX_STR];

#ifdef DONTTEST
	return TRUE;
#endif

	return cdspan_GetDriveName( FALSE, szDriveName,
								cdspan_GetDiskLabel(index), "" );
}

// -----------------------------------------------------------------------

BOOL cdspan_CheckCDAvailable( void )
{
	STRING	szDriveName[MAX_STR];

#ifdef DONTTEST
	return TRUE;
#endif

	return cdspan_GetDriveName( TRUE, szDriveName, "" );
}

// -----------------------------------------------------------------------

UDWORD	getCDRealNumberIndex( CD_INDEX index )
{
	if( index == DISC_ONE )
	{
		return(1);
	}
	else if( index == DISC_TWO )
	{
		return(2);
	}
	else
	{
		return(100);
	}
}

// -----------------------------------------------------------------------
/* 	Returns the other CD number - uses a UDWORD cos we need the number
	from 1 and not from 0 
*/
UDWORD	getOtherCDRealNumberIndex(void)
{
	if( cdspan_CheckCDPresent( DISC_ONE ) )
	{
		return(2);
	}
	else
	{
		return(1);
	}
}
// -----------------------------------------------------------------------
/*	Establishes whether the cd that's requested is valid - ie - have we 
	already	got it in the drive? 
*/
BOOL	cdIsValid(CD_INDEX cdRequired)
{
#ifdef DONTTEST
	return TRUE;
#endif

	/* Make sure it's a good CD */
	ASSERT((cdRequired == DISC_ONE OR cdRequired == DISC_TWO,
		"Invalid CD name request for cdIsValid"));

	/* Look at what we've got in the drive */
	if( cdspan_CheckCDPresent( cdRequired ))
	{
		/* It's the one we want - so return TRUE */
		return(TRUE);
	}
	else
	{
		/* We need the other CD so return FALSE */
		return(FALSE);
	}
}

// -----------------------------------------------------------------------
/*	Returns the cd index (using the typedef names) for the requested 
	campaign number 1,2,3 and NOT 0,1,2
*/
CD_INDEX	getCDForCampaign(UDWORD camNumber)
{
#ifdef DONTTEST
	return DISC_INVALID;
#endif

 	/* Campaign one is on CD 1, others on CD 2 */
	if(camNumber == 1)
	{
		return(DISC_ONE);
	}
	else if(camNumber == 2 || camNumber == 3)
	{
		return(DISC_TWO);
	}
	else
	{
		return(DISC_INVALID);
	}
}

// -----------------------------------------------------------------------

BOOL	showChangeCDBox( W_SCREEN *psCurWScreen, CD_INDEX CDrequired,
	CDSPAN_CALLBACK fpOKCallback, CDSPAN_CALLBACK fpCancelCallback )
{
	W_FORMINIT		sFormInit;
	W_BUTINIT		sButInit;
	W_LABINIT		sLabInit;
	static STRING	szMessage[255];

#ifdef DONTTEST
	return TRUE;
#endif

	/* stop cd audio */
	cdAudio_Stop();

	/* save callbacks */
	g_fpOKCallback     = fpOKCallback;
	g_fpCancelCallback = fpCancelCallback;

	/* save CD required */
	g_CDrequired = CDrequired;

	/* save current screen */
	g_psCurWScreen = psCurWScreen;

	/* set visible flag */
	g_bBoxVisible = TRUE;

	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	memset(&sButInit, 0, sizeof(W_BUTINIT));
	memset(&sLabInit, 0, sizeof(W_LABINIT));

	/* Add box */
	sFormInit.formID = 0;
	sFormInit.id = ID_WIDG_CDSPAN_FORM;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = (SWORD)CDLEFT;
	sFormInit.y = (SWORD)TOP;
	sFormInit.width  = (UWORD)(CDSPAN_BOX_WIDTH);
	sFormInit.height = (UWORD)(CDSPAN_BOX_DEPTH);
	sFormInit.pDisplay = intDisplayPlainForm;
	if (!widgAddForm(psCurWScreen, &sFormInit))
	{
		return FALSE;
	}

	/* Get the message text */
	if ( CDrequired == DISC_EITHER )
	{
		sprintf( szMessage, strresGetString(psStringRes,STR_CD_CHANGE_1OR2) );
	}
	else
	{
		sprintf( szMessage, strresGetString(psStringRes,STR_CD_CHANGE),
					getCDRealNumberIndex( CDrequired ) );
	}

	iV_SetTextColour(PIE_TEXT_WHITE);

	/* cd load message label */
	sLabInit.formID = ID_WIDG_CDSPAN_FORM;
	sLabInit.id = ID_WIDG_CDSPAN_LABEL;
	sLabInit.style = WLAB_PLAIN;
	sLabInit.x = 20;
	sLabInit.y = 10;
	sLabInit.width = 40;
	sLabInit.height = 20;
	sLabInit.pText = szMessage;
//	sLabInit.pTip = "";
	sLabInit.FontID = WFont;
	if (!widgAddLabel(psCurWScreen, &sLabInit))
	{
		return TRUE;
	}

	/* OK button */
	sButInit.formID = ID_WIDG_CDSPAN_FORM;
	sButInit.id = ID_WIDG_CDSPAN_BUTTON_OK;
	sButInit.style = WBUT_PLAIN;
	sButInit.x = 60;
	sButInit.y = 30;
	sButInit.width = iV_GetImageWidth(IntImages, IMAGE_CDCHANGE_OK);
	sButInit.height = iV_GetImageHeight(IntImages, IMAGE_CDCHANGE_OK);
//	sButInit.pTip = "";
	sButInit.pDisplay = intDisplayImageHilight;
	sButInit.pUserData = (void*)PACKDWORD_TRI(0,IMAGE_CDCHANGE_OK , IMAGE_CDCHANGE_OK);
	sButInit.FontID = WFont;
	if (!widgAddButton(psCurWScreen, &sButInit))
	{
		return FALSE;
	}

	/* cancel button */
	sButInit.formID = ID_WIDG_CDSPAN_FORM;
	sButInit.id = ID_WIDG_CDSPAN_BUTTON_CANCEL;
	sButInit.style = WBUT_PLAIN;
	sButInit.x = 200;
	sButInit.y = 30;
	sButInit.width = iV_GetImageWidth(IntImages, IMAGE_CDCHANGE_CANCEL);
	sButInit.height = iV_GetImageHeight(IntImages, IMAGE_CDCHANGE_CANCEL);
//	sButInit.pTip = "";
	sButInit.pDisplay = intDisplayImageHilight;
	sButInit.pUserData = (void*)PACKDWORD_TRI(0,IMAGE_CDCHANGE_CANCEL , IMAGE_CDCHANGE_CANCEL);
	sButInit.FontID = WFont;
	if (!widgAddButton(psCurWScreen, &sButInit))
	{
		return FALSE;
	}

	g_bBoxVisible = TRUE;

	return TRUE;
}
// -----------------------------------------------------------------------

void	cdspan_RemoveChangeCDBox( void )
{

#ifdef DONTTEST
	return;
#endif

	widgDelete(g_psCurWScreen, ID_WIDG_CDSPAN_FORM);
	g_psCurWScreen = NULL;
	intMode = INT_NORMAL;
	g_bBoxVisible = FALSE;
}

// -----------------------------------------------------------------------
/* cdspan_ProcessCDChange returns TRUE if event processed */
BOOL	cdspan_ProcessCDChange( UDWORD iID )
{
	STRING	*pStr;
	BOOL	bOK;

#ifdef DONTTEST
	return FALSE;
#endif

	switch ( iID )
	{
		case ID_WIDG_CDSPAN_BUTTON_OK:
			if ( g_CDrequired == DISC_EITHER )
			{
				pStr = cdspan_GetCDStrFromIndex( DISC_ONE );
				bOK = cdspan_CheckForCD( pStr );
				if ( bOK == FALSE )
				{
					pStr = cdspan_GetCDStrFromIndex( DISC_TWO );
					bOK = cdspan_CheckForCD( pStr );
				}
			}
			else
			{
				pStr = cdspan_GetCDStrFromIndex(g_CDrequired);
				bOK = cdspan_CheckForCD( pStr );
			}

			/* continue if disc ok else wait for cancel or ok clicked again */
			if ( bOK )
			{
				cdspan_RemoveChangeCDBox();
				/* do callback */
				if ( g_fpOKCallback != NULL )
				{
					(g_fpOKCallback)();
				}
			}

			return TRUE;

		case ID_WIDG_CDSPAN_BUTTON_CANCEL:
			cdspan_RemoveChangeCDBox();
			/* do callback */
			if ( g_fpCancelCallback != NULL )
			{
				(g_fpCancelCallback)();
			}
			return TRUE;

		default:
			if ( g_bBoxVisible )
			{
				return TRUE;
			}
			else
			{
				return FALSE;
			}
	}
}

// -----------------------------------------------------------------------
// determine which cd is in, then return TRUE if it's a warzone cd.

BOOL cdspan_initialCDcheck()
{

#ifdef DONTTEST
	return TRUE;
#endif

	// determine which CD is available.
	if ( cdspan_CheckForCD( LABEL1 ) == TRUE ||
		 cdspan_CheckForCD( LABEL2 ) == TRUE    )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

// -----------------------------------------------------------------------

BOOL	cdspan_DontTest( void )
{
#ifdef DONTTEST
	return TRUE;
#else
	return FALSE;
#endif
}

// -----------------------------------------------------------------------

STRING * cdspan_GetDiskLabel( CD_INDEX cdIndex )
{
	switch ( cdIndex )
	{
		case DISC_ONE:
			return LABEL1;

		case DISC_TWO:
			return LABEL2;

		default:
			return "";
	}
}

// -----------------------------------------------------------------------
