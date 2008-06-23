.PHONY: all clean $(SUBDIRS)

all clean: $(SUBDIRS)

$(SUBDIRS):
	$(TEST_D) $(builddir)/$@ || $(MKDIR_P) $(builddir)/$@
	$(MAKE) -f $(srcdir)/$@/Makefile.raw -C $(builddir)/$@ $(MAKECMDGOALS)

clean: $(SUBDIRS)
	$(RMDIR) $(SUBDIRS)
