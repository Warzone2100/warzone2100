SUBDIRS = \
	  win32 \
	  3rdparty/miniupnpc \
	  lib/framework \
	  lib/exceptionhandler \
	  lib/script \
	  lib/sequence \
	  lib/ivis_opengl \
	  lib/gamelib \
	  lib/sound \
	  lib/netplay \
	  lib/widget \
	  lib/qtgame \
	  tools/map \
	  src \
	  data \
	  po \
	  doc \
	  icons \
	  pkg/nsis \
	  tests

dist_doc_DATA = \
	AUTHORS \
	ChangeLog \
	COPYING \
	COPYING.NONGPL \
	COPYING.README

ACLOCAL_AMFLAGS = -I m4

# workaround for e.g. git, which doesn't include empty directories
macosx/prebuilt:
	$(MKDIR_P) macosx/prebuilt

EXTRA_DIST= \
	build_tools/autorevision.sh \
	autogen.sh \
	autorevision.conf \
	config.rpath \
	configure.ac \
	pkg/dpkg \
	macosx \
	po/custom/mac-infoplist.txt \
	po/custom/warzone2100.desktop.txt

# get rid of SVN stuff in the tarball
dist-hook:
	rm -rf `find $(distdir) -type d -name .svn`

