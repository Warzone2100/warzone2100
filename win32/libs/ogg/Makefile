PKG_NAME:=libogg
PKG_VERSION:=1.2.2

PKG_SOURCEBASE=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_SOURCEBASE).tar.gz
PKG_SOURCE_URL:= \
	http://downloads.xiph.org/releases/ogg/ \
	http://wzhost.wz2100.net/~buildbot/dev/
PKG_MD5SUM:=5a9fcabc9a1b7c6f1cd75ddc78f36c56

TARGETS:= \
	$(TOPDIR)/build/libs/include/ogg/ogg.h \
	$(TOPDIR)/build/libs/lib/libogg.la

include $(TOPDIR)/rules.mk

$(PKG_SOURCEBASE) extract-stamp: $(DOWNLOADS)/$(PKG_SOURCE)
	tar xzf $^
	@echo autoconf ; \
	cd $(PKG_SOURCEBASE) && autoconf || $(RM) -r $(PKG_SOURCEBASE)
	touch extract-stamp

$(PKG_SOURCEBASE)/config.status: extract-stamp
	cd $(PKG_SOURCEBASE) && ./configure --host=$(HOST_TRIPLET) --enable-static --disable-shared --prefix=`pwd`/../$(TOPDIR)/build/libs

$(PKG_SOURCEBASE)/src/libogg.la: $(PKG_SOURCEBASE)/config.status
	$(MAKE) -C $(PKG_SOURCEBASE)/src libogg.la

$(PKG_SOURCEBASE)/include/ogg/ogg.h: extract-stamp

$(TOPDIR)/build/libs/include/ogg/ogg.h: $(PKG_SOURCEBASE)/include/ogg/ogg.h $(PKG_SOURCEBASE)/config.status
	$(MAKE) -C $(PKG_SOURCEBASE)/include install

$(TOPDIR)/build/libs/lib/libogg.la: $(PKG_SOURCEBASE)/src/libogg.la
	$(MAKE) -C $(PKG_SOURCEBASE)/src install

all: build
build: $(TARGETS)

clean:
	$(RM) -r $(PKG_SOURCEBASE) extract-stamp

.PHONY: all build clean
