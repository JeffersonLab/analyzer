/////////////////////////////////////////////////////////////////////
//
//   THaCodaData
//   Abstract class of CODA data.
//
//   THaCodaData is an abstract class of CODA data.  Derived
//   classes will be typically either a CODA file (a disk
//   file) or a connection to the ET system.  Public methods
//   allow to open (i.e. set up), read, write, etc.
//
//   author Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "Decoder.h"
#include "THaCodaData.h"

using namespace Decoder;

ClassImp(Decoder::THaCodaData)

THaCodaData::THaCodaData() {
   evbuffer = new int[MAXEVLEN];         // Raw data     
};

THaCodaData::~THaCodaData() { 
   delete [] evbuffer;
};







