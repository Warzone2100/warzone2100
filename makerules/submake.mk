.PHONY: all clean $(SUBDIRS)

all clean: $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -f makefile.win32 -C $@ $(MAKECMDGOALS)
