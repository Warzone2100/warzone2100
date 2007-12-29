.rc.o:
	$(WINDRES) -DVERSION="\"$(VERSION)\"" -o$@ $<

dist_noinst_DATA = warzone2100.rc

if MINGW32
nodist_noinst_DATA = \
		warzone2100.o
CLEANFILES = $(nodist_noinst_DATA)
endif
