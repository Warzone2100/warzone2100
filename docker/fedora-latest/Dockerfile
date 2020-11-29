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

WORKDIR /code
CMD ["/bin/sh"]

