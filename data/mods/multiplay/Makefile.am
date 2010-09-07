stamp:
	touch stamp

%.wz: $(srcdir)/% stamp
	(cd $(srcdir)/$(notdir $<) && $(ZIP) -r0 $(ZIP_UPDATE) $(abs_builddir)/$@ $(filter-out stamp,$(filter-out $<,$(^:$</%=%))) -x '*svn*' || [ $$? -eq 12 ] && true) # zip returns 12 on "nothing to do"
	$(ZIP) -T $@
	rm -f stamp

DYDOAILIST = \
	multiplay \
	images \
	challenges \
	2c-Caladan.addon.lev \
	2c-CaladanB.addon.lev \
	4c-Arrakis.addon.lev \
	4c-ArrakisB.addon.lev \
	4c-Printus.addon.lev

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
	$(UNZIP) -u $(DYDOAIARCHIVE) -d $(DESTDIR)$(distdir)/$(basename $(DYDOAIARCHIVE))
	$(UNZIP) -u $(OLD110ARCHIVE) -d $(DESTDIR)$(distdir)/$(basename $(OLD110ARCHIVE))
	$(UNZIP) -u $(NTWARCHIVE) -d $(DESTDIR)$(distdir)/$(basename $(NTWARCHIVE))
