Hall A C++ Analyzer Software Development Kit
============================================

This is the standard Software Development Kit (SDK) for
[Hall A](http://hallaweb.jlab.org/) at [Jefferson Lab](http://www.jlab.org).
This version is compatible with version 1.7 of the analyzer.

21 Apr 2021

Overview
--------

This package contains a examples of user code for the Hall A analyzer.
It it intended as a starting point for building your own library of custom,
experiment-specific analysis modules.

The following examples are included for illustration
purposes. They do not do particularly interesting work, but do show
possibly useful programming techniques.

    UserModule:           new Physics Module
    UserScintillator:     a THaScintillator extended by user code
    UserDetector:         new detector
    UserApparatus:        new apparatus
    UserEvtHandler:       an example of an Event Type handler
    SkeletonModule:       example decoder module (for DAQ experts)
    
    db_U.u1.dat:          database for UserDetector with name "u1" contained
                          in apparatus named "U".
    db_R.s1.dat:          dto. for UserScintillator with name "s1" contained in
                          apparatus named "R" (right HRS), based on standard R.s1
    
    User_LinkDef.h:       ROOT dictionary link file, required to build the ROOT
                          interpreter dictionary. Even though you will not get
                          compilation errors, your library will not load if your
                          classes are not listed in this file at build time.
    
    CMakeLists.txt:       CMake build script, configured to build all
                          six example modules and to create a user library named
                          libUser.so.

Prerequisites
-------------

The following packages must be installed to support this SDK

* Hall A analyzer version 1.7
* ``ROOT`` version 6.
* ``CMake`` 3.5 or higher. 3.13 or higher recommended.

The ``$ANALYZER`` environment variable must be set and must point to the root
directory of the analyzer installation

```shell
echo $ANALYZER
```

Be sure that the ``root-config`` script is in your ``PATH``:

```shell
which root-config
```

should print the location of the script.

Practically all recent versions of Linux and macOS should work, using the default
system compiler. The compiler must be same as used for building the analyzer.

Compiling
---------

CMake is supported as of analyzer 1.7. For the CMake scripts to work with the SDK,
the analyzer must have been built and installed with CMake as well so that certain
configuration scripts have been generated.
Be sure that ``$ANALYZER`` is set correctly to point to the analyzer
*installation* directory. 

As usual, CMake expects code to be built out of source in a separate build directory.
As of CMake 3.13, creating and configuring the build directory can be done in a
single step from the command line (assuming the current directory is this SDK):

```shell
cmake -B build -S . 
```

One can add the usual CMake options to the command above to select a non-standard build
tool, configure the build type, an installation location, etc. Here is an example
that configures the directory ``build-debug`` for building a debug version with the
``ninja`` build tool, using ``<install-dir>`` as the installation prefix:

```shell
cmake -GNinja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=<install-dir> -B build-debug -S .
```
See ``cmake --help`` and ``man cmake`` for more information.

To build the SDK, run CMake again, pointing it to the build directory just created:

```shell
cmake --build build [--clean-first] [-jN]
```
where, as usual, you can add the optional ``-j`` argument for a parallel build
on ``N`` CPUs. Cleaning the build directory with the ``--clean-first`` option
should rarely be necessary.

In case of repeated build problems, try completely deleting and recreating the build 
directory. This may be necessary, for example, when switching to a different analyzer
installation, a different version of ROOT, a different ``git`` branch that contains
additional or fewer source files, or similar major environment changes.

Installation
------------

To install the SDK files (library and headers) in appropriate directories under
``<install-dir>``, do

```shell
cmake --install build
```

Uninstallation is not directly supported, but the following shell
command will remove all files (but not directories) most recently installed:

```shell
xargs rm [-v] < build/install_manifest.txt
```

Using your library
------------------

In your analysis macro/script (or interactively on the analyzer
command line) issue the following command

```
gSystem->Load("libUser")
```
Of course, replace ``libUser`` with the actual path to and name of your
library, for instance ``$EXPERIMENT/lib/libKaon``. You don't need to
specify an explicit path, however, if the location of the library is
included in ``LD_LIBRARY_PATH``.

Once the library is loaded, your classes will be available like any other
ROOT and Analyzer classes. For example, you can inspect their interface
using

```
.class UserModule
```
create instances,

```
m = new UserModule("u","User Module")
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
  but in general for every implementation file there must be a corresponding header file.
  You may define more than one class per header/implementation file.
  See ``CMakeLists.txt`` for instructions how to add standalone headers. 
2. Adapt ``CMakeLists.txt``. Typically, you need to
    * Change ``PACKAGE`` to give your library a clear and meaningful name
    (don't want dozens of ``libUser.so`` containing mystery code)
    * Edit ``src`` to specify your own ``.cxx `` source files
3. Rename ``User_LinkDef.h`` to ``<libname>_LinkDef.h`` and modify
  it to specify your own class name(s). All classes defined with
  ClassDef macros MUST be listed here.

Of course, you can delete the original ``User*`` files once you don't need
them for guidance any more. They are not required for building a user library.

Compatibility
-------------

The examples have been tested with C++ Analyzer 1.7. Earlier analyzer versions will
not work. Later versions may work, but this is not guaranteed.
