/*
 * clParse.h
 *
 * All the command line values
 *
 */
#ifndef _clparse_h
#define _clparse_h

#define MAX_MODS 100

// whether to play the intro video
extern BOOL	clIntroVideo;
// parse the commandline
extern BOOL ParseCommandLine( int argc, char** argv );
BOOL ParseCommandLineEarly(int argc, char** argv);

#endif


