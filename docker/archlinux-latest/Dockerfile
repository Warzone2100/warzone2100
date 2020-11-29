FROM archlinux:latest

RUN mkdir -p /scripts
COPY get-dependencies_linux.sh /scripts
RUN chmod +x /scripts/get-dependencies_linux.sh

RUN pacman -Syyu --noconfirm && pacman-db-upgrade
RUN pacman -S --noconfirm lsb-release git base-devel clang cmake ninja gdb p7zip gettext asciidoctor \
 && pacman --sync --clean --clean --noconfirm

# Install WZ dependencies
RUN /scripts/get-dependencies_linux.sh archlinux build-dependencies \
 && pacman --sync --clean --clean --noconfirm

RUN lsb_release -a

WORKDIR /code
CMD ["/bin/sh"]
