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

//_____________________________________________________________________________
THaCodaRun::THaCodaRun( const char* description ) : 
  THaRunBase(description), fCodaData(NULL)
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
    delete fCodaData;
    fCodaData = NULL;
  }
  return *this;
}

//_____________________________________________________________________________
Int_t THaCodaRun::Close()
{
  // Close the CODA run

  fOpened = kFALSE;
  if( !IsOpen() )
    return 0;

  return fCodaData->codaClose();
}

//_____________________________________________________________________________
const Int_t* THaCodaRun::GetEvBuffer() const
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

  return fCodaData->codaRead();
}

//_____________________________________________________________________________
ClassImp(THaCodaRun)
