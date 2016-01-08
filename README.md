Hall A C++ Analyzer
===================

This is the standard data analysis software for
[Hall A](http://hallaweb.jlab.org/) at [Jefferson Lab](http://www.jlab.org).
The current stable version is 1.5.29.

Overview
--------
The analyzer is an object-oriented, highly modular and extensible
framework built on top of the [ROOT](http://root.cern.ch) framework
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
analysis framework jointly with [Hall C](http://www.jlab.org/Hall-C/)
which will be based on the current Hall A software, and (2) to update
the analyzer for the more demanding analysis requirements of 12 GeV
experiments.

Further information can be found online [here](http://hallaweb.jlab.org/podd/).

Compiling
---------
The analyzer may be compiled with either make or SCons. The following
software is prerequisite for analyzer 1.5

* [ROOT](http://root.cern.ch) version 4 or higher, latest version (5.34)
  strongly recommended. root-config must be in your PATH.

For analyzer 1.6 or higher, you will also need

* [EVIO](https://coda.jlab.org/drupal/content/event-io-evio) version 4.0
 or higher. EVIO_INCDIR and EVIO_LIBDIR must be set to point to the EVIO
 installation locations.

### Compiling with make
    make

### Compiling with scons
Ensure that you have SCons version is 2.1.0 or higher. Then simply do

    scons

### Additional SCons features
To do the equivalent of "make clean", do
`scons -c`
To compile with debug capabilities, do
`scons debug=1`

Contributing
------------
To participate in development, please contact Ole Hansen.

Note that issues may be submitted to the
[github issue tracker](https://github.com/JeffersonLab/analyzer/issues?state=open)
by anyone. For issues that may be of general interest, you are strongly
encouraged to use the issue tracker system. In this way, all active
developers are notified and able to respond quickly.