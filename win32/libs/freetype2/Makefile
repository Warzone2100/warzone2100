PKG_NAME:=freetype
PKG_VERSION=2.3.11

PKG_SOURCEBASE:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_SOURCEBASE).tar.bz2
PKG_SOURCE_URL:= \
	@SF/freetype \
	http://www.il.fontys.nl/~giel/warzone/devpkg/
PKG_MD5SUM:=519c7cbf5cbd72ffa822c66844d3114c

TARGET:=$(TOPDIR)/build/libs/lib/libfreetype.la

include $(TOPDIR)/rules.mk

extract-stamp: $(TOPDIR)/downloads/$(PKG_SOURCE)
	tar xjf $^
	touch extract-stamp

$(PKG_SOURCEBASE)/builds/unix/config.status: extract-stamp
	cd $(PKG_SOURCEBASE) && ./configure --host=$(HOST_TRIPLET) --enable-static --disable-shared --prefix=`pwd`/../$(TOPDIR)/build/libs

$(TARGET): $(PKG_SOURCEBASE)/builds/unix/config.status
	$(MAKE) -C $(PKG_SOURCEBASE) install

all: build
build: $(TARGET)

clean:
	$(RM) -rf $(PKG_SOURCEBASE) extract-stamp

.PHONY: all build clean
