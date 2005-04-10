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

SUBDIRS_CLEAN=	framework \
		gamelib \
		netplay \
		script \
		sequence \
		sound \
		widget \
		ivis02 \
		ivis_opengl

all dep:
	@for i in $(SUBDIRS); do (cd $$i; $(MAKE) $@); done

clean:
	@for i in $(SUBDIRS_CLEAN); do (cd $$i; $(MAKE) $@); done

