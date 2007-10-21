noinst_PROGRAMS = auto_revision
auto_revision_SOURCES = autorevision.cpp

# Add Autorevision header to source directory
all-local: auto_revision
	./auto_revision +cstr -v $(top_srcdir) $(top_srcdir)/src/autorevision.h

# Remove Autorevision header from source directory
clean-local:
	-rm -f $(top_srcdir)/src/autorevision.h
