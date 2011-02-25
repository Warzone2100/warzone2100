PKG_NAME:=libvorbis
PKG_VERSION:=1.3.2

PKG_SOURCEBASE=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_SOURCEBASE).tar.gz
PKG_SOURCE_URL:= \
	http://downloads.xiph.org/releases/vorbis/ \
	http://wzhost.wz2100.net/~buildbot/dev/
PKG_MD5SUM:=c870b9bd5858a0ecb5275c14486d9554

TARGETS:= \
	$(TOPDIR)/build/libs/include/vorbis/vorbisfile.h \
	$(TOPDIR)/build/libs/lib/libvorbisfile.la

include $(TOPDIR)/rules.mk

$(PKG_SOURCEBASE) extract-stamp: $(DOWNLOADS)/$(PKG_SOURCE)
	tar xzf $^
	touch extract-stamp

$(PKG_SOURCEBASE)/config.status: extract-stamp
	cd $(PKG_SOURCEBASE) && ./configure --host=$(HOST_TRIPLET) --enable-static --disable-shared --prefix=`pwd`/../$(TOPDIR)/build/libs CPPFLAGS=-I`pwd`/../$(TOPDIR)/build/libs/include LDFLAGS=-L`pwd`/../$(TOPDIR)/build/libs/lib --disable-oggtest --disable-docs

$(PKG_SOURCEBASE)/lib/libvorbisfile.la: $(PKG_SOURCEBASE)/config.status
	$(MAKE) -C $(PKG_SOURCEBASE)/lib libvorbisfile.la

$(PKG_SOURCEBASE)/include/vorbis/vorbisfile.h: extract-stamp

$(TOPDIR)/build/libs/include/vorbis/vorbisfile.h: $(PKG_SOURCEBASE)/include/vorbis/vorbisfile.h $(PKG_SOURCEBASE)/config.status
	$(MAKE) -C $(PKG_SOURCEBASE)/include install

$(TOPDIR)/build/libs/lib/libvorbisfile.la: $(PKG_SOURCEBASE)/lib/libvorbisfile.la
	$(MAKE) -C $(PKG_SOURCEBASE)/lib install

all: build
build: $(TARGETS)

clean:
	$(RM) -r $(PKG_SOURCEBASE) extract-stamp

.PHONY: all build clean
