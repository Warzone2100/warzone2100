%.o: %.rc
	$(WINDRES) $(CPPFLAGS) -o$@ $<

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o$@ $<

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o$@ $<

%.lex.c: %.l
	$(FLEX) $(FLEXFLAGS) -o$@ $<

%.tab.h %.tab.c: %.y
	$(BISON) -d $(BISONFLAGS) -o$@ $<
