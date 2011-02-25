PKG_NAME:=fontconfig
PKG_VERSION:=2.8.0

PKG_SOURCEBASE=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_SOURCEBASE).tar.gz
PKG_SOURCE_URL:= \
	http://fontconfig.org/release/ \
	http://wzhost.wz2100.net/~buildbot/dev/
PKG_MD5SUM:=77e15a92006ddc2adbb06f840d591c0e

TARGETS:= \
	$(TOPDIR)/build/libs/etc/fonts/fonts.conf.wd_enable \
	$(TOPDIR)/build/libs/etc/fonts/fonts.conf.wd_disable \
	$(TOPDIR)/build/libs/lib/libfontconfig.la

include $(TOPDIR)/rules.mk

$(PKG_SOURCEBASE) extract-stamp: $(DOWNLOADS)/$(PKG_SOURCE)
	tar xzf $^
	touch extract-stamp

$(PKG_SOURCEBASE)/config.status: extract-stamp
	cd $(PKG_SOURCEBASE) && ./configure --host=$(HOST_TRIPLET) --enable-static --disable-shared --prefix=`pwd`/../$(TOPDIR)/build/libs CPPFLAGS=-I`pwd`/../$(TOPDIR)/build/libs/include LDFLAGS=-L`pwd`/../$(TOPDIR)/build/libs/lib --disable-docs --with-arch=$(HOST_TRIPLET)

$(PKG_SOURCEBASE)/src/libfontconfig.la: $(PKG_SOURCEBASE)/config.status
	$(MAKE) -C $(PKG_SOURCEBASE) all || $(MAKE) -C $(PKG_SOURCEBASE) all

$(TOPDIR)/build/libs/lib/libfontconfig.la: $(PKG_SOURCEBASE)/src/libfontconfig.la
	$(MAKE) -C $(PKG_SOURCEBASE) install-configDATA install-pkgconfigDATA install-exec
	$(MAKE) -C $(PKG_SOURCEBASE)/fontconfig install

$(TOPDIR)/build/libs/etc/fonts:
	@test -d $@ || { \
	    echo mkdir -p $@ ; \
	    mkdir -p $@ || exit ; \
	}

$(TOPDIR)/build/libs/etc/fonts/fonts.conf.wd_enable:  $(TOPDIR)/../pkg/nsis/fonts.conf.wd_enable  $(TOPDIR)/build/libs/etc/fonts
	cp -p $< $@

$(TOPDIR)/build/libs/etc/fonts/fonts.conf.wd_disable: $(TOPDIR)/../pkg/nsis/fonts.conf.wd_disable $(TOPDIR)/build/libs/etc/fonts
	cp -p $< $@

all: build
build: $(TARGETS)

clean:
	$(RM) -r $(PKG_SOURCEBASE) extract-stamp

.PHONY: all build clean
