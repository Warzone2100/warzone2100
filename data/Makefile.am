## Process this file with automake to produce Makefile.in

BASELIST = anims audio components effects features \
	gamedesc.lev images messages misc multiplay novideo.rpl palette.bin script \
	sequenceaudio stats structs texpages wrf locale
PATCHLIST = addon.lev anims components effects \
	messages multiplay stats structs wrf
GRIMLIST = components structs texpages wrf

BASEARCHIVE=warzone.wz
PATCHARCHIVE=mp.wz
GRIMARCHIVE=grim.wz

CLEANFILES=$(BASEARCHIVE) $(PATCHARCHIVE) $(GRIMARCHIVE)

# Remove this line when Automake 1.10 becomes standard
MKDIR_P=$(mkdir_p)

data: $(BASEARCHIVE) $(PATCHARCHIVE) $(GRIMARCHIVE)

$(BASEARCHIVE): $(BASELIST)
	zip -ru0 $@ $(BASELIST) -x *svn*
	zip -T $@

$(PATCHARCHIVE): $(PATCHLIST:%=mp/%)
	cd mp && zip -ru0 ../$@ $(PATCHLIST) -x *svn*
	zip -T $@

$(GRIMARCHIVE): $(GRIMLIST:%=grim/%)
	cd grim && zip -ru0 ../$@ $(GRIMLIST) -x *svn*
	zip -T $@

install-data-local: $(BASEARCHIVE) $(PATCHARCHIVE) $(GRIMARCHIVE)
	$(MKDIR_P) $(DESTDIR)$(pkgdatadir)
	$(INSTALL_DATA) $(srcdir)/$(BASEARCHIVE) $(DESTDIR)$(pkgdatadir)/$(BASEARCHIVE)
	$(INSTALL_DATA) $(srcdir)/$(PATCHARCHIVE) $(DESTDIR)$(pkgdatadir)/$(PATCHARCHIVE)
	$(INSTALL_DATA) $(srcdir)/$(GRIMARCHIVE) $(DESTDIR)$(pkgdatadir)/mods/global/$(GRIMARCHIVE)

uninstall-local:
	rm -f $(DESTDIR)$(pkgdatadir)/$(BASEARCHIVE)
	rm -f $(DESTDIR)$(pkgdatadir)/$(PATCHARCHIVE)
	rm -f $(DESTDIR)$(pkgdatadir)/mods/global/$(GRIMARCHIVE)

installcheck:
	[ -f $(BASEARCHIVE) ] && zip -T $(BASEARCHIVE)
	[ -f $(PATCHARCHIVE) ] && zip -T $(PATCHARCHIVE)
	[ -f $(GRIMARCHIVE) ] && zip -T $(GRIMARCHIVE)

dist-hook: $(BASEARCHIVE) $(PATCHARCHIVE)
	cp -fu $(BASEARCHIVE) $(DESTDIR)$(distdir)
	cp -fu $(PATCHARCHIVE) $(DESTDIR)$(distdir)
	cp -fu $(GRIMARCHIVE) $(DESTDIR)$(distdir)
