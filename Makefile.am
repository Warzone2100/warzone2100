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
	README.md \
	COPYING \
	COPYING.NONGPL \
	COPYING.README

ACLOCAL_AMFLAGS = -I m4

# ignore the autorevision.cache file on make distcheck
distcleancheck_listfiles =  find . -type f -print | grep -v 'autorevision.cache'

EXTRA_DIST= \
	lib/framework/resource_lexer.lpp \
	lib/framework/resource_parser.ypp \
	lib/framework/strres_lexer.lpp \
	lib/framework/strres_parser.ypp \
	lib/gamelib/audp_lexer.lpp \
	lib/gamelib/audp_parser.ypp \
	lib/script/chat_lexer.lpp \
	lib/script/chat_parser.ypp \
	lib/script/script_lexer.lpp \
	lib/script/script_parser.ypp \
	src/level_lexer.lpp \
	src/scriptvals_lexer.lpp \
	src/scriptvals_parser.ypp \
	build_tools/autorevision \
	autogen.sh \
	config.rpath \
	configure.ac \
	pkg/dpkg \
	macosx \
	3rdparty/SDL/mac \
	3rdparty/glm \
	tools/image/image.cpp \
	tools/image/configs \
	tools/image/Image.xcodeproj \
	po/custom/mac-infoplist.txt \
	po/custom/warzone2100.desktop.txt

# get rid of SVN stuff in the tarball
dist-hook:
	rm -rf `find $(distdir) -type d -name .svn`

javascript-doc-base:
	grep $(top_srcdir)/src/qtscript.cpp -e '//==' | sed 's/\/\/==//' > doc/globals.tex
	grep $(top_srcdir)/src/qtscriptfuncs.cpp -e '//==' | sed 's/\/\/==//' >> doc/globals.tex
	grep $(top_srcdir)/src/qtscript.cpp -e '//__' | sed 's/\/\/__//' > doc/events.tex
	grep $(top_srcdir)/src/qtscript.cpp -e '//--' | sed 's/\/\/--//' > doc/functions.tex
	grep $(top_srcdir)/src/qtscriptfuncs.cpp -e '//--' | sed 's/\/\/--//' >> doc/functions.tex
	grep $(top_srcdir)/src/qtscriptfuncs.cpp -e '//;;' | sed 's/\/\/;;//' > doc/objects.tex
	grep $(top_srcdir)/data/base/script/campaign/libcampaign.js -e '//;;' | sed 's/\/\/;;//' > doc/campaign.tex
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
