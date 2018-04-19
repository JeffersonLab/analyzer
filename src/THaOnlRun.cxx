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

#include "THaEtClient.h"
#include "TClass.h"
#include "TError.h"

using namespace std;

//______________________________________________________________________________
THaOnlRun::THaOnlRun() : THaCodaRun()
{
  // Default constructor

  // NOW is the correct time
  TDatime now;
  SetDate(now);
  // Use ET client as data source
  fCodaData = new THaEtClient();
  fMode = 1;
}

//______________________________________________________________________________
THaOnlRun::THaOnlRun( const char* computer, const char* session, UInt_t mode) :
  THaCodaRun(session), fComputer(computer), fSession(session), fMode(mode)
{
  // Normal constructor

  // NOW is the correct time
  TDatime now;
  SetDate(now);

  // Use ET client as data source
  fCodaData = new THaEtClient();
}

//______________________________________________________________________________
THaOnlRun::THaOnlRun( const THaOnlRun& rhs ) : 
  THaCodaRun(rhs), fComputer(rhs.fComputer), fSession(rhs.fSession),
  fMode(rhs.fMode)
{
  // Copy constructor

  // NOW is the correct time
  TDatime now;
  SetDate(now);

  fCodaData = new THaEtClient();
}

//_____________________________________________________________________________
THaOnlRun& THaOnlRun::operator=(const THaRunBase& rhs)
{
  // Assignment operator.

  if (this != &rhs) {
     THaCodaRun::operator=(rhs);
     if( rhs.InheritsFrom("THaOnlRun") ) {
       fComputer = static_cast<const THaOnlRun&>(rhs).fComputer;
       fSession  = static_cast<const THaOnlRun&>(rhs).fSession;
       fMode     = static_cast<const THaOnlRun&>(rhs).fMode;
     }
     //     delete fCodaData; //already done in THaCodaRun
     fCodaData = new THaEtClient;
  }
  return *this;
}


//______________________________________________________________________________
Int_t THaOnlRun::Open()
{
  // Open ET connection. 

  if (fComputer.IsNull() || fSession.IsNull()) {
    Error( "Open", "Computer and Session must be set. "
	   "Cannot open ET run." );
    return -2;   // must set computer and session, at least;
  } 

  Int_t st = fCodaData->codaOpen(fComputer, fSession, fMode);
  st = ReturnCode(st);
  if( st == READ_OK )
    fOpened = kTRUE;
  return st;
}

//______________________________________________________________________________
Int_t THaOnlRun::OpenConnection( const char* computer, const char* session, 
				 UInt_t mode )
{
  // Set the computer name, session name, and mode, then open the 'file'.
  // It isn't really a file.  It is an ET connection.

  fComputer = computer;
  fSession = session;
  fMode = mode;
  return ReturnCode( Open() );
}

//______________________________________________________________________________
ClassImp(THaOnlRun)
