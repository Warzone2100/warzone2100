.PHONY: all clean $(SUBDIRS)

all clean: $(SUBDIRS)

$(SUBDIRS):
	-@$(MKDIR_P) $@
	@$(MAKE) -f ../$(srcdir)/$@/makefile.raw -C $@ $(MAKECMDGOALS) top_srcdir=../$(top_srcdir) top_builddir=../$(top_builddir)

clean: $(SUBDIRS)
	@$(RMDIR) $(SUBDIRS)
