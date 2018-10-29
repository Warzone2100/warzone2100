# for 3rdparty projects that don't support automake, provide support here

AM_CPPFLAGS = $(WZ_CPPFLAGS)
AM_CFLAGS = $(WZ_CFLAGS) $(WZ_C99FLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS)

# sha2

noinst_LIBRARIES = sha2/libsha2.a
noinst_HEADERS = \
	sha2/brg_endian.h \
	sha2/brg_types.h \
	sha2/sha2.h

sha2_libsha2_a_SOURCES = \
	sha2/sha2.c


# utf8proc

utf8proc_libutf8proc_a_CFLAGS = $(WZ_CFLAGS) -fPIC -std=c99 -Wall -pedantic -I$(srcdir)/utf8proc -DUTF8PROC_EXPORTS $(UTF8PROC_DEFINES)

noinst_LIBRARIES += utf8proc/libutf8proc.a
noinst_HEADERS += \
	utf8proc/utf8proc.h

utf8proc_libutf8proc_a_SOURCES = \
	utf8proc/utf8proc.c


# miniupnpc

miniupnp_libminiupnpc_a_CFLAGS = $(WZ_CFLAGS) -DSTATICLIB -DMINIUPNP_STATICLIB -Wall -W -Wstrict-prototypes -fno-common -DMINIUPNPC_SET_SOCKET_TIMEOUT -DMINIUPNPC_GET_SRC_ADDR -D_BSD_SOURCE -D_DEFAULT_SOURCE -std=c99 -I$(srcdir)/miniupnp/miniupnpc -I$(builddir)/miniupnp/miniupnpc

MINIUPNPC_SH = /bin/sh
MINIUPNPC_VERSION = $(shell cat miniupnp/miniupnpc/VERSION)

$(builddir)/miniupnp/miniupnpc/miniupnpcstrings.h: miniupnp/miniupnpc/miniupnpcstrings.h.in miniupnp/miniupnpc/updateminiupnpcstrings.sh miniupnp/miniupnpc/VERSION
	if [ "$(srcdir)" != "$(builddir)" ]; then \
		cp "$(srcdir)/miniupnp/miniupnpc/miniupnpcstrings.h.in" "$(builddir)/miniupnp/miniupnpc/miniupnpcstrings.h.in"; \
		cp "$(srcdir)/miniupnp/miniupnpc/updateminiupnpcstrings.sh" "$(builddir)/miniupnp/miniupnpc/updateminiupnpcstrings.sh"; \
		cp "$(srcdir)/miniupnp/miniupnpc/VERSION" "$(builddir)/miniupnp/miniupnpc/VERSION"; \
	fi; \
	cd "$(builddir)/miniupnp/miniupnpc" && \
	$(MINIUPNPC_SH) updateminiupnpcstrings.sh

BUILT_SOURCES = $(builddir)/miniupnp/miniupnpc/miniupnpcstrings.h

noinst_LIBRARIES += miniupnp/libminiupnpc.a
noinst_HEADERS += \
	miniupnp/miniupnpc/codelength.h \
	miniupnp/miniupnpc/connecthostport.h \
	miniupnp/miniupnpc/igd_desc_parse.h \
	miniupnp/miniupnpc/minisoap.h \
	miniupnp/miniupnpc/minissdpc.h \
	miniupnp/miniupnpc/miniupnpc.h \
	miniupnp/miniupnpc/miniupnpc_declspec.h \
	miniupnp/miniupnpc/miniupnpc_socketdef.h \
	miniupnp/miniupnpc/miniupnpctypes.h \
	miniupnp/miniupnpc/miniwget.h \
	miniupnp/miniupnpc/minixml.h \
	miniupnp/miniupnpc/portlistingparse.h \
	miniupnp/miniupnpc/receivedata.h \
	miniupnp/miniupnpc/upnpcommands.h \
	miniupnp/miniupnpc/upnpdev.h \
	miniupnp/miniupnpc/upnperrors.h \
	miniupnp/miniupnpc/upnpreplyparse.h

miniupnp_libminiupnpc_a_SOURCES = \
	miniupnp/miniupnpc/connecthostport.c \
	miniupnp/miniupnpc/igd_desc_parse.c \
	miniupnp/miniupnpc/minisoap.c \
	miniupnp/miniupnpc/minissdpc.c \
	miniupnp/miniupnpc/miniupnpc.c \
	miniupnp/miniupnpc/miniwget.c \
	miniupnp/miniupnpc/minixml.c \
	miniupnp/miniupnpc/portlistingparse.c \
	miniupnp/miniupnpc/receivedata.c \
	miniupnp/miniupnpc/upnpcommands.c \
	miniupnp/miniupnpc/upnpdev.c \
	miniupnp/miniupnpc/upnperrors.c \
	miniupnp/miniupnpc/upnpreplyparse.c

nodist_miniupnp_libminiupnpc_a_SOURCES = $(builddir)/miniupnp/miniupnpc/miniupnpcstrings.h


EXTRA_DIST = \
	utf8proc \
	miniupnp
