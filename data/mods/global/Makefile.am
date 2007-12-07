## Process this file with automake to produce Makefile.in

AIVOLUTIONLIST = multiplay commands.txt installation.txt
GRIMLIST = components structs texpages wrf COPYING

AIVOLUTIONARCHIVE=aivolution.wz
GRIMARCHIVE=grim.wz

CLEANFILES=$(AIVOLUTIONARCHIVE) $(GRIMARCHIVE)

data: $(AIVOLUTIONARCHIVE) $(GRIMARCHIVE)

$(AIVOLUTIONARCHIVE): $(AIVOLUTIONLIST:%=aivolution/%)
	cd aivolution && zip -ru0 ../$@ $(AIVOLUTIONLIST) -x *svn*
	zip -T $@

$(GRIMARCHIVE): $(GRIMLIST:%=grim/%)
	cd grim && zip -ru0 ../$@ $(GRIMLIST) -x *svn*
	zip -T $@

install-data-local: $(AIVOLUTIONARCHIVE) $(GRIMARCHIVE)
	$(MKDIR_P) $(DESTDIR)$(pkgdatadir)/mods/global
	$(INSTALL_DATA) $(srcdir)/$(AIVOLUTIONARCHIVE) $(DESTDIR)$(pkgdatadir)/mods/global/$(AIVOLUTIONARCHIVE)
	$(INSTALL_DATA) $(srcdir)/$(GRIMARCHIVE) $(DESTDIR)$(pkgdatadir)/mods/global/$(GRIMARCHIVE)

uninstall-local:
	rm -f $(DESTDIR)$(pkgdatadir)/mods/global/$(AIVOLUTIONARCHIVE)
	rm -f $(DESTDIR)$(pkgdatadir)/mods/global/$(GRIMARCHIVE)

installcheck:
	[ -f $(AIVOLUTIONARCHIVE) ] && zip -T $(AIVOLUTIONARCHIVE)
	[ -f $(GRIMARCHIVE) ] && zip -T $(GRIMARCHIVE)

dist-hook: $(AIVOLUTIONARCHIVE) $(GRIMARCHIVE)
	cp -fu $(AIVOLUTIONARCHIVE) $(DESTDIR)$(distdir)
	cp -fu $(GRIMARCHIVE) $(DESTDIR)$(distdir)
