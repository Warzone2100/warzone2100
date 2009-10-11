stamp:
	touch stamp

%.wz: $(srcdir)/% stamp
	(cd $(srcdir)/$(notdir $<) && zip -ru0 $(abs_builddir)/$@ $(filter-out stamp,$(filter-out $<,$(^:$</%=%))) -x '*svn*' || [ $$? -eq 12 ] && true) # zip returns 12 on "nothing to do"
	zip -T $@
	rm -f stamp

OLD110LIST = \
	doc \
	images \
	messages \
	stats \
	wrf
	
OLD110ARCHIVE = original.wz


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

CLEANFILES = $(OLD110ARCHIVE) $(NTWARCHIVE) $(AIVOLUTIONARCHIVE)

$(OLD110ARCHIVE): $(OLD110LIST:%=$(basename $(OLD110ARCHIVE))/%)
$(NTWARCHIVE): $(NTWLIST:%=$(basename $(NTWARCHIVE))/%)
$(AIVOLUTIONARCHIVE): $(AIVOLUTIONLIST:%=$(basename $(AIVOLUTIONARCHIVE))/%)

multiplaymodsdir = $(pkgdatadir)/mods/multiplay
nodist_multiplaymods_DATA = $(OLD110ARCHIVE) $(NTWARCHIVE) $(AIVOLUTIONARCHIVE)

dist-hook: $(OLD110ARCHIVE) $(NTWARCHIVE) $(AIVOLUTIONARCHIVE)
	unzip -u $(OLD110ARCHIVE) -d $(DESTDIR)$(distdir)/$(basename $(OLD110ARCHIVE))
	unzip -u $(NTWARCHIVE) -d $(DESTDIR)$(distdir)/$(basename $(NTWARCHIVE))
	unzip -u $(AIVOLUTIONARCHIVE) -d $(DESTDIR)$(distdir)/$(basename $(AIVOLUTIONARCHIVE))
