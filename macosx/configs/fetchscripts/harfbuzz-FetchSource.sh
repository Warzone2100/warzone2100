#!/bin/sh

VerLib="1.4.8"
OutDir="harfbuzz"
DirectorY="${OutDir}-${VerLib}"
FileName="${DirectorY}.tar.bz2"
SourceDLP="https://www.freedesktop.org/software/harfbuzz/release/${FileName}"
SHA256Sum="ccec4930ff0bb2d0c40aee203075447954b64a8c2695202413cc5e428c907131"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${SHA256Sum}"
exit ${?}
