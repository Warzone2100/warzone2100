%.o: %.rc
	$(WINDRES) $(CPPFLAGS) -o $(subst /,$(DIRSEP),$@) $(subst /,$(DIRSEP),$<)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $(subst /,$(DIRSEP),$@) $(subst /,$(DIRSEP),$<)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $(subst /,$(DIRSEP),$@) $(subst /,$(DIRSEP),$<)

%.lex.h %.lex.c: %.l
	$(FLEX) $(FLEXFLAGS) -o $(subst /,$(DIRSEP),$@) $(subst /,$(DIRSEP),$<)

%.tab.h %.tab.c: %.y
	$(BISON) -d $(BISONFLAGS) -o $(subst /,$(DIRSEP),$@) $(subst /,$(DIRSEP),$<)
