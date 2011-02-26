PKG_NAME:=devpkg_tarbomb
PKG_VERSION:=2.3

PKG_SOURCEBASE=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_SOURCEBASE).tar.bz2
PKG_SOURCE_URL:= \
	http://download.gna.org/warzone/development/devpkg/ \
	http://wzhost.wz2100.net/~buildbot/dev/
PKG_MD5SUM:=6a3a37022ffc3c0a573ad2d6c4766b6d

include $(TOPDIR)/rules.mk

extract-stamp: $(DOWNLOADS)/$(PKG_SOURCE)
	@test -d $(TOPDIR)/build/libs || { \
	    echo mkdir -p $(TOPDIR)/build/libs ; \
	    mkdir -p $(TOPDIR)/build/libs || exit ; \
	}
	tar -x --keep-newer-files -j -C $(TOPDIR)/build/libs -f $^
	touch extract-stamp

all: build
build: extract-stamp

clean:
	$(RM) extract-stamp

.PHONY: all build clean
