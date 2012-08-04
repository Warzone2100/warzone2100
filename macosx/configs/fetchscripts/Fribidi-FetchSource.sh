#!/bin/sh

DirectorY="fribidi-0.10.9"
OutDir="fribidi"
FileName="fribidi-0.10.9.tar.gz"
SourceDLP="http://fribidi.org/download/fribidi-0.10.9.tar.gz"
MD5Sum="647aee89079b056269ff0918dc1c6d28"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${MD5Sum}"
exit ${?}
