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
ADD https://packages.lunarg.com/vulkan/1.2.148/lunarg-vulkan-1.2.148-bionic.list /etc/apt/sources.list.d
RUN apt-key add /tmp/lunarg-signing-key-pub.asc
RUN apt-get -u update \
 && DEBIAN_FRONTEND=noninteractive apt-get -y install vulkan-sdk \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /code
CMD ["sh"]

