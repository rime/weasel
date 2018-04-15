# How to Rime with Weasel

## Preparation

Assume we already have an installation of **Visual Studio 2015** including the component "MFC and ATL support".

Install dev tools: `git`, `cmake`
Download third-party libraries: `boost(>=1.60.0)`

Optional:
install `bash` (available via Git for Windows) for installing data files from `plum`;
install `python` for building OpenCC dictionaries;
install [NSIS](http://nsis.sourceforge.net/Download) for creating installer.

## Checkout source code

Make sure you also checkout submodules `librime` and `plum` under `weasel` directory.

```batch
git clone --recursive https://github.com/rime/weasel.git
```

## Build and Install Weasel

Locate `weasel` source directory.

### Setup build environment

Start a Developer Command Prompt window.

Set `BOOST_ROOT` environment variable:

```batch
set BOOST_ROOT=X:\path\to\boost_N_NN_N
```

Alternatively, save your build environment settings in `env.bat`.
You can create the file by copying `env.bat.template` and make modifications.
Make sure `BOOST_ROOT` is set to existing `X:\path\to\boost_<version_number>` in your copy.

### Build

```batch
cd weasel
build.bat all
```

Voila.

Installer will be generated in `output\archives` directory.

### Alternative: using prebuilt Rime binaries

If you've already got a copy of prebuilt binaries of librime,
you can simply copy `.dll`s / `.lib`s into `weasel\output` / `weasel\lib` directories respectively.
Then build Weasel without the `all` command line option.

```batch
build.bat boost data hant
```

### Install and try it live

```batch
cd output
install.bat
```

### Optional: play with Rime command line tools

Rime comes with a REPL application which can be used to test if the library is working.

Windows command line does not use UTF-8 character encoding, thus we save the output to a file.

```batch
cd librime
copy /Y thirdparty\bin\*.dll build\bin\
copy /Y build\lib\Release\rime.dll build\bin\
cd build\bin
echo zhongzhouyunshurufa | Release\rime_api_console.exe > output.txt
```
