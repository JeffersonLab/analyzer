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

using namespace std;

static const int   fgMaxScan   = 5000;
static const char* fgThisClass = "THaRun";

//_____________________________________________________________________________
THaRun::THaRun( const char* fname, const char* description ) : 
  THaCodaRun(description), fFilename(fname), fMaxScan(fgMaxScan)
{
  // Normal & default constructor

  fCodaData = new THaCodaFile;  //Specifying the file name would open the file
  FindSegmentNumber();
}

//_____________________________________________________________________________
THaRun::THaRun( const THaRun& rhs ) : 
  THaCodaRun(rhs), fFilename(rhs.fFilename), fMaxScan(rhs.fMaxScan)
{
  // Copy ctor

  fCodaData = new THaCodaFile;
  FindSegmentNumber();
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
     //     delete fCodaData; //already done in THaRunBase
     fCodaData   = new THaCodaFile;
     if( rhs.InheritsFrom(fgThisClass) ) {
       fFilename   = static_cast<const THaRun&>(rhs).fFilename;
       fMaxScan    = static_cast<const THaRun&>(rhs).fMaxScan;
       FindSegmentNumber();
     } else {
       fMaxScan    = fgMaxScan;
       fSegment    = 0;
     }
  }
  return *this;
}

//_____________________________________________________________________________
bool THaRun::operator==( const THaRunBase& rhs ) const
{
  if( THaCodaRun::operator!=(rhs) )
    return false;

  if(rhs.InheritsFrom(fgThisClass) &&
     fSegment != static_cast<const THaRun&>(rhs).fSegment )
    return false;

  return true;
}

//_____________________________________________________________________________
bool THaRun::operator!=( const THaRunBase& rhs ) const
{
  return ! (*this==(rhs));
}

//_____________________________________________________________________________
bool THaRun::operator<( const THaRunBase& rhs ) const
{
  if( THaCodaRun::operator<(rhs) )
    return true;

  if( THaCodaRun::operator==(rhs) && rhs.InheritsFrom(fgThisClass) &&
      fSegment < static_cast<const THaRun&>(rhs).fSegment )
    return true;

  return false;
}

//_____________________________________________________________________________
bool THaRun::operator>( const THaRunBase& rhs ) const
{
  if( THaCodaRun::operator>(rhs) )
    return true;

  if( THaCodaRun::operator==(rhs) && rhs.InheritsFrom(fgThisClass) &&
      fSegment > static_cast<const THaRun&>(rhs).fSegment )
    return true;

  return false;
}

//_____________________________________________________________________________
bool THaRun::operator<=( const THaRunBase& rhs ) const
{
  if( THaCodaRun::operator<(rhs) )
    return true;

  if( THaCodaRun::operator==(rhs) && rhs.InheritsFrom(fgThisClass) &&
      fSegment <= static_cast<const THaRun&>(rhs).fSegment )
    return true;

  return false;
}

//_____________________________________________________________________________
bool THaRun::operator>=( const THaRunBase& rhs ) const
{
  if( THaCodaRun::operator>(rhs) )
    return true;

  if( THaCodaRun::operator==(rhs) && rhs.InheritsFrom(fgThisClass) &&
      fSegment >= static_cast<const THaRun&>(rhs).fSegment )
    return true;

  return false;
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

  TString sopt(opt);
  bool doing_init = (sopt == "INIT");

  // Reset the entire run object EXCEPT when initializing a continuation
  // segment of a run. 
  // NB: Segment files in Hall A do not have header info, and so it would be 
  // difficult to initialize them on their own. Continuation segment run objects
  // should be created as copies of the initialized first segment, followed by
  // setting the file name.

  if( !doing_init || fSegment==0 )
    THaCodaRun::Clear(opt);

  if( !doing_init )
    fMaxScan = fgMaxScan;

  // Disable scanning for continuation segments - Hall A continuation runs
  // have no headers.
  if( fSegment>0 )
    fMaxScan = 0;
}

//_____________________________________________________________________________
Int_t THaRun::Compare( const TObject* obj ) const
{
  // Compare a THaRun object to another run. Returns 0 when equal, 
  // -1 when 'this' is smaller and +1 when bigger (like strcmp).
  // Used by ROOT containers.

  if (this == obj) return 0;
  const THaRunBase* rhs = dynamic_cast<const THaRunBase*>(obj);
  if( !rhs ) return -1;
  if( *this < *rhs )       return -1;
  else if( *this > *rhs )  return  1;
  return 0;
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
  cout << "Max # scan:     " << fMaxScan  << endl;
  cout << "CODA file:      " << fFilename << endl;
  cout << "Segment number: " << fSegment  << endl;
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
    // Disable advanced processing
    evdata->EnableScalers(kFALSE);
    evdata->EnableHelicity(kFALSE);
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
Int_t THaRun::SetFilename( const char* name )
{
  // Set the name of the raw data file.
  // If the name changes, the existing input, if any, will be closed.
  // Return -1 if illegal name, 1 if name not changed, 0 otherwise.

  static const char* const here = "SetFilename";

  if( !name ) {
    Error( here, "Illegal file name." );
    return -1;
  }

  if( fFilename == name )
    return 1;

  Close();
  fFilename = name;
  FindSegmentNumber();

  // The run becomes uninitialized only if this is not a continuation segment
  if( fSegment == 0 )
    fIsInit = kFALSE;

  return 0;
}

//_____________________________________________________________________________
void THaRun::SetNscan( UInt_t n )
{
  // Set number of events to prescan during Init(). Default is 5000.

  fMaxScan = n;
}

#if ROOT_VERSION_CODE < ROOT_VERSION(5,8,0)
#include <cstdlib>
#endif

//_____________________________________________________________________________
Int_t THaRun::FindSegmentNumber()
{
  // Determine the segment number, if any. For Hall A CODA disk files, we can 
  // safely assume that the suffix of the file name will tell us.
  // Internal function.

  Ssiz_t dot = fFilename.Last('.');
  if( dot != kNPOS ) {
    TString s = fFilename(dot+1,fFilename.Length()-dot-1);
#if ROOT_VERSION_CODE < ROOT_VERSION(5,8,0)
    fSegment = atoi(s.Data());
#else
    // NB: This ignores whitespace. Shouldn't matter though.
    fSegment = s.Atoi();
#endif
  } else
    fSegment = 0;

  return fSegment;
}

//_____________________________________________________________________________
ClassImp(THaRun)
