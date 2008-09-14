stamp:
	touch stamp

%.wz: % stamp
	(cd $(srcdir)/$< && zip -ru0 $(abs_builddir)/$@ $(filter-out stamp,$(filter-out $<,$(^:$</%=%))) -x '*svn*' || [ $$? -eq 12 ] && true) # zip returns 12 on "nothing to do"
	zip -T $@
	rm -f stamp

NTWLIST = addon.lev audio components COPYING doc effects images messages multiplay stats structs texpages wrf

NTWARCHIVE = ntw.wz

CLEANFILES = $(NTWARCHIVE)

data: $(NTWARCHIVE)

$(NTWARCHIVE): $(NTWLIST:%=$(basename $(NTWARCHIVE))/%)

install-data-local: $(NTWARCHIVE)
	$(MKDIR_P) $(DESTDIR)$(pkgdatadir)/mods/global
	$(INSTALL_DATA) $(NTWARCHIVE) $(DESTDIR)$(pkgdatadir)/mods/global/$(NTWARCHIVE)

uninstall-local:
	rm -f $(DESTDIR)$(pkgdatadir)/mods/global/$(NTWARCHIVE)

installcheck:
	[ -f $(NTWARCHIVE) ] && zip -T $(NTWARCHIVE)

dist-hook: $(AIVOLUTIONARCHIVE) $(GRIMARCHIVE) $(NEWST_TILESARCHIVE)
	cp -fu $(NTWARCHIVE) $(DESTDIR)$(distdir)
