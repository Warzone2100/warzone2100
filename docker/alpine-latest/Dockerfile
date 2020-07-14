FROM alpine:latest

RUN apk add --no-cache git build-base clang cmake ninja gdb p7zip gettext asciidoctor
RUN apk add --no-cache sdl2-dev physfs-dev libpng-dev openal-soft-dev libogg-dev libvorbis-dev libtheora-dev glew-dev freetype-dev harfbuzz-dev curl-dev libsodium-dev qt5-qtbase-dev qt5-qtscript-dev

RUN cat /etc/alpine-release

WORKDIR /code
CMD ["/bin/sh"]
