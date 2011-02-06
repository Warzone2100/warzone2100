all: build

$(DOWNLOADS)/$(PKG_SOURCE):
	@if [ ! -d "$(DOWNLOADS)" ] ; then \
	    echo mkdir -p $(DOWNLOADS) ; \
	    mkdir -p $(DOWNLOADS) || exit ; \
	fi
	$(TOPDIR)/download.pl $(DOWNLOADS) "$(PKG_SOURCE)" "$(PKG_MD5SUM)" $(PKG_SOURCE_URL)

.PHONY: all build clean
