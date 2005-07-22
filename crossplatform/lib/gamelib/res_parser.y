%{

#include <stdio.h>

#include "frame.h"
#include "parser.h"
#include "audio.h"
#include "anim.h"

static int			g_iCurAnimID = 0;
static int			g_iDummy;
static VECTOR3D		vecPos, vecRot, vecScale;

%}

%union {
	float		fval;
	long		ival;
	signed char	bval;
	char		sval[100];
}

	/* value tokens */
%token <fval> FLOAT
%token <ival> INTEGER
%token <sval> TEXT
%token <sval> text
%token <sval> QTEXT			/* Text with double quotes surrounding it */
%token <ival> LOOP
%token <ival> ONESHOT

	/* types */
%type <sval> text

	/* keywords */
%token AUDIO
%token ANIM2D
%token ANIM3DFRAMES
%token ANIM3DTRANS
%token ANIM3DFILE
%token AUDIO_MODULE
%token ANIM_MODULE
%token ANIMOBJECT

	/* module names */

%%

data_file:				module_file | anim_file
						;

module_file:			module_file data_list |
						data_list
						;

data_list:				audio_module | anim_module
						;

audio_header:			AUDIO_MODULE "{"
						;

audio_module:			audio_header audio_list "}" |
						audio_header  "}"
						;

audio_list:				audio_list audio_track |
						audio_track
						;
	/*
	 * BOOL audio_SetTrackVals( char szFileName[], BOOL bLoop, int *piID, int iVol,
	 *					int iPriority, int iAudibleRadius )
	 */

audio_track:			AUDIO QTEXT LOOP INTEGER INTEGER INTEGER
						{
							audio_SetTrackVals( $2, TRUE, &g_iDummy, $4, $5, $6, 0 );
						}
						| AUDIO QTEXT ONESHOT INTEGER INTEGER INTEGER
						{
							audio_SetTrackVals( $2, FALSE, &g_iDummy, $4, $5, $6, 0 );
						}
						;

anim_module_header:		ANIM_MODULE "{"
						{
						}
						;

anim_module:			anim_module_header anim_file_list   "}" |
						anim_module_header anim_config_list "}" |
						/* NULL */
						;

anim_config_list:		anim_config_list anim_config |
						anim_config |
						/* NULL */
						;

anim_file_list:			anim_file_list anim_file |
						anim_file |
						/* NULL */
						;

/*
anim_file:				ANIM3DFILE QTEXT INTEGER
						{
							g_iCurAnimID = $3;
							IncludeFile( $2 );
						}
						anim_trans | anim_frames
						;
*/

anim_file:				anim_trans | anim_frames
						;

anim_config:			QTEXT INTEGER
						{
							g_iCurAnimID = $2;
							anim_SetVals( $1, $2 );
						}
						;

	/*
	 * anim_Create3D( char szAniFileName[], char szPieFileName[], UWORD uwFrames,
	 *					UWORD uwFrameRate, UWORD uwNumObj, char cType, UWORD uwID )
	 */

anim_trans:				ANIM3DTRANS QTEXT INTEGER INTEGER INTEGER
						{
							anim_Create3D( $2, $3, $4, $5, ANIM_3D_TRANS, g_iCurAnimID );
						}
						"{" anim_obj_list "}"
						{
							g_iCurAnimID++;
						}
						;

anim_frames:			ANIM3DFRAMES QTEXT INTEGER INTEGER
						{
							anim_Create3D( $2, $3, $4, 1, ANIM_3D_FRAMES, g_iCurAnimID );
						}
						"{"
						{
							anim_BeginScript();
						}
						anim_script "}"
						{
							anim_EndScript();
							g_iCurAnimID++;
						}
						;

anim_obj_list:			anim_obj anim_obj_list |
						anim_obj
						;

anim_obj:				ANIMOBJECT INTEGER QTEXT "{"
						{
							anim_BeginScript();
						}
						anim_script "}"
						{
							anim_EndScript();
						}
						;

anim_script:			anim_script anim_state |
						anim_state
						;

anim_state:				INTEGER INTEGER INTEGER INTEGER INTEGER INTEGER INTEGER INTEGER INTEGER INTEGER
						{
							vecPos.x   = $2;
							vecPos.y   = $3;
							vecPos.z   = $4;
							vecRot.x   = $5;
							vecRot.y   = $6;
							vecRot.z   = $7;
							vecScale.x = $8;
							vecScale.y = $9;
							vecScale.z = $10;
							anim_AddFrameToAnim( $1, vecPos, vecRot, vecScale );
						}
						;

%%

/***************************************************************************/
/* A simple error reporting routine */

void audp_error(char *pMessage,...)
{
	int		line;
	char	*pText;
	char	aTxtBuf[1024];
	va_list	args;

	va_start(args, pMessage);

	vsprintf(aTxtBuf, pMessage, args);
	parseGetErrorData( &line, &pText );
	DBERROR(("RES file parse error:\n%s at line %d\nToken: %d, Text: '%s'\n",
			  aTxtBuf, line, audp_char, pText));

	va_end(args);
}

/***************************************************************************/
/* Read a resource file */

BOOL ParseResourceFile( UBYTE *pData, UDWORD fileSize )
{
	// Tell lex about the input buffer
	parserSetInputBuffer( pData, fileSize );

	audp_parse();

	return TRUE;
}

/***************************************************************************/
