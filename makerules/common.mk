%.o: $(srcdir)/%.rc
	$(WINDRES) $(CPPFLAGS) -o $@ $<

%.o: $(srcdir)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

%.o: $(srcdir)/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

%.lex.h %.lex.c: $(srcdir)/%.l
	$(FLEX) $(FLEXFLAGS) -o $@ $<

%.tab.h %.tab.c: $(srcdir)/%.y
	$(BISON) -d $(BISONFLAGS) -o $@ $<
