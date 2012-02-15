if BACKEND_QT
backend_subdir=lib/qtgame
else
backend_subdir=lib/sdl
endif

SUBDIRS = \
	win32 \
	3rdparty/miniupnpc \
	3rdparty/quesoglc \
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

javascript-doc-base:
	grep $(top_srcdir)/src/qtscript.cpp -e '//==' | sed 's/\/\/==//' > doc/globals.tex
	grep $(top_srcdir)/src/qtscript.cpp -e '//__' | sed 's/\/\/__//' > doc/events.tex
	grep $(top_srcdir)/src/qtscript.cpp -e '//--' | sed 's/\/\/--//' > doc/functions.tex
	grep $(top_srcdir)/src/qtscriptfuncs.cpp -e '//--' | sed 's/\/\/--//' >> doc/functions.tex
	grep $(top_srcdir)/src/qtscriptfuncs.cpp -e '//;;' | sed 's/\/\/;;//' > doc/objects.tex
	test $(abs_top_srcdir) = $(abs_top_builddir) || cp $(top_srcdir)/doc/javascript.tex doc/

javascript-doc: javascript-doc-base
if PDFLATEX
	(cd doc ; pdflatex javascript.tex)
else
	echo "You need pdflatex installed!"
endif

javascript-doc-html: javascript-doc-base
if HEVEA
	(cd doc; hevea javascript.tex; hevea javascript.tex)
if HACHA
	(cd doc; hacha -o javascript-index.html javascript.html)
endif
else
	echo "You need hevea installed!"
endif
