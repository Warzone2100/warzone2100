## Process this file with automake to produce Makefile.in

BASELIST = anims audio components effects features \
	gamedesc.lev images messages misc multiplay novideo.rpl palette.bin script \
	sequenceaudio stats structs texpages wrf
PATCHLIST = addon.lev anims audio components effects \
	messages multiplay stats structs texpages wrf

install-data-local:
	[ -f gamedesc.lev ] && { zip -ru9 warzone.wz $(ZIPLIST) -x *svn* || zip -T warzone.wz ; }
	[ -f gamedesc.lev ] && { cd mp && { zip -ru9 ../mp.wz mp -x *svn* || zip -T ../mp.wz ; } && cd .. ; }
	[ -d "$(DESTDIR)$(datadir)" ] || $(mkdir_p) "$(DESTDIR)$(datadir)"
	[ -f warzone.wz ] && $(INSTALL_DATA) warzone.wz $(DESTDIR)$(datadir)/
	[ -f mp.wz ] && $(INSTALL_DATA) mp.wz $(DESTDIR)$(datadir)/

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
	[ -f gamedesc.lev ] && { zip -ru9 warzone.wz $(ZIPLIST) -x *svn* || zip -T warzone.wz; }
	[ -f gamedesc.lev ] && { cd mp && { zip -ru9 ../mp.wz mp -x *svn* || zip -T ../mp.wz ; } && cd .. ; }
	[ -f warzone.wz ] && cp -fu warzone.wz mp.wz $(DESTDIR)$(distdir)
