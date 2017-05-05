Hall A C++ Analyzer Software Development Kit
============================================

This is the standard Software Development Kit (SDK) for
[Hall A](http://hallaweb.jlab.org/) at [Jefferson Lab](http://www.jlab.org).
This version has been tested with Version 1.6 of the Analyzer.

5 May 2017

Overview
--------

This package contains a examples of user code for the C++ Analyzer.
It is a good starting point for building experiment-specific 
module libraries. 

The following examples are included. These are examples for illustration
purposes only. They don't do particularly valuable work, but do contain
examples of possibly useful programming techniques.

UserModule:	      new Physics Module
UserScintillator:     a THaScintillator extended by user code
UserDetector:	      new detector
UserApparatus:	      new apparatus
UserEvtHandler:	      an example of an Event Type handler

db_U.u1.dat:	      database for UserDetector with name "u1"
db_R.s1.dat:	      dto. for UserScintillator with name "s1" contained in
		      apparatus named "R" (right HRS), based on standard R.s1

SConstruct:	      Main scons build file
SConscript.py:	      Library build script for scons, configured to build all 
		      five example modules and to create a user library named libUser.so.

configure.py:	      configure module for scons
darwin64.py:	      MacOSX environment module for scons
linux64.py:	      Linux 64-bit environment module for scons
linux32.py:	      Linux 32-bit environment module for scons

User_LinkDef.h:	      ROOT dictionary link file, required to build the CINT
		      dictionary. Even though you will not get compilation
		      errors, your library will not load if your classes 
		      are not completely listed in this file at build time.

Incorporating your own code
---------------------------

- Write header (.h) and implementation (.cxx) files as needed, along the lines
  of the provided examples. The module(s) can have any name(s) you like,
  but for every implementation file there must be a corresponding header file.
  You may define more than one class per header/implementation file.
- Adapt SConscript.py 

  At the minimum:
  * Edit userheaders to specify your own .h files (necessary for ROOT dictionary)
  * Edit list to specify your own .cxx files 
  * Change sotarget to give your library a clear and meaningful name 
    (don't want dozens of libUser.so containing mystery code)
- Rename User_LinkDef.h to <sotarget>_LinkDef.h and modify to specify your own
  class name(s). All classes defined with ClassDef macros MUST be listed here.

Of course, you can delete the User* files once you don't need them for
guidance any more. They are not required for building a user library.

Before building, verify that ANALYZER is set and points to the root
of the C++ Analyzer installation that you want to build against.
$ANALYZER should contain the header files in either $ANALYZER/include
or $ANALYZER/{src,hana_decode}. Normally, all you have to do is
to set $ANALYZER; the Makefile will determine the include directories 
automatically.

Compiling
---------

To build, just type

   scons
            
To clean up object files, ROOT dictionary files, and libary:

   scons -c

To build in debug mode:

   scons debug=1

If all goes well, you a shared library callled lib$PACKAGE.so will
be created, e.g. libUser.so.

You will need to move the newly built library to the appropriate location
(replay directory or analyzer directory) to be used in analysis.

Using your library
------------------

Somewhere in your analysis macro/script, or interactively on the CINT
command line, issue the following command:

gSystem->Load("libUser.so")

Of course, replace "libUser.so" the actual path to and name of your library,
for instance "$EXPERIMENT/lib/libKaon.so". You don't need to specify
a path if the location of the library is in your LD_LIBRARY_PATH.

Once the library is loaded, your classes will be available like any other
ROOT and Analyzer classes. For example, you can inspect their interface
using

.class UserModule

create instances,

UserModule* m = new UserModule("u","User Module")

and add them to the analysis chain,

gHaPhysics->Add(m)

For modules/detectors etc. that need database files, be sure to
put appropriate files in your database directory.

Compatibility
-------------

The examples have been tested with C++ Analyzer 1.6
