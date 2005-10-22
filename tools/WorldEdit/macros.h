#ifndef _INCLUDED_MACROS_
#define _INCLUDED_MACROS_

#define DXCALL(func) if( (ddrval = func) != DD_OK) { DisplayError(ddrval,__FILE__,__LINE__,0); DebugClose(); return FALSE; }
#define DXCALL_INT(func) if( (ddrval = func) != DD_OK) { DisplayError(ddrval,__FILE__,__LINE__,0); DebugClose(); return -1; }
#define DXCALL_HRESULT(func) if( (ddrval = func) != DD_OK) { DisplayError(ddrval,__FILE__,__LINE__,0); DebugClose(); return ddrval; }

#define DXCALL_RESTORE(func) if( (ddrval = func) != DD_OK) \
{ \
	if(ddrval == DDERR_SURFACELOST) { \
		RestoreSurfaces(); \
	} else { \
		DisplayError(ddrval,__FILE__,__LINE__,0); \
		DebugClose(); \
		return FALSE; \
	} \
}

#define RELEASE(x) if (x) { x->Release(); x = NULL; }

#define DELOBJ(x) if (x) { delete x; x = NULL; }

#endif
