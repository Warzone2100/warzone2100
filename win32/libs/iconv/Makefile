PKG_NAME:=libiconv
PKG_VERSION:=1.13.1

PKG_SOURCEBASE=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_SOURCEBASE).tar.gz
PKG_SOURCE_URL:= \
	http://ftp.gnu.org/pub/gnu/libiconv/ \
	http://wzhost.wz2100.net/~buildbot/dev/
PKG_MD5SUM:=7ab33ebd26687c744a37264a330bbe9a

TARGET:=$(TOPDIR)/build/libs/lib/libiconv.la

include $(TOPDIR)/rules.mk

$(PKG_SOURCEBASE) extract-stamp: $(DOWNLOADS)/$(PKG_SOURCE)
	tar xzf $^
	touch extract-stamp

$(PKG_SOURCEBASE)/config.status: extract-stamp
	cd $(PKG_SOURCEBASE) && ./configure --host=$(HOST_TRIPLET) --enable-static --disable-shared --prefix=`pwd`/../$(TOPDIR)/build/libs

$(PKG_SOURCEBASE)/lib/libiconv.la: $(PKG_SOURCEBASE)/config.status
	$(MAKE) -C $(PKG_SOURCEBASE)

$(TARGET): $(PKG_SOURCEBASE)/lib/libiconv.la
	$(MAKE) -C $(PKG_SOURCEBASE) install-lib

all: build
build: $(TARGET)

clean:
	$(RM) -r $(PKG_SOURCEBASE) extract-stamp

.PHONY: all build clean
