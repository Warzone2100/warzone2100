if BACKEND_QT
backend_subdir=lib/qtgame
else
backend_subdir=lib/sdl
endif

UTF8PROC_DEFINES = -DUTF8PROC_STATIC
AM_MAKEFLAGS = UTF8PROC_DEFINES=$(UTF8PROC_DEFINES)

SUBDIRS = \
	win32 \
	3rdparty/miniupnpc \
	3rdparty/micro-ecc \
	3rdparty/sha2 \
	3rdparty \
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
	pkg/nsis

dist_doc_DATA = \
	AUTHORS \
	ChangeLog \
	README.md \
	doc/Scripting.md \
	doc/js-globals.md \
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

doc/js-globals.md:
	grep $(top_srcdir)/src/qtscript.cpp -e '//==' | sed 's:.*//== \?::' > $(top_srcdir)/doc/js-globals.md
	grep $(top_srcdir)/src/qtscriptfuncs.cpp -e '//==' | sed 's:.*//== \?::' >> $(top_srcdir)/doc/js-globals.md
	grep $(top_srcdir)/src/qtscript.cpp -e '//__' | sed 's:.*//__ \?::' > $(top_srcdir)/doc/js-events.md
	grep $(top_srcdir)/src/qtscript.cpp -e '//--' | sed 's:.*//-- \?::' > $(top_srcdir)/doc/js-functions.md
	grep $(top_srcdir)/src/qtscriptfuncs.cpp -e '//--' | sed 's:.*//-- \?::' >> $(top_srcdir)/doc/js-functions.md
	grep $(top_srcdir)/src/qtscript.cpp -e '//;;' | sed 's:.*//;; \?::' > $(top_srcdir)/doc/js-objects.md
	grep $(top_srcdir)/src/qtscriptfuncs.cpp -e '//;;' | sed 's:.*//;; \?::' >> $(top_srcdir)/doc/js-objects.md
	grep $(top_srcdir)/data/base/script/campaign/libcampaign.js -e '.*//;;' | sed 's://;; \?::' > $(top_srcdir)/doc/js-campaign.md
	test $(abs_top_srcdir) = $(abs_top_builddir) || cp $(top_srcdir)/doc/*.md doc/
.PHONY: doc/js-globals.md
