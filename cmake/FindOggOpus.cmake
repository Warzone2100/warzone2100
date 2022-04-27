# - Try to find the OggOpus libraries
# Once done this will define
#
#  OGGOPUS_FOUND - system has OggOpus
#  OGGOPUS_VERSION - set either to 1 or 2
#  OGGOPUS_INCLUDE_DIR - the OggOpus include directory
#  OGGOPUS_LIBRARIES - The libraries needed to use OggOpus
#  OGG_LIBRARY        - The Ogg library
#  OPUS_LIBRARY      - The Opus library
#  OPUSFILE_LIBRARY  - The OpusFile library
#  OPUSENC_LIBRARY   - The OpusEnc library

# Copyright (c) 2006, Richard Laerkaeng, <richard@goteborg.utfors.se>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# COPYING-CMAKE-SCRIPTS:
# ----------------------
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products 
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# ----------------------
#


include (CheckLibraryExists)

find_path(OPUS_INCLUDE_DIR opus/opusfile.h)
find_path(OPUS_INCLUDE_DIR opus/opus.h)
find_path(OPUS_INCLUDE_DIR opus/opusenc.h)
find_path(OGG_INCLUDE_DIR  ogg/ogg.h)
find_path(OPUS_MULTISTREAM_DIR_TMP opus/opus_multistream.h)

# opusfile.h includes "opus_multistream.h" instead of "opus/opus_multistream.h"
# so add "/usr/include/opus to fix that
set(OPUS_MULTISTREAM_DIR "${OPUS_MULTISTREAM_DIR_TMP}/opus")

find_library(OGG_LIBRARY NAMES ogg)
find_library(OPUS_LIBRARY NAMES opus)
find_library(OPUSFILE_LIBRARY NAMES opusfile)

# Remove Enc - WZ
mark_as_advanced(OPUS_INCLUDE_DIR OGG_INCLUDE_DIR
                 OGG_LIBRARY OPUS_LIBRARY OPUSFILE_LIBRARY)

if (OPUS_INCLUDE_DIR AND OPUS_LIBRARY AND OPUSFILE_LIBRARY)
   set(OGGOPUS_FOUND TRUE)
   set(OGGOPUS_LIBRARIES ${OGG_LIBRARY} ${OPUS_LIBRARY} ${OPUSFILE_LIBRARY})
else (OPUS_INCLUDE_DIR AND OPUS_LIBRARY AND OPUSFILE_LIBRARY)
   set (OGGOPUS_VERSION)
   set(OGGOPUS_FOUND FALSE)
endif (OPUS_INCLUDE_DIR AND OPUS_LIBRARY AND OPUSFILE_LIBRARY)
# set(OPUS_INCLUDE_DIR ${})

# Add FindPackageHandleStdArgs - WZ
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OggOpus REQUIRED_VARS OGGOPUS_LIBRARIES OPUS_LIBRARY OGG_LIBRARY OPUSFILE_LIBRARY
                                                          OPUS_INCLUDE_DIR OGG_INCLUDE_DIR)

# check_include_files(opus/opusfile.h HAVE_OPUSFILE_H)
# check_library_exists(ogg ogg_page_version "" HAVE_LIBOGG)
# check_library_exists(opus opus_info_init "" HAVE_LIBOPUS)
# check_library_exists(opusfile ov_open "" HAVE_LIBOPUSFILE)
# check_library_exists(opusenc opus_info_clear "" HAVE_LIBOPUSENC)
# check_library_exists(opus opus_bitrate_addblock "" HAVE_LIBOPUSENC2)

if (OGGOPUS_FOUND)
     message(STATUS "Ogg/Opus & OpusFile found " ${OPUS_INCLUDE_DIR} ";" ${OPUS_MULTISTREAM_DIR})
#    set (OPUS_LIBS "-lopus -logg")
#    set (OPUSFILE_LIBS "-lopusfile")
#    set (OPUSENC_LIBS "-lopusenc")
#    set (OGGOPUS_FOUND TRUE)
#    if (HAVE_LIBOPUSENC2)
#        set (HAVE_OPUS 2)
#    else (HAVE_LIBOPUSENC2)
#        set (HAVE_OPUS 1)
#    endif (HAVE_LIBOPUSENC2)
#else (HAVE_LIBOGG AND HAVE_OPUSFILE_H AND HAVE_LIBOPUS AND HAVE_LIBOPUSFILE AND HAVE_LIBOPUSENC)
#    message(STATUS "Ogg/Opus not found")
endif (OGGOPUS_FOUND)
