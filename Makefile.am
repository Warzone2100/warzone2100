if BACKEND_QT
backend_subdir=lib/qtgame
else
backend_subdir=lib/sdl
endif

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
	$(backend_subdir) \
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

EXTRA_DIST= \
	build_tools/autorevision.sh \
	autogen.sh \
	config.rpath \
	configure.ac \
	pkg/dpkg \
	macosx \
	3rdparty/SDL/mac \
	po/custom/mac-infoplist.txt \
	po/custom/warzone2100.desktop.txt

# get rid of SVN stuff in the tarball
dist-hook:
	rm -rf `find $(distdir) -type d -name .svn`

javascript-doc:
	grep $(top_srcdir)/src/qtscript.cpp -e '//==' | sed 's/\/\/==//' > doc/globals.tex
	grep $(top_srcdir)/src/qtscriptfuncs.cpp -e '//==' | sed 's/\/\/==//' >> doc/globals.tex
	grep $(top_srcdir)/src/qtscript.cpp -e '//__' | sed 's/\/\/__//' > doc/constants.tex
	grep $(top_srcdir)/src/qtscriptfuncs.cpp -e '//__' | sed 's/\/\/__//' >> doc/constants.tex
	grep $(top_srcdir)/src/qtscript.cpp -e '//--' | sed 's/\/\/--//' > doc/functions.tex
	grep $(top_srcdir)/src/qtscriptfuncs.cpp -e '//--' | sed 's/\/\/--//' >> doc/functions.tex
	cp $(top_srcdir)/doc/javascript.tex doc/
	(cd doc ; pdflatex javascript.tex)
