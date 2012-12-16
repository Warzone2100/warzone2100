SUBDIRS = music

stamp:
	touch stamp

%.wz: $(abs_srcdir)/% stamp
	(cd $(srcdir)/$(notdir $<) && $(ZIP) -r0 $(ZIP_UPDATE) $(abs_builddir)/$@ $(filter-out stamp,$(filter-out $<,$(^:$</%=%))) -x '*svn*' -x '*Makefile*' -x '*.svg' || [ $$? -eq 12 ] && true) # zip returns 12 on "nothing to do"
	$(ZIP) -T $@
	rm -f stamp

BASELIST = \
	palette.txt \
	anims \
	audio \
	campaigns \
	components \
	effects \
	features \
	gamedesc.lev \
	ruleset.ini \
	images \
	messages \
	misc \
	script \
	sequenceaudio \
	shaders\
	stats \
	structs \
	texpages \
	tileset \
	wrf

PATCHLIST = \
	addon.lev \
	ruleset.ini \
	anims \
	challenges \
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

dist-hook: $(BASEARCHIVE) $(PATCHARCHIVE)
	$(UNZIP) -u $(BASEARCHIVE) -d $(DESTDIR)$(distdir)/$(basename $(BASEARCHIVE))
	$(UNZIP) -u $(PATCHARCHIVE) -d $(DESTDIR)$(distdir)/$(basename $(PATCHARCHIVE))
