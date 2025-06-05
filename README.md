## Why does this fork exist?
This fork does not depend on glib; it uses `Mono`'s clean-room reimplementation under MIT/X11 license.
The most restrictive license down the chain now should be MPL (Cairo), I hope. The companion `eglib` is [here](https://github.com/ris-work/eglib). Add glib-2.0.pc to pkg-config and the rest should work. Put eglib in /root/eglib or modify `glib-2.0.pc`. Oh, and we have HarfBuzz now (the default renderer in this, do not need Pango anymore unless you need spacing calculations). To compile:
```
export LIBS=-lharfbuzz -lharfbuzz-icu
```

This does not respect your font settings; use these instead:
```
export GDIPLUS_FONT_PATH=
export GDIPLUS_FONT_SIZE=
```
default for fonts is `NotoSans-Regular.ttf`, **should be present in the working directory**. Also: HarfBuzz script can be set at `g_hb_script` enum (`harfbuzz-private.h`). The default is set to Tamil. Or you can build it with Pango if you want (LGPL) but might as well use the LGPL'd `glib` then.

### Compiling
##### With careless optimizations
Debian:
```
apt install lld-19 llvm-19 clang-19
```
```
PKG_CONFIG_PATH=/root/eglib:/root/libgdiplus:/root/libgdiplus:
LD=ld.lld-19
AR=llvm-ar-19
RANLIB=llvm-ranlib-19
CXX=clang++-19 -fuse-ld=lld-19
CXXFLAGS=-O3 -flto -Ofast -funroll-loops -finline-functions -ffast-math -fomit-frame-pointer
LDFLAGS=-lharfbuzz-icu -lharfbuzz -flto -fuse-ld=lld-19 -Wl,--gc-sections -Wl,--icf=all -Wl,--lto-O3 -Wl,--strip-all
CC=clang-19 -fuse-ld=lld-19 -Wno-error=int-conversion -Wno-error=implicit-function-declaration -Wno-error=return-mismatch
CFLAGS=-O3 -flto -Ofast -funroll-loops -finline-functions -ffast-math -fomit-frame-pointer
```

### License
Copyright (c) 2025 Rishikeshan Sulochana/Lavakumar, my contributions are under MIT/X11 license and so is this repository unless otherwise noted in other files.

## libgdiplus: An Open Source implementation of the GDI+ API.

This is part of the [Mono project](http://www.mono-project.com/).

Build status:

[![Build Status](https://dev.azure.com/dnceng/public/_apis/build/status/mono/mono-libgdiplus-ci?branchName=main)](https://dev.azure.com/dnceng/public/_build/latest?definitionId=617&branchName=main)

### Requirements:

This requires the libraries used by the Cairo vector graphics library to build (freetype2, fontconfig, Xft2 and libpng).

On **OSX** you can use [Homebrew](https://brew.sh/) to install the dependencies:

	brew install glib cairo libexif libjpeg giflib libtiff autoconf libtool automake pango pkg-config
	brew link gettext --force

On **Debian-based Linux distributions** you can use `apt-get` to install the dependencies:

	sudo apt-get install libgif-dev autoconf libtool automake build-essential gettext libglib2.0-dev libcairo2-dev libtiff-dev libexif-dev

On **Windows** you can use [Vcpkg](https://github.com/Microsoft/vcpkg) to install the dependencies. Run the following commands from the root of the repository from an admin command prompt:

	bootstrap-vcpkg.bat
	vcpkg.exe install giflib libjpeg-turbo libpng cairo glib tiff libexif pango --triplet x86-windows
	vcpkg.exe install giflib libjpeg-turbo libpng cairo glib tiff libexif pango --triplet x64-windows
	vcpkg.exe integrate install

### Build instructions

To build on **OSX** without X11:

	./autogen.sh --without-x11 --prefix=YOUR_PREFIX
	make

To build on **OSX with X11** (e.g. from XQuartz):

	PKG_CONFIG_PATH=/opt/X11/lib/pkgconfig ./autogen.sh --prefix=YOUR_PREFIX
	make

To build on **Linux**:

	./autogen.sh --prefix=YOUR_PREFIX
	make

To build on **Windows**, open `libgdiplus.sln`.

### Running the unit tests

Run the following command from the root of the repository:

	make check

To run the tests with Clang sanitizers, run the following command from the root of the repository:

	./autogen.sh --enable-asan
	make check

To run the unit tests with leak sanitizers, run the following command from the root of the repository:

	./autogen.sh --enable-asan
	export ASAN_OPTIONS=detect_leaks=1:fast_unwind_on_malloc=0
	export LSAN_OPTIONS=suppressions=lsansuppressions.txt
	make check

### Code coverage

Code coverage stats are generated with `lcov`. You can use [Homebrew](https://brew.sh/) on **OSX** to install the dependencies:

	brew install lcov

To run the tests with code coverage, run the following commands from the root of the repository:

	./autogen.sh --enable-coverage
	make check
	lcov --capture --directory src --output-file coverage.info
	genhtml coverage.info --output-directory coverage

To view the coverage report, navigate to the `coverage` directory in the root of the repository and open `index.html`.

### Installing libgdiplus

Run the following command from the root of the repository:

	make install

### Optional build options

	--with-pango

	This builds libgdiplus using Pango to render (measure and draw) 
	all of it's text. This requires Pango version 1.38 (or later).
