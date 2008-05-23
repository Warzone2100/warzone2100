SUBDIRS = mods

BASELIST = anims audio components effects features tagdefinitions \
	gamedesc.lev images messages misc multiplay novideo.rpl script \
	sequenceaudio stats stats-sql structs texpages wrf
PATCHLIST = addon.lev anims components effects \
	messages multiplay stats structs wrf

BASEARCHIVE=base.wz
PATCHARCHIVE=mp.wz

CLEANFILES=$(BASEARCHIVE) $(PATCHARCHIVE)

data: $(BASEARCHIVE) $(PATCHARCHIVE)
	$(MAKE) $(AM_MAKEFLAGS) -C mods data

$(BASEARCHIVE): $(BASELIST:%=base/%)
	(cd $(srcdir)/base && zip -ru0 $(abs_builddir)/$@ $(BASELIST) -x *svn*)
	zip -T $@

$(PATCHARCHIVE): $(PATCHLIST:%=mp/%)
	(cd $(srcdir)/mp && zip -ru0 $(abs_builddir)/$@ $(PATCHLIST) -x *svn*)
	zip -T $@

install-data-local: $(BASEARCHIVE) $(PATCHARCHIVE)
	$(MKDIR_P) $(DESTDIR)$(pkgdatadir)
	$(INSTALL_DATA) $(BASEARCHIVE) $(DESTDIR)$(pkgdatadir)/$(BASEARCHIVE)
	$(INSTALL_DATA) $(PATCHARCHIVE) $(DESTDIR)$(pkgdatadir)/$(PATCHARCHIVE)

uninstall-local:
	rm -f $(DESTDIR)$(pkgdatadir)/$(BASEARCHIVE)
	rm -f $(DESTDIR)$(pkgdatadir)/$(PATCHARCHIVE)

installcheck:
	[ -f $(BASEARCHIVE) ] && zip -T $(BASEARCHIVE)
	[ -f $(PATCHARCHIVE) ] && zip -T $(PATCHARCHIVE)

dist-hook: $(BASEARCHIVE) $(PATCHARCHIVE)
	cp -fu $(BASEARCHIVE) $(DESTDIR)$(distdir)
	cp -fu $(PATCHARCHIVE) $(DESTDIR)$(distdir)
