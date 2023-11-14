Analyzer Integration Tests
==========================

This directory contains a small replay and test setup to check the overall
functionality of the analyzer.

Usage
-----
First, set up the analyzer to be tested. The quickest way to do so is typically
to run the provided setup script. To run from the CMake build location, do

```shell
source BUILDDIR/cmake/setup_inbuild.(c)sh
```
where BUILDDIR is the top-level build directory that was specified
when configuring with CMake.

To test an analyzer from its installation location (`make install`), instead do

```shell
source INSTALLDIR/bin/setup.(c)sh
```
where INSTALLDIR is the installation directory (i.e. `CMAKE_INSTALL_PREFIX`).
To check which executable is set up and will be run, do `which analyzer`.

Next, run the test driver script. The script takes one optional argument,
the name of a directory for temporary files. About 1 GB free space should be
available in this location. If the directory does not exist, it will be created.
If no argument is given, `/var/tmp` is used.
Note that the driver script must be *run*, not sourced:

```shell
THISDIR/run_tests.sh [directory_for_temporary_files]
```

The script downloads a relatively large (700 MB) raw data file to the specified
work directory and analyzes (replays) this file, writing an output ROOT file
(ca. 200 MB) to the same location as the raw data. The output is then compared
to reference data in `ref.log` and `ref.root`.  If successful, the script exits
with status 0, otherwise with a non-zero error code.

If the raw data file has already been downloaded, it will not be downloaded
again. This obviously saves time when running the tests repeatedly for
troubleshooting. An error will occur if the file checksum does not match.
In this case, delete the file and try downloading it again. 

The analysis should take between one and two minutes on a modern CPU, but the
time can get a little long on older machines. There is no progress indicator.
Be patient. The analysis is very unlikely to get stuck.

Notes about the data
--------------------
The data analyzed here are from an optics calibration run taken in March 2012
as part of the g2p experiment in Hall A,
[run number 3132](https://hallaweb.jlab.org/halog/html/1203_archive/120312194519.html).
The experimental configuration was as follows:

* Left HRS spectrometer with vertical drift chamber (VDC), two scintillator 
  planes, gas Cherenkov, and preshower/shower calorimeter.
* 2.253 GeV beam energy. Ca. 85 nA beam current. Unrastered beam.
* 2.228 GeV/c central HRS momentum. H-elastic setting ("delta = +1%").
  Septum magnet @718A. Ca. 6.4 degrees central scattering angle.
* 12C "thin" foil target (single foil).
* Sieve slit collimator.

The primary prupose of these data is to provide valid input for the analyzer
regardless of physics content.
There is no particular significance to this choice of data other than that it
was conveniently available at the time of writing. The experimental setup was
somehwat unusual in that it employed septa in front of the HRS spectrometers,
which requires non-standard optics coefficients and a special calculation of
the effective central scattering angle. The experiment also used a non-standard
polarized liquid ammonia target.
The optics calibration in the database appears to be reasonably good; other
calibrations (e.g. VDC drift time, scintillator timing, PMT gain matching,
Cherenkov amplitude) may only be approximate.

In the future, different data may be chosen for the integration test suite.
