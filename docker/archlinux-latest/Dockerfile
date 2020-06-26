FROM archlinux:latest

RUN pacman -Syyu --noconfirm && pacman-db-upgrade
RUN pacman -S --noconfirm lsb-release git base-devel clang cmake ninja gdb p7zip gettext asciidoctor \
 && pacman --sync --clean --clean --noconfirm

# Install WZ dependencies
RUN pacman -S --noconfirm sdl2 physfs libpng openal libogg libvorbis libtheora glew xorg-xrandr freetype2 harfbuzz curl libsodium qt5-base qt5-script \
 && pacman --sync --clean --clean --noconfirm

RUN lsb_release -a

WORKDIR /code
CMD ["/bin/sh"]
