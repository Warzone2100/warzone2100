include $(MAKERULES)/common.mk

CFLAGS+=-I . `$(SDLCONFIG) --cflags` $(LIBS:%=-I ../lib/%)
LDFLAGS+=-L ../lib $(LIBS:%=-l%)  $(EXT_LIBS:%=-l%)\
	`$(SDLCONFIG) --libs`

all: $(EXE)

dep:
	rm -f make.depend
	touch make.depend
	makedepend -fmake.depend -I . $(LIBS:%=-I ../lib/%) $(SRC_FILES) >& /dev/null
	rm -f make.depend.bak

$(EXE): $(OBJ_FILES) $(LIBS:%=../lib/lib%.a)
	$(CC) -o $@ $(OBJ_FILES) $(LDFLAGS)

clean:
	rm -f $(EXE) *.o *~

-include make.depend

