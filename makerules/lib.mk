include $(MAKERULES)/common.mk

all: ../lib$(LIB).a
../lib$(LIB).a: $(SRC:.c=.o) ; $(AR) rcv ../lib$(LIB).a $(patsubst %.c,%.o, $(SRC:%.cpp=%.o))
clean: ; $(RMF) ..$(DIRSEP)lib$(LIB).a *.o
