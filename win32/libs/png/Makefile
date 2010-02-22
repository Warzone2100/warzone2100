PKG_NAME:=libpng
PKG_VERSION=1.2.36

PKG_SOURCEBASE:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_SOURCEBASE).tar.bz2
PKG_SOURCE_URL:= \
	@SF/libpng \
	http://www.il.fontys.nl/~giel/warzone/devpkg/
PKG_MD5SUM:=325d67a8dd4a1f672d395c339fddd75c

TARGET:=$(TOPDIR)/build/libs/lib/libpng.la

include $(TOPDIR)/rules.mk

extract-stamp: $(TOPDIR)/downloads/$(PKG_SOURCE)
	tar xjf $^
	touch extract-stamp

$(PKG_SOURCEBASE)/configure: extract-stamp

$(PKG_SOURCEBASE)/config.status: $(PKG_SOURCEBASE)/configure
	cd $(PKG_SOURCEBASE) && ./configure --host=$(HOST_TRIPLET) --prefix=`pwd`/../$(TOPDIR)/build/libs --disable-shared --enable-static CPPFLAGS=-I`pwd`/../$(TOPDIR)/build/libs/include LDFLAGS=-L`pwd`/../$(TOPDIR)/build/libs/lib

$(PKG_SOURCEBASE)/libpng.la: extract-stamp $(PKG_SOURCEBASE)/config.status
	$(MAKE) -C $(PKG_SOURCEBASE) libpng.la

$(TARGET): $(PKG_SOURCEBASE)/libpng.la
	$(MAKE) -C $(PKG_SOURCEBASE) install

build: $(TARGET)

clean:
	$(RM) -rf $(PKG_SOURCEBASE) extract-stamp
