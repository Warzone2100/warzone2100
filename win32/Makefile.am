%.o: %.rc
	$(WINDRES) -DVERSION="\"$(VERSION)\"" -o$@ $<

dist_noinst_DATA = \
		warzone2100.rc \
		warzone2100.ico

if MINGW32
nodist_noinst_DATA = \
		warzone2100.o
endif

CLEANFILES = $(nodist_noinst_DATA)
