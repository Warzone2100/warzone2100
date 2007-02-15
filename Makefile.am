SUBDIRS = win32 lib src data nsis

dist_doc_DATA = AUTHORS ChangeLog README COPYING COPYING.README

# Remove this line when Automake 1.10 becomes standard
MKDIR_P=$(mkdir_p)

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(datadir)/icons
	$(MKDIR_P) $(DESTDIR)$(datadir)/applications
	$(INSTALL_DATA) $(srcdir)/debian/warzone2100.png $(DESTDIR)$(datadir)/icons/warzone2100.png
	$(INSTALL_DATA) $(srcdir)/debian/warzone2100.desktop $(DESTDIR)$(datadir)/applications/warzone2100.desktop

uninstall-local:
	$(RM) $(DESTDIR)$(datadir)/icons/warzone2100.png
	$(RM) $(DESTDIR)$(datadir)/applications/warzone2100.desktop

EXTRA_DIST=	\
	autogen.sh \
	configure.ac \
	m4/ogg.m4 \
	m4/sdl.m4 \
	m4/sdl_net.m4 \
	m4/vorbis.m4
