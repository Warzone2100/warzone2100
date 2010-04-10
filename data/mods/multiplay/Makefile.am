stamp:
	touch stamp

%.wz: $(srcdir)/% stamp
	(cd $(srcdir)/$(notdir $<) && zip -ru0 $(abs_builddir)/$@ $(filter-out stamp,$(filter-out $<,$(^:$</%=%))) -x '*svn*' || [ $$? -eq 12 ] && true) # zip returns 12 on "nothing to do"
	zip -T $@
	rm -f stamp

DYDOAILIST = \
	multiplay \
	images \
	challenges

DYDOAIARCHIVE = dydo-ai.wz

OLD110LIST = \
	doc \
	images \
	messages \
	stats \
	wrf
	
OLD110ARCHIVE = old-1.10-balance.wz


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

CLEANFILES = $(DYDOAIARCHIVE) $(OLD110ARCHIVE) $(NTWARCHIVE)

$(DYDOAIARCHIVE): $(DYDOAILIST:%=$(basename $(DYDOAIARCHIVE))/%)
$(OLD110ARCHIVE): $(OLD110LIST:%=$(basename $(OLD110ARCHIVE))/%)
$(NTWARCHIVE): $(NTWLIST:%=$(basename $(NTWARCHIVE))/%)

multiplaymodsdir = $(pkgdatadir)/mods/multiplay
nodist_multiplaymods_DATA =  $(DYDOAIARCHIVE) $(OLD110ARCHIVE) $(NTWARCHIVE)

dist-hook: $(DYDOAIARCHIVE) $(OLD110ARCHIVE) $(NTWARCHIVE)
	unzip -u $(DYDOAIARCHIVE) -d $(DESTDIR)$(distdir)/$(basename $(DYDOAIARCHIVE))
	unzip -u $(OLD110ARCHIVE) -d $(DESTDIR)$(distdir)/$(basename $(OLD110ARCHIVE))
	unzip -u $(NTWARCHIVE) -d $(DESTDIR)$(distdir)/$(basename $(NTWARCHIVE))
