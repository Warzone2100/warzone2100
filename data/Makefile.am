SUBDIRS = mods music

stamp:
	touch stamp

%.wz: $(abs_srcdir)/% stamp
	(cd $< && $(ZIP) -r0 $(ZIP_UPDATE) $(abs_builddir)/$@ $(filter-out stamp,$(filter-out $<,$(^:$</%=%))) -x '*svn*' -x '*Makefile*' || [ $$? -eq 12 ] && true) # zip returns 12 on "nothing to do"
	$(ZIP) -T $@
	rm -f stamp

BASELIST = \
	anims \
	audio \
	challenges \
	components \
	effects \
	features \
	gamedesc.lev \
	novideo.ogg \
	images \
	messages \
	misc \
	multiplay \
	script \
	sequenceaudio \
	sequences \
	stats \
	structs \
	tagdefinitions \
	texpages \
	wrf

PATCHLIST = \
	addon.lev \
	anims \
	components \
	effects \
	messages \
	multiplay \
	stats \
	structs \
	wrf

BASEARCHIVE = base.wz
PATCHARCHIVE = mp.wz

CLEANFILES = $(BASEARCHIVE) $(PATCHARCHIVE)

$(BASEARCHIVE): $(BASELIST:%=$(abs_srcdir)/$(basename $(BASEARCHIVE))/%)
$(PATCHARCHIVE): $(PATCHLIST:%=$(abs_srcdir)/$(basename $(PATCHARCHIVE))/%)

nodist_pkgdata_DATA =  $(BASEARCHIVE) $(PATCHARCHIVE)

dist-hook:
	$(UNZIP) -u $(BASEARCHIVE) -d $(DESTDIR)$(distdir)/$(basename $(BASEARCHIVE))
	$(UNZIP) -u $(PATCHARCHIVE) -d $(DESTDIR)$(distdir)/$(basename $(PATCHARCHIVE))
