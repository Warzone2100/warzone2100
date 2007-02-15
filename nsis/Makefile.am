INSTALLERFILE=warzone2100-$(VERSION).exe

CLEANFILES=$(INSTALLERFILE)

if INSTALLER
all: $(INSTALLERFILE)
endif

$(INSTALLERFILE): warzone2100.nsi ../src/warzone2100.exe ../data/warzone.wz ../data/mp.wz ../data/grim.wz
	$(MAKENSIS) -DVERSION="$(VERSION)" -DVERSIONNUM="$(INSTALLER_VERSION)" -DLIBDIR="$(INSTALLER_LIBDIR)" $<
