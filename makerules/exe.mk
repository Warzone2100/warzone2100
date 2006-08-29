include $(MAKERULES)/common.mk

DEPS=$(patsubst %.c,%.o, $(SRC:%.rc=%.o)) $(LIBS:%=../lib/lib%.a)

all: $(EXE)$(EXEEXT)
$(EXE)$(EXEEXT): $(DEPS) ; $(CC) $(CFLAGS) -o $(EXE)$(EXEEXT) $(DEPS) $(LDFLAGS)
clean: ; $(RMF) $(EXE)$(EXEEXT) *.o
