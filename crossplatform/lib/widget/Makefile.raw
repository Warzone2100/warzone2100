SRC_FILES=	bar.c \
		button.c \
		editbox.c \
		form.c \
		label.c \
		slider.c \
		tip.c \
		widget.c

LIBS=framework ivis_common

ifeq ($(GFX), software)
LIBS+=ivis02
else
LIBS+=ivis_opengl
endif


LIB=widget

MAKERULES=../../makerules
include $(MAKERULES)/lib.mk

