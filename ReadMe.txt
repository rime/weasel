
* Howto Rime with Weasel

** Preparation

Fetch svn, cmake executables.
Fetch gtest, yaml-cpp, boost source.

** Set environment variables

Edit librime\env.bat.template, save it as librime\env.bat .

** Start the shell with librime\shell.bat

** Checkout source code

svn checkout https://rimeime.googlecode.com/svn/trunk/librime --username YOUR_ACCOUNT@gmail.com

svn checkout https://rimeime.googlecode.com/svn/trunk/weasel --username YOUR_ACCOUNT@gmail.com

** Build Boost

cd \code\boost_1_47_0
bjam toolset=msvc threading=multi link=static runtime-link=static stage --with-date_time --with-filesystem --with-regex --with-signals --with-system --with-thread

** Build GTest

cd \code\gtest-1.6.0
mkdir msbuild
cd msbuild
cmake ..
start gtest.sln

Make a Release build.

** Build Yaml-cpp

cd \code\yaml-cpp

Edit CMakeLists.txt MSVC build option for linking with static runtime (/MT):

#option(MSVC_SHARED_RT "MSVC: Build with shared runtime libs (/MD)" ON)
option(MSVC_SHARED_RT "MSVC: Build with shared runtime libs (/MD)" OFF)

mkdir msbuild
cd msbuild
cmake ..
start YAML_CPP.sln

Make a release build.

** Build librime

xcopy /S /I \code\gtest-1.6.0\include\gtest \code\librime\thirdparty\include\gtest\
copy \code\gtest-1.6.0\msbuild\Release\gtest*.lib \code\librime\thirdparty\lib\

xcopy /S /I \code\yaml-cpp\include\yaml-cpp \code\librime\thirdparty\include\yaml-cpp\
copy \code\yaml-cpp\msbuild\Release\libyaml-cppmt.lib \code\librime\thirdparty\lib\

cd \code\librime
msbuild.bat
start msbuild\rime.sln

Make a release build.

** Build weasel

Edit weasel\weasel.vsprops.template, save it to weasel\weasel.vsprops .

copy \code\librime\include\rime_api.h \code\weasel\include\
copy \code\librime\msbuild\lib\Release\librime.lib \code\weasel\lib\
copy \code\yaml-cpp\msbuild\Release\libyaml-cppmt.lib \code\weasel\lib\

start \code\weasel\weasel.sln

Make a Release build for weasel\output\WeaselServer.exe
Make a ReleaseHans build for weasel\output\weasels.ime
Make a ReleaseHant build for weasel\output\weaselt.ime

To run the input method, first register the IME from an admin console:

cd \code\weasel\output
rundll32 weasels.ime install

