include $(MAKERULES)/common.mk

all: ../lib$(LIB).a

../lib$(LIB).a: $(patsubst %.c,%.o, $(SRC:%.cpp=%.o))
	$(AR) rcv $@ $^

clean:
	$(RMF) ..$(DIRSEP)lib$(LIB).a *.o

.PHONY: all clean
