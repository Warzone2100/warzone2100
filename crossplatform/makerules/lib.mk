LIB_FILE=$(LIB:%=../lib%.a)

include $(MAKERULES)/common.mk

CFLAGS+=`$(SDLCONFIG) --cflags` -I . $(LIBS:%=-I ../%) \
	-I ../../src

all: $(LIB_FILE)

dep:
	rm -f make.depend
	touch make.depend
	makedepend -fmake.depend -I . $(LIBS:%=-I ../%) -I ../../src $(SRC_FILES) >& /dev/null
	rm -f make.depend.bak

$(LIB_FILE): $(OBJ_FILES)
	rm -f $(LIB_FILE)
	ar rcv $(LIB_FILE) $(OBJ_FILES)

clean:
	rm -f $(LIB_FILE) *.o $(GENERATED_SRC) *~

-include make.depend

