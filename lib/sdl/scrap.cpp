/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/** @file
 *  Handle clipboard text and data in arbitrary formats
 */

#include "lib/framework/frame.h"

#include <SDL.h>
#include <SDL_syswm.h>

#include "scrap.h"

/* System dependent data types */
#if defined(WZ_WS_X11)
/* * */
typedef Atom scrap_type;

#elif defined(WZ_WS_WIN)
/* * */
typedef UDWORD scrap_type;

#elif defined(WZ_WS_QNX)
/* * */
typedef uint32_t scrap_type;
#define Ph_CL_TEXT T('T', 'E', 'X', 'T')

#elif defined(WZ_WS_MAC)
/* * */
typedef uint32_t scrap_type;	/* FIXME */

#endif /* scrap type */

/* System dependent variables */
#if defined(WZ_WS_X11)
/* * */
static Display *SDL_Display;
static Window SDL_Window;
static void (*Lock_Display)(void);
static void (*Unlock_Display)(void);

#elif defined(WZ_WS_WIN)
/* * */
static HWND SDL_Window;

#elif defined(WZ_WS_QNX)
/* * */
static unsigned short InputGroup;

#endif /* scrap type */

#define FORMAT_PREFIX	"SDL_scrap_0x"

static scrap_type
convert_format(int type)
{
switch (type)
	{

	case T('T', 'E', 'X', 'T'):
#if defined(WZ_WS_X11)
/* * */
	return XA_STRING;

#elif defined(WZ_WS_WIN)
/* * */
	return CF_TEXT;

#elif defined(WZ_WS_QNX)
/* * */
	return Ph_CL_TEXT;

#endif /* scrap type */

	default:
	{
		char format[sizeof(FORMAT_PREFIX)+8+1];

		snprintf(format, sizeof(format), "%s%08lx", FORMAT_PREFIX, (unsigned long)type);

#if defined(WZ_WS_X11)
/* * */
		return XInternAtom(SDL_Display, format, False);

#elif defined(WZ_WS_WIN)
/* * */
		return RegisterClipboardFormatA(format);

#elif defined(WZ_WS_MAC)
/* * */
		// Meaningless value to prevent "control reaches end of non-void function" warning
		return 0;

#endif /* scrap type */
	}
	}
}

/* Convert internal data to scrap format */
static int
convert_data(int type, char *dst, char *src, int srclen)
{
	int dstlen;

	dstlen = 0;
	switch (type)
	{
	case T('T', 'E', 'X', 'T'):
	if ( dst )
		{
		while ( --srclen >= 0 )
			{
#if defined(__unix__)
			if ( *src == '\r' )
				{
				*dst++ = '\n';
				++dstlen;
				}
			else
#elif defined(__WIN32__)
			if ( *src == '\r' )
				{
				*dst++ = '\r';
				++dstlen;
				*dst++ = '\n';
				++dstlen;
				}
			else
#endif
				{
				*dst++ = *src;
				++dstlen;
				}
			++src;
			}
			*dst = '\0';
			++dstlen;
		}
	else
		{
		while ( --srclen >= 0 )
			{
#if defined(__unix__)
			if ( *src == '\r' )
				{
				++dstlen;
				}
			else
#elif defined(__WIN32__)
			if ( *src == '\r' )
				{
				++dstlen;
				++dstlen;
				}
			else
#endif
				{
				++dstlen;
				}
			++src;
			}
			++dstlen;
		}
	break;

	default:
		if ( dst )
		{
			*(int *)dst = srclen;
			dst += sizeof(int);
			memcpy(dst, src, srclen);
		}
		dstlen = sizeof(int)+srclen;
		break;
	}
	return(dstlen);
}

