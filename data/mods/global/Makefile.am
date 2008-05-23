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

AIVOLUTIONARCHIVE = aivolution.wz
GRIMARCHIVE = grim.wz
NEWST_TILESARCHIVE = newst_tiles.wz

CLEANFILES = $(AIVOLUTIONARCHIVE) $(GRIMARCHIVE) $(NEWST_TILESARCHIVE)

data: $(AIVOLUTIONARCHIVE) $(GRIMARCHIVE) $(NEWST_TILESARCHIVE)

$(AIVOLUTIONARCHIVE): $(AIVOLUTIONLIST:%=$(basename $(AIVOLUTIONARCHIVE))/%)
$(GRIMARCHIVE): $(GRIMLIST:%=$(basename $(GRIMARCHIVE))/%)
$(NEWST_TILESARCHIVE): $(NEWST_TILESLIST:%=$(basename $(NEWST_TILESARCHIVE))/%)

install-data-local: $(AIVOLUTIONARCHIVE) $(GRIMARCHIVE) $(NEWST_TILESARCHIVE)
	$(MKDIR_P) $(DESTDIR)$(pkgdatadir)/mods/global
	$(INSTALL_DATA) $(AIVOLUTIONARCHIVE) $(DESTDIR)$(pkgdatadir)/mods/global/$(AIVOLUTIONARCHIVE)
	$(INSTALL_DATA) $(GRIMARCHIVE) $(DESTDIR)$(pkgdatadir)/mods/global/$(GRIMARCHIVE)
	$(INSTALL_DATA) $(NEWST_TILESARCHIVE) $(DESTDIR)$(pkgdatadir)/mods/global/$(NEWST_TILESARCHIVE)

uninstall-local:
	rm -f $(DESTDIR)$(pkgdatadir)/mods/global/$(AIVOLUTIONARCHIVE)
	rm -f $(DESTDIR)$(pkgdatadir)/mods/global/$(GRIMARCHIVE)
	rm -f $(DESTDIR)$(pkgdatadir)/mods/global/$(NEWST_TILESARCHIVE)

installcheck:
	[ -f $(AIVOLUTIONARCHIVE) ] && zip -T $(AIVOLUTIONARCHIVE)
	[ -f $(GRIMARCHIVE) ] && zip -T $(GRIMARCHIVE)
	[ -f $(NEWST_TILESARCHIVE) ] && zip -T $(NEWST_TILESARCHIVE)

dist-hook: $(AIVOLUTIONARCHIVE) $(GRIMARCHIVE) $(NEWST_TILESARCHIVE)
	cp -fu $(AIVOLUTIONARCHIVE) $(DESTDIR)$(distdir)
	cp -fu $(GRIMARCHIVE) $(DESTDIR)$(distdir)
	cp -fu $(NEWST_TILESARCHIVE) $(DESTDIR)$(distdir)
