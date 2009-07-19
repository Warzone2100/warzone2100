stamp:
	touch stamp

%.wz: $(srcdir)/% stamp
	(cd $(srcdir)/$(notdir $<) && zip -ru0 $(abs_builddir)/$@ $(filter-out stamp,$(filter-out $<,$(^:$</%=%))) -x '*svn*' || [ $$? -eq 12 ] && true) # zip returns 12 on "nothing to do"
	zip -T $@
	rm -f stamp

NTWLIST = \
	addon.lev \
	audio \
	components \
	COPYING \
	doc \
	effects \
	images \
	messages \
	multiplay \
	stats \
	structs \
	texpages \
	wrf

NTWARCHIVE = ntw.wz

AIVOLUTIONLIST = \
	commands.txt \
	multiplay

AIVOLUTIONARCHIVE = aivolution.wz

CLEANFILES = $(NTWARCHIVE) $(AIVOLUTIONARCHIVE)

$(NTWARCHIVE): $(NTWLIST:%=$(basename $(NTWARCHIVE))/%)
$(AIVOLUTIONARCHIVE): $(AIVOLUTIONLIST:%=$(basename $(AIVOLUTIONARCHIVE))/%)

multiplaymodsdir = $(pkgdatadir)/mods/multiplay
nodist_multiplaymods_DATA = $(NTWARCHIVE) $(AIVOLUTIONARCHIVE)

dist-hook: $(NTWARCHIVE) $(AIVOLUTIONARCHIVE)
	unzip -u $(NTWARCHIVE) -d $(DESTDIR)$(distdir)/$(basename $(NTWARCHIVE))
	unzip -u $(AIVOLUTIONARCHIVE) -d $(DESTDIR)$(distdir)/$(basename $(AIVOLUTIONARCHIVE))
