SRC_FILES=	anim.c \
		animobj.c \
		gtime.c \
		hashtabl.c \
		parser_l.c \
		parser_y.c \
		pqueue.c \
		ptrlist.c

LIBS=framework sound

ifeq ($(GFX), opengl)
LIBS+=ivis_opengl
else
LIBS+=ivis02
endif

LIB=gamelib

MAKERULES=../../makerules
include $(MAKERULES)/lib.mk

