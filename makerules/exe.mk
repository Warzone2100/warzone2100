include $(MAKERULES)/common.mk

DEPS=$(patsubst %.c,%.o, $(SRC:%.rc=%.o)) $(LIBS)

all: $(EXE)$(EXEEXT)
$(EXE)$(EXEEXT): $(DEPS) ; $(CC) $(CFLAGS) -o $(EXE)$(EXEEXT) $(DEPS) $(LDFLAGS)
clean: ; $(RMF) $(EXE)$(EXEEXT) *.o
