#!/bin/sh

VerLib="1.13.0"
OutDir="glew"
DirectorY="${OutDir}-${VerLib}"
FileName="${DirectorY}.tgz"
SourceDLP="http://downloads.sourceforge.net/project/glew/glew/${VerLib}/${FileName}"
SHA256Sum="aa25dc48ed84b0b64b8d41cdd42c8f40f149c37fa2ffa39cd97f42c78d128bc7"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${SHA256Sum}"
exit ${?}
