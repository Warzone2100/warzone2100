stamp:
	touch stamp

%.wz: % stamp
	(cd $(srcdir)/$(notdir $<) && zip -ru0 $(abs_builddir)/$@ $(filter-out stamp,$(filter-out $<,$(^:$</%=%))) -x '*svn*' || [ $$? -eq 12 ] && true) # zip returns 12 on "nothing to do"
	zip -T $@
	rm -f stamp

AIVOLUTIONLIST = \
	commands.txt \
	multiplay

GRIMLIST = \
	components \
	COPYING \
	structs \
	texpages \
	wrf

NEWST_TILESLIST = \
	COPYING \
	readme.txt \
	texpages

AIVOLUTIONARCHIVE = aivolution.wz
GRIMARCHIVE = grim.wz
NEWST_TILESARCHIVE = newst_tiles.wz

CLEANFILES = $(AIVOLUTIONARCHIVE) $(GRIMARCHIVE) $(NEWST_TILESARCHIVE)

$(AIVOLUTIONARCHIVE): $(AIVOLUTIONLIST:%=$(basename $(AIVOLUTIONARCHIVE))/%)
$(GRIMARCHIVE): $(GRIMLIST:%=$(basename $(GRIMARCHIVE))/%)
$(NEWST_TILESARCHIVE): $(NEWST_TILESLIST:%=$(basename $(NEWST_TILESARCHIVE))/%)

globalmodsdir = $(pkgdatadir)/mods/global
nodist_globalmods_DATA = $(AIVOLUTIONARCHIVE) $(GRIMARCHIVE) $(NEWST_TILESARCHIVE)
