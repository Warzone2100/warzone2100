FROM ubuntu:16.04

RUN cat /etc/lsb-release

RUN apt-get -u update \
 && apt-get -y install build-essential automake ninja-build pkg-config libpng12-dev libopenal-dev libphysfs-dev libvorbis-dev libtheora-dev libglew1.5-dev libxrandr-dev zip unzip qtscript5-dev qt5-default libfribidi-dev libfreetype6-dev libharfbuzz-dev libfontconfig1-dev docbook-xsl xsltproc asciidoc gettext git cmake sudo libcurl4-gnutls-dev gnutls-dev \
 && rm -rf /var/lib/apt/lists/*

# Download, build, & install SDL 2.0.5 from source
# For why --enable-mir-shared=no is needed, see: https://bugzilla.libsdl.org/show_bug.cgi?id=3539
ENV SDL2_SHA256 442038cf55965969f2ff06d976031813de643af9c9edc9e331bd761c242e8785
ADD https://www.libsdl.org/release/SDL2-2.0.5.tar.gz /tmp
RUN echo "$SDL2_SHA256 /tmp/SDL2-2.0.5.tar.gz" | sha256sum -c \
 && tar -C /tmp -xzf /tmp/SDL2-2.0.5.tar.gz \
 && rm /tmp/SDL2-2.0.5.tar.gz \
 && cd /tmp/SDL2-2.0.5 \
 && mkdir build \
 && cd build \
 && ../configure --enable-mir-shared=no \
 && make \
 && sudo make install \
 && cd /tmp \
 && rm -rf /tmp/SDL2-2.0.5

# Download, build, & install libsodium from source
ENV SODIUM_SHA256 6f504490b342a4f8a4c4a02fc9b866cbef8622d5df4e5452b46be121e46636c1
ADD https://download.libsodium.org/libsodium/releases/libsodium-1.0.18.tar.gz /tmp
RUN echo "$SODIUM_SHA256 /tmp/libsodium-1.0.18.tar.gz" | sha256sum -c \
 && tar -C /tmp -xzf /tmp/libsodium-1.0.18.tar.gz \
 && rm /tmp/libsodium-1.0.18.tar.gz \
 && cd /tmp/libsodium-1.0.18 \
 && mkdir build \
 && cd build \
 && ../configure \
 && make && make check \
 && sudo make install \
 && cd /tmp \
 && rm -rf /tmp/libsodium-1.0.18

WORKDIR /code
CMD ["sh"]

