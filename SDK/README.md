Hall A C++ Analyzer Software Development Kit
============================================

This is the standard Software Development Kit (SDK) for
[Hall A](http://hallaweb.jlab.org/) at [Jefferson Lab](http://www.jlab.org).
This version has been tested with version 1.7 of the analyzer.

24 Oct 2018

Overview
--------

This package contains a examples of user code for the Hall A analyzer.
It it intended as a starting point for building your own library of custom,
experiment-specific analysis modules.

The following examples are included for illustration
purposes. They don't do particularly valuable work, but do show
possibly useful programming techniques.

    UserModule:           new Physics Module
    UserScintillator:     a THaScintillator extended by user code
    UserDetector:         new detector
    UserApparatus:        new apparatus
    UserEvtHandler:       an example of an Event Type handler
    SkeletonModule:       example decoder module (for DAQ experts)
    
    db_U.u1.dat:          database for UserDetector with name "u1"
    db_R.s1.dat:          dto. for UserScintillator with name "s1" contained in
                          apparatus named "R" (right HRS), based on standard R.s1
    
    SConstruct:           SCons configuration script
    SConscript.py:        Library build script for SCons, configured to build all
                          six example modules and to create a user library named
                          libUser.so.
    
    User_LinkDef.h:       ROOT dictionary link file, required to build the ROOT
                          interpreter dictionary. Even though you will not get
                          compilation errors, your library will not load if your
                          classes are not listed in this file at build time.
    
    CMakeLists.txt:       CMake build script (experimental)

Prerequisites
-------------

The following packages must be installed to support this SDK

* Hall A analyzer
* ``ROOT`` version 5 or 6. Version 6 recommended
* ``SCons`` 2.3 or higher (requires Python) -OR- ``CMake`` version 3.5 or higher

The ``ANALYZER`` environment variable must be set and must point to the root
directory of the analyzer installation

```
echo $ANALYZER
```

Be sure that the ``root-config`` script is in your ``PATH``.

```
which root-config
```

should print the location of the script.

Practically all recent versions of Linux and macOS should work, using the default
system compiler.

Compiling
---------

To build the SDK examples, just type

```
scons [-jN]
```
where, as usual, you can add the optional ``-j`` argument for a parallel build
on ``N`` CPUs.

To build debug code

```
scons debug=1 [-jN]
```
If all goes well, a shared library named ``libUser.so`` will be created.

To clean up your build directory (rarely needed)

```
scons -c
```

SCons is very good at detecting which files need to be rebuilt, even if timestamps
change, for example when switching between ``git`` branches.

Installation
------------

The SCons scripts support installation under a certain
top-level directory, which should be outside of your build location.
The SCons installation location defaults to ``$HOME/.local`` and
can be set by defining ``SCONS_INSTALL_PREFIX``.

To install with SCons, type

```
scons install
```

To remove all files installed in this way, you can do

```
scons uninstall
```

Using your library
------------------

Somewhere in your analysis macro/script, or interactively on the analyzer
command line, issue the following command

```
gSystem->Load("libUser.so")
```
Of course, replace ``libUser.so`` with the actual path to and name of your
library, for instance ``$EXPERIMENT/lib/libKaon.so``. You don't need to
specify a path if the location of the library is included in ``LD_LIBRARY_PATH``.

Once the library is loaded, your classes will be available like any other
ROOT and Analyzer classes. For example, you can inspect their interface
using

```
.class UserModule
```
create instances,

```
UserModule* m = new UserModule("u","User Module")
```
and add them to the analysis chain,

```
gHaPhysics->Add(m)
```
For modules/detectors etc. that need database files, be sure to
put appropriate files in your database directory.

Incorporating your own code
---------------------------

1. Write header (``.h``) and implementation (``.cxx``) files as needed, along the lines
  of the provided examples. The module(s) can have any name(s) you like,
  but for every implementation file there must be a corresponding header file.
  You may define more than one class per header/implementation file.
2. Adapt ``SConscript.py`` (or ``CMakeLists.txt``). At the minimum
    * Change ``libname`` to give your library a clear and meaningful name
    (don't want dozens of ``libUser.so`` containing mystery code)
    * Edit ``src`` to specify your own ``.cxx `` source files
3. Rename ``User_LinkDef.h`` to ``<libname>_LinkDef.h`` and modify
  it to specify your own class name(s). All classes defined with
  ClassDef macros MUST be listed here.

Of course, you can delete the original ``User*`` files once you don't need
them for guidance any more. They are not required for building a user library.

Using CMake
-----------

CMake is being supported as of analyzer 1.7. For the CMake scripts to work with the SDK,
the analyzer must have been built with CMake as well so that certain configuration scripts
have been generated. Be sure that ``ANALYZER`` points to the root of either the
build or installation location of the analyzer.

As usual, CMake expects code to be built out of source. To do so, create a build
directory, change into it, then run CMake to configure the build

```
mkdir build
cd build
cmake ..
```
This will crete standard Unix Makefiles. To build the SDK, you can then run ``make``
as usual

```
make [-jN]
```
To configure an installation location, define ``CMAKE_INSTALL_PREFIX``

```
cmake -DCMAKE_INSTALL_PREFIX=<install-dir> ..
```
Then

```
make install
```
will install the SDK files (library and headers) in appropriate directories under
``<install-dir>``.

To clean up the build directory (rarely necessary)

```
make clean
```

Compatibility
-------------

The examples have been tested with C++ Analyzer 1.7
