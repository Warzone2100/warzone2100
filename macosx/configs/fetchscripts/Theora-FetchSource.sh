#!/bin/sh

DirectorY="libtheora-1.1.1"
OutDir="libtheora"
FileName="libtheora-1.1.1.tar.gz"
SourceDLP="http://downloads.xiph.org/releases/theora/libtheora-1.1.1.tar.gz"
SHA256Sum="40952956c47811928d1e7922cda3bc1f427eb75680c3c37249c91e949054916b"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${SHA256Sum}"
exit ${?}
