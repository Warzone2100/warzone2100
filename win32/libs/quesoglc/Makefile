PKG_NAME:=quesoglc
PKG_VERSION:=0.7.3~svn913

PKG_SOURCEBASE=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_SOURCEBASE).tar.gz
PKG_SOURCE_URL:= \
        @SF/warzone2100 \
        http://wzhost.wz2100.net/~buildbot/dev/
PKG_MD5SUM:=b009f87f5cc2e79106cf1bf8ca45418d

TARGETS:= \
	$(TOPDIR)/build/libs/lib/libGLC.la \
	$(TOPDIR)/build/libs/include/GL/glc.h \
	$(TOPDIR)/build/libs/lib/pkgconfig/quesoglc.pc

include $(TOPDIR)/rules.mk

extract-stamp: $(DOWNLOADS)/$(PKG_SOURCE)
	tar xzf $^
	touch extract-stamp

$(PKG_SOURCEBASE)/configure.in: patch-stamp

patch-stamp: extract-stamp
	@for patch in patches/*.diff; do \
		echo sed -e "s/\$$sourcebase\b/$(PKG_SOURCEBASE)/g" -e "s/\$$host_triplet\b/$(HOST_TRIPLET)/g" "$$patch" \| patch -p0; \
		sed -e "s/\$$sourcebase\b/$(PKG_SOURCEBASE)/g" -e "s/\$$host_triplet\b/$(HOST_TRIPLET)/g" "$$patch" | patch -p0 || exit ; \
	done
	touch patch-stamp

$(PKG_SOURCEBASE)/configure: $(PKG_SOURCEBASE)/configure.in patch-stamp
	cd $(PKG_SOURCEBASE) && ./bootstrap

$(PKG_SOURCEBASE)/config.status: $(PKG_SOURCEBASE)/configure
	cd $(PKG_SOURCEBASE) && ./configure --host=$(HOST_TRIPLET) --enable-static --disable-shared --prefix=`pwd`/../$(TOPDIR)/build/libs CPPFLAGS="-I`pwd`/../$(TOPDIR)/build/libs/include -DGLEW_STATIC" LDFLAGS=-L`pwd`/../$(TOPDIR)/build/libs/lib PKG_CONFIG_PATH=`pwd`/../$(TOPDIR)/build/libs/lib/pkgconfig --disable-executables --without-x --without-glew --without-fribidi --disable-tls ac_cv_func_malloc_0_nonnull=yes ac_cv_func_realloc_0_nonnull=yes

$(PKG_SOURCEBASE)/build/libGLC.la: $(PKG_SOURCEBASE)/config.status
	$(MAKE) -C $(PKG_SOURCEBASE)/build libGLC.la

$(TOPDIR)/build/libs/lib/libGLC.la: $(PKG_SOURCEBASE)/build/libGLC.la
	$(MAKE) -C $(PKG_SOURCEBASE)/build install-libLTLIBRARIES

$(PKG_SOURCEBASE)/quesoglc.pc: $(PKG_SOURCEBASE)/config.status
	$(MAKE) -C $(PKG_SOURCEBASE) quesoglc.pc

$(TOPDIR)/build/libs/lib/pkgconfig/quesoglc.pc: $(PKG_SOURCEBASE)/quesoglc.pc
	$(MAKE) -C $(PKG_SOURCEBASE) install-pkgconfigDATA

$(PKG_SOURCEBASE)/include/GL/glc.h: extract-stamp

$(TOPDIR)/build/libs/include/GL/glc.h: $(PKG_SOURCEBASE)/include/GL/glc.h $(PKG_SOURCEBASE)/config.status
	$(MAKE) -C $(PKG_SOURCEBASE)/include install-nobase_includeHEADERS

all: build
build: $(TARGETS)

clean:
	$(RM) -r $(PKG_SOURCEBASE) extract-stamp patch-stamp

.PHONY: all build clean
