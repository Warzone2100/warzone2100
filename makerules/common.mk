%.o: %.c
	$(CC) $(CFLAGS) -c -o$@ $<

%.c: %.l
	$(FLEX) $(FLEXFLAGS) -o$@ $<

%.c: %.y
	$(BISON) -d $(BISONFLAGS) -o$@ $<
