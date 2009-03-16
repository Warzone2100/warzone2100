stamp:
	touch stamp

%.wz: % stamp
	(cd $(srcdir)/$(notdir $<) && zip -ru0 $(abs_builddir)/$@ $(filter-out stamp,$(filter-out $<,$(^:$</%=%))) -x '*svn*' || [ $$? -eq 12 ] && true) # zip returns 12 on "nothing to do"
	zip -T $@
	rm -f stamp

AIVOLUTIONLIST = \
	commands.txt \
	multiplay

AIVOLUTIONARCHIVE = aivolution.wz

CLEANFILES = $(AIVOLUTIONARCHIVE) $(GRIMARCHIVE) $(NEWST_TILESARCHIVE)

$(AIVOLUTIONARCHIVE): $(AIVOLUTIONLIST:%=$(basename $(AIVOLUTIONARCHIVE))/%)

globalmodsdir = $(pkgdatadir)/mods/global
nodist_globalmods_DATA = $(AIVOLUTIONARCHIVE)
