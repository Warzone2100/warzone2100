noinst_LIBRARIES = libsequence.a
AM_CPPFLAGS = -I$(top_srcdir)/lib/framework -I$(top_srcdir)
libsequence_a_SOURCES = \
	adpcm.c \
	dec130.c \
	rpl_reader.c \
	sequence_stub.c \
	adpcm.h \
	rpl_reader.h \
	sequence.h \
	streamer.h

libsequence_a_libs = ../framework/libframework.a
libsequence_a_DEPENDENCIES = $(libsequence_a_libs)
libsequence_a_LIBADD = $(libsequence_a_libs)

