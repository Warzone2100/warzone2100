PKG_NAME:=libpng
PKG_VERSION=1.4.4

PKG_SOURCEBASE:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_SOURCEBASE).tar.bz2
PKG_SOURCE_URL:= \
	@SF/libpng \
	http://www.il.fontys.nl/~giel/warzone/devpkg/
PKG_MD5SUM:=e6446d6cc10621ce0ed27ff82a70a7e8

TARGET:=$(TOPDIR)/build/libs/lib/libpng14.la

include $(TOPDIR)/rules.mk

extract-stamp: $(TOPDIR)/downloads/$(PKG_SOURCE)
	tar xjf $^
	touch extract-stamp

$(PKG_SOURCEBASE)/configure: extract-stamp

$(PKG_SOURCEBASE)/config.status: $(PKG_SOURCEBASE)/configure
	cd $(PKG_SOURCEBASE) && ./configure --host=$(HOST_TRIPLET) --prefix=`pwd`/../$(TOPDIR)/build/libs --disable-shared --enable-static CPPFLAGS=-I`pwd`/../$(TOPDIR)/build/libs/include LDFLAGS=-L`pwd`/../$(TOPDIR)/build/libs/lib

$(PKG_SOURCEBASE)/libpng14.la: extract-stamp $(PKG_SOURCEBASE)/config.status
	$(MAKE) -C $(PKG_SOURCEBASE) libpng14.la

$(TARGET): $(PKG_SOURCEBASE)/libpng14.la
	$(MAKE) -C $(PKG_SOURCEBASE) install

build: $(TARGET)

clean:
	$(RM) -rf $(PKG_SOURCEBASE) extract-stamp
