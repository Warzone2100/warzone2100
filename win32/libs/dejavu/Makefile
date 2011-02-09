PKG_NAME:=dejavu-fonts-ttf
PKG_VERSION=2.32

PKG_SOURCEBASE:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_SOURCEBASE).tar.bz2
PKG_SOURCE_URL:= \
	@SF/dejavu \
	http://wzhost.wz2100.net/~buildbot/dev/
PKG_MD5SUM:=63b1bf644cd54188a2ec43aff1846104

TARGETS:= \
	$(TOPDIR)/build/libs/etc/fonts/DejaVuSans.ttf \
	$(TOPDIR)/build/libs/etc/fonts/DejaVuSans-Bold.ttf

include $(TOPDIR)/rules.mk

extract-stamp: $(DOWNLOADS)/$(PKG_SOURCE)
	tar xjf $^
	touch extract-stamp

$(PKG_SOURCEBASE)/ttf/DejaVuSans.ttf $(PKG_SOURCEBASE)/ttf/DejaVuSans-Bold.ttf: extract-stamp

$(TOPDIR)/build/libs/etc/fonts:
	@test -d $@ || { \
	    echo mkdir -p $@ ; \
	    mkdir -p $@ || exit ; \
	}

$(TOPDIR)/build/libs/etc/fonts/%.ttf: $(PKG_SOURCEBASE)/ttf/%.ttf $(TOPDIR)/build/libs/etc/fonts
	cp -p $< $@

build: $(TARGETS)

clean:
	$(RM) -rf $(PKG_SOURCEBASE) extract-stamp
