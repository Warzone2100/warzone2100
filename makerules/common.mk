%.o: %.rc
	$(WINDRES) -DVERSION=\"$(VERSION)\" -o$@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o$@ $<

%.c: %.l
	$(FLEX) $(FLEXFLAGS) -o$@ $<

%.c: %.y
	$(BISON) -d $(BISONFLAGS) -o$@ $<
