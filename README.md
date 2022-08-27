Hall A C++ Analyzer
===================

This is the standard data analysis software for
[Hall A](http://hallaweb.jlab.org/) at [Jefferson Lab](https://www.jlab.org).
The current stable version is 1.7.0.
This software is also known as "Podd".

Overview
--------
The analyzer is an object-oriented, modular and extensible
physics event processing framework built on top of the
[ROOT](https://root.cern) data analysis framework from CERN.
Classes are available for the most common analysis tasks
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

The analyzer core libraries are also used by the 
[Hall C](https://www.jlab.org/Hall-C/) analysis software
[hcana](https://github.com/JeffersonLab/hcana) as well as the
analysis package for the SuperBigBite series of experiments
in Hall A,
[SBS-offline](https://github.com/JeffersonLab/SBS-offline).

More documentation can be found in the 
[Wiki](https://redmine.jlab.org/projects/podd/wiki/).

System Requirements
-------------------
* Linux or macOS with a C++11-capable compiler (gcc 4.8+ or clang 9+).
* Essentially any hardware powerful enough to support ROOT 6 will
  support the analyzer as well.

Dependencies
------------
* [ROOT](https://root.cern.ch) version 6 or higher. The latest version
  is recommended. For the analyzer to find ROOT,
  either `$ROOTSYS` must point to the top of the ROOT installation
  or the `root-config` script must be in your PATH.

* [EVIO](https://coda.jlab.org/drupal/content/event-io-evio) version 4.0
  or higher. If not installed, the build system will download and build
  a copy of EVIO automatically. This requires Internet access.
  If using a preinstalled version, `$CODA` must point to the top of its
  installation location.

* [CMake](https://cmake.org) 3.5 or higher (3.15 or newer recommended). 

* GNU make or [ninja](https://ninja-build.org/).

* Instead of CMake and make/ninja, [SCons](https://scons.org) 2.3 or newer
  (3.0 or newer recommended) in combination with Python 2.7 or newer
  can be used (see below).
  
Building
--------
The build system is a standard CMake setup. In the top-level analyzer
directory, the following commands will configure, build and install 
the software:

```shell
cmake -DCMAKE_INSTALL_PREFIX=$HOME/local/analyzer -B builddir -S .
cmake --build builddir -j8
cmake --install builddir
```

Here, `$HOME/local/analyzer` is an example installation destination.
Modify this variable as appropriate. If omitted, it defaults to `/usr/local`.
Additional CMake variables and options can be set as desired, for example
`-DCMAKE_BUILD_TYPE=Debug` or `-GNinja`. Consult the 
[CMake documentation](https://cmake.org/cmake/help/latest/index.html)
for details.

The analyzer can be run directly form the build location
(`builddir/apps/analyzer` in the example above) without any further 
configuration. However, installation is recommended since it is required for
building analyzer plugin modules. To work most effectively with the installed version,
several environment variables should be set appropriately. This can be done
conveniently with the included setup scripts, for example, for the bash shell:

```bash
source $HOME/local/analyzer/bin/setup.sh
```

Alternatively, if [environment modules](http://modules.sourceforge.net)
are available on your system, the following instructions will set up the
analyzer environment:

```shell
module use $HOME/local/analyzer/share/modulefiles
module load analyzer
```

The modulefiles may also be copied to a central location for such files, e.g.
`$HOME/local/modulefiles`.

The binaries are built with `RPATH` set for the build or install location, as
applicable, so including the library directory in `(DY)LD_LIBRARY_PATH` should 
not be necessary.

### Building with SCons (deprecated)

For backward compatibility, the [SCons](https://scons.org) build system is 
still supported in this version of the analyzer, although it is no longer
recommended and will be removed in a future version. SCons requires Python. 
The following steps will accomplish the same as the CMake example above:

```bash
export SCONS_INSTALL_PREFIX=$HOME/local/analyzer
scons -j8
scons install
```

Documentation
-------------
Please see the [Wiki](https://redmine.jlab.org/projects/podd/wiki/) 

Contributing
------------
To participate in development, please contact
[the developers](https://redmine.jlab.org/projects/podd/)

Bug reports and other issues may be submitted to the
[Redmine issue tracker](https://redmine.jlab.org/projects/podd/issues/)
by anyone. You are strongly encouraged to use the issue tracker system.
In this way, all active developers are notified and able to respond quickly.
