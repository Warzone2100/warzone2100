SUBDIRS = build_tools win32 lib src data po doc icons pkg tests

dist_doc_DATA = \
	AUTHORS \
	ChangeLog \
	COPYING \
	COPYING.NONGPL \
	COPYING.README

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST= \
	autogen.sh \
	autorevision.conf \
	config.rpath \
	configure.ac \
	macosx/external/bison.sh \
	macosx/prebuilt/ \
	macosx/README.BUILD.txt \
	macosx/README.txt \
	macosx/Resources/GenericFramework-Info.plist \
	macosx/Resources/ogg/English.lproj/InfoPlist.strings \
	macosx/Resources/ogg/Info.plist \
	macosx/Resources/theora/English.lproj/InfoPlist.strings \
	macosx/Resources/theora/Info.plist \
	macosx/Resources/vorbis/English.lproj/InfoPlist.strings \
	macosx/Resources/vorbis/Info.plist \
	macosx/Resources/Warzone.icns \
	macosx/Resources/Warzone-Info.plist \
	macosx/Resources/wzlocal/cs.lproj/InfoPlist.strings \
	macosx/Resources/wzlocal/da.lproj/InfoPlist.strings \
	macosx/Resources/wzlocal/de.lproj/InfoPlist.strings \
	macosx/Resources/wzlocal/en_GB.lproj/InfoPlist.strings \
	macosx/Resources/wzlocal/English.lproj/InfoPlist.strings \
	macosx/Resources/wzlocal/English.lproj/locale/ \
	macosx/Resources/wzlocal/es.lproj/InfoPlist.strings \
	macosx/Resources/wzlocal/et_EE.lproj/InfoPlist.strings \
	macosx/Resources/wzlocal/fi.lproj/InfoPlist.strings \
	macosx/Resources/wzlocal/fr.lproj/InfoPlist.strings \
	macosx/Resources/wzlocal/fy.lproj/InfoPlist.strings \
	macosx/Resources/wzlocal/ga.lproj/InfoPlist.strings \
	macosx/Resources/wzlocal/hr.lproj/InfoPlist.strings \
	macosx/Resources/wzlocal/it.lproj/InfoPlist.strings \
	macosx/Resources/wzlocal/la.lproj/InfoPlist.strings \
	macosx/Resources/wzlocal/lt.lproj/InfoPlist.strings \
	macosx/Resources/wzlocal/nb.lproj/InfoPlist.strings \
	macosx/Resources/wzlocal/nl.lproj/InfoPlist.strings \
	macosx/Resources/wzlocal/pl.lproj/InfoPlist.strings \
	macosx/Resources/wzlocal/pt_BR.lproj/InfoPlist.strings \
	macosx/Resources/wzlocal/pt.lproj/InfoPlist.strings \
	macosx/Resources/wzlocal/ro.lproj/InfoPlist.strings \
	macosx/Resources/wzlocal/ru.lproj/InfoPlist.strings \
	macosx/Resources/wzlocal/sl.lproj/InfoPlist.strings \
	macosx/Resources/wzlocal/uk_UA.lproj/InfoPlist.strings \
	macosx/Resources/wzlocal/zh_CN.lproj/InfoPlist.strings \
	macosx/Resources/wzlocal/zh_TW.lproj/InfoPlist.strings \
	macosx/Warzone.xcodeproj/project.pbxproj

# HACK Don't give make errors when switching between trunk and branches.
# Now with PD-ksh support.
all: fixdeps
fixdeps:
	cd "$(top_srcdir)/src" ; mkdir -p "$(abs_top_builddir)/src/.deps" ; for ext in c cpp ; do for a in *.$$ext ; do grep -q "\b$$a\b" "$(abs_top_builddir)/src/.deps/`echo "$$a" | sed s/\.$$ext/.Po/`" 2> /dev/null || echo "`echo "$$a" | sed s/\.$$ext/.o/`: ../$(top_srcdir)/src/$$a" | tee "$(abs_top_builddir)/src/.deps/`echo "$$a" | sed s/\.$$ext/.Po/`" ; done ; done
