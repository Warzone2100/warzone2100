## Process this file with automake to produce Makefile.in

ZIPLIST = 01 02 03 04 05 06 07 08 09 10 11 anims audio components effects features \
	gamedesc.lev images messages misc multiplay novideo.rpl palette.bin script \
	sequenceaudio stats structs texpages wrf

install-data-local:
	[ -d 01 ] && { zip -ru9 warzone.wz $(ZIPLIST) -x *svn* || zip -T warzone.wz ; } || true
	[ -d "$(datadir)" ] || $(mkdir_p) "$(datadir)"
	[ -d "$(datadir)/@PACKAGE@" ] || $(mkdir_p) "$(datadir)/@PACKAGE@"
	[ -f warzone.wz ] && $(INSTALL_DATA) warzone.wz $(datadir)/@PACKAGE@/ || true

uninstall-local:
	rm -f $(datadir)/@PACKAGE@/warzone.wz

clean-local:
	rm -f warzone.wz

installcheck:
	[ -f warzone.wz ] && zip -T warzone.wz || true

dist-hook:
	[ -d 01 ] && { zip -ru9 warzone.wz $(ZIPLIST) -x *svn* || zip -T warzone.wz ; } || true
	[ -d 01 ] && cp -fu warzone.wz $(distdir) || true
