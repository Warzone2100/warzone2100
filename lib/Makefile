SUBDIRS=framework \
	gamelib \
	netplay \
	script \
	sequence \
	sound \
	widget

ifeq ($(GFX), opengl)
SUBDIRS+=ivis_opengl
else
SUBDIRS+=ivis02
endif

all clean dep:
	@for i in $(SUBDIRS); do (cd $$i; $(MAKE) $@); done

