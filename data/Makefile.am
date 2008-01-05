SUBDIRS = mods

BASELIST = anims audio components effects features tagdefinitions \
	gamedesc.lev images messages misc multiplay novideo.rpl palette.bin script \
	sequenceaudio stats structs texpages wrf
PATCHLIST = addon.lev anims components effects \
	messages multiplay stats structs wrf

BASEARCHIVE=warzone.wz
PATCHARCHIVE=mp.wz

CLEANFILES=$(BASEARCHIVE) $(PATCHARCHIVE)

data: $(BASEARCHIVE) $(PATCHARCHIVE)
	$(MAKE) $(AM_MAKEFLAGS) -C mods data

$(BASEARCHIVE): $(BASELIST)
	zip -ru0 $@ $(BASELIST) -x *svn*
	zip -T $@

$(PATCHARCHIVE): $(PATCHLIST:%=mp/%)
	cd mp && zip -ru0 ../$@ $(PATCHLIST) -x *svn*
	zip -T $@

install-data-local: $(BASEARCHIVE) $(PATCHARCHIVE)
	$(MKDIR_P) $(DESTDIR)$(pkgdatadir)
	$(INSTALL_DATA) $(srcdir)/$(BASEARCHIVE) $(DESTDIR)$(pkgdatadir)/$(BASEARCHIVE)
	$(INSTALL_DATA) $(srcdir)/$(PATCHARCHIVE) $(DESTDIR)$(pkgdatadir)/$(PATCHARCHIVE)

uninstall-local:
	rm -f $(DESTDIR)$(pkgdatadir)/$(BASEARCHIVE)
	rm -f $(DESTDIR)$(pkgdatadir)/$(PATCHARCHIVE)

installcheck:
	[ -f $(BASEARCHIVE) ] && zip -T $(BASEARCHIVE)
	[ -f $(PATCHARCHIVE) ] && zip -T $(PATCHARCHIVE)

dist-hook: $(BASEARCHIVE) $(PATCHARCHIVE)
	cp -fu $(BASEARCHIVE) $(DESTDIR)$(distdir)
	cp -fu $(PATCHARCHIVE) $(DESTDIR)$(distdir)
