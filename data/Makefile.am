SUBDIRS = mods

stamp:
	touch stamp

%.wz: % stamp
	(cd $(srcdir)/$< && zip -ru0 $(abs_builddir)/$@ $(filter-out stamp,$(filter-out $<,$(^:$</%=%))) -x '*svn*' || [ $$? -eq 12 ] && true) # zip returns 12 on "nothing to do"
	zip -T $@
	rm -f stamp

BASELIST = anims audio components effects features tagdefinitions \
	gamedesc.lev images messages misc multiplay music novideo.rpl script \
	sequenceaudio stats stats-sql structs texpages wrf
PATCHLIST = addon.lev anims components effects \
	messages multiplay stats structs wrf

BASEARCHIVE = base.wz
PATCHARCHIVE = mp.wz

CLEANFILES = $(BASEARCHIVE) $(PATCHARCHIVE)

$(BASEARCHIVE): $(BASELIST:%=$(basename $(BASEARCHIVE))/%)
$(PATCHARCHIVE): $(PATCHLIST:%=$(basename $(PATCHARCHIVE))/%)

nodist_pkgdata_DATA =  $(BASEARCHIVE) $(PATCHARCHIVE)
