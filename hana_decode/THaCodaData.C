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

#include "THaCodaData.h"
#include "evio.h"
#include <cassert>

namespace Decoder {

//_____________________________________________________________________________
THaCodaData::THaCodaData() {
   evbuffer = new UInt_t[MAXEVLEN];         // Raw data
};

//_____________________________________________________________________________
THaCodaData::~THaCodaData() {
   delete [] evbuffer;
};

//_____________________________________________________________________________
Int_t THaCodaData::ReturnCode( Long64_t evio_retcode )
{
  // Convert EVIO return codes to THaRunBase codes

  switch( evio_retcode ) {

  case S_SUCCESS:
    return CODA_OK;

  case EOF:
  case S_EVFILE_UNXPTDEOF:
    return CODA_EOF;

  case S_EVFILE_TRUNC:
    return CODA_ERROR;

  case S_EVFILE_BADBLOCK:
  case S_EVFILE_BADHANDLE:
  case S_EVFILE_ALLOCFAIL:
  case S_EVFILE_BADFILE:
  case S_EVFILE_BADSIZEREQ:
    return CODA_FATAL;

    // The following indicate a programming error and so should be trapped
  case S_EVFILE_UNKOPTION:
  case S_EVFILE_BADARG:
  case S_EVFILE_BADMODE:
    assert( false );
    return CODA_FATAL;

  default:
    return CODA_ERROR;
  }
  return CODA_FATAL; // not reached
}

}

ClassImp(Decoder::THaCodaData)
