FROM ubuntu

RUN cat /etc/lsb-release

RUN apt-get -u update \
 && apt-get -y install build-essential automake ninja-build pkg-config libpng12-dev libopenal-dev libphysfs-dev libvorbis-dev libtheora-dev libglew1.5-dev libxrandr-dev zip unzip qtscript5-dev qt5-default libfribidi-dev libfreetype6-dev libharfbuzz-dev libfontconfig1-dev docbook-xsl xsltproc asciidoc gettext git cmake sudo \
 && rm -rf /var/lib/apt/lists/*

# Download, build, & install SDL 2.0.5 from source
# For why --enable-mir-shared=no is needed, see: https://bugzilla.libsdl.org/show_bug.cgi?id=3539
ADD https://www.libsdl.org/release/SDL2-2.0.5.tar.gz /tmp
RUN tar -C /tmp -xzf /tmp/SDL2-2.0.5.tar.gz \
 && rm /tmp/SDL2-2.0.5.tar.gz \
 && cd /tmp/SDL2-2.0.5 \
 && mkdir build \
 && cd build \
 && ../configure --enable-mir-shared=no \
 && make \
 && sudo make install \
 && cd /tmp \
 && rm -rf /tmp/SDL2-2.0.5

WORKDIR /code
CMD ["sh"]

