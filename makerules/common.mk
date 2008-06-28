ifeq ($(MAKELEVEL),0)
include $(top_srcdir)/makerules/configure.mk
endif

RM_CPPFLAGS:=-I$(top_srcdir)

%.o: %.rc
	$(WINDRES) $(RM_CPPFLAGS) $(CPPFLAGS) -o $(subst /,$(DIRSEP),$@) $(subst /,$(DIRSEP),$<)

%.o: %.c
	$(CC) $(RM_CPPFLAGS) $(CPPFLAGS) $(CFLAGS) -c -o $(subst /,$(DIRSEP),$@) $(subst /,$(DIRSEP),$<)

%.o: %.cpp
	$(CXX) $(RM_CPPFLAGS) $(CPPFLAGS) $(CXXFLAGS) -c -o $(subst /,$(DIRSEP),$@) $(subst /,$(DIRSEP),$<)

%.lex.h %.lex.c: %.l
	$(FLEX) $(FLEXFLAGS) -o $(subst /,$(DIRSEP),$@) $(subst /,$(DIRSEP),$<)

%.tab.h %.tab.c: %.y
	$(BISON) -d $(BISONFLAGS) -o $(subst /,$(DIRSEP),$@) $(subst /,$(DIRSEP),$<)
