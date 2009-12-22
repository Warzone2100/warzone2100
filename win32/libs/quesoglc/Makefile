PKG_NAME:=quesoglc
PKG_VERSION:=0.7.3~svn913

PKG_SOURCEBASE=$(PKG_NAME)-$(PKG_VERSION)
PKG_CHECKOUT_URL=https://quesoglc.svn.sourceforge.net/svnroot/quesoglc/branches/release-0.7/quesoglc

include $(TOPDIR)/rules.mk

$(PKG_SOURCEBASE) fetch-stamp:
	svn export -r 913 $(PKG_CHECKOUT_URL) $(PKG_SOURCEBASE)
	touch fetch-stamp

$(PKG_SOURCEBASE)/configure.in: fetch-stamp

$(PKG_SOURCEBASE)/configure: $(PKG_SOURCEBASE)/configure.in
	cd $(PKG_SOURCEBASE) && ./bootstrap

$(PKG_SOURCEBASE)/config.status: $(PKG_SOURCEBASE)/configure
	cd $(PKG_SOURCEBASE) && ./configure --host=$(HOST_TRIPLET) --enable-static --disable-shared --prefix=`pwd`/../$(TOPDIR)/build/libs CPPFLAGS="-I`pwd`/../$(TOPDIR)/build/libs/include -DGLEW_STATIC" LDFLAGS=-L`pwd`/../$(TOPDIR)/build/libs/lib PKG_CONFIG_PATH=`pwd`/../$(TOPDIR)/build/libs/lib/pkgconfig --disable-executables --without-x --without-glew --without-fribidi --disable-tls ac_cv_func_malloc_0_nonnull=yes ac_cv_func_realloc_0_nonnull=yes

$(PKG_SOURCEBASE)/build/libGLC.la: $(PKG_SOURCEBASE)/config.status
	$(MAKE) -C $(PKG_SOURCEBASE) install-exec install-data

all: build
build: $(PKG_SOURCEBASE)/build/libGLC.la

clean:
	$(RM) -r $(PKG_SOURCEBASE) fetch-stamp

.PHONY: all build clean
