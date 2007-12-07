noinst_PROGRAMS = autorevision
autorevision_SOURCES = autorevision.cpp

# Add Autorevision header to source directory
all-local: autorevision
	$(builddir)/autorevision$(EXEEXT) +cstr -v $(top_srcdir) $(top_srcdir)/src/autorevision.h

# Remove Autorevision header from source directory
clean-local:
	-rm -f $(top_srcdir)/src/autorevision.h
