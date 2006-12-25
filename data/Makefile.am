## Process this file with automake to produce Makefile.in

BASELIST = anims audio components effects features \
	gamedesc.lev images messages misc multiplay novideo.rpl palette.bin script \
	sequenceaudio stats structs texpages wrf
PATCHLIST = addon.lev anims audio components effects \
	messages multiplay stats structs texpages wrf

# Remove this line when Automake 1.10 becomes standard
MKDIR_P=$(mkdir_p)

all: mp.wz warzone.wz

mp.wz:
	cd mp && { zip -ru0 ../mp.wz $(PATCHLIST) -x *svn* || zip -T ../mp.wz ; } && cd ..

warzone.wz:
	zip -ru0 warzone.wz $(BASELIST) -x *svn* || zip -T warzone.wz

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(datadir)/warzone2100
	$(INSTALL_DATA) $(srcdir)/warzone.wz $(DESTDIR)$(datadir)/warzone2100/warzone.wz
	$(INSTALL_DATA) $(srcdir)/mp.wz $(DESTDIR)$(datadir)/warzone2100/mp.wz

uninstall-local:
	rm -f $(DESTDIR)$(datadir)/warzone.wz
	rm -f $(DESTDIR)$(datadir)/mp.wz

clean-local:
	rm -f warzone.wz
	rm -f mp.wz

installcheck:
	[ -f warzone.wz ] && zip -T warzone.wz
	[ -f mp.wz ] && zip -T mp.wz

dist-hook:
	[ -f gamedesc.lev ] && { zip -ru9 warzone.wz $(BASELIST) -x *svn* || zip -T warzone.wz; }
	[ -f gamedesc.lev ] && { cd mp && { zip -ru9 ../mp.wz ${PATCHLIST} -x *svn* || zip -T ../mp.wz ; } && cd .. ; }
	[ -f warzone.wz ] && cp -fu warzone.wz mp.wz $(DESTDIR)$(distdir)
