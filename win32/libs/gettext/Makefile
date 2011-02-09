PKG_NAME:=gettext
PKG_VERSION:=0.18.1.1

PKG_SOURCEBASE=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_SOURCEBASE).tar.gz
PKG_SOURCE_URL:= \
	http://ftp.gnu.org/pub/gnu/gettext/ \
	http://wzhost.wz2100.net/~buildbot/dev/
PKG_MD5SUM:=3dd55b952826d2b32f51308f2f91aa89

TARGET:=$(TOPDIR)/build/libs/lib/libintl.la
SHELL:=$(shell which bash)

include $(TOPDIR)/rules.mk

$(PKG_SOURCEBASE) extract-stamp: $(DOWNLOADS)/$(PKG_SOURCE)
	tar xzf $^
	touch extract-stamp

$(PKG_SOURCEBASE)/config.status: extract-stamp
	cd $(PKG_SOURCEBASE) && "$(SHELL)" ./configure --host=$(HOST_TRIPLET) --enable-static --disable-shared --prefix=`pwd`/../$(TOPDIR)/build/libs --disable-java

$(PKG_SOURCEBASE)/gettext-runtime/intl/libintl.la: $(PKG_SOURCEBASE)/config.status
	env SHELL="$(SHELL)" $(MAKE) SHELL="$(SHELL)" -C $(PKG_SOURCEBASE)/gettext-runtime/intl libintl.la

$(TARGET): $(PKG_SOURCEBASE)/gettext-runtime/intl/libintl.la
	env SHELL="$(SHELL)" $(MAKE) SHELL="$(SHELL)" -C $(PKG_SOURCEBASE)/gettext-runtime/intl install-exec

all: build
build: $(TARGET)

clean:
	$(RM) -r $(PKG_SOURCEBASE) extract-stamp

.PHONY: all build clean
