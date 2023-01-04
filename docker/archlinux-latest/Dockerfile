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
