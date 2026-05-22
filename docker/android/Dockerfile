FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get -u update \
 && apt-get -y install \
    openjdk-17-jdk \
    wget unzip curl git \
    cmake ninja-build \
    build-essential \
    python3 pkg-config \
    zip tar \
    autoconf autoconf-archive automake libtool \
 && ln -sf /usr/lib/jvm/java-17-openjdk-$(dpkg --print-architecture) /usr/lib/jvm/java-17-openjdk \
 && rm -rf /var/lib/apt/lists/*

ENV JAVA_HOME=/usr/lib/jvm/java-17-openjdk
ENV NDK_VERSION=28.2.13676358
ENV ANDROID_API_LEVEL=36
ENV ANDROID_HOME=/opt/android-sdk

# Install Android command-line tools
RUN mkdir -p "${ANDROID_HOME}/cmdline-tools" \
 && wget -q "https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip" \
         -O /tmp/cmdline-tools.zip \
 && unzip -q /tmp/cmdline-tools.zip -d /tmp/cmdline-tools-extracted \
 && mv /tmp/cmdline-tools-extracted/cmdline-tools "${ANDROID_HOME}/cmdline-tools/latest" \
 && rm /tmp/cmdline-tools.zip

ENV PATH="${ANDROID_HOME}/cmdline-tools/latest/bin:${ANDROID_HOME}/platform-tools:${PATH}"

# Accept licenses and install SDK packages
RUN yes | sdkmanager --licenses > /dev/null 2>&1 || true \
 && sdkmanager \
    "platforms;android-${ANDROID_API_LEVEL}" \
    "build-tools;36.0.0" \
    "ndk;${NDK_VERSION}" \
    "cmake;3.22.1"

ENV ANDROID_NDK_HOME="${ANDROID_HOME}/ndk/${NDK_VERSION}"

# Install Gradle 8.11.1
RUN wget -q "https://services.gradle.org/distributions/gradle-8.11.1-bin.zip" \
         -O /tmp/gradle.zip \
 && unzip -q /tmp/gradle.zip -d /opt \
 && rm /tmp/gradle.zip

ENV PATH="/opt/gradle-8.11.1/bin:${PATH}"

# Install vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git /opt/vcpkg \
 && /opt/vcpkg/bootstrap-vcpkg.sh -disableMetrics

ENV VCPKG_ROOT=/opt/vcpkg
ENV VCPKG_DEFAULT_TRIPLET=arm64-android

ARG USER=warzone2100
ARG UID=1000
RUN useradd -d /home/warzone2100 -m -g root -u "${UID}" "${USER}" \
 && chown -R "${USER}:root" "${ANDROID_HOME}" /opt/vcpkg /opt/gradle-8.11.1

USER warzone2100

WORKDIR /src
CMD ["sh"]
