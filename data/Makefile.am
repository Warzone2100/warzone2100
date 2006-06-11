## Process this file with automake to produce Makefile.in

ZIPLIST = anims audio components effects features \
	gamedesc.lev images messages misc mp multiplay novideo.rpl palette.bin script \
	sequenceaudio stats structs texpages wrf

install-data-local:
	[ -f gamedesc.lev ] && { zip -ru9 warzone.wz $(ZIPLIST) -x *svn* || zip -T warzone.wz; }
	[ -d "$(DESTDIR)$(datadir)" ] || $(mkdir_p) "$(DESTDIR)$(datadir)"
	[ -f warzone.wz ] && $(INSTALL_DATA) warzone.wz $(DESTDIR)$(datadir)/

uninstall-local:
	rm -f $(DESTDIR)$(datadir)/warzone.wz

clean-local:
	rm -f warzone.wz

installcheck:
	[ -f warzone.wz ] && zip -T warzone.wz

dist-hook:
	[ -f gamedesc.lev ] && { zip -ru9 warzone.wz $(ZIPLIST) -x *svn* || zip -T warzone.wz; }
	[ -f warzone.wz ] && cp -fu warzone.wz $(DESTDIR)$(distdir)
