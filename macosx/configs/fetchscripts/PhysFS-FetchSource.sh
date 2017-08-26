#!/bin/sh

OutDir="physfs"
DirectorY="${OutDir}-2.0.3"
FileName="${DirectorY}.tar.bz2"
SourceDLP="http://icculus.org/physfs/downloads/${FileName}"
SHA256Sum="ca862097c0fb451f2cacd286194d071289342c107b6fe69079c079883ff66b69"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${SHA256Sum}"
exit ${?}
