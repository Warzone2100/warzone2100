FROM ubuntu:18.04

RUN cat /etc/lsb-release

RUN apt-get -u update \
 && DEBIAN_FRONTEND=noninteractive apt-get -y install gcc g++ clang libc-dev dpkg-dev ninja-build pkg-config libpng-dev libsdl2-dev libopenal-dev libphysfs-dev libvorbis-dev libtheora-dev libxrandr-dev zip unzip qtscript5-dev qt5-default libfribidi-dev libfreetype6-dev libharfbuzz-dev libfontconfig1-dev docbook-xsl xsltproc asciidoc gettext git cmake libcurl4-gnutls-dev gnutls-dev libsodium-dev \
 && rm -rf /var/lib/apt/lists/*

# Install Vulkan SDK
#RUN DEBIAN_FRONTEND=noninteractive apt-get -y install libvulkan-dev
ADD https://packages.lunarg.com/lunarg-signing-key-pub.asc /tmp
ADD https://packages.lunarg.com/vulkan/1.2.148/lunarg-vulkan-1.2.148-bionic.list /etc/apt/sources.list.d
RUN apt-key add /tmp/lunarg-signing-key-pub.asc
RUN apt-get -u update \
 && DEBIAN_FRONTEND=noninteractive apt-get -y install vulkan-sdk \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /code
CMD ["sh"]

