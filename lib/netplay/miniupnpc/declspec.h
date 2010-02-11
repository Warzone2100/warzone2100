#ifndef __DECLSPEC_H__
#define __DECLSPEC_H__

#if defined(WIN32) && !defined(STATICLIB)
	#ifdef MINIUPNP_EXPORTS
		#define LIBSPEC __declspec(dllexport)
	#else
// NOTE: with the cross compiler, even if we have -DSTATICLIB, for some odd reason it still does this
// #define LIBSPEC __declspec(dllimport)  which is not what we want.  So we hack the line like so.
		#define LIBSPEC 
	#endif
#else
	#define LIBSPEC
#endif

#endif

