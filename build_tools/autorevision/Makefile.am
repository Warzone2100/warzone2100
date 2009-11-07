CC = @CC_FOR_BUILD@
CXX = @CXX_FOR_BUILD@
CFLAGS = @CFLAGS_FOR_BUILD@
CXXFLAGS = @CXXFLAGS_FOR_BUILD@
CPPFLAGS = @CPPFLAGS_FOR_BUILD@

noinst_PROGRAMS = autorevision
autorevision_SOURCES = autorevision.cpp

# Add Autorevision header to source directory
all-local: $(builddir)/autorevision$(EXEEXT)
	$(builddir)/autorevision$(EXEEXT) +cstr -v $(top_srcdir) $(top_builddir)/src/autorevision.h

# Remove Autorevision header from source directory
clean-local:
	-rm -f $(top_builddir)/src/autorevision.h
