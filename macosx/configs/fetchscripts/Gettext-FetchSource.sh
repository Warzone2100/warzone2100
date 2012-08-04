#!/bin/sh

DirectorY="gettext-0.18.1"
OutDir="gettext"
FileName="gettext-0.18.1.tar.gz"
SourceDLP="http://ftp.gnu.org/pub/gnu/gettext/gettext-0.18.1.tar.gz"
MD5Sum="2ae04f960d5fa03774636ddef19ebdbf"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${MD5Sum}"
exit ${?}
