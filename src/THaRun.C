//*-- Author :    Ole Hansen   13/05/00

//////////////////////////////////////////////////////////////////////////
//
// THaRun
//
// Description of a CODA run on disk.
//
//////////////////////////////////////////////////////////////////////////

#include "THaRun.h"
#include "THaEvData.h"
#include "THaCodaFile.h"
#include "THaGlobals.h"
#include "TClass.h"
#include "TError.h"
#include <iostream>
#include <cstring>

using namespace std;

static const int MAXSCAN = 5000;

//_____________________________________________________________________________
THaRun::THaRun( const char* fname, const char* description ) : 
  THaCodaRun(description), fFilename(fname), fMaxScan(MAXSCAN)
{
  // Normal & default constructor

  fCodaData = new THaCodaFile;  //Specifying the file name would open the file
}

//_____________________________________________________________________________
THaRun::THaRun( const THaRun& rhs ) : 
  THaCodaRun(rhs), fFilename(rhs.fFilename), fMaxScan(rhs.fMaxScan)
{
  // Copy ctor

  fCodaData = new THaCodaFile;
}

//_____________________________________________________________________________
THaRun& THaRun::operator=(const THaRunBase& rhs)
{
  // Assignment operator. We allow assignment to generic THaRun objects
  // so that we can copy run objects through a virtual operator=.
  // If 'rhs' is of different actual class (e.g. THaOnlRun - an ET run)
  // than this object, then its special properties are lost.

  if (this != &rhs) {
     THaCodaRun::operator=(rhs);
     if( rhs.InheritsFrom("THaRun") ) {
       fFilename   = static_cast<const THaRun&>(rhs).fFilename;
       fMaxScan    = static_cast<const THaRun&>(rhs).fMaxScan;
     } else {
       fMaxScan    = MAXSCAN;
     }
     //     delete fCodaData; //already done in THaRunBase
     fCodaData   = new THaCodaFile;
  }
  return *this;
}

//_____________________________________________________________________________
THaRun::~THaRun()
{
  // Destructor.

}

//_____________________________________________________________________________
void THaRun::Clear( const Option_t* opt )
{
  // Reset the run object.

  bool doing_init = !strcmp(opt,"INIT");

  THaCodaRun::Clear(opt);

  if( !doing_init ) {
    fMaxScan = MAXSCAN;
  }
}

//_____________________________________________________________________________
Int_t THaRun::Open()
{
  // Open CODA file for read-only access.

  static const char* const here = "Open";

  if( fFilename.IsNull() ) {
    Error( here, "CODA file name not set. Cannot open the run." );
    return -2;  // filename not set
  }
  
  Int_t st = fCodaData->codaOpen( fFilename );
  if( st == 0 )
    fOpened = kTRUE;
  return st;
}

//_____________________________________________________________________________
void THaRun::Print( Option_t* opt ) const
{
  THaCodaRun::Print( opt );
  cout << "Max # scan: " << fMaxScan  << endl;
  cout << "CODA file:  " << fFilename << endl;

}

//_____________________________________________________________________________
Int_t THaRun::ReadInitInfo()
{
  // Read initial info from the CODA file. This is done by prescanning
  // the run file in a local mini event loop via a local decoder.

  static const char* const here = "ReadInitInfo";

  // If pre-scanning of the data file requested, read up to fMaxScan events
  // and extract run parameters from the data.
  if( fMaxScan > 0 ) {
    UInt_t wanted_info = kDate|kRunNumber|kRunType|kPrescales;
    THaEvData* evdata = static_cast<THaEvData*>(gHaDecoder->New());
    UInt_t nev = 0;
    Int_t status = S_SUCCESS;
    while( nev<fMaxScan && !HasInfo(wanted_info) && 
	   (status = ReadEvent()) == S_SUCCESS ) {

      // Decode events. Skip bad events.
      nev++;
      if( evdata->LoadEvent( GetEvBuffer()) != THaEvData::HED_OK ) {
	Warning( here, "Error decoding event buffer at event %u", nev );
	continue;
      }

      // Inspect event and extract run parameters if appropriate
      Int_t st = Update( evdata );
      //FIXME: debug
      if( st == 1 )
	cout << "Prestart at " << nev << endl;
      else if( st == 2 )
	cout << "Prescales at " << nev << endl;

    }//end while
    delete evdata;

    if( status != S_SUCCESS && status != EOF ) {
      Error( here, "Error %d reading CODA file %s. Check file permissions.",
	     status, GetFilename());
      return status;
    }
  } //end if(fMaxScan>0)

  return 0;
}

//_____________________________________________________________________________
void THaRun::SetFilename( const char* name )
{
  // Set the name of the raw data file.
  // If the name changes, the run will become uninitialized and the input
  // will be closed.

  if( !name || fFilename != name ) {
    Close();
    fIsInit = kFALSE;
  }

  fFilename = name;
}

//_____________________________________________________________________________
void THaRun::SetNscan( UInt_t n )
{
  // Set number of events to prescan during Init(). Default is 5000.

  fMaxScan = n;
}

//_____________________________________________________________________________
ClassImp(THaRun)
