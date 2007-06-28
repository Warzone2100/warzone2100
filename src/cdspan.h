#ifndef _cdspan_h
#define _cdspan_h

#define	ID_WIDG_CDSPAN_FORM				9800
#define	ID_WIDG_CDSPAN_LABEL			9801
#define	ID_WIDG_CDSPAN_BUTTON_OK		9802
#define	ID_WIDG_CDSPAN_BUTTON_CANCEL	9803

// -----------------------------------------------------------------------
// Typedefs

/* The types of CD we can use and make sure ye bloody use 'em!! */
typedef enum
{
DISC_ONE,
DISC_TWO,
DISC_EITHER,
DISC_INVALID
}CD_INDEX;

/* callback func to call after CD validated */
typedef void (* CDSPAN_CALLBACK) ( void );

extern	BOOL		cdspan_CheckCDPresent( CD_INDEX cdIndex );
extern	BOOL		showChangeCDBox( W_SCREEN *psCurWScreen,
						CD_INDEX CDrequired,
						CDSPAN_CALLBACK fpOKCallback,
						CDSPAN_CALLBACK fpCancelCallback );
extern	BOOL		cdIsValid( CD_INDEX cdRequired );
extern	CD_INDEX	getCDForCampaign( UDWORD camNumber );
extern	void		cdspan_RemoveChangeCDBox( void );
extern	BOOL		cdspan_ProcessCDChange( UDWORD iID );
extern	BOOL		cdspan_initialCDcheck();
extern	BOOL		cdspan_DontTest( void );
extern	STRING *	cdspan_GetDiskLabel( CD_INDEX cdIndex );

extern	void		cdspan_PlayInGameAudio( STRING szFileName[], SDWORD iVol );
extern	BOOL		cdspan_GetCDLetter( STRING szDriveName[], CD_INDEX index );
extern	BOOL		cdspan_CheckCDAvailable( void );

#endif
