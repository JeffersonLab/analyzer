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
#include "TClass.h"
#include <cassert>

using namespace std;
using namespace Decoder;

//_____________________________________________________________________________
THaCodaRun::THaCodaRun( const char* description )
  : THaRunBase(description), fCodaData(nullptr)
{
  // Normal & default constructor
}

//_____________________________________________________________________________
THaCodaRun::THaCodaRun( const THaCodaRun& rhs )
  : THaRunBase(rhs), fCodaData(nullptr)
{
  // Normal & default constructor
}

//_____________________________________________________________________________
// Destructor. The CODA data will be closed by the THaCodaData
// destructor if necessary.
THaCodaRun::~THaCodaRun() = default;

//_____________________________________________________________________________
THaCodaRun& THaCodaRun::operator=(const THaRunBase& rhs)
{
  // Assignment operator

  if( this != &rhs ) {
    THaRunBase::operator=(rhs);
    fCodaData = nullptr;
  }
  return *this;
}

//_____________________________________________________________________________
Int_t THaCodaRun::ReturnCode( Int_t coda_retcode )
{
  // Convert THaCodaData return codes to THaRunBase codes

  switch( coda_retcode ) {
  case CODA_OK:
    return READ_OK;

  case CODA_EOF:
    return READ_EOF;

  case CODA_ERROR:
    return READ_ERROR;

  case CODA_FATAL:
    return READ_FATAL;

  default:
    return READ_ERROR;
  }
  // not reached
}

//_____________________________________________________________________________
Int_t THaCodaRun::Close()
{
  // Close the CODA run

  fOpened = false;
  if( !IsOpen() )
    return READ_OK;

  return ReturnCode( fCodaData->codaClose() );
}

//_____________________________________________________________________________
Int_t THaCodaRun::GetCodaVersion() // NOLINT(misc-no-recursion)
{
  // Get CODA format version of current data source.
  // Returns either 2 or 3, or -1 on error (file not open, etc.)

  if( fDataVersion > 0 ) // Override with user-specified value
    return fDataVersion;
  //FIXME Workaround for this function not being virtual
  if( !fCodaData && IsA() != THaCodaRun::Class() ) {
    if( IsA()->GetMethodAllAny("GetDataVersion") ==
        THaCodaRun::Class()->GetMethodAllAny("GetDataVersion") ) {
      return -1;
    }
    return GetDataVersion();
  }
  assert(fCodaData);
  return (fDataVersion = fCodaData->getCodaVersion());
}

//_____________________________________________________________________________
Int_t THaCodaRun::SetCodaVersion( Int_t vers )
{
  const char* const here = "THaCodaRun::SetCodaVersion";

  if (vers != 2 && vers != 3) {
    Error( here, "Illegal CODA version = %d. Must be 2 or 3.", vers );
    return -1;
  }
  if( IsOpen() ) {
    Error( here, "CODA data source is open, cannot set version" );
    return -1;
  }
  return (fDataVersion = vers);
}

//_____________________________________________________________________________
const UInt_t* THaCodaRun::GetEvBuffer() const
{
  // Return address of the event buffer allocated and filled
  // by fCodaData->codaRead()

  assert( fCodaData );
  return fCodaData->getEvBuffer();
}

//_____________________________________________________________________________
Bool_t THaCodaRun::IsOpen() const
{
  assert( fCodaData );
  return fCodaData->isOpen();
}

//_____________________________________________________________________________
Int_t THaCodaRun::ReadEvent()
{
  // Read one event from CODA file.

  assert( fCodaData );
  return ReturnCode( fCodaData->codaRead() );
}

//_____________________________________________________________________________
ClassImp(THaCodaRun)
