if PORTABLE
INSTALLERFILE=$(PACKAGE)_portable-$(VERSION).exe
else
INSTALLERFILE=$(PACKAGE)-$(VERSION).exe
endif

UPDATERFILE=$(PACKAGE)-$(VERSION).update.exe

EXTRA_DIST = fonts.conf.wd_enable \
	     fonts.conf.wd_disable \
	     warzone2100.nsi

CLEANFILES=$(INSTALLERFILE)

if INSTALLER
GMOFILES = $(addprefix $(top_srcdir)/po/,$(addsuffix .gmo,$(basename $(notdir $(wildcard $(top_srcdir)/po/*.po)))))

all: $(INSTALLERFILE)

else
GMOFILES =
endif

$(top_srcdir)/po/%.gmo: $(top_srcdir)/po/%.po
	$(MAKE) $(AM_MAKEFLAGS) -C $(top_srcdir)/po $(notdir $@)

data:
	$(MAKE) $(AM_MAKEFLAGS) -C ../../data/ all
if PORTABLE
if VIDEOS
$(INSTALLERFILE): warzone2100.nsi ../../src/warzone2100_portable.exe data $(GMOFILES)
	$(MAKENSIS) -DOUTFILE="$(abs_builddir)/$(PACKAGE)-$(PACKAGE_VERSION)-vids_portable.exe" -DPACKAGE="$(PACKAGE)_portable" -DPORTABLE -DINSTALLVIDS -DPACKAGE_NAME="$(PACKAGE_NAME)_portable" -DPACKAGE_VERSION="$(PACKAGE_VERSION)" -DPACKAGE_BUGREPORT="$(PACKAGE_BUGREPORT)" -DVERSIONNUM="$(INSTALLER_VERSION)" -DEXTDIR="$(INSTALLER_EXTDIR)" -DTOP_SRCDIR="$(abs_top_srcdir)" -DTOP_BUILDDIR="$(abs_top_builddir)" -X"SetCompressor /FINAL /SOLID $(INSTALLER_COMPRESSION)" $<
else
$(INSTALLERFILE): warzone2100.nsi ../../src/warzone2100_portable.exe data $(GMOFILES)
	$(MAKENSIS) -DOUTFILE="$(abs_builddir)/$(PACKAGE)-$(PACKAGE_VERSION)_portable.exe" -DPACKAGE="$(PACKAGE)_portable" -DPORTABLE -DPACKAGE_NAME="$(PACKAGE_NAME)_portable" -DPACKAGE_VERSION="$(PACKAGE_VERSION)" -DPACKAGE_BUGREPORT="$(PACKAGE_BUGREPORT)" -DVERSIONNUM="$(INSTALLER_VERSION)" -DEXTDIR="$(INSTALLER_EXTDIR)" -DTOP_SRCDIR="$(abs_top_srcdir)" -DTOP_BUILDDIR="$(abs_top_builddir)" -X"SetCompressor /FINAL /SOLID $(INSTALLER_COMPRESSION)" $<
endif
else
$(INSTALLERFILE): warzone2100.nsi ../../src/warzone2100.exe data $(GMOFILES)
	$(MAKENSIS) -DOUTFILE="$(abs_builddir)/$@" -DPACKAGE="$(PACKAGE)" -DPACKAGE_NAME="$(PACKAGE_NAME)" -DPACKAGE_VERSION="$(PACKAGE_VERSION)" -DPACKAGE_BUGREPORT="$(PACKAGE_BUGREPORT)" -DVERSIONNUM="$(INSTALLER_VERSION)" -DEXTDIR="$(INSTALLER_EXTDIR)" -DTOP_SRCDIR="$(abs_top_srcdir)" -DTOP_BUILDDIR="$(abs_top_builddir)" -X"SetCompressor /FINAL /SOLID $(INSTALLER_COMPRESSION)" $<
endif

$(UPDATERFILE): warzone2100.update.nsi
	$(MAKENSIS) -DVERSION="$(VERSION)" -DVERSIONNUM="$(INSTALLER_VERSION)" -DEXTDIR="$(INSTALLER_EXTDIR)" -X"SetCompressor /FINAL /SOLID $(INSTALLER_COMPRESSION)" $<

.PHONY: all data
