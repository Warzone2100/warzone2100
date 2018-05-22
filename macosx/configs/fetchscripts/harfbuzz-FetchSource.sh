#!/bin/sh

VerLib="1.7.6"
OutDir="harfbuzz"
DirectorY="${OutDir}-${VerLib}"
FileName="${DirectorY}.tar.bz2"
SourceDLP="https://www.freedesktop.org/software/harfbuzz/release/${FileName}"
SHA256Sum="da7bed39134826cd51e57c29f1dfbe342ccedb4f4773b1c951ff05ff383e2e9b"

configs/FetchSource.sh "${DirectorY}" "${OutDir}" "${FileName}" "${SourceDLP}" "${SHA256Sum}"
exit ${?}
