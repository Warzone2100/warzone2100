include $(top_srcdir)/makerules/common.mk

DEPS=$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(patsubst %.rc,%.o,$(SRC)))) $(LIBS)

.PHONY: all clean

all: $(BUILT_SOURCES) $(EXE)$(EXEEXT)

$(EXE)$(EXEEXT): $(DEPS)
	$(CXX) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	$(RM_F) $(EXE)$(EXEEXT) *.o $(CLEANFILES)
