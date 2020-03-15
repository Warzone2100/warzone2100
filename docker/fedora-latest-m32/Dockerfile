FROM fedora:latest

RUN cat /etc/fedora-release

RUN dnf -y update && dnf clean all
RUN dnf -y install git gcc gcc-c++ cmake ninja-build p7zip gettext rubygem-asciidoctor \
 && dnf clean all

# Install WZ dependencies - 32-bit (i686, for cross-compile)
RUN dnf -y install glibc-devel.i686 \
 && dnf -y install qt5-qtbase-devel.i686 qt5-qtscript-devel.i686 libpng-devel.i686 SDL2-devel.i686 openal-soft-devel.i686 physfs-devel.i686 libogg-devel.i686 libvorbis-devel.i686 libtheora-devel.i686 glew-devel.i686 freetype-devel.i686 harfbuzz-devel.i686 libcurl-devel.i686 openssl-devel.i686 \
 && dnf clean all

WORKDIR /code
CMD ["/bin/sh"]

