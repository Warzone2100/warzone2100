include $(abs_top_srcdir)/makerules/common.mk

.PHONY: all clean

all: $(abs_top_builddir)/lib/lib$(LIB).a

$(abs_top_builddir)/lib/lib$(LIB).a: $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SRC)))
	$(AR) rcv $@ $^

clean:
	$(RM_F) $(abs_top_builddir)/lib/lib$(LIB).a *.o $(CLEANFILES)
