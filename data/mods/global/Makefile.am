SUBDIRS = aivolution/multiplay/skirmish

stamp:
	touch stamp

%.wz: % stamp
	(cd $(srcdir)/$< && zip -ru0 $(abs_builddir)/$@ $(filter-out stamp,$(filter-out $<,$(^:$</%=%))) -x '*svn*' || [ $$? -eq 12 ] && true) # zip returns 12 on "nothing to do"
	zip -T $@
	rm -f stamp

AIVOLUTIONLIST = multiplay commands.txt
GRIMLIST = components structs texpages wrf COPYING
NEWST_TILESLIST = texpages COPYING readme.txt
NTWLIST = addon.lev audio components COPYING doc effects images messages multiplay stats structs texpages wrf

AIVOLUTIONARCHIVE = aivolution.wz
GRIMARCHIVE = grim.wz
NEWST_TILESARCHIVE = newst_tiles.wz
NTWARCHIVE = ntw.wz

CLEANFILES = $(AIVOLUTIONARCHIVE) $(GRIMARCHIVE) $(NEWST_TILESARCHIVE) $(NTWARCHIVE)

data: $(AIVOLUTIONARCHIVE) $(GRIMARCHIVE) $(NEWST_TILESARCHIVE) $(NTWARCHIVE)

$(AIVOLUTIONARCHIVE): $(AIVOLUTIONLIST:%=$(basename $(AIVOLUTIONARCHIVE))/%)
$(GRIMARCHIVE): $(GRIMLIST:%=$(basename $(GRIMARCHIVE))/%)
$(NEWST_TILESARCHIVE): $(NEWST_TILESLIST:%=$(basename $(NEWST_TILESARCHIVE))/%)
$(NTWARCHIVE): $(NTWLIST:%=$(basename $(NTWARCHIVE))/%)

install-data-local: $(AIVOLUTIONARCHIVE) $(GRIMARCHIVE) $(NEWST_TILESARCHIVE) $(NTWARCHIVE)
	$(MKDIR_P) $(DESTDIR)$(pkgdatadir)/mods/global
	$(INSTALL_DATA) $(AIVOLUTIONARCHIVE) $(DESTDIR)$(pkgdatadir)/mods/global/$(AIVOLUTIONARCHIVE)
	$(INSTALL_DATA) $(GRIMARCHIVE) $(DESTDIR)$(pkgdatadir)/mods/global/$(GRIMARCHIVE)
	$(INSTALL_DATA) $(NEWST_TILESARCHIVE) $(DESTDIR)$(pkgdatadir)/mods/global/$(NEWST_TILESARCHIVE)
	$(INSTALL_DATA) $(NTWARCHIVE) $(DESTDIR)$(pkgdatadir)/mods/global/$(NTWARCHIVE)

uninstall-local:
	rm -f $(DESTDIR)$(pkgdatadir)/mods/global/$(AIVOLUTIONARCHIVE)
	rm -f $(DESTDIR)$(pkgdatadir)/mods/global/$(GRIMARCHIVE)
	rm -f $(DESTDIR)$(pkgdatadir)/mods/global/$(NEWST_TILESARCHIVE)
	rm -f $(DESTDIR)$(pkgdatadir)/mods/global/$(NTWARCHIVE)

installcheck:
	[ -f $(AIVOLUTIONARCHIVE) ] && zip -T $(AIVOLUTIONARCHIVE)
	[ -f $(GRIMARCHIVE) ] && zip -T $(GRIMARCHIVE)
	[ -f $(NEWST_TILESARCHIVE) ] && zip -T $(NEWST_TILESARCHIVE)
	[ -f $(NTWARCHIVE) ] && zip -T $(NTWARCHIVE)

dist-hook: $(AIVOLUTIONARCHIVE) $(GRIMARCHIVE) $(NEWST_TILESARCHIVE)
	cp -fu $(AIVOLUTIONARCHIVE) $(DESTDIR)$(distdir)
	cp -fu $(GRIMARCHIVE) $(DESTDIR)$(distdir)
	cp -fu $(NEWST_TILESARCHIVE) $(DESTDIR)$(distdir)
	cp -fu $(NTWARCHIVE) $(DESTDIR)$(distdir)
