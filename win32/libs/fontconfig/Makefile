PKG_NAME:=fontconfig
PKG_VERSION:=2.8.0

PKG_SOURCEBASE=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_SOURCEBASE).tar.gz
PKG_SOURCE_URL:=http://fontconfig.org/release/
PKG_MD5SUM:=77e15a92006ddc2adbb06f840d591c0e

include $(TOPDIR)/rules.mk

$(PKG_SOURCEBASE) extract-stamp: $(TOPDIR)/downloads/$(PKG_SOURCE)
	tar xzf $^
	touch extract-stamp

$(PKG_SOURCEBASE)/config.status: extract-stamp
	cd $(PKG_SOURCEBASE) && ./configure --host=$(HOST_TRIPLET) --enable-static --disable-shared --prefix=`pwd`/../$(TOPDIR)/build/libs CPPFLAGS=-I`pwd`/../$(TOPDIR)/build/libs/include LDFLAGS=-L`pwd`/../$(TOPDIR)/build/libs/lib --disable-docs --with-arch=$(HOST_TRIPLET)

$(PKG_SOURCEBASE)/src/libfontconfig.la: $(PKG_SOURCEBASE)/config.status
	$(MAKE) -C $(PKG_SOURCEBASE) all || $(MAKE) -C $(PKG_SOURCEBASE) all
	$(MAKE) -C $(PKG_SOURCEBASE) install-configDATA install-pkgconfigDATA install-exec
	$(MAKE) -C $(PKG_SOURCEBASE)/fontconfig install

all: build
build: $(PKG_SOURCEBASE)/src/libfontconfig.la

clean:
	$(RM) -r $(PKG_SOURCEBASE) extract-stamp

.PHONY: all build clean
