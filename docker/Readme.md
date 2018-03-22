# How to build the docker images:

In the `Dockerfile` directory:
- `docker build -t <build_image_name> .`

For example:
```
cd ubuntu
docker build -t ubuntu .
```

For more information, see the documentation on [`docker build`](https://docs.docker.com/engine/reference/commandline/build/).

# How to build Warzone using the docker image:

Beware of line ending mismatch between Windows and Linux when cloning repo.
Replace `${pwd}` with your pwd command on your platform.

### Ubuntu

- via Makefile
```
docker run --rm -v ${pwd}:/code <build_image_name> ./autogen.sh
docker run --rm -v ${pwd}:/code <build_image_name> ./configure
docker run --rm -v ${pwd}:/code <build_image_name> make
```

- via CMake
```
docker run --rm -v ${pwd}:/code <build_image_name> cmake '-H.' -Bbuild -DCMAKE_BUILD_TYPE=Debug -G"Ninja"
docker run --rm -v ${pwd}:/code <build_image_name> cmake --build build
```

### Cross-compile (for Windows)

- via CMake
```
docker run --rm -v ${pwd}:/code <build_image_name> i686-w64-mingw32.static-cmake '-H.' -Bbuild -DCMAKE_BUILD_TYPE=Debug -G"Ninja"
docker run --rm -v ${pwd}:/code <build_image_name> cmake --build build --target package
```
This will build the full Windows (portable) installer package.

# Tips

### Using CMake

The examples above build `DEBUG` builds.

To build a Release mode build (with debugging symbols), change:
- `-DCMAKE_BUILD_TYPE=Debug`
to:
- `-DCMAKE_BUILD_TYPE=RelWithDebInfo`
