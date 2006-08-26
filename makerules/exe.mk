include $(MAKERULES)/common.mk

DEPS=$(patsubst %.c,%.o, $(SRC:%.rc=%.o)) $(LIBS:%=../lib/lib%.a)

all: $(EXE).exe
$(EXE).exe: $(DEPS) ; $(CC) $(CFLAGS) -o $(EXE).exe $(DEPS) $(LDFLAGS)
clean: ; $(RMF) $(EXE).exe *.o
