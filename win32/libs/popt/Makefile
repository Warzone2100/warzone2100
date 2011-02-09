PKG_NAME:=popt
PKG_VERSION:=1.12

PKG_SOURCEBASE=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_SOURCEBASE).tar.gz
PKG_SOURCE_URL:= \
	http://rpm5.org/files/popt/ \
	http://wzhost.wz2100.net/~buildbot/dev/
PKG_MD5SUM:=f45290e9ac4b1cf209d0042eb6755543

TARGETS:= \
	$(TOPDIR)/build/libs/include/popt.h \
	$(TOPDIR)/build/libs/lib/libpopt.la

include $(TOPDIR)/rules.mk

$(PKG_SOURCEBASE) extract-stamp: $(TOPDIR)/downloads/$(PKG_SOURCE)
	tar xzf $^
	touch extract-stamp

patch-stamp: extract-stamp
	@for patch in patches/*.diff; do \
	    echo sed -e "s/\$$sourcebase\b/$(PKG_SOURCEBASE)/g" -e "s/\$$host_triplet\b/$(HOST_TRIPLET)/g" "$$patch" \| patch -p0 ; \
	    sed -e "s/\$$sourcebase\b/$(PKG_SOURCEBASE)/g" -e "s/\$$host_triplet\b/$(HOST_TRIPLET)/g" "$$patch" | patch -p0 || exit ; \
	done
	touch patch-stamp

$(PKG_SOURCEBASE)/configure: patch-stamp
	cd $(PKG_SOURCEBASE) && libtoolize --copy --force
	cd $(PKG_SOURCEBASE) && perl -p -i~ -e 's/(po\/Makefile\.in)\s+po\/Makefile\.in/$1/' configure.ac
	cd $(PKG_SOURCEBASE) && perl -p -i~ -e 's/(SUBDIRS\s+=\s+po)\s+po/$1/' Makefile.am
	cd $(PKG_SOURCEBASE) && aclocal -I m4
	cd $(PKG_SOURCEBASE) && autoheader
	cd $(PKG_SOURCEBASE) && automake -a -c
	cd $(PKG_SOURCEBASE) && autoconf

$(PKG_SOURCEBASE)/config.status: $(PKG_SOURCEBASE)/configure
	cd $(PKG_SOURCEBASE) && ./configure --host=$(HOST_TRIPLET) --enable-static --disable-shared --prefix=`pwd`/../$(TOPDIR)/build/libs

$(PKG_SOURCEBASE)/libpopt.la: $(PKG_SOURCEBASE)/config.status
	$(MAKE) -C $(PKG_SOURCEBASE) libpopt.la

$(PKG_SOURCEBASE)/popt.h: extract-stamp

$(TOPDIR)/build/libs/include/popt.h: $(PKG_SOURCEBASE)/popt.h $(PKG_SOURCEBASE)/config.status
	$(MAKE) -C $(PKG_SOURCEBASE) install-includeHEADERS

$(TOPDIR)/build/libs/lib/libpopt.la: $(PKG_SOURCEBASE)/libpopt.la
	$(MAKE) -C $(PKG_SOURCEBASE) install-usrlibLTLIBRARIES

all: build
build: $(TARGETS)

clean:
	$(RM) -r $(PKG_SOURCEBASE) extract-stamp patch-stamp

.PHONY: all build clean
