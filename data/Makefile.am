## Process this file with automake to produce Makefile.in

BASELIST = anims audio components effects features \
	gamedesc.lev images messages misc multiplay novideo.rpl palette.bin script \
	sequenceaudio stats structs texpages wrf
PATCHLIST = addon.lev anims components effects \
	messages multiplay stats structs wrf

BASEARCHIVE=warzone.wz
PATCHARCHIVE=mp.wz

# Remove this line when Automake 1.10 becomes standard
MKDIR_P=$(mkdir_p)

if DATA
all: $(BASEARCHIVE) $(PATCHARCHIVE)
endif

$(BASEARCHIVE): $(BASELIST)
	zip -ru0 $@ $(BASELIST) -x *svn*
	zip -T $@

$(PATCHARCHIVE): $(PATCHLIST:%=mp/%)
	cd mp && zip -ru0 ../$@ $(PATCHLIST) -x *svn*
	zip -T $@

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(datadir)/warzone2100
	$(INSTALL_DATA) $(srcdir)/$(BASEARCHIVE) $(DESTDIR)$(datadir)/warzone2100/$(BASEARCHIVE)
	$(INSTALL_DATA) $(srcdir)/$(PATCHARCHIVE) $(DESTDIR)$(datadir)/warzone2100/$(PATCHARCHIVE)

uninstall-local:
	rm -f $(DESTDIR)$(datadir)/$(BASEARCHIVE)
	rm -f $(DESTDIR)$(datadir)/$(PATCHARCHIVE)

clean-local:
	rm -f $(BASEARCHIVE)
	rm -f $(PATCHARCHIVE)

installcheck:
	[ -f $(BASEARCHIVE) ] && zip -T $(BASEARCHIVE)
	[ -f $(PATCHARCHIVE) ] && zip -T $(PATCHARCHIVE)

dist-hook: $(BASEARCHIVE) $(PATCHARCHIVE)
	cp -fu $(BASEARCHIVE) $(DESTDIR)$(distdir)
	cp -fu $(PATCHARCHIVE) $(DESTDIR)$(distdir)
