FROM ubuntu:18.04

RUN cat /etc/lsb-release

RUN mkdir -p /scripts
COPY get-dependencies_linux.sh /scripts
RUN chmod +x /scripts/get-dependencies_linux.sh

RUN apt-get -u update \
 && DEBIAN_FRONTEND=noninteractive apt-get -y install gcc g++ clang libc-dev dpkg-dev ninja-build pkg-config \
 && /scripts/get-dependencies_linux.sh ubuntu build-dependencies \
 && rm -rf /var/lib/apt/lists/*

# Install Vulkan SDK
#RUN DEBIAN_FRONTEND=noninteractive apt-get -y install libvulkan-dev
ADD https://packages.lunarg.com/lunarg-signing-key-pub.asc /tmp
ADD https://packages.lunarg.com/vulkan/1.2.182/lunarg-vulkan-1.2.182-bionic.list /etc/apt/sources.list.d
RUN apt-key add /tmp/lunarg-signing-key-pub.asc
RUN apt-get -u update \
 && DEBIAN_FRONTEND=noninteractive apt-get -y install vulkan-sdk \
 && rm -rf /var/lib/apt/lists/*

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
CMD ["sh"]

