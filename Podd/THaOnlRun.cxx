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
#include "THaCodaData.h"
#include "THaEtClient.h"
#include "TClass.h"
#include "TError.h"
#include <stdexcept>
#include <memory>

using namespace std;

#if __cplusplus >= 201402L
# define MKETCLIENT make_unique<Decoder::THaEtClient>()
#else
# define MKETCLIENT unique_ptr<Decoder::THaEtClient>(new Decoder::THaEtClient)
#endif

//______________________________________________________________________________
THaOnlRun::THaOnlRun() : fMode(1)
{
  // Default constructor

  // NOW is the correct time
  TDatime now;
  SetDate(now);
  // Use ET client as data source
  fCodaData = MKETCLIENT;
}

//______________________________________________________________________________
THaOnlRun::THaOnlRun( const char* computer, const char* session, Int_t mode) :
  THaCodaRun(session), fComputer(computer), fSession(session), fMode(mode)
{
  // Normal constructor

  // NOW is the correct time
  TDatime now;
  SetDate(now);

  // Use ET client as data source
  fCodaData = MKETCLIENT;
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

  fCodaData = MKETCLIENT;
}

//_____________________________________________________________________________
THaOnlRun& THaOnlRun::operator=(const THaRunBase& rhs)
{
  // Assignment operator.

  if (this != &rhs) {
    THaCodaRun::operator=(rhs);
    try {
      const auto& obj = dynamic_cast<const THaOnlRun&>(rhs);
      fComputer = obj.fComputer;
      fSession  = obj.fSession;
      fMode     = obj.fMode;
    }
    catch( const std::bad_cast& e ) {}
    fCodaData = MKETCLIENT;
  }
  return *this;
}


//______________________________________________________________________________
Int_t THaOnlRun::Open()
{
  // Open ET connection. 

  static const char* const here = "Open";

  if (fComputer.IsNull() || fSession.IsNull()) {
    Error( "Open", "Computer and Session must be set. "
	   "Cannot open ET run." );
    return ReturnCode(CODA_FATAL);   // must set computer and session, at least
  } 

  Int_t st = fCodaData->codaOpen(fComputer, fSession, fMode);
  if( st == CODA_OK ) {
    // Get CODA version from data; however, if a version was set
    // explicitly by the user, use that instead
    if( fDataVersion <= 0 )
      fDataVersion = fCodaData->getCodaVersion();
    if( fDataVersion <= 0 ) {
      Error( here, "Cannot determine CODA version from ET. "
                   "Try analyzer->SetCodaVersion(2)");
      st = CODA_FATAL;
      Close();
    }
    //cout << "in THaOnlRun::Open:  coda version "<<fDataVersion<<endl;
    if( st == CODA_OK )
      fOpened = true;
  }
  return ReturnCode(st);
}

//______________________________________________________________________________
Int_t THaOnlRun::OpenConnection( const char* computer, const char* session, 
				 Int_t mode )
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
