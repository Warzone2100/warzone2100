PKG_NAME:=libtheora
PKG_VERSION:=1.1.1

PKG_SOURCEBASE=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_SOURCEBASE).tar.bz2
PKG_SOURCE_URL:= \
	http://downloads.xiph.org/releases/theora/ \
	http://wzhost.wz2100.net/~buildbot/dev/
PKG_MD5SUM:=292ab65cedd5021d6b7ddd117e07cd8e

TARGETS:= \
	$(TOPDIR)/build/libs/include/theora/theora.h \
	$(TOPDIR)/build/libs/lib/libtheora.la

include $(TOPDIR)/rules.mk

$(PKG_SOURCEBASE) extract-stamp: $(DOWNLOADS)/$(PKG_SOURCE)
	tar xjf $^
	touch extract-stamp

$(PKG_SOURCEBASE)/config.status: extract-stamp
	cd $(PKG_SOURCEBASE) && ./configure --host=$(HOST_TRIPLET) --enable-static --disable-shared --prefix=`pwd`/../$(TOPDIR)/build/libs CPPFLAGS=-I`pwd`/../$(TOPDIR)/build/libs/include LDFLAGS=-L`pwd`/../$(TOPDIR)/build/libs/lib --disable-oggtest --disable-vorbistest --disable-sdltest --disable-examples --disable-encode --disable-spec HAVE_DOXYGEN=false

$(PKG_SOURCEBASE)/lib/libtheora.la: $(PKG_SOURCEBASE)/config.status
	$(MAKE) -C $(PKG_SOURCEBASE)/lib libtheora.la

$(PKG_SOURCEBASE)/include/theora/theora.h: extract-stamp

$(TOPDIR)/build/libs/include/theora/theora.h: $(PKG_SOURCEBASE)/include/theora/theora.h $(PKG_SOURCEBASE)/config.status
	$(MAKE) -C $(PKG_SOURCEBASE)/include install

$(TOPDIR)/build/libs/lib/libtheora.la: $(PKG_SOURCEBASE)/lib/libtheora.la
	$(MAKE) -C $(PKG_SOURCEBASE)/lib install

all: build
build: $(TARGETS)

clean:
	$(RM) -r $(PKG_SOURCEBASE) extract-stamp

.PHONY: all build clean
