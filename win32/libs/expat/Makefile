PKG_NAME:=expat
PKG_VERSION=2.0.1

PKG_SOURCEBASE:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_SOURCEBASE).tar.gz
PKG_SOURCE_URL:= \
	@SF/expat \
	http://wzhost.wz2100.net/~buildbot/dev/
PKG_MD5SUM:=ee8b492592568805593f81f8cdf2a04c

TARGET:=$(TOPDIR)/builds/libs/lib/libexpat.la

include $(TOPDIR)/rules.mk

extract-stamp: $(DOWNLOADS)/$(PKG_SOURCE)
	tar xzf $^
	touch extract-stamp

$(PKG_SOURCEBASE)/config.status: extract-stamp
	cd $(PKG_SOURCEBASE) && ./configure --host=$(HOST_TRIPLET) --enable-static --disable-shared --prefix=`pwd`/../$(TOPDIR)/build/libs

$(PKG_SOURCEBASE)/libexpat.la: $(PKG_SOURCEBASE)/config.status
	$(MAKE) -C $(PKG_SOURCEBASE) libexpat.la

$(TARGET): $(PKG_SOURCEBASE)/libexpat.la
	$(MAKE) -C $(PKG_SOURCEBASE) installlib

all: build
build: $(TARGET)

clean:
	$(RM) -rf $(PKG_SOURCEBASE) extract-stamp

.PHONY: all build clean
