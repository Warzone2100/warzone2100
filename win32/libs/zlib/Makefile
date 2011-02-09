PKG_NAME:=zlib
PKG_VERSION=1.2.5

PKG_SOURCEBASE:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_SOURCEBASE).tar.gz
PKG_SOURCE_URL:= \
	@SF/libpng \
	http://wzhost.wz2100.net/~buildbot/dev/
PKG_MD5SUM:=c735eab2d659a96e5a594c9e8541ad63

TARGETS:=$(TOPDIR)/build/libs/lib/libz.a $(TOPDIR)/build/libs/include/zconf.h $(TOPDIR)/build/libs/include/zlib.h

include $(TOPDIR)/rules.mk

extract-stamp: $(DOWNLOADS)/$(PKG_SOURCE)
	tar xzf $^
	rm $(PKG_SOURCEBASE)/Makefile
	touch extract-stamp

$(PKG_SOURCEBASE)/configure: extract-stamp

$(PKG_SOURCEBASE)/Makefile: $(PKG_SOURCEBASE)/configure
	cd $(PKG_SOURCEBASE) && CC=$(HOST_TRIPLET)-gcc RANLIB=$(HOST_TRIPLET)-ranlib AR='$(HOST_TRIPLET)-ar' ./configure

$(PKG_SOURCEBASE)/libz.a: extract-stamp $(PKG_SOURCEBASE)/Makefile
	$(MAKE) -C $(PKG_SOURCEBASE) libz.a

$(PKG_SOURCEBASE)/zconf.h: $(PKG_SOURCEBASE)/Makefile
$(PKG_SOURCEBASE)/zlib.h: extract-stamp

$(TOPDIR)/build/libs/lib/libz.a: $(PKG_SOURCEBASE)/libz.a
	cp -p $< $@

$(TOPDIR)/build/libs/include/zconf.h: $(PKG_SOURCEBASE)/zconf.h
	cp -p $< $@

$(TOPDIR)/build/libs/include/zlib.h: $(PKG_SOURCEBASE)/zlib.h
	cp -p $< $@

all: build
build: $(TARGETS)

clean:
	$(RM) -rf $(PKG_SOURCEBASE) extract-stamp

.PHONY: all build clean
