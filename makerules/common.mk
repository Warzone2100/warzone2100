%.o: %.rc
	$(WINDRES) -DVERSION=\"$(VERSION)\" -o$@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o$@ $<

%.lex.c: %.l
	$(FLEX) $(FLEXFLAGS) -o$@ $<

%.tab.h %.tab.c: %.y
	$(BISON) -d $(BISONFLAGS) -o$@ $<
