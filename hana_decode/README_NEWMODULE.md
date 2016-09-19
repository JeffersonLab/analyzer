Writing a new Object Oriented Decoder module
============================================

It is possible to develop and test without needing to
make changes to the analyzer source code.   The new
module can be dynamically loaded into the analyzer
similar to how new detectors or physics modules are loaded.
In the discussion below, we'll assume that we are developing
a module manufactured by Yoyodyne with a model number of 49.

Create Class for new module
---------------------------


Start writing a decoder for a new hardware module
by copying ``SkeletonModule.[Ch]`` or one of the existing
decoder classes to new files, ``Yoyodyne49Module.cxx``
and ``Yoyodyne49Module.h``, and edit those files to reflect the new class name, YoyodyneModule49.   Make sure that the class name is unique.

In the ``.cxx`` file, choose a module ID number and put it as the
second argument of ``ModuleType`` in the  ``Module::TypeIter_t`` line which should then be:
~~~~
Module::TypeIter_t Yoyodune49Module::fgThisType =
  DoRegister( ModuleType( "Decoder::Yoyodyne49Module", 49 ));
~~~~
This ID number is ideally the model number of the module,
but must be different from any of the module IDs already in
use in the analyzer.

In addition to the the source code for decoding the new module, create a Linkdef file.  (We assume here it is called ``Yoyodyne_Linkdef.h``).  Its contents are:
~~~~
#ifdef __CINT__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;
#pragma link C++ class Decoder::Yoyodyne49Module+;
#endif
~~~~

Compiling
---------

Copy the ``Makefile`` from the ``SDK`` directory.  change
the ``SRC`` line to:
~~~~
SRC = Yoyodnye49Module.cxx
~~~~
and the ``PACKAGE`` line to
~~~~
PACKAGE = Yoyodyne
~~~~

Running make should produce the file ``libYoyodyne.so``.

Usage
-----

Add the new module to your cratemap and detector parameter
files.  In your analysis script,
~~~~
gSystem->Load("src/libYoyodyne.so");
~~~~

Once debugging of the new decoder code is done, submit the
code to one of the software developers for inclusion into
the analyzer.
