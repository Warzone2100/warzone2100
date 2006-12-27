%.o: %.rc
	$(WINDRES) -o$@ $<

dist_noinst_DATA = \
		warzone2100.rc \
		warzone2100.ico

nodist_noinst_DATA = \
		warzone2100.o

CLEANFILES = $(nodist_noinst_DATA)
