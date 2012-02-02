#!/bin/sh

DirectorY="physfs-2.0.2"
OutDir="physfs"
FileName="physfs-2.0.2.tar.gz"
SourceDLP="http://icculus.org/physfs/downloads/physfs-2.0.2.tar.gz"
MD5Sum="4e8927c3d30279b03e2592106eb9184a"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${MD5Sum}"
exit ${?}
