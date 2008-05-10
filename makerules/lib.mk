include $(MAKERULES)/common.mk

all: $(BUILT_SOURCES) ../lib$(LIB).a

../lib$(LIB).a: $(patsubst %.c,%.o, $(SRC:%.cpp=%.o))
	$(AR) rcv $@ $^

clean:
	$(RMF) ..$(DIRSEP)lib$(LIB).a *.o $(CLEANFILES)

.PHONY: all clean
