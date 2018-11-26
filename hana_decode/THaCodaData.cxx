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
#include <iostream>
#include <cstring>  // for strdup
#include <cstdlib>  // for free

using namespace std;

namespace Decoder {

//_____________________________________________________________________________
THaCodaData::THaCodaData() : handle(0), fIsGood(true)
{
   evbuffer = new UInt_t[MAXEVLEN];         // Raw data
};

//_____________________________________________________________________________
THaCodaData::~THaCodaData()
{
   delete [] evbuffer;
};

//_____________________________________________________________________________
Int_t THaCodaData::getCodaVersion()
{
  // Get CODA version from current data source
  int32_t EvioVersion = 0;
  char *d_v = strdup("v");
  int status = evIoctl(handle, d_v, &EvioVersion);
  fIsGood = (status == S_SUCCESS);
  free(d_v);
  if( status != S_SUCCESS ) {
    staterr("ioctl",status);
    codaClose();
    return -1;
  }
  cout << "Evio file EvioVersion = "<< EvioVersion << endl;
  return (EvioVersion < 4) ? 2 : 3;
}

//_____________________________________________________________________________
void THaCodaData::staterr(const char* tried_to, Int_t status) const
{
  // staterr gives the non-expert user a reasonable clue
  // of what the status returns from evio mean.
  // Note: severe errors can cause job to exit(0)
  // and the user has to pay attention to why.
  if (status == S_SUCCESS) return;  // everything is fine.
  if (status == EOF) {
    if(CODA_VERBOSE) {
      cout << endl << "Normal end of file " << filename << " encountered"
          << endl;
    }
    return;
  }
  cerr << endl << Form("THaCodaFile: ERROR while trying to %s %s: ",
      tried_to, filename.Data());
  Long64_t code = static_cast<Long64_t>(status);
  switch( code ) {
  case S_EVFILE_TRUNC :
    cerr << "Truncated event on file read. Evbuffer size is too small. "
    << endl;
    break;
  case S_EVFILE_BADBLOCK :
    cerr << "Bad block number encountered " << endl;
    break;
  case S_EVFILE_BADHANDLE :
    cerr << "Bad handle (file/stream not open) " << endl;
    break;
  case S_EVFILE_ALLOCFAIL :
    cerr << "Failed to allocate event I/O" << endl;
    break;
  case S_EVFILE_BADFILE :
    cerr << "File format error" << endl;
    break;
  case S_EVFILE_UNKOPTION :
    cerr << "Unknown file open option specified" << endl;
    break;
  case S_EVFILE_UNXPTDEOF :
    cerr << "Unexpected end of file while reading event" << endl;
    break;
  default:
    errno = status;
    perror(0);
  }
}

//_____________________________________________________________________________
Int_t THaCodaData::ReturnCode( Int_t evio_retcode )
{
  // Convert EVIO return codes to THaRunBase codes

  Long64_t code = static_cast<Long64_t>(evio_retcode);
  switch( code ) {

  case S_SUCCESS:
    return CODA_OK;

  case EOF:
    return CODA_EOF;

  case S_EVFILE_UNXPTDEOF:
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
