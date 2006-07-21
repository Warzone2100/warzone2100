include $(MAKERULES)/common.mk

all: $(EXE).exe
$(EXE).exe: $(SRC:.c=.o) ; $(CC) $(CFLAGS) -o $(EXE).exe $(SRC:.c=.o) $(LIBS:%=../lib/lib%.a) $(LDFLAGS)
clean: ; $(RMF) $(EXE).exe *.o

