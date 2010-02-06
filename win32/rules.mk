all: build

$(TOPDIR)/downloads/$(PKG_SOURCE):
	@if [ ! -d "$(TOPDIR)/downloads" ] ; then \
	    echo mkdir -p $(TOPDIR)/downloads ; \
	    mkdir -p $(TOPDIR)/downloads || exit ; \
	fi
	$(TOPDIR)/../build_tools/download.pl $(TOPDIR)/downloads "$(PKG_SOURCE)" "$(PKG_MD5SUM)" $(PKG_SOURCE_URL)

.PHONY: all build clean
