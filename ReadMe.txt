developer notice:

build boost libraries with msvc toolset (see boost documentation);

then

copy <boost home>\boost\* to $(SolutionDir)\include\boost;
copy <boost home>\stage\lib\* to $(SolutionDir)\lib;
copy X:\Python27\include\* to $(SolutionDir)\include\python;
copy X:\Python27\libs\* to $(SolutionDir)\lib\python;

or, alternatively

add boost home path and Python include path to:
Visual Studio Options > Projects and Solutions > VC++ Directories > Include files;
add <boost home>\stage\lib and X:\Python27\libs to:
Visual Studio Options > Projects and Solutions > VC++ Directories > Library files;

voila! 

now, make a release build, and run $(SolutionDir)\release.bat.
the product will be place in $(SolutionDir)\output.


usage:

0. before you go...

install Python 2.7;

set Python path in $(WeaselRoot)\env.bat;
in this case, $(WeaselRoot) refers to the directory $(SolutionDir)\output\weasel.

issue the command populate-db.bat in $(SolutionDir)\data
in case you never had a database at %UserProfile%\.ibus\zime\zime.db.
if you don't, you will be noticed a message "NO SCHEMA" (in Chinese) once the IME is activated.

1. to install...

install.bat

2. to uninstall...

uninstall.bat

stop_service.bat
you may use this to terminate the service process.


