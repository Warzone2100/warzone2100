.rc.o:
	$(WINDRES) -DVERSION="\"$(VERSION)\"" -o$@ $<
if PORTABLE
dist_noinst_DATA = warzone2100_portable.rc
else
dist_noinst_DATA = warzone2100.rc
endif

if MINGW32
if PORTABLE
nodist_noinst_DATA = \
		warzone2100_portable.o
else
nodist_noinst_DATA = \
		warzone2100.o
endif

CLEANFILES = $(nodist_noinst_DATA)
endif

