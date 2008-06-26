.PHONY: all clean $(SUBDIRS)

all clean: $(SUBDIRS)

$(SUBDIRS):
ifneq ($(strip $(top_srcdir)),$(strip $(top_builddir)))
	-$(MKDIR_P) $@
endif
	@$(MAKE) -f ../$(srcdir)/$@/makefile.raw -C $@ $(MAKECMDGOALS) top_srcdir=../$(top_srcdir) top_builddir=../$(top_builddir)

clean: $(SUBDIRS)
ifneq ($(strip $(top_srcdir)),$(strip $(top_builddir)))
	$(RMDIR) $(SUBDIRS)
endif
