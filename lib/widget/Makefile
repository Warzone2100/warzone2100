SRC_FILES=	bar.c \
		button.c \
		editbox.c \
		form.c \
		label.c \
		slider.c \
		tip.c \
		widget.c

LIBS=framework

ifeq ($(GFX), opengl)
LIBS+=ivis_opengl
else
LIBS+=ivis02
endif


LIB=widget

MAKERULES=../../makerules
include $(MAKERULES)/lib.mk

