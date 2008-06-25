include $(top_srcdir)/makerules/common.mk

.PHONY: all clean

all: $(top_builddir)/lib/lib$(LIB).a

$(top_builddir)/lib/lib$(LIB).a: $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SRC)))
	$(AR) rcv $(subst /,$(DIRSEP),$@) $^

clean:
	$(RM_F) $(subst /,$(DIRSEP),$(top_builddir)/lib/lib$(LIB).a) *.o $(CLEANFILES)
