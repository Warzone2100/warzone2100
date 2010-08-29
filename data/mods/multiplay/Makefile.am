stamp:
	touch stamp

%.wz: $(srcdir)/% stamp
	(cd $(srcdir)/$(notdir $<) && $(ZIP) -r0 $(ZIP_UPDATE) $(abs_builddir)/$@ $(filter-out stamp,$(filter-out $<,$(^:$</%=%))) -x '*svn*' || [ $$? -eq 12 ] && true) # zip returns 12 on "nothing to do"
	$(ZIP) -T $@
	rm -f stamp

DYDOAILIST = \
	multiplay \
	images

DYDOAIARCHIVE = dydo-ai.wz

OLD110LIST = \
	doc \
	images \
	messages \
	stats \
	wrf

OLD110ARCHIVE = old-1.10-balance.wz


SEMPERFILIST = multiplay
SEMPERFIARCHIVE = semperfi.wz

CLEANFILES = $(DYDOAIARCHIVE) $(OLD110ARCHIVE) $(SEMPERFIARCHIVE)

$(DYDOAIARCHIVE): $(DYDOAILIST:%=$(basename $(DYDOAIARCHIVE))/%)
$(OLD110ARCHIVE): $(OLD110LIST:%=$(basename $(OLD110ARCHIVE))/%)
$(SEMPERFIARCHIVE): $(SEMPERFILIST:%=$(basename $(SEMPERFIARCHIVE))/%)

multiplaymodsdir = $(pkgdatadir)/mods/multiplay
nodist_multiplaymods_DATA =  $(DYDOAIARCHIVE) $(OLD110ARCHIVE) $(SEMPERFIARCHIVE)

dist-hook: $(DYDOAIARCHIVE) $(OLD110ARCHIVE) $(SEMPERFIARCHIVE)
	$(UNZIP) -u $(DYDOAIARCHIVE) -d $(DESTDIR)$(distdir)/$(basename $(DYDOAIARCHIVE))
	$(UNZIP) -u $(OLD110ARCHIVE) -d $(DESTDIR)$(distdir)/$(basename $(OLD110ARCHIVE))
	$(UNZIP) -u $(SEMPERFIARCHIVE) -d $(DESTDIR)$(distdir)/$(basename $(SEMPERFIARCHIVE))
