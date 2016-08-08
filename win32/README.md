BUILD INSTRUCTION

Please note the build instruction on the wiki are obsolete, please follow the ones below.

Requirements:
* Visual Studio 2015 (tested with Community Edition)
* CMake 3.5 (Older version may work though)
* Qt5 32bits MSVC 2015 package and QtScript
* Quesoglc dll (https://sourceforge.net/projects/quesoglc/files/).
Please note that CMake must be accessible with your PATH envvar and you need the QT5DIR envvar as well.
GLC_PATH envvar is also needed to load your system fonts (usually C:\Windows\fonts)

Since the solution relies on Nuget to fetch additionals dependencies (openssl libpng zlib sdl2 glew physFS) you also need a working Internet connection.

Instructions:
* Clone the git directory.
* Get the git submodule ("git submodule update --init"). Note that github windows tool fetch the submodule when cloning the whole repo.
* Open the sln file.
* Update libtheora_static project (right clic> "Upgrade VC++ compiler and lib ")
* Then build with F5.
* You need to make Quesoglc dll (and dependant dlls) available to the executable. The simplest way is to copy glc32.dll, freetype6.dll, libexpat.dll, libfontconfig-1.dll, glut32.dll, xmlparse.dll and zlib.dll to the datadir.

