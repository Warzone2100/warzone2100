/*
 * clParse.h
 *
 * All the command line values
 *
 */
#ifndef _clparse_h
#define _clparse_h

// whether to start windowed
extern BOOL	clStartWindowed;
// whether to play the intro video
extern BOOL	clIntroVideo;
// parse the commandline
extern BOOL ParseCommandLine( LPSTR psCmdLineBOOL, BOOL bGlideDllPresent);


#endif


