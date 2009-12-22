PKG_NAME:=devpkg_tarbomb
PKG_VERSION:=2.3

PKG_SOURCEBASE=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_SOURCEBASE).tar.bz2
PKG_SOURCE_URL:= \
	http://download.gna.org/warzone/development/devpkg/ \
	http://www.il.fontys.nl/~giel/warzone/devpkg/
PKG_MD5SUM:=6afc9f975d217fd372ddcd7bbf8e1671

include $(TOPDIR)/rules.mk

extract-stamp: $(TOPDIR)/downloads/$(PKG_SOURCE)
	@test -d ../../build/libs || { \
	    echo mkdir -p ../../build/libs ; \
	    mkdir -p ../../build/libs || exit ; \
	}
	tar -x -j -C $(TOPDIR)/build/libs -f $^
	touch extract-stamp

all: build
build: extract-stamp

clean:
	$(RM) extract-stamp

.PHONY: all build clean
