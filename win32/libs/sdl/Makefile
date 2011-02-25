PKG_NAME:=SDL
PKG_VERSION:=1.2.14

PKG_SOURCEBASE=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_SOURCEBASE).tar.gz
PKG_SOURCE_URL:= \
	http://www.libsdl.org/release/ \
	http://wzhost.wz2100.net/~buildbot/dev/
PKG_MD5SUM:=e52086d1b508fa0b76c52ee30b55bec4

TARGETS:= \
	$(TOPDIR)/build/libs/include/SDL/SDL.h \
	$(TOPDIR)/build/libs/lib/pkgconfig/sdl.pc \
	$(TOPDIR)/build/libs/lib/libSDL.la

include $(TOPDIR)/rules.mk

$(PKG_SOURCEBASE) extract-stamp: $(DOWNLOADS)/$(PKG_SOURCE)
	tar xzf $^
	touch extract-stamp

patch-stamp: extract-stamp
	@for patch in patches/*.diff; do \
	    echo sed -e "s/\$$sourcebase\b/$(PKG_SOURCEBASE)/g" -e "s/\$$host_triplet\b/$(HOST_TRIPLET)/g" "$$patch" \| patch -p0 ; \
	    sed -e "s/\$$sourcebase\b/$(PKG_SOURCEBASE)/g" -e "s/\$$host_triplet\b/$(HOST_TRIPLET)/g" "$$patch" | patch -p0 || exit ; \
	done
	touch patch-stamp

$(PKG_SOURCEBASE)/config.status: patch-stamp
	cd $(PKG_SOURCEBASE) && ./configure --host=$(HOST_TRIPLET) --enable-static --disable-shared --prefix=`pwd`/../$(TOPDIR)/build/libs --disable-audio --disable-cdrom

$(PKG_SOURCEBASE)/build/libSDL.la: $(PKG_SOURCEBASE)/config.status
	$(MAKE) -C $(PKG_SOURCEBASE) build

$(PKG_SOURCEBASE)/include/SDL.h $(PKG_SOURCEBASE)/sdl.pc: extract-stamp

$(TOPDIR)/build/libs/include/SDL/SDL.h: $(PKG_SOURCEBASE)/include/SDL.h $(PKG_SOURCEBASE)/config.status
	$(MAKE) -C $(PKG_SOURCEBASE) install-hdrs

$(TOPDIR)/build/libs/lib/pkgconfig/sdl.pc: $(PKG_SOURCEBASE)/sdl.pc $(PKG_SOURCEBASE)/config.status
	$(MAKE) -C $(PKG_SOURCEBASE) install-data

$(TOPDIR)/build/libs/lib/libSDL.la: $(PKG_SOURCEBASE)/build/libSDL.la
	$(MAKE) -C $(PKG_SOURCEBASE) install-lib

all: build
build: $(TARGETS)

clean:
	$(RM) -r $(PKG_SOURCEBASE) extract-stamp patch-stamp

.PHONY: all build clean
