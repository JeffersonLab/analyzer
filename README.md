Hall A C++ Analyzer
===================

[![Build Status](https://travis-ci.org/JeffersonLab/analyzer.svg?branch=master)](https://travis-ci.org/JeffersonLab/analyzer)

This is the standard data analysis software for
[Hall A](http://hallaweb.jlab.org/) at [Jefferson Lab](https://www.jlab.org).
The current stable version is 1.6.3.

Overview
--------
The analyzer is an object-oriented, highly modular and extensible
framework built on top of the [ROOT](https://root.cern.ch) framework
from CERN.  Classes are available for the most common analysis tasks
involving data from the standard Hall A experimental equipment, in
particular the HRS spectrometers and detectors. Standard physics
calculations for single arm (e,e'), conincidence (e,e'X) and
photoproduction reactions are available, as well as for auxiliary
tasks such as energy loss corrections, vertex position calculations,
etc.

A Software Development Kit (SDK) is included that provides users with a
rapid development environment for building experiment-specific extension
libraries. One can quickly implement new detectors, physics computation
modules and even entire spectrometers.

Currently, major efforts are underway (1) to develop an improved
analysis framework jointly with [Hall C](https://www.jlab.org/Hall-C/)
which will be based on the current Hall A software, and (2) to update
the analyzer for the more demanding analysis requirements of 12 GeV
experiments.

For more information, please see the [Wiki](https://redmine.jlab.org/projects/podd/wiki/).

Compiling
---------
The analyzer may be compiled with either make or SCons (recommended). The following
are the main prerequisites for analyzer 1.6:

* [ROOT](https://root.cern.ch) version 5 or higher. The latest version
  (currently 6.12/06) is strongly recommended. root-config must be in your PATH.

* [EVIO](https://coda.jlab.org/drupal/content/event-io-evio) version 4.0
  or higher. CODA must be set to point to the top of the installation location.

### Compiling with scons
Ensure that you have SCons version is 2.1.0 or higher. Then simply do

    scons

### Additional SCons features
To do the equivalent of "make clean", do
`scons -c`
To compile with debug capabilities, do
`scons debug=1`

### Compiling with CMake (experimental)

Do the usual CMake setup

```
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=$HOME/local/analyzer ..
make -jN install
```

Here `$HOME/local/analyzer` is an example installation destination;
modify as appropriate. You will need to add the `bin` and `lib` sub-directories
under the installation prefix to your environment:

```
export PATH=$HOME/local/analyzer/bin:$PATH
export LD_LIBRARY_PATH=$HOME/local/analyzer/lib:$LD_LIBRARY_PATH
```

On 64-bit Linux, the library directory is usually `lib64` instead of `lib`.

### Compiling with make (deprecated)
    make


Contributing
------------
To participate in development, please contact
[the developers](https://redmine.jlab.org/projects/podd/)

Bug reports and other issues may be submitted to the
[Redmine issue tracker](https://redmine.jlab.org/projects/podd/issues/)
by anyone. You are strongly encouraged to use the issue tracker system.
In this way, all active developers are notified and able to respond quickly.
