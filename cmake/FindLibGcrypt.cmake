
# - Try to find the Gcrypt library
# Once run this will define
#
#  LIBGCRYPT_FOUND - set if the system has the gcrypt library
#  LIBGCRYPT_CFLAGS - the required gcrypt compilation flags
#  LIBGCRYPT_LIBRARIES - the linker libraries needed to use the gcrypt library
#  LIBGCRYPT_INCLUDE_DIR
#
# libgcrypt is moving to pkg-config, but earlier version don't have it
# 
# Copyright (c) 2006 Brad Hards <bradh@kde.org>
#
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

if(NOT LibGcrypt_FIND_VERSION)
    set(LibGcrypt_FIND_VERSION "1.6.1")
endif()

#search in typical paths for libgcrypt-config
FIND_PROGRAM(LIBGCRYPTCONFIG_EXECUTABLE NAMES libgcrypt-config)

#reset variables
set(LIBGCRYPT_LIBRARIES)
set(LIBGCRYPT_CFLAGS)

# if libgcrypt-config has been found
IF(LIBGCRYPTCONFIG_EXECUTABLE)

  # workaround for MinGW/MSYS
  # CMake can't starts shell scripts on windows so it need to use sh.exe
  EXECUTE_PROCESS(COMMAND sh ${LIBGCRYPTCONFIG_EXECUTABLE} --libs RESULT_VARIABLE _return_VALUE OUTPUT_VARIABLE LIBGCRYPT_LIBRARIES OUTPUT_STRIP_TRAILING_WHITESPACE)
  EXECUTE_PROCESS(COMMAND sh ${LIBGCRYPTCONFIG_EXECUTABLE} --cflags RESULT_VARIABLE _return_VALUE OUTPUT_VARIABLE LIBGCRYPT_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
  EXECUTE_PROCESS(COMMAND sh ${LIBGCRYPTCONFIG_EXECUTABLE} --version RESULT_VARIABLE _return_VALUEVersion OUTPUT_VARIABLE LIBGCRYPT_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)

  IF(NOT LIBGCRYPT_CFLAGS AND NOT _return_VALUE)
    SET(LIBGCRYPT_CFLAGS " ")
  ENDIF(NOT LIBGCRYPT_CFLAGS AND NOT _return_VALUE)

  IF(LIBGCRYPT_LIBRARIES AND LIBGCRYPT_CFLAGS)
    SET(LIBGCRYPT_FOUND TRUE)
  ENDIF(LIBGCRYPT_LIBRARIES AND LIBGCRYPT_CFLAGS)

  if(LIBGCRYPT_VERSION VERSION_LESS ${LibGcrypt_FIND_VERSION})
     message(WARNING "libgcrypt found but version is less than required, Found ${LIBGCRYPT_VERSION} Required ${LibGcrypt_FIND_VERSION}")
     SET(LIBGCRYPT_FOUND FALSE)
  endif()

ENDIF(LIBGCRYPTCONFIG_EXECUTABLE)

FIND_PATH(LIBGCRYPT_INCLUDE_DIR gcrypt.h
  HINTS
    "${LIBGCRYPT_HINTS}"
  PATH_SUFFIXES
    include
)

if (LIBGCRYPT_FOUND)
   if (NOT LibGcrypt_FIND_QUIETLY)
      message(STATUS "Found libgcrypt: ${LIBGCRYPT_LIBRARIES}")
   endif (NOT LibGcrypt_FIND_QUIETLY)
else (LIBGCRYPT_FOUND)
   if (LibGcrypt_FIND_REQUIRED)
      message(WARNING "Could not find libgcrypt libraries")
   endif (LibGcrypt_FIND_REQUIRED)
endif (LIBGCRYPT_FOUND)

MARK_AS_ADVANCED(LIBGCRYPT_CFLAGS LIBGCRYPT_LIBRARIES LIBGCRYPT_INCLUDE_DIR)
