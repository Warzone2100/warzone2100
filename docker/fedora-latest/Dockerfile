FROM fedora:latest

RUN cat /etc/fedora-release

RUN dnf -y update && dnf clean all
RUN dnf -y install git gcc gcc-c++ cmake ninja-build p7zip gettext rubygem-asciidoctor \
 && dnf -y install qt5-qtbase-devel qt5-qtscript-devel libpng-devel SDL2-devel openal-soft-devel physfs-devel libvorbis-devel libtheora-devel glew-devel freetype-devel harfbuzz-devel && dnf clean all

WORKDIR /code
CMD ["/bin/sh"]

