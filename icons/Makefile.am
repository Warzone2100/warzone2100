dist_noinst_DATA = \
	warzone2100.desktop \
	warzone2100.ico \
	warzone2100.obsolete.ico \
	warzone2100.png \
	warzone2100.large.png \
	warzone2100logo.png \
	warzone2100.splash.png \
	warzone2100.uninstall.ico \
	wz2100portable.ico \
	wz2100portableU.ico \
	wz2100header.bmp \
	wz2100headerP.bmp \
	wz2100welcomeP.bmp \
	wz2100welcome.bmp

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(icondir)
	$(MKDIR_P) $(DESTDIR)$(applicationdir)
	$(INSTALL_DATA) $(srcdir)/warzone2100.png $(DESTDIR)$(icondir)/warzone2100.png
	$(INSTALL_DATA) $(srcdir)/warzone2100.desktop $(DESTDIR)$(applicationdir)/warzone2100.desktop

uninstall-local:
	$(RM) $(DESTDIR)$(icondir)/warzone2100.png
	$(RM) $(DESTDIR)$(applicationdir)/warzone2100.desktop
