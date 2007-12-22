## Process this file with automake to produce Makefile.in

AIVOLUTIONLIST = multiplay commands.txt installation.txt
GRIMLIST = components structs texpages wrf COPYING
NEWST_TILESLIST = texpages COPYING readme.txt

AIVOLUTIONARCHIVE=aivolution.wz
GRIMARCHIVE=grim.wz
NEWST_TILESARCHIVE=newst_tiles.wz

CLEANFILES=$(AIVOLUTIONARCHIVE) $(GRIMARCHIVE) $(NEWST_TILESARCHIVE)

data: $(AIVOLUTIONARCHIVE) $(GRIMARCHIVE) $(NEWST_TILESARCHIVE)

$(AIVOLUTIONARCHIVE): $(AIVOLUTIONLIST:%=aivolution/%)
	cd aivolution && zip -ru0 ../$@ $(AIVOLUTIONLIST) -x *svn*
	zip -T $@

$(GRIMARCHIVE): $(GRIMLIST:%=grim/%)
	cd grim && zip -ru0 ../$@ $(GRIMLIST) -x *svn*
	zip -T $@

$(NEWST_TILESARCHIVE): $(NEWST_TILESLIST:%=newst_tiles/%)
	cd newst_tiles && zip -ru0 ../$@ $(NEWST_TILESLIST) -x *svn*
	zip -T $@

install-data-local: $(AIVOLUTIONARCHIVE) $(GRIMARCHIVE) $(NEWST_TILESARCHIVE)
	$(MKDIR_P) $(DESTDIR)$(pkgdatadir)/mods/global
	$(INSTALL_DATA) $(srcdir)/$(AIVOLUTIONARCHIVE) $(DESTDIR)$(pkgdatadir)/mods/global/$(AIVOLUTIONARCHIVE)
	$(INSTALL_DATA) $(srcdir)/$(GRIMARCHIVE) $(DESTDIR)$(pkgdatadir)/mods/global/$(GRIMARCHIVE)
	$(INSTALL_DATA) $(srcdir)/$(NEWST_TILESARCHIVE) $(DESTDIR)$(pkgdatadir)/mods/global/$(NEWST_TILESARCHIVE)

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
