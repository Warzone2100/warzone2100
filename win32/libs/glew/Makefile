PKG_NAME:=glew
PKG_VERSION=1.5.7

PKG_SOURCEBASE:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_SOURCEBASE).tgz
PKG_SOURCE_URL:= \
	@SF/glew
PKG_MD5SUM:=f913ce9dbde4cd250b932731b3534ded

TARGET:=$(TOPDIR)/build/libs/lib/libglew32.a

include $(TOPDIR)/rules.mk

extract-stamp: $(TOPDIR)/downloads/$(PKG_SOURCE)
	tar xzf $^
	touch extract-stamp

$(PKG_SOURCEBASE)/Makefile: extract-stamp

patch-stamp: extract-stamp
	@for patch in patches/*.diff; do \
		echo sed -e "s/\$$sourcebase\b/$(PKG_SOURCEBASE)/g" -e "s/\$$host_triplet\b/$(HOST_TRIPLET)/g" "$$patch" \| patch -p0; \
		sed -e "s/\$$sourcebase\b/$(PKG_SOURCEBASE)/g" -e "s/\$$host_triplet\b/$(HOST_TRIPLET)/g" "$$patch" | patch -p0 || exit ; \
	done
	touch patch-stamp

$(TOPDIR)/build/libs/lib/libglew32.a: $(PKG_SOURCEBASE)/lib/libglew32.a
	$(MAKE) -C $(PKG_SOURCEBASE) GLEW_DEST=../$(TOPDIR)/build/libs SYSTEM=linux-mingw32 install

$(PKG_SOURCEBASE)/lib/libglew32.a: $(PKG_SOURCEBASE)/Makefile patch-stamp
	$(MAKE) -C $(PKG_SOURCEBASE) GLEW_DEST=../$(TOPDIR)/build/libs SYSTEM=linux-mingw32

build: $(TARGET)

clean:
	$(RM) -rf $(PKG_SOURCEBASE) extract-stamp
