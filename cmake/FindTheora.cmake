# Source: https://github.com/Kitware/VTK/blob/0593d416f56ee2919c3ca0a2b7da1e00be3cad45/CMake/FindTHEORA.cmake
#
# License: https://github.com/Kitware/VTK/blob/master/Copyright.txt
#
# /*=========================================================================
#
# Program:   Visualization Toolkit
# Module:    Copyright.txt
#
# Copyright (c) 1993-2015 Ken Martin, Will Schroeder, Bill Lorensen
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
# of any contributors may be used to endorse or promote products derived
# from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# =========================================================================*/
#

find_path(THEORA_INCLUDE_DIR
  NAMES
    theora/theora.h)

get_filename_component(computed_theora_root "${THEORA_INCLUDE_DIR}" DIRECTORY)

find_library(THEORA_enc_LIBRARY
  NAMES
    theoraenc
  HINTS
    "${computed_theora_root}/lib"
    "${computed_theora_root}/lib64")

find_library(THEORA_dec_LIBRARY
  NAMES
    theoradec
  HINTS
    "${computed_theora_root}/lib"
    "${computed_theora_root}/lib64")

set(THEORA_LIBRARIES ${THEORA_enc_LIBRARY} ${THEORA_dec_LIBRARY})

add_library(theora::enc UNKNOWN IMPORTED)
set_target_properties(theora::enc
  PROPERTIES
  IMPORTED_LOCATION ${THEORA_enc_LIBRARY}
  INTERFACE_INCLUDE_DIRECTORIES ${THEORA_INCLUDE_DIR})

add_library(theora::dec UNKNOWN IMPORTED)
set_target_properties(theora::dec
  PROPERTIES
  IMPORTED_LOCATION ${THEORA_dec_LIBRARY}
  INTERFACE_INCLUDE_DIRECTORIES ${THEORA_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(theora DEFAULT_MSG THEORA_enc_LIBRARY THEORA_dec_LIBRARY THEORA_INCLUDE_DIR)

mark_as_advanced(THEORA_enc_LIBRARY THEORA_dec_LIBRARY THEORA_INCLUDE_DIR)
