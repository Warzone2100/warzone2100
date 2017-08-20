#!/bin/sh

VerLib="0.19.8.1"
OutDir="gettext"
DirectorY="${OutDir}-${VerLib}"
FileName="${DirectorY}.tar.gz"
SourceDLP="http://ftp.gnu.org/pub/gnu/gettext/${FileName}"
SHA256Sum="ff942af0e438ced4a8b0ea4b0b6e0d6d657157c5e2364de57baa279c1c125c43"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${SHA256Sum}"
exit ${?}