/* Convert scrap data to internal format */
#if (defined(WZ_WS_X11) || defined(WZ_WS_WIN) || defined(WZ_WS_QNX))
static int
convert_scrap(int type, char *dst, char *src, int srclen)
{
int dstlen;

dstlen = 0;
switch (type)
	{
	case T('T', 'E', 'X', 'T'):
	{
		if ( srclen == 0 )
		srclen = strlen(src);
		if ( dst )
		{
			while ( --srclen >= 0 )
			{
#if defined(__WIN32__)
				if ( *src == '\r' )
				/* drop extraneous '\r' */;
				else
#endif
				if ( *src == '\n' )
				{
					*dst++ = '\r';
					++dstlen;
				}
				else
				{
					*dst++ = *src;
					++dstlen;
				}
				++src;
			}
			*dst = '\0';
			++dstlen;
		}
		else
		{
			while ( --srclen >= 0 )
			{
#if defined(__WIN32__)
				if ( *src == '\r' )
				/* drop extraneous '\r' */;
				else
#endif
				++dstlen;
				++src;
			}
			++dstlen;
		}
		}
	break;

	default:
	dstlen = *(int *)src;
	if ( dst )
		{
		if ( srclen == 0 )
			memcpy(dst, src+sizeof(int), dstlen);
		else
			memcpy(dst, src+sizeof(int), srclen-sizeof(int));
		}
	break;
	}
return dstlen;
}
#endif

#if defined(WZ_WS_X11)
/* The system message filter function -- handle clipboard messages */
static int clipboard_filter(const SDL_Event *event);
#endif

int
init_scrap(void)
{
SDL_SysWMinfo info;
int retval;

/* Grab the window manager specific information */
retval = -1;
SDL_SetError("SDL is not running on known window manager");

SDL_VERSION(&info.version);
if ( SDL_GetWMInfo(&info) )
	{
	/* Save the information for later use */
#if defined(WZ_WS_X11)
/* * */
	if ( info.subsystem == SDL_SYSWM_X11 )
		{
		SDL_Display = info.info.x11.display;
		SDL_Window = info.info.x11.window;
		Lock_Display = info.info.x11.lock_func;
		Unlock_Display = info.info.x11.unlock_func;

		/* Enable the special window hook events */
		SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
		SDL_SetEventFilter(clipboard_filter);

		retval = 0;
		}
	else
		{
		SDL_SetError("SDL is not running on X11");
		}

#elif defined(WZ_WS_WIN)
/* * */
	SDL_Window = info.window;
	retval = 0;

#elif defined(WZ_WS_QNX)
/* * */
	InputGroup=PhInputGroup(NULL);
	retval = 0;

#endif /* scrap type */
	}
return(retval);
}

int lost_scrap(void)
{
	int retval = 0;

#if defined(WZ_WS_X11)
	Lock_Display();
	retval = ( XGetSelectionOwner(SDL_Display, XA_PRIMARY) != SDL_Window );
	Unlock_Display();
#elif defined(WZ_WS_WIN)
	retval = ( GetClipboardOwner() != SDL_Window );
#elif defined(WZ_WS_QNX)
	retval = ( PhInputGroup(NULL) != InputGroup );
#endif /* scrap type */
	return(retval);
}

