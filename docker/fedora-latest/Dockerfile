FROM fedora:latest

RUN cat /etc/fedora-release

RUN mkdir -p /scripts
COPY get-dependencies_linux.sh /scripts
RUN chmod +x /scripts/get-dependencies_linux.sh

RUN dnf -y update && dnf -y install git gcc gcc-c++ cmake ninja-build p7zip gettext rubygem-asciidoctor \
 && dnf clean all

# Install WZ dependencies
RUN dnf -y update \
 && /scripts/get-dependencies_linux.sh fedora build-dependencies \
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

