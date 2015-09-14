# How to Rime with Weasel

## Preparation

Assume we already have a default installation of **Visual Studio 2013** on drive `C:`.

Install dev tools: `git`, `cmake`
Download third-party libraries: `boost(>=1.56.0)`

## Checkout source code

Also checkout submodules `brise` and `librime` under `weasel` directory.

```batch
git clone git@github.com:lotem/weasel.git
cd weasel
git submodule update --init
```

## Build librime

This section guides you to build librime, the core input method library used by Weasel.

To be verbose, you'll get the following binaries as the build output of librime, as well as its
dependent third-party libraries including boost, glog, marisa-trie, leveldb, opencc, yaml-cpp.

  * `librime\build\lib\Release\rime.dll` / `rime.lib`
  * `thirdparty\bin\libglog.dll`
  * `thirdparty\bin\opencc.dll`

### Using prebuilt binaries

If you've already got a copy of prebuilt binaries of librime,
you can simply copy `.dll`s / `.lib`s into `weasel\output` / `weasel\lib` directories respectively.
Then go straight to build Weasel.

### Setup build environment

Edit `librime\env.bat.template`, save your copy as `librime\env.bat`.

Make sure `BOOST_ROOT` is set to `\path\to\boost_1_57_0` in `librime\env.bat`.

Then, start VC command line tools from `librime\shell.bat`.

### Build librime and its dependencies

At the command prompt, enter the following command:
```batch
rem cd librime
rem shell.bat

build.bat boost thirdparty librime
```

With some luck, you now have a copy of the built library in `librime\build\lib\Release`:
  * shared library - `rime.dll` / `rime.lib`, these will be used by Weasel
  * static library - `librime.lib`, if built with `build.bat static`

### Play with Rime command line tools

Rime comes with a REPL application by which you can test if the library is working.

Windows command line does not use UTF-8 character encoding, thus we save the output to a file.
```batch
rem cd librime
copy /Y thirdparty\bin\*.dll build\bin\
copy /Y build\lib\Release\rime.dll build\bin\
cd build\bin
echo zhongzhouyunshurufa | Release\rime_api_console.exe > output.txt
```

## Build and Install Weasel

Back to `weasel` directory.

### Setup build environment

Edit `env.bat.template`, save your copy as `env.bat`.

Edit `weasel.props.template`, save your copy as `weasel.props`.
Forget about Python. It's not used anyway.

Then, start VC command line tools from `shell.bat`.

### Build

```batch
rem cd \path\to\weasel
rem shell.bat

build.bat all
```

Or, when using prebuilt librime libraries (see "Using prebuilt binaries" section above):
```batch
build.bat boost data hant
```

Voila.

### Install and try it live

```batch
cd output
install.bat
```
