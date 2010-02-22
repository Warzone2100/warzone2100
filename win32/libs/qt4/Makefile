PKG_NAME:=qt
PKG_VERSION:=4.6.2

PKG_SOURCEBASE:=qt-everywhere-opensource-src-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_SOURCEBASE).tar.gz
PKG_SOURCE_URL:= \
	http://get.qt.nokia.com/qt/source/ \
	http://www.il.fontys.nl/~giel/warzone/devpkg/
PKG_MD5SUM:=eb651ee4b157c01696aa56777fc6e0e5

TARGETS:= \
	extract-stamp

include $(TOPDIR)/rules.mk

$(PKG_SOURCEBASE) extract-stamp: $(TOPDIR)/downloads/$(PKG_SOURCE)
	tar xzf $^
	touch extract-stamp

all: build
build: $(TARGETS)

clean:
	$(RM) -r $(PKG_SOURCEBASE) extract-stamp

.PHONY: all build clean
