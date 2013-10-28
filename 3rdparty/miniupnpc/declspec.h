#ifndef DECLSPEC_H_INCLUDED
#define DECLSPEC_H_INCLUDED

#if defined(_WIN32) && !defined(STATICLIB)
	#ifdef MINIUPNP_EXPORTS
		#define LIBSPEC __declspec(dllexport)
	#else
		#define LIBSPEC __declspec(dllimport)
	#endif
#else
	#define LIBSPEC
#endif

#endif

