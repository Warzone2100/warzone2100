SRC_FILES=	audio.c \
		cdaudio.c \
		mixer_stub.c \
		openal_track.c \
		playlist.c \
		track.c

LIBS=framework gamelib

ifeq ($(GFX), software)
LIBS+=ivis02
else
LIBS+=ivis_opengl
endif

LIB=sound

MAKERULES=../../makerules
include $(MAKERULES)/lib.mk

