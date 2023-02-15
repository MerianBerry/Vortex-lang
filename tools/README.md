# VORTEX TOOLCHAIN FILES

## *Contents*

1. [Linux](#if-you-are-on-linux)
    1. [Linux to Linux](#linux--linux)
    2. [Linux to Windows](#linux--windows)
2. [Windows](#if-you-are-on-windows)
    1. [Windows to Windows](#windows--windows)

## If you are on Linux

Most linux distrobutions have cmake and make pre installed, but just make sure you do by running ``cmake --version`` and ``make --version``. Make sure cmake's version is above 3.18. If you do not have either of those programs, get them from your distrobutions software manager.

## If you are on Windows

If you haven't already, you must install cmake 3.18 or up and Visual Studio 17 or up in order to build this program. You can get them from their official websites. I will check how to build to Linux from Windows in the future.

## Linux > Linux

Cmake won't need much extra in order to build. Most linux distrobutions have gcc and g++ pre installed, but just make sure they are there and have the right version with ``gcc --version & g++ --version``. This program is able to compile with gcc and g++ version 12.2. You can get the versions of clang on you computer with ``clang --version & clang++ --version``, but you don't have to use clang.

1. Make sure you have gcc and g++ version 12.2 or up, or clang and clang++ version 14 or up.
2. Make and enter the build folder in the root folder using ``mkdir build; cd build``
3. Configure the project using ``cmake ..``
4. Build the project using ``make``

## Linux > Windows

1. Download x86_64-w64-mingw32 packages from your software manager if you havent already.
    1. Check that your mingw32-gcc/mingw32-g++ versions are installed: ``x86_63-w64-mingw32-gcc --version; x86_63-w64-mingw32-g++ --version``
2. Make and enter the build folder in the root folder using ``mkdir build; cd build``
3. Make and enter the build folder in the root folder using ``mkdir build; cd build``
4. Configure the project using ``cmake .. -DCMAKE_TOOLCHAIN_FILE=../tools/linux-win32.cmake``
5. Build the project using ``make``

## Windows > Windows

1. Make the build folder in the root folder using ``mkdir build``
2. Enter the build folder using ``cd build``
3. Configure the project using ``cmake ..``
4. Open the generated *.sln in Visual Studio and build it from there.
