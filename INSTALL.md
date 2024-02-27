# How to Rime with Weasel

## Preparation

  - Install **Visual Studio 2017** for *Desktop development in C++*
    with components *ATL*, *MFC* and *Windows XP support*.
    Visual Studio 2015 or later versions may work with additional configuration.

  - Install dev tools: `git`, `cmake`， `clang-format(>=17.0.6)`

  - Download third-party libraries: `boost(>=1.60.0)`

Optional:

  - install `bash` via *Git for Windows*, for installing data files with `plum`;
  - install `python` for building OpenCC dictionaries;
  - install [NSIS](http://nsis.sourceforge.net/Download) for creating installer.

## Checkout source code

Make sure all git submodules are checked out recursively.

```batch
git clone --recursive https://github.com/rime/weasel.git
```

## Build and Install Weasel

Locate `weasel` source directory.

### Setup build environment

Edit your build environment settings in `env.bat`.
You can create the file by copying `env.bat.template` in the source tree.

Make sure `BOOST_ROOT` is set to the existing path `X:\path\to\boost_<version>`.

When using a different version of Visual Studio or platform toolset, un-comment
lines to set corresponding variables.

Alternatively, start a *Developer Command Prompt* window and set environment
variables directly in the console, before invocation of `build.bat`:

```batch
set BOOST_ROOT=X:\path\to\boost_N_NN_N
```

### Build

```batch
cd weasel
build.bat all
```

Voila.

Installer will be generated in `output\archives` directory.

### Alternative: using prebuilt Rime binaries

If you've already got a copy of prebuilt binaries of librime, you can simply
copy `.dll`s / `.lib`s into `weasel\output` / `weasel\lib` directories
respectively, then build Weasel without the `all` command line option.

```batch
build.bat boost data opencc
build.bat weasel
```

### Install and try it live

```batch
cd output
install.bat
```

### Optional: play with Rime command line tools

`librime` comes with a REPL application which can be used to test if the library
is working.

```batch
cd librime
copy /Y build\lib\Release\rime.dll build\bin
cd build\bin
echo zhongzhouyunshurufa | Release\rime_api_console.exe > output.txt
```

Instead of redirecting output to a file, you can set appropriate code page
(`chcp 65001`) and font in the console to work with the REPL interactively.
