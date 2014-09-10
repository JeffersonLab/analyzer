//*-- Author :    Ole Hansen   13/05/00

//////////////////////////////////////////////////////////////////////////
//
// THaCodaRun
//
// Description of a run based on CODA data.
//
//////////////////////////////////////////////////////////////////////////

#include "THaCodaRun.h"
#include "THaCodaData.h"
#include <cassert>
#include "evio.h"

using namespace std;

//_____________________________________________________________________________
THaCodaRun::THaCodaRun( const char* description )
  : THaRunBase(description), fCodaData(0)
{
  // Normal & default constructor
}

//_____________________________________________________________________________
THaCodaRun::THaCodaRun( const THaCodaRun& rhs )
  : THaRunBase(rhs), fCodaData(0)
{
  // Normal & default constructor
}

//_____________________________________________________________________________
THaCodaRun::~THaCodaRun()
{
  // Destructor. The CODA data will be closed by the THaCodaData
  // destructor if necessary.

  delete fCodaData; fCodaData = 0;
}

//_____________________________________________________________________________
THaCodaRun& THaCodaRun::operator=(const THaRunBase& rhs)
{
  // Assignment operator

  if( this != &rhs ) {
    THaRunBase::operator=(rhs);
    delete fCodaData; fCodaData = 0;
  }
  return *this;
}

//_____________________________________________________________________________
Int_t THaCodaRun::ReturnCode( Int_t evio_retcode )
{
  // Convert EVIO return codes to THaRunBase codes

  switch( evio_retcode ) {

  case S_SUCCESS:
    return READ_OK;

  case EOF:
  case S_EVFILE_UNXPTDEOF:
    return READ_EOF;

  case S_EVFILE_TRUNC:
    return READ_ERROR;

  case S_EVFILE_BADBLOCK:
  case S_EVFILE_BADHANDLE:
  case S_EVFILE_ALLOCFAIL:
  case S_EVFILE_BADFILE:
  case S_EVFILE_BADSIZEREQ:
    return READ_FATAL;

    // The following indicate a programming error and so should be trapped
  case S_EVFILE_UNKOPTION:
  case S_EVFILE_BADARG:
  case S_EVFILE_BADMODE:
    assert( false );
    return READ_FATAL;

  default:
    return READ_ERROR;
  }
  return READ_FATAL; // not reached
}

//_____________________________________________________________________________
Int_t THaCodaRun::Close()
{
  // Close the CODA run

  fOpened = kFALSE;
  if( !IsOpen() )
    return 0;

  return ReturnCode( fCodaData->codaClose() );
}

//_____________________________________________________________________________
const UInt_t* THaCodaRun::GetEvBuffer() const
{
  // Return address of the event buffer allocated and filled
  // by fCodaData->codaRead()

  return fCodaData->getEvBuffer();
}

//_____________________________________________________________________________
Bool_t THaCodaRun::IsOpen() const
{
  return fCodaData->isOpen();
}

//_____________________________________________________________________________
Int_t THaCodaRun::ReadEvent()
{
  // Read one event from CODA file.

  return ReturnCode( fCodaData->codaRead() );
}

//_____________________________________________________________________________
ClassImp(THaCodaRun)
