include $(MAKERULES)/common.mk

DEPS=$(patsubst %.c,%.o, $(SRC:%.rc=%.o)) $(LIBS)

all: $(BUILT_SOURCES) $(EXE)$(EXEEXT)

$(EXE)$(EXEEXT): $(DEPS)
	$(CXX) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	$(RMF) $(EXE)$(EXEEXT) *.o $(CLEANFILES)

.PHONY: all clean
