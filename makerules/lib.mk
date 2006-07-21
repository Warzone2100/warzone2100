include $(MAKERULES)/common.mk

all: ../lib$(LIB).a
../lib$(LIB).a: $(SRC:.c=.o) ; ar rcv ../lib$(LIB).a $(SRC:.c=.o)
clean: ; $(RMF) ..$(DIRSEP)lib$(LIB).a *.o
