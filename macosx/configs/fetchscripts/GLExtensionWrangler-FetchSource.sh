#!/bin/sh

DirectorY="glew-1.7.0"
OutDir="glew"
FileName="glew-1.7.0.tgz"
SourceDLP="http://downloads.sourceforge.net/project/glew/glew/1.7.0/glew-1.7.0.tgz"
MD5Sum="fb7a8bb79187ac98a90b57f0f27a3e84"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${MD5Sum}"
exit ${?}
