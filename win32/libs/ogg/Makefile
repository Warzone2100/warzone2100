PKG_NAME:=libogg
PKG_VERSION:=1.1.4

PKG_SOURCEBASE=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_SOURCEBASE).tar.gz
PKG_SOURCE_URL:=http://downloads.xiph.org/releases/ogg/
PKG_MD5SUM:=10200ec22543841d9d1c23e0aed4e5e9

include $(TOPDIR)/rules.mk

$(PKG_SOURCEBASE) extract-stamp: $(TOPDIR)/downloads/$(PKG_SOURCE)
	tar xzf $^
	@echo autoconf ; \
	cd $(PKG_SOURCEBASE) && autoconf || $(RM) -r $(PKG_SOURCEBASE)
	touch extract-stamp

$(PKG_SOURCEBASE)/config.status: extract-stamp
	cd $(PKG_SOURCEBASE) && ./configure --host=$(HOST_TRIPLET) --enable-static --disable-shared --prefix=`pwd`/../$(TOPDIR)/build/libs

$(PKG_SOURCEBASE)/src/libogg.la: $(PKG_SOURCEBASE)/config.status
	$(MAKE) -C $(PKG_SOURCEBASE) install

all: build
build: $(PKG_SOURCEBASE)/src/libogg.la

clean:
	$(RM) -r $(PKG_SOURCEBASE) extract-stamp

.PHONY: all build clean
