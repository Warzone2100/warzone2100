# Building Warzone 2100 for the Web

## Prerequisites:

- **Git**
- [**Emscripten 3.1.58+**](https://emscripten.org/docs/getting_started/downloads.html)
- [**CMake 3.27+**](https://cmake.org/download/#latest)
- [**workbox-cli**](https://developer.chrome.com/docs/workbox/modules/workbox-cli) (to generate a service worker)
- For language support: [_Gettext_](https://www.gnu.org/software/gettext/)
- To generate documentation: [_Asciidoctor_](https://asciidoctor.org/)

## Building:

1. [Install the Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html)
2. Install [workbox-cli](https://developer.chrome.com/docs/workbox/modules/workbox-cli)
   ```
   npm install --ignore-scripts workbox-cli --global
   ```
3. Follow the instructions for [Warzone 2100: Getting the Source](https://github.com/Warzone2100/warzone2100#getting-the-source)
4. `mkdir` a new build folder (as a sibling directory to the warzone2100 repo)
5. `cd` into the build folder
6. Clone vcpkg into the build folder
   ```
   git clone https://github.com/microsoft/vcpkg.git vcpkg
   ```
7. Run CMake configure:
   ```shell
   # Specify your own install dir
   export WZ_INSTALL_DIR="~/wz_web/installed"
   cmake -S ../warzone2100/ -B . -DCMAKE_BUILD_TYPE=Release "-DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake" "-DVCPKG_TARGET_TRIPLET=wasm32-emscripten" "-DCMAKE_INSTALL_PREFIX:PATH=${WZ_INSTALL_DIR}"
   ```
8. Run CMake build & install:
   ```
   cmake --build . --target install
   ```

## Testing:

1. Run a local web server and open the browser to the compiled WebAssembly version of WZ.  
   From the build directory:
   ```
   emrun --browser chrome src/index.html
   ```
   From the install directory:
   ```shell
   cd "${WZ_INSTALL_DIR}"
   emrun --browser chrome ./index.html
   ```
   Reference: [`emrun` documentation](https://emscripten.org/docs/compiling/Running-html-files-with-emrun.html)
