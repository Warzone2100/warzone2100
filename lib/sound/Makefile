SRC_FILES=	audio.c \
		cdaudio.c \
		mixer_stub.c \
		openal_track.c \
		track.c

LIBS=framework gamelib

ifeq ($(GFX), opengl)
LIBS+=ivis_opengl
else
LIBS+=ivis02
endif

LIB=sound

MAKERULES=../../makerules
include $(MAKERULES)/lib.mk

