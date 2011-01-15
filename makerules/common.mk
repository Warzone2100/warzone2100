ifeq ($(MAKELEVEL),0)
include $(top_srcdir)/makerules/configure.mk
endif

RM_CPPFLAGS:=-I$(top_srcdir)

%.o: %.rc
	$(WINDRES) $(RM_CPPFLAGS) $(CPPFLAGS) -o $(subst /,\\,$@) $(subst /,\\,$<)

%.o: %.c
	$(CC) $(RM_CPPFLAGS) $(CPPFLAGS) $(CFLAGS) -c -o $(subst /,\\,$@) $(subst /,\\,$<)

%.o: %.cpp
	$(CXX) $(RM_CPPFLAGS) $(CPPFLAGS) $(CXXFLAGS) -c -o $(subst /,\\,$@) $(subst /,\\,$<)

%.lex.h %.lex.cpp: %.l
	$(FLEX) $(FLEXFLAGS) -o$(subst /,\\,$@) $(subst /,\\,$<)

%.tab.h %.tab.cpp: %.y
	$(BISON) -d $(BISONFLAGS) -o $(subst /,\\,$@) $(subst /,\\,$<)
