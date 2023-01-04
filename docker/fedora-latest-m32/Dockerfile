FROM fedora:latest
#FROM fedora:latest

RUN cat /etc/fedora-release

RUN dnf -y update && dnf -y install git gcc gcc-c++ cmake ninja-build p7zip gettext rubygem-asciidoctor \
 && dnf clean all

# Install WZ dependencies - 32-bit (i686, for cross-compile)
RUN dnf -y update && dnf -y install glibc-devel.i686 \
 && dnf -y install libpng-devel.i686 SDL2-devel.i686 openal-soft-devel.i686 physfs-devel.i686 libogg-devel.i686 libvorbis-devel.i686 opus-devel.i686 libtheora-devel.i686 fribidi-devel.i686 freetype-devel.i686 harfbuzz-devel.i686 libcurl-devel.i686 openssl-devel.i686 libsodium-devel.i686 sqlite-devel.i686 \
 && dnf clean all

# Defines arguments which can be passed during build time.
ARG USER=warzone2100
ARG UID=1000
# Create a user with given UID.
RUN useradd -d /home/warzone2100 -m -g root -u "$UID" "$USER"
# Switch to warzone2100 user by default.
USER warzone2100
# Check the current uid of the user.
RUN id

WORKDIR /code
CMD ["/bin/sh"]

