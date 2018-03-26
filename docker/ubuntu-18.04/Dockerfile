FROM ubuntu:18.04

RUN cat /etc/lsb-release

RUN apt-get -u update \
 && DEBIAN_FRONTEND=noninteractive apt-get -y install build-essential automake ninja-build pkg-config libpng-dev libsdl2-dev libopenal-dev libphysfs-dev libvorbis-dev libtheora-dev libglew-dev libxrandr-dev zip unzip qtscript5-dev qt5-default libfribidi-dev libfreetype6-dev libharfbuzz-dev libfontconfig1-dev docbook-xsl xsltproc asciidoc gettext git cmake sudo \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /code
CMD ["sh"]

