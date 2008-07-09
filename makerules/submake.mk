.PHONY: all clean $(SUBDIRS)

all clean: $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -f makefile.win32 -C $@ $(MAKECMDGOALS) top_srcdir=../$(top_srcdir) top_builddir=../$(top_builddir)
