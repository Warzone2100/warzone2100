FROM fedora:latest

RUN cat /etc/fedora-release

RUN dnf -y update && dnf -y install git gcc gcc-c++ cmake ninja-build p7zip gettext rubygem-asciidoctor \
 && dnf clean all

# Install WZ dependencies
RUN dnf -y update && dnf -y install qt5-qtbase-devel qt5-qtscript-devel libpng-devel SDL2-devel openal-soft-devel physfs-devel libogg-devel libvorbis-devel libtheora-devel freetype-devel harfbuzz-devel libcurl-devel openssl-devel libsodium-devel \
 && dnf -y install vulkan-devel glslc \
 && dnf clean all

WORKDIR /code
CMD ["/bin/sh"]

