SUBDIRS=lib src

all clean dep:
	@for i in $(SUBDIRS); do (cd $$i; $(MAKE) $@); done

