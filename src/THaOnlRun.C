//*-- Author :    Robert Michaels    March 2001 

//////////////////////////////////////////////////////////////////////////
//
// THaOnlRun :: THaRun
//
// Description of an online run using ET system to get data in real time.
// Inheriting from THaRun, this is essentially a TNamed object with
// additional support for a run number, filename, CODA file IO, 
// and run statistics.
//
//////////////////////////////////////////////////////////////////////////

#include "THaOnlRun.h"

#include <iostream>
#include "THaCodaData.h"
#include "THaEtClient.h"

using namespace std;

//______________________________________________________________________________
THaOnlRun::THaOnlRun() : THaRun()
{
  // Default constructor
  fDate.Set();  // NOW is the correct date
  delete fCodaData;
  fCodaData = new THaEtClient();
  fMode = 1;
  ClearEventRange();
}

//______________________________________________________________________________
THaOnlRun::THaOnlRun( const char* computer, const char* session, UInt_t mode) :
  THaRun(session), fComputer(computer), fSession(session), fMode(mode)
{
  // Normal constructor

  fDate.Set();  // NOW is the correct date
  delete fCodaData;
  fCodaData = new THaEtClient();
  ClearEventRange();
}

//______________________________________________________________________________
THaOnlRun::THaOnlRun( const THaOnlRun& rhs ) : 
  THaRun(rhs), fComputer(rhs.fComputer), fSession(rhs.fSession),
  fMode(rhs.fMode), fOpened(0)
{
  // Copy constructor
  delete fCodaData;
  fCodaData = new THaEtClient();
}

//_____________________________________________________________________________
THaOnlRun& THaOnlRun::operator=(const THaOnlRun& rhs)
{
  // Assignment operator.

  if (this != &rhs) {
     THaRun::operator=(rhs);
     delete fCodaData;
     fCodaData = new THaEtClient;
     fComputer = rhs.fComputer;
     fSession  = rhs.fSession;
     fMode     = rhs.fMode;
     fOpened   = 0;
  }
  return *this;
}

//______________________________________________________________________________
Int_t THaOnlRun::OpenFile()
{
  // Open ET connection. 

  if (fComputer.IsNull() || fSession.IsNull()) {
      return -2;   // must set computer and session, at least;
  } 

  return fCodaData->codaOpen(fComputer, fSession, fMode);

}

//______________________________________________________________________________
Int_t THaOnlRun::OpenFile( const char* computer, const char* session, UInt_t mode )
{
// Set the computer name, session name, and mode, then open the 'file'.
// It isn't really a file.  It is an ET connection.

  fComputer = computer;
  fSession = session;
  fMode = mode;
  return OpenFile();
}

ClassImp(THaOnlRun)
