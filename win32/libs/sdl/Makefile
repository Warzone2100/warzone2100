PKG_NAME:=SDL
PKG_VERSION:=1.2.14

PKG_SOURCEBASE=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_SOURCEBASE).tar.gz
PKG_SOURCE_URL:=http://www.libsdl.org/release/
PKG_MD5SUM:=e52086d1b508fa0b76c52ee30b55bec4

include $(TOPDIR)/rules.mk

$(PKG_SOURCEBASE) extract-stamp: $(TOPDIR)/downloads/$(PKG_SOURCE)
	tar xzf $^
	touch extract-stamp

$(PKG_SOURCEBASE)/config.status: extract-stamp
	cd $(PKG_SOURCEBASE) && ./configure --host=$(HOST_TRIPLET) --enable-static --disable-shared --prefix=`pwd`/../$(TOPDIR)/build/libs --disable-audio --disable-cdrom

$(PKG_SOURCEBASE)/build/libSDL.la $(PKG_SOURCEBASE)/build/libSDLmain.a: $(PKG_SOURCEBASE)/config.status
	$(MAKE) -C $(PKG_SOURCEBASE) install-lib install-hdrs install-data

all: build
build: $(PKG_SOURCEBASE)/build/libSDL.la $(PKG_SOURCEBASE)/build/libSDLmain.a

clean:
	$(RM) -r $(PKG_SOURCEBASE) extract-stamp

.PHONY: all build clean
