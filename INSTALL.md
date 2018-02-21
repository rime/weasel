# How to Rime with Weasel

## Preparation

Assume we already have a default installation of **Visual Studio 2015**.

Install dev tools: `git`, `cmake`
Download third-party libraries: `boost(>=1.60.0)`

## Checkout source code

Make sure you also checkout submodules `brise` and `librime` under `weasel` directory.

```batch
git clone --recursive https://github.com/rime/weasel.git
```

## Build and Install Weasel

Locate `weasel` source directory.

### Setup build environment

Edit `librime\env.bat.template`, save your copy as `librime\env.bat`.
Make sure `BOOST_ROOT` is set to `X:\path\to\boost_N_NN_N` in `librime\env.bat`.

Edit `weasel.props.template`, save your copy as `weasel.props`.
Make sure `BOOST_ROOT` is set to `X:\path\to\boost_N_NN_N` in `weasel.props`.
Forget about Python. It's not used anyway.

Then, start a Developer Command Prompt window.
Set `BOOST_ROOT` environmental variable:

```batch
set BOOST_ROOT=X:\path\to\boost_N_NN_N
```

### Build

```batch
cd weasel
build.bat all
```

Voila.

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

### Create installer packages

To create installer packages, you need to download and install [Unicode NSIS] (https://github.com/jimpark/unsis/releases).

Then go to `output` directory, right-click and choose "Compile Unicode NSIS Script" with `install.nsi` and `expansion.nsi`.

Installers will be generated in `output\archives` directory.