void
put_scrap(int type, int srclen, char *src)
{
	scrap_type format;
	int dstlen;
#if (defined(WZ_WS_X11) || defined(WZ_WS_WIN) || defined(WZ_WS_QNX))
	char *dst;
#endif

	format = convert_format(type);
	dstlen = convert_data(type, NULL, src, srclen);

#if defined(WZ_WS_X11)
	dst = (char *)malloc(dstlen);
	if ( dst != NULL )
	{
		Lock_Display();
		convert_data(type, dst, src, srclen);
		XChangeProperty(SDL_Display, DefaultRootWindow(SDL_Display),
			XA_CUT_BUFFER0, format, 8, PropModeReplace, (unsigned char *)dst, dstlen);
		free(dst);
		if ( lost_scrap() )
			XSetSelectionOwner(SDL_Display, XA_PRIMARY, SDL_Window, CurrentTime);
		Unlock_Display();
	}

#elif defined(WZ_WS_WIN)
/* * */
if ( OpenClipboard(SDL_Window) )
	{
	HANDLE hMem;

	hMem = GlobalAlloc((GMEM_MOVEABLE|GMEM_DDESHARE), dstlen);
	if ( hMem != NULL )
		{
		dst = (char *)GlobalLock(hMem);
		convert_data(type, dst, src, srclen);
		GlobalUnlock(hMem);
		EmptyClipboard();
		SetClipboardData(format, hMem);
		}
	CloseClipboard();
	}

#elif defined(WZ_WS_QNX)
/* * */
#if (_NTO_VERSION < 620) /* before 6.2.0 releases */
{
	PhClipHeader clheader={Ph_CLIPBOARD_TYPE_TEXT, 0, NULL};
	int* cldata;
	int status;

	dst = (char *)malloc(dstlen+4);
	if (dst != NULL)
	{
		cldata=(int*)dst;
		*cldata=type;
		convert_data(type, dst+4, src, srclen);
		clheader.data=dst;
		if (dstlen>65535)
		{
		clheader.length=65535; /* maximum photon clipboard size :( */
		}
		else
		{
		clheader.length=dstlen+4;
		}
		status=PhClipboardCopy(InputGroup, 1, &clheader);
		if (status==-1)
		{
		fprintf(stderr, "Photon: copy to clipboard was failed !\n");
		}
		free(dst);
	}
}
#else /* 6.2.0 and 6.2.1 and future releases */
{
	PhClipboardHdr clheader={Ph_CLIPBOARD_TYPE_TEXT, 0, NULL};
	int* cldata;
	int status;

	dst = (char *)malloc(dstlen+4);
	if (dst != NULL)
	{
		cldata=(int*)dst;
		*cldata=type;
		convert_data(type, dst+4, src, srclen);
		clheader.data=dst;
		clheader.length=dstlen+4;
		status=PhClipboardWrite(InputGroup, 1, &clheader);
		if (status==-1)
		{
		fprintf(stderr, "Photon: copy to clipboard was failed !\n");
		}
		free(dst);
	}
}
#endif
#endif /* scrap type */
}

void
get_scrap(int type, int *dstlen, char **dst)
{
	scrap_type format;

	*dstlen = 0;
	format = convert_format(type);

#if defined(WZ_WS_X11)
/* * */
{
	Window owner;
	Atom selection;
	Atom seln_type;
	int seln_format;
	unsigned long nbytes;
	unsigned long overflow;
	unsigned char * src;

	Lock_Display();
	owner = XGetSelectionOwner(SDL_Display, XA_PRIMARY);
	Unlock_Display();
	if ( (owner == None) || (owner == SDL_Window) )
	{
		owner = DefaultRootWindow(SDL_Display);
		selection = XA_CUT_BUFFER0;
	}
	else
	{
		int selection_response = 0;
		SDL_Event event;

		owner = SDL_Window;
		Lock_Display();
		selection = XInternAtom(SDL_Display, "SDL_SELECTION", False);
		XConvertSelection(SDL_Display, XA_PRIMARY, format,
										selection, owner, CurrentTime);
		Unlock_Display();
		while ( ! selection_response )
		{
			SDL_WaitEvent(&event);
			if ( event.type == SDL_SYSWMEVENT )
			{
				XEvent xevent = event.syswm.msg->event.xevent;

				if ( (xevent.type == SelectionNotify) &&
					(xevent.xselection.requestor == owner) )
					selection_response = 1;
			}
		}
	}
	Lock_Display();
	if ( XGetWindowProperty(SDL_Display, owner, selection, 0, INT_MAX/4,
							False, format, &seln_type, &seln_format,
					&nbytes, &overflow, &src) == Success )
	{
		if ( seln_type == format )
		{
			*dstlen = convert_scrap(type, NULL, (char*)src, nbytes);
			*dst = (char *)realloc(*dst, *dstlen);
			if ( *dst == NULL )
				*dstlen = 0;
			else
				convert_scrap(type, *dst, (char*)src, nbytes);
		}
		XFree(src);
	}
	Unlock_Display();
}

#elif defined(WZ_WS_WIN)
/* * */
	if ( IsClipboardFormatAvailable(format) && OpenClipboard(SDL_Window) )
	{
	HANDLE hMem;
	char *src;

	hMem = GetClipboardData(format);
	if ( hMem != NULL )
		{
		src = (char *)GlobalLock(hMem);
		*dstlen = convert_scrap(type, NULL, src, 0);
		*dst = (char *)realloc(*dst, *dstlen);
		if ( *dst == NULL )
			*dstlen = 0;
		else
			convert_scrap(type, *dst, src, 0);
		GlobalUnlock(hMem);
		}
	CloseClipboard();
	}
#elif defined(WZ_WS_QNX)
/* * */
#if (_NTO_VERSION < 620) /* before 6.2.0 releases */
{
	void* clhandle;
	PhClipHeader* clheader;
	int* cldata;

	clhandle=PhClipboardPasteStart(InputGroup);
	if (clhandle!=NULL)
	{
		clheader=PhClipboardPasteType(clhandle, Ph_CLIPBOARD_TYPE_TEXT);
		if (clheader!=NULL)
		{
		cldata=clheader->data;
		if ((clheader->length>4) && (*cldata==type))
		{
			*dstlen = convert_scrap(type, NULL, (char*)clheader->data+4, clheader->length-4);
			*dst = (char *)realloc(*dst, *dstlen);
			if (*dst == NULL)
			{
				*dstlen = 0;
			}
			else
			{
				convert_scrap(type, *dst, (char*)clheader->data+4, clheader->length-4);
			}
		}
		}
		PhClipboardPasteFinish(clhandle);
	}
}
#else /* 6.2.0 and 6.2.1 and future releases */
{
	void* clhandle;
	PhClipboardHdr* clheader;
	int* cldata;

	clheader=PhClipboardRead(InputGroup, Ph_CLIPBOARD_TYPE_TEXT);
	if (clheader!=NULL)
	{
		cldata=clheader->data;
		if ((clheader->length>4) && (*cldata==type))
		{
		*dstlen = convert_scrap(type, NULL, (char*)clheader->data+4, clheader->length-4);
		*dst = (char *)realloc(*dst, *dstlen);
		if (*dst == NULL)
		{
			*dstlen = 0;
		}
		else
		{
			convert_scrap(type, *dst, (char*)clheader->data+4, clheader->length-4);
		}
		}
	}
}
#endif
#endif /* scrap type */
}

