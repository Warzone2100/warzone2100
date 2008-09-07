#ifndef __INCLUDED_LIB_BETAWIDGET_LUA_WRAP_H__
#define __INCLUDED_LIB_BETAWIDGET_LUA_WRAP_H__

#if defined(__cplusplus)
extern "C"
{
#endif

#include <lua.h>

#if (__GNUC__ >= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#  ifndef GCC_HASCLASSVISIBILITY
#    define GCC_HASCLASSVISIBILITY
#  endif
#endif

#ifndef SWIGEXPORT
# if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
#   if defined(STATIC_LINKED)
#     define SWIGEXPORT
#   else
#     define SWIGEXPORT __declspec(dllimport)
#   endif
# else
#   if defined(__GNUC__) && defined(GCC_HASCLASSVISIBILITY)
#     define SWIGEXPORT __attribute__ ((visibility("default")))
#   else
#     define SWIGEXPORT
#   endif
# endif
#endif

SWIGEXPORT int luaopen_betawidget(lua_State* L);

#if defined(__cplusplus)
}
#endif

#endif // __INCLUDED_LIB_BETAWIDGET_LUA_WRAP_H__
