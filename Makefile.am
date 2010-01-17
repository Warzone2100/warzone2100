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
all: fixdeps
fixdeps:
	cd src ; for ext in c cpp ; do for a in *.$$ext ; do grep -q "\b$$a\b" ".deps/$${a/.$$ext/.Po}" > /dev/null || echo "$${a/.$$ext/.o}: $$a" | tee ".deps/$${a/.$$ext/.Po}" ; done ; done