#if defined(WZ_WS_X11)
static int clipboard_filter(const SDL_Event *event)
{
	/* Post all non-window manager specific events */
	if (event->type != SDL_SYSWMEVENT)
	{
		return(1);
	}

	/* Handle window-manager specific clipboard events */
	switch (event->syswm.msg->event.xevent.type) {
	/* Copy the selection from XA_CUT_BUFFER0 to the requested property */
	case SelectionRequest: {
	XSelectionRequestEvent *req;
	XEvent sevent;
	int seln_format;
	unsigned long nbytes;
	unsigned long overflow;
	unsigned char *seln_data;

	req = &event->syswm.msg->event.xevent.xselectionrequest;
	sevent.xselection.type = SelectionNotify;
	sevent.xselection.display = req->display;
	sevent.xselection.selection = req->selection;
	sevent.xselection.target = None;
	sevent.xselection.property = None;
	sevent.xselection.requestor = req->requestor;
	sevent.xselection.time = req->time;
	if ( XGetWindowProperty(SDL_Display, DefaultRootWindow(SDL_Display),
							XA_CUT_BUFFER0, 0, INT_MAX/4, False, req->target,
							&sevent.xselection.target, &seln_format,
							&nbytes, &overflow, &seln_data) == Success )
		{
		if ( sevent.xselection.target == req->target )
			{
			if ( sevent.xselection.target == XA_STRING )
				{
				if ( seln_data[nbytes-1] == '\0' )
					--nbytes;
				}
			XChangeProperty(SDL_Display, req->requestor, req->property,
				sevent.xselection.target, seln_format, PropModeReplace,
													seln_data, nbytes);
			sevent.xselection.property = req->property;
			}
		XFree(seln_data);
		}
	XSendEvent(SDL_Display,req->requestor,False,0,&sevent);
	XSync(SDL_Display, False);
	}
	break;
}

/* Post the event for X11 clipboard reading above */
return(1);
}
#endif /* WZ_WS_X11 */
