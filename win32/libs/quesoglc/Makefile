PKG_NAME:=quesoglc
PKG_VERSION:=0.7.3~svn913

PKG_SOURCEBASE=$(PKG_NAME)-$(PKG_VERSION)
PKG_CHECKOUT_URL=https://quesoglc.svn.sourceforge.net/svnroot/quesoglc/branches/release-0.7/quesoglc

TARGETS:= \
	$(TOPDIR)/build/libs/lib/libGLC.la \
	$(TOPDIR)/build/libs/include/GL/glc.h

include $(TOPDIR)/rules.mk

$(PKG_SOURCEBASE) fetch-stamp:
	svn export --force -r 913 $(PKG_CHECKOUT_URL) $(PKG_SOURCEBASE)
	touch fetch-stamp

$(PKG_SOURCEBASE)/configure.in: fetch-stamp

patch-stamp: fetch-stamp
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

$(PKG_SOURCEBASE)/include/GL/glc.h: fetch-stamp

$(TOPDIR)/build/libs/include/GL/glc.h: $(PKG_SOURCEBASE)/include/GL/glc.h $(PKG_SOURCEBASE)/config.status
	$(MAKE) -C $(PKG_SOURCEBASE)/include install-nobase_includeHEADERS

all: build
build: $(TARGETS)

clean:
	$(RM) -r $(PKG_SOURCEBASE) fetch-stamp patch-stamp

.PHONY: all build clean
