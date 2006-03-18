#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && (__GNUC__ - 0 > 3 || (__GNUC__ - 0 == 3 && __GNUC_MINOR__ - 0 >= 2))
#  define WZ_DEPRECATED __attribute__ ((__deprecated__))
#elif defined(_MSVC_VER) && (_MSC_VER >= 1300)
#  define WZ_DEPRECATED __declspec(deprecated)
#else
#  define WZ_DEPRECATED
#endif
