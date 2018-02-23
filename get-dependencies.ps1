git clone https://github.com/Microsoft/vcpkg.git;
pushd vcpkg;
.\bootstrap-vcpkg.bat;
.\vcpkg install physfs harfbuzz libiconv libogg libtheora libvorbis libpng openal-soft sdl2 glew freetype gettext zlib
popd;
