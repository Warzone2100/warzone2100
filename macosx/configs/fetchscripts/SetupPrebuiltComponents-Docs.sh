#!/bin/sh

OutDir="WarzoneHelp"
DirectorY="${OutDir}-c93e414"
FileName="${DirectorY}.tgz"
BuiltDLP="http://downloads.sourceforge.net/project/warzone2100/build-tools/mac/${FileName}"
MD5Sum="0640db04ec5feca524215699b2eeebf0"

configs/FetchPrebuilt.sh "${DirectorY}" "${OutDir}" "${FileName}" "${BuiltDLP}" "${MD5Sum}"
exit ${?}
