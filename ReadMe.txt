
* How to Rime with Weasel

** Preparation

Assume we already have a default installation of Visual Studio 2008.

Fetch svn, cmake executables.
Fetch gtest, yaml-cpp, boost source.

** Set environment variables

Edit librime\env.bat.template, save it as librime\env.bat .

** Start VC command line tools from librime\shell.bat .

** Checkout source code

svn checkout https://rimeime.googlecode.com/svn/trunk/librime --username YOUR_ACCOUNT@gmail.com

svn checkout https://rimeime.googlecode.com/svn/trunk/weasel --username YOUR_ACCOUNT@gmail.com

** Build Boost

cd \code\boost_1_47_0
bjam toolset=msvc threading=multi link=static runtime-link=static stage --with-date_time --with-filesystem --with-regex --with-signals --with-system --with-thread

** Build GTest

cd \code\gtest-1.6.0
mkdir vcbuild
cd vcbuild
cmake ..
devenv gtest.sln /Build Release

** Build Yaml-cpp

cd \code\yaml-cpp

Edit CMakeLists.txt MSVC build option for linking with static runtime (/MT):

#option(MSVC_SHARED_RT "MSVC: Build with shared runtime libs (/MD)" ON)
option(MSVC_SHARED_RT "MSVC: Build with shared runtime libs (/MD)" OFF)

mkdir vcbuild
cd vcbuild
cmake ..
devenv YAML_CPP.sln /Build Release

** Build Kyoto Cabinet

TODO: We have to apply a patch in order to build version 1.2.70 of kyotocabinet with V.S. 2008.

Checkout patch files (including zlib 1.2.3):

cd \code
svn checkout http://rimeime.googlecode.com/svn/trunk/misc/kyotocabinet-1.2.70-vs2008-patch

Then, copy them to kyotocabinet source folder, and proceed with building the library.

copy \code\kyotocabinet-1.2.70-vs2008-patch\* \code\kyotocabinet-1.2.70\

cd \code\kyotocabinet-1.2.70
nmake -f VCmakefile
nmake -f VCmakefile binpkg
move kcwin32 \code\

** Build librime

xcopy /S /I \code\gtest-1.6.0\include\gtest \code\librime\thirdparty\include\gtest\
copy \code\gtest-1.6.0\vcbuild\Release\gtest*.lib \code\librime\thirdparty\lib\

xcopy /S /I \code\yaml-cpp\include\yaml-cpp \code\librime\thirdparty\include\yaml-cpp\
copy \code\yaml-cpp\vcbuild\Release\libyaml-cppmt.lib \code\librime\thirdparty\lib\

copy \code\kcwin32\include\*.h \code\librime\thirdparty\include\
copy \code\kcwin32\lib\*.lib \code\librime\thirdparty\lib\

cd \code\librime
vcbuild.bat

That will make a release build for librime.

** Build weasel

Edit weasel\weasel.vsprops.template, save it to weasel\weasel.vsprops .

copy \code\yaml-cpp\vcbuild\Release\libyaml-cppmt.lib \code\weasel\lib\
copy \code\kcwin32\lib\*.lib \code\weasel\lib\

cd \code\weasel

rem  for weasel\output\WeaselServer.exe
devenv weasel.sln /Build Release

rem  for weasel\output\weasels.ime
devenv weasel.sln /Build ReleaseHans

rem  for weasel\output\weasels.ime
devenv weasel.sln /Build ReleaseHant

Voila.

To try the input method, first register the IME from an administrator command window:

cd \code\weasel\output
rundll32 weasels.ime install
