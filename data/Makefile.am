SUBDIRS = mods

stamp:
	touch stamp

%.wz: % stamp
	(cd $(srcdir)/$< && zip -ru0 $(abs_builddir)/$@ $(filter-out stamp,$(filter-out $<,$(^:$</%=%))) -x '*svn*' || [ $$? -eq 12 ] && true) # zip returns 12 on "nothing to do"
	zip -T $@
	rm -f stamp

BASELIST = anims audio components effects features tagdefinitions \
	gamedesc.lev images messages misc multiplay music novideo.rpl script \
	sequenceaudio stats stats-sql structs texpages wrf
PATCHLIST = addon.lev anims components effects \
	messages multiplay stats structs wrf

BASEARCHIVE = base.wz
PATCHARCHIVE = mp.wz

CLEANFILES = $(BASEARCHIVE) $(PATCHARCHIVE)

data: $(BASEARCHIVE) $(PATCHARCHIVE)
	$(MAKE) $(AM_MAKEFLAGS) -C mods data

$(BASEARCHIVE): $(BASELIST:%=$(basename $(BASEARCHIVE))/%)
$(PATCHARCHIVE): $(PATCHLIST:%=$(basename $(PATCHARCHIVE))/%)

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
