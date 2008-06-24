include $(top_srcdir)/makerules/common.mk

.PHONY: all clean

all: $(top_builddir)/lib/lib$(LIB).a

$(top_builddir)/lib/lib$(LIB).a: $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SRC)))
	$(AR) rcv $@ $^

clean:
	$(RM_F) $(top_builddir)/lib/lib$(LIB).a *.o $(CLEANFILES)
