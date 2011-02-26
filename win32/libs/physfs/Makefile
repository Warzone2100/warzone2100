PKG_NAME:=physfs
PKG_VERSION=2.0.2

PKG_SOURCEBASE:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_SOURCEBASE).tar.gz
PKG_SOURCE_URL:= \
	http://icculus.org/physfs/downloads/
PKG_MD5SUM:=4e8927c3d30279b03e2592106eb9184a

TARGET:=$(TOPDIR)/build/libs/lib/libphysfs.a

include $(TOPDIR)/rules.mk

extract-stamp: $(DOWNLOADS)/$(PKG_SOURCE)
	tar xvzf $^
	touch extract-stamp

$(PKG_SOURCEBASE)/Makefile: extract-stamp
	cd $(PKG_SOURCEBASE) && cmake -DCMAKE_TOOLCHAIN_FILE=`pwd`/../$(TOPDIR)/Toolchain-mingw32.cmake -DCMAKE_INSTALL_PREFIX=`pwd`/../$(TOPDIR)/build/libs -DPHYSFS_BUILD_SHARED=FALSE -DPHYSFS_INTERNAL_ZLIB=FALSE -DZLIB_INCLUDE_DIR=`pwd`/../$(TOPDIR)/build/libs/include -DZLIB_LIBRARY=`pwd`/../$(TOPDIR)/build/libs/lib/libz.a -DPHYSFS_BUILD_TEST=FALSE -DPHYSFS_BUILD_WX_TEST=FALSE .

$(TARGET): $(PKG_SOURCEBASE)/Makefile
	$(MAKE) -C $(PKG_SOURCEBASE) $(MAKE_FLAGS) install

all: build
build: $(TARGET)

clean:
	$(MAKE) -C $(PKG_SOURCEBASE) clean

.PHONY: all build clean
