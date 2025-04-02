# libwebtransport - Building instructions

## Clone repository

```bash
$ git clone https://github.com/danielv4/libwebtransport.git
```

## Init submodules

This step initializes any git submodules required by the project. It ensures that all dependencies and auxiliary libraries are fetched and kept in sync with the main repository.

```bash
$ cd libwebtransport
$ git submodule update --init --recursive --depth 1
```

## Build on Windows

Before building on Windows, ensure you have all necessary dependencies. In this example, the build process requires nasm installed via Chocolatey and a working installation of Visual Studio 2022 with ClangCL support. The CMake command is configured to generate a Release build with proper runtime settings.

```bash
$ choco install nasm

$ mkdir build && cd build
$ cmake --no-warn-unused-cli -G "Visual Studio 17 2022" -A x64 -T ClangCL -DCMAKE_JS_VERSION=7.2.1 -DCMAKE_BUILD_TYPE=Release -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug> ..
$ cmake --build .
```

### Build on Linux

Ensure that all the required development packages are installed before building. For Linux, the example uses libicu-dev and build-essential for compiling the project. The build process creates a dedicated build directory, configures the project with CMake, and then compiles it.

```bash
$ apt-get update
$ apt-get install -y libicu-dev build-essential

$ mkdir build && cd build
$ cmake ..
$ cmake --build .
```