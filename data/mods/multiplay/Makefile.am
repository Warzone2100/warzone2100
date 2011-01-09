stamp:
	touch stamp

%.wz: $(srcdir)/% stamp
	(cd $(srcdir)/$(notdir $<) && $(ZIP) -r0 $(ZIP_UPDATE) $(abs_builddir)/$@ $(filter-out stamp,$(filter-out $<,$(^:$</%=%))) -x '*svn*' || [ $$? -eq 12 ] && true) # zip returns 12 on "nothing to do"
	$(ZIP) -T $@
	rm -f stamp

OLD110LIST = \
	doc \
	images \
	messages \
	stats \
	wrf

OLD110ARCHIVE = old-1.10-balance.wz

CLEANFILES = $(OLD110ARCHIVE)

$(OLD110ARCHIVE): $(OLD110LIST:%=$(basename $(OLD110ARCHIVE))/%)

multiplaymodsdir = $(pkgdatadir)/mods/multiplay
nodist_multiplaymods_DATA =  $(OLD110ARCHIVE)

dist-hook: $(OLD110ARCHIVE)
	$(UNZIP) -u $(OLD110ARCHIVE) -d $(DESTDIR)$(distdir)/$(basename $(OLD110ARCHIVE))
