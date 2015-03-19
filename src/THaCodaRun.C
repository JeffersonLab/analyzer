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

using namespace std;
using namespace Decoder;

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
