## Process this file with automake to produce Makefile.in

GRIMLIST = components structs texpages wrf

GRIMARCHIVE=grim.wz

CLEANFILES=$(GRIMARCHIVE)

# Remove this line when Automake 1.10 becomes standard
MKDIR_P=$(mkdir_p)

data: $(GRIMARCHIVE)

$(GRIMARCHIVE): $(GRIMLIST:%=grim/%)
	cd grim && zip -ru0 ../$@ $(GRIMLIST) -x *svn*
	zip -T $@

install-data-local: $(GRIMARCHIVE)
	$(MKDIR_P) $(DESTDIR)$(pkgdatadir)/mods/global
	$(INSTALL_DATA) $(srcdir)/$(GRIMARCHIVE) $(DESTDIR)$(pkgdatadir)/mods/global/$(GRIMARCHIVE)

uninstall-local:
	rm -f $(DESTDIR)$(pkgdatadir)/mods/global/$(GRIMARCHIVE)

installcheck:
	[ -f $(GRIMARCHIVE) ] && zip -T $(GRIMARCHIVE)

dist-hook: $(BASEARCHIVE) $(PATCHARCHIVE)
	cp -fu $(GRIMARCHIVE) $(DESTDIR)$(distdir)
