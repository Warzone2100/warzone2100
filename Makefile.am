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
	config.rpath \
	configure.ac

# HACK Don't give make errors when switching between trunk and branches.
# Now with PD-ksh support.
all: fixdeps
fixdeps:
	cd "$(top_srcdir)/src" ; mkdir -p "$(abs_top_builddir)/src/.deps" ; for ext in c cpp ; do for a in *.$$ext ; do grep -q "\b$$a\b" "$(abs_top_builddir)/src/.deps/`echo "$$a" | sed s/\.$$ext/.Po/`" 2> /dev/null || echo "`echo "$$a" | sed s/\.$$ext/.o/`: ../$(top_srcdir)/src/$$a" | tee "$(abs_top_builddir)/src/.deps/`echo "$$a" | sed s/\.$$ext/.Po/`" ; done ; done
