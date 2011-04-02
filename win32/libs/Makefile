SUBDIRS=dejavu devpkg gettext glew zlib physfs iconv ogg png vorbis theora qt4

all: all-recursive
build: build-recursive
clean: clean-recursive

all-recursive:
	@list='$(SUBDIRS)'; for subdir in $$list; do \
	  echo $(MAKE) -C $$subdir TOPDIR=../.. -$(MAKEFLAGS) all ; \
	  $(MAKE) -C $$subdir TOPDIR=../.. -$(MAKEFLAGS) all || exit; \
	done

build-recursive:
	@list='$(SUBDIRS)'; for subdir in $$list; do \
	  echo $(MAKE) -C $$subdir TOPDIR=../.. -$(MAKEFLAGS) build ; \
	  $(MAKE) -C $$subdir TOPDIR=../.. -$(MAKEFLAGS) build || exit; \
	done

clean-recursive:
	@list='$(SUBDIRS)'; for subdir in $$list; do \
	  echo $(MAKE) -C $$subdir TOPDIR=../.. -$(MAKEFLAGS) clean ; \
	  $(MAKE) -C $$subdir TOPDIR=../.. -$(MAKEFLAGS) clean || exit; \
	done

.PHONY: all build clean all-recursive build-recursive clean-recursive
