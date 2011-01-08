SUBDIRS = \
	  build_tools/autorevision \
	  win32 \
	  lib/framework \
	  lib/exceptionhandler \
	  lib/script \
	  lib/sequence \
	  lib/iniparser \
	  lib/ivis_opengl \
	  lib/gamelib \
	  lib/sound \
	  lib/netplay \
	  lib/widget \
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
	autogen.sh \
	autorevision.conf \
	config.rpath \
	configure.ac \
	macosx

# get rid of SVN stuff in the tarball
dist-hook:
	rm -rf `find $(distdir) -type d -name .svn`

