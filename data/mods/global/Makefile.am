stamp:
	touch stamp

%.wz: % stamp
	(cd $(srcdir)/$< && zip -ru0 $(abs_builddir)/$@ $(filter-out stamp,$(filter-out $<,$(^:$</%=%))) -x '*svn*' || [ $$? -eq 12 ] && true) # zip returns 12 on "nothing to do"
	zip -T $@
	rm -f stamp

AIVOLUTIONLIST = multiplay commands.txt
GRIMLIST = components structs texpages wrf COPYING
NEWST_TILESLIST = texpages COPYING readme.txt

AIVOLUTIONARCHIVE = aivolution.wz
GRIMARCHIVE = grim.wz
NEWST_TILESARCHIVE = newst_tiles.wz

CLEANFILES = $(AIVOLUTIONARCHIVE) $(GRIMARCHIVE) $(NEWST_TILESARCHIVE)

$(AIVOLUTIONARCHIVE): $(AIVOLUTIONLIST:%=$(basename $(AIVOLUTIONARCHIVE))/%)
$(GRIMARCHIVE): $(GRIMLIST:%=$(basename $(GRIMARCHIVE))/%)
$(NEWST_TILESARCHIVE): $(NEWST_TILESLIST:%=$(basename $(NEWST_TILESARCHIVE))/%)

globalmodsdir = $(pkgdatadir)/mods/global
nodist_globalmods_DATA = $(AIVOLUTIONARCHIVE) $(GRIMARCHIVE) $(NEWST_TILESARCHIVE)
