include $(MAKERULES)/common.mk

DEPS=$(patsubst %.c,%.o, $(SRC:%.rc=%.o)) $(LIBS)

all: $(EXE)$(EXEEXT)

$(EXE)$(EXEEXT): $(DEPS)
	$(CXX) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	$(RMF) $(EXE)$(EXEEXT) *.o

.PHONY: all clean
